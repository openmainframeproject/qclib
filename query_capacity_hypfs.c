/* Copyright IBM Corp. 2013, 2015 */

#define _GNU_SOURCE
#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <mntent.h>


#include "query_capacity_int.h"
#include "query_capacity_data.h"


#define QC_HYPFS_LPAR		"/s390_hypfs/diag_204"
#define QC_HYPFS_ZVM		"/s390_hypfs/diag_2fc"
#define QC_NAME_LEN		8
#define QC_CPU_TYPE_CP		0
#define QC_CPU_TYPE_AP		2
#define QC_CPU_TYPE_IFL		3
#define QC_CPU_TYPE_IIP		5

#define HYPFS_NA		0
#define HYPFS_AVAIL_ASCII_LPAR	1
#define HYPFS_AVAIL_ASCII_ZVM	2
#define HYPFS_AVAIL_BIN_LPAR	3
#define HYPFS_AVAIL_BIN_ZVM	4


struct dfs_diag_hdr {
	__u64     len;
	__u16     version;
	__u8      tod_ext[16];
	__u64     count;
	__u8      reserved[30];
} __attribute__ ((packed));

struct dfs_info_blk_hdr {
	__u8      npar;
	__u8      flags;
	__u8      reserved1[6];
	__u64     curtod1;
	__u64     curtod2;
	__u8      reserved[40];
} __attribute__ ((packed));

struct dfs_sys_hdr {
	__u8      reserved1;
	__u8      cpus;
	__u8      rcpus;
	__u8      reserved2[5];
	char      sys_name[8];
	__u8      reserved3[80];
} __attribute__ ((packed));

struct dfs_cpu_info {
	__u16     cpu_addr;
	__u8      reserved1[2];
	__u8      ctidx;
	__u8      reserved2[3];
	__u64     acc_time;
	__u64     lp_time;
	__u8      reserved3[6];
	__u8      reserved4[2];
	__u64     online_time;
	__u8      reserved5[56];
} __attribute__ ((packed));

struct dfs_phys_hdr {
	__u8      reserved1[1];
	__u8      cpus;
	__u8      reserved2[94];
} __attribute__ ((packed));

struct dfs_phys_cpu {
	__u16     cpu_addr;
	__u8      reserved1[2];
	__u8      ctidx;
	__u8      reserved2[3];
	__u64     mgm_time;
	__u8      reserved3[80];
} __attribute__ ((packed));

struct dfs_diag2fc {
	__u32     version;
	__u32     flags;
	__u64     used_cpu;
	__u64     el_time;
	__u64     mem_min_kb;
	__u64     mem_max_kb;
	__u64     mem_share_kb;
	__u64     mem_used_kb;
	__u32     pcpus;
	__u32     lcpus;
	__u32     vcpus;
	__u32     ocpus;
	__u32     cpu_max;
	__u32     cpu_shares;
	__u32     cpu_use_samp;
	__u32     cpu_delay_samp;
	__u32     page_wait_samp;
	__u32     idle_samp;
	__u32     other_samp;
	__u32     total_samp;
	char    guest_name[QC_NAME_LEN];
} __attribute__ ((packed));

struct hypfs_priv {
	char   *data;
	int 	avail;
	ssize_t len;
	char   *diag;
	char   *hypfs;
};

// Returns a malloc'd string with the concatenated path
static char *qc_get_path(struct qc_handle *hdl, const char *dbgfs, const char *file) {
	char *buf;

	if (asprintf(&buf, "%s%s", dbgfs, file) == -1) {
		qc_debug(hdl, "Error: Buffer allocation failed\n");
		buf = NULL;
	}

	return buf;
}

static void qc_dump_hypfs(struct qc_handle *hdl, char *hypfs) {
	char *cmd;
	int rc;

	qc_debug_indent_inc();
	/* dumping textual hypfs this way and a lot later than the actual parse can give
	   us different data from what we parsed before - but that is the best that we can do */
	if (!hypfs) {
		qc_debug(hdl, "Error: Failed to dump textual hypfs as hypfs==NULL\n");
		qc_mark_dump_incomplete(hdl, "hypfs textual");
		qc_debug_indent_dec();
		return;
	}
	/* We read all files individually during regular processing, so we can do now is to
	   copy the content with 'cp -r' */
	if (asprintf(&cmd, "cp -r %s/hyp %s/cpus %s/systems %s > /dev/null 2>&1",
						hypfs, hypfs, hypfs, qc_dbg_dump_dir) == -1) {
		qc_debug(hdl, "Error: Mem alloc failure, cannot dump textual hypfs\n");
		qc_mark_dump_incomplete(hdl, "hypfs textual");
		qc_debug_indent_dec();
		return;
	}
	if ((rc = system(cmd)) == 0) {
		qc_debug(hdl, "hypfs (textual) dumped to '%s'\n", qc_dbg_dump_dir);
	} else {
		qc_debug(hdl, "Error: Failed to dump textual hypfs with command '%s', rc=%d\n", cmd, rc);
		qc_mark_dump_incomplete(hdl, "hypfs textual");
	}
	free(cmd);
	qc_debug_indent_dec();

	return;
}

static void qc_dump_hypfs_bin(struct qc_handle *hdl, const char *diag, __u8 *data, ssize_t len) {
	char *fname = NULL, *cmd;
	int fd, rc, success = 0;

	/* We re-create the same directory/file structure that we read from */
	// first off, create a subdirectory so the files look exactly like on dbgfs
	if (!data) {
		qc_debug(hdl, "Error: No data passed in, cannot write binary dump\n");
		goto out;
	}
	if (asprintf(&fname, "%s/s390_hypfs", qc_dbg_dump_dir) == -1) {
		qc_debug(hdl, "Error: Mem alloc error, cannot create dump dir\n");
		goto out;
	}
	mkdir(fname, S_IRWXU);	// we don't care about a failure - could exist from a previous dump,
				// and we'll know when we store the actual data if things are good
	free(fname);
	if (asprintf(&fname, "%s/%s", qc_dbg_dump_dir,
		strcmp(diag, QC_HYPFS_LPAR) ? QC_HYPFS_ZVM : QC_HYPFS_LPAR) == -1) {
		qc_debug(hdl, "Error: Mem alloc error, cannot write dump\n");
		goto out;
	}
	fd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		qc_debug(hdl, "Error: Failed to open file '%s' to write dump\n", fname);
		goto out;
	}
	rc = write(fd, data, len);
	close(fd);
	if (rc == -1) {
		qc_debug(hdl, "Error: Failed to write binary hypfs data into dump: %s\n",
									strerror(errno));
	} else {
		qc_debug(hdl, "hypfs binary data dumped to '%s'\n", fname);
		success = 1;
	}

	if (strcmp(diag, QC_HYPFS_ZVM) == 0) {
		// if we're on z/VM, we need to make sure that the LPAR file exists, as logic
		// uses it as a flag to indicate presence of the binary hypfs API
		if (asprintf(&cmd, "touch %s/%s > /dev/null 2>&1", qc_dbg_dump_dir,
								QC_HYPFS_LPAR) == -1) {
			qc_debug(hdl, "Error: Mem alloc failure, could not touch '%s'. "
				"Dump will not work without, fix by adding it manually later on.\n",
				QC_HYPFS_LPAR);
			qc_mark_dump_incomplete(hdl, QC_HYPFS_LPAR);
			goto out;
		}
		if ((rc = system(cmd)) != 0) {
			qc_debug(hdl, "Error: Command '%s' failed, rc=%d. Dump will not work "
				"without, fix by adding it manually later on.\n", cmd, rc);
			qc_mark_dump_incomplete(hdl, QC_HYPFS_LPAR);
		}
		free(cmd);
	}

out:
	free(fname);
	if (!success)
		qc_mark_dump_incomplete(hdl, "hypfs binary");
}

static void qc_hypfs_dump(struct qc_handle *hdl, char *buf) {
	struct hypfs_priv *priv = (struct hypfs_priv *)buf;

	qc_debug(hdl, "Dump hypfs\n");
	qc_debug_indent_inc();
	if (!priv)
		goto out;
	switch(priv->avail) {
	case HYPFS_AVAIL_ASCII_LPAR:
	case HYPFS_AVAIL_ASCII_ZVM:
		qc_dump_hypfs(hdl, priv->hypfs);
		break;
	case HYPFS_AVAIL_BIN_LPAR:
	case HYPFS_AVAIL_BIN_ZVM:
		qc_dump_hypfs_bin(hdl, priv->diag, (__u8 *)priv->data, priv->len);
		break;
	case HYPFS_NA:
	default:
		break;
	}

out:
	qc_debug_indent_dec();

	return;
}

// path must be hypfs path ending with '.../cpus'
static int qc_get_hypfs_cpu_types(struct qc_handle *hdl, const char *path,
				  int *cpu_total, int *ifl_total, int *cp_total) {
	char str_buf[STR_BUF_SIZE];
	struct dirent **namelist;
	int n, no_files, un_total = 0, rc = 0;
	FILE *file;
	char *tmp;

	*cpu_total = *ifl_total = *cp_total = 0;
	no_files = scandir(path, &namelist, NULL, alphasort);
	for (n = 0; n < no_files; n++) {
		if (*namelist[n]->d_name == '.')
			// skip '.' and '..' to avoid false positives
			continue;
		if (asprintf(&tmp, "%s/%s/type", path, namelist[n]->d_name) == -1) {
			qc_debug(hdl, "Error: Couldn't allocate buffer for hypfs type path\n");
			rc = -1;
			goto out;
		}
		file = fopen(tmp, "r");
		if (!file) {
			free(tmp);
			continue;
		}
		qc_debug(hdl, "Parsing file %s\n", tmp);
		free(tmp);
		memset(str_buf, 0, STR_BUF_SIZE);
		if (fread(str_buf, 1, STR_BUF_SIZE, file) > 0) {
			if (!strncmp("CP", str_buf, strlen("CP"))) {
				(*cp_total)++;
				(*cpu_total)++;
			} else if (!strncmp("IFL", str_buf, strlen("IFL"))) {
				(*ifl_total)++;
				(*cpu_total)++;
			} else if (!strncmp("IIP", str_buf, strlen("IIP"))
				|| !strncmp("AP", str_buf, strlen("AP"))) {
				un_total++;
				(*cpu_total)++;
			} else {
				qc_debug(hdl, "Error: CPU of unknown type '%s' "
						"encountered, discarding results\n", str_buf);
				fclose(file);
				rc = -2;
				goto out;
			}
		}
		fclose(file);
	}
	qc_debug(hdl, "Found %d cpus total (%d CP, %d IFL, %d UN)\n", *cpu_total,
								*cp_total, *ifl_total, un_total);

out:
	for (n = 0; n < no_files; n++)
		free(namelist[n]);
	if (no_files > 0)
		free(namelist);

	return rc;
}

// Returns the sys hdr from the LPAR matching the name of the respective layer 1 attribute in hdl,
// and sets hdl to point to the LPAR layer.
static struct dfs_sys_hdr *qc_get_lpar_sys_hdr(struct qc_handle **hdl, iconv_t *cd, __u8 *data) {
	char lpar_name[QC_NAME_LEN + 1];
	struct dfs_info_blk_hdr *time_hdr;
	struct dfs_sys_hdr *sys_hdr;
	const char *s;
	int i;

	*hdl = (*hdl)->next;	// points to LPAR layer now
	if ((s = qc_get_attr_value_string(*hdl, qc_layer_name)) == NULL)
		return NULL;
	time_hdr = (struct dfs_info_blk_hdr *)(data + sizeof(struct dfs_diag_hdr));
	data = (__u8 *)(time_hdr + 1);
	qc_debug(*hdl, "Found %d LPARs\n", time_hdr->npar);
	for (i = 0; i < time_hdr->npar; ++i) {
		sys_hdr = (struct dfs_sys_hdr*)data;
		memset(lpar_name, 0, QC_NAME_LEN + 1);
		memcpy(&lpar_name, sys_hdr->sys_name, QC_NAME_LEN);
		if (qc_ebcdic_to_ascii(*hdl, cd, lpar_name, QC_NAME_LEN) != 0)
			return NULL;
		if (strcmp(s, lpar_name) == 0)
			break;
		data += (sizeof(struct dfs_sys_hdr) + (sys_hdr->rcpus * sizeof(struct dfs_cpu_info)));
	}
	if (i == time_hdr->npar) {
		qc_debug(*hdl, "Error: LPAR '%s' not found in sys hdrs\n", s);
		sys_hdr = NULL;
	}

	return sys_hdr;
}


static int qc_fill_in_hypfs_lpar_values(struct qc_handle *hdl, const char *hypfs) {
	int num_cpu = 0, num_ifl = 0, num_cp = 0;
	char *fpath = NULL;
	const char *s;
	int rc = 0;

	qc_debug(hdl, "Add LPAR values from textual hypfs API\n");
	qc_debug_indent_inc();
	hdl = hdl->next;	// points to LPAR layer now
	if ((s = qc_get_attr_value_string(hdl, qc_layer_name)) == NULL) {
		rc = -1;
		goto out;
	}
	if (asprintf(&fpath, "%s/systems/%s/cpus", hypfs, s) == -1) {
		qc_debug(hdl, "Error: Couldn't allocate buffer for hypfs systems path\n");
		rc = -4;
		goto out;
	}
	rc = qc_get_hypfs_cpu_types(hdl, fpath, &num_cpu, &num_ifl, &num_cp);
	free(fpath);
	if (rc) {
		rc = -5;
		goto out;
	}

	if (qc_set_attr_int(hdl, qc_num_cp_total, num_cp, ATTR_SRC_HYPFS) ||
	    qc_set_attr_int(hdl, qc_num_ifl_total, num_ifl, ATTR_SRC_HYPFS))
		rc = -6;

out:
	qc_debug_indent_dec();

	return rc;
}

static int qc_fill_in_hypfs_lpar_values_bin(struct qc_handle *hdl, iconv_t *cd, __u8 *data) {
	int num_cpu = 0, num_ifl = 0, num_cp = 0, num_un = 0, i, rc = 0;
	struct dfs_sys_hdr *sys_hdr;
	struct dfs_cpu_info *cpu;

	qc_debug(hdl, "Add LPAR values from binary hypfs API\n");
	qc_debug_indent_inc();
	if ((sys_hdr = qc_get_lpar_sys_hdr(&hdl, cd, data)) == NULL) {
		rc = -1;
		goto out;
	}

	cpu = (struct dfs_cpu_info*)(sys_hdr + 1);
	for (i = 0; i < sys_hdr->rcpus; ++i, ++cpu) {
		switch (cpu->ctidx) {
		case QC_CPU_TYPE_CP:
			num_cp++;
			num_cpu++;
			break;
		case QC_CPU_TYPE_IFL:
			num_ifl++;
			num_cpu++;
			break;
		case QC_CPU_TYPE_AP:
		case QC_CPU_TYPE_IIP:
			num_un++;
			num_cpu++;
			break;
		default:
			qc_debug(hdl, "Error: Unknown CPU type '%d' in cpu %d in phys hdr "
						"encountered, discarding results\n", cpu->ctidx, i);
			rc = -2;
			goto out;
		}
	}
	qc_debug(hdl, "Found %d cpus total (%d CP, %d IFL, %d UN)\n", num_cpu, num_cp, num_ifl,
								      num_un);
	if (qc_set_attr_int(hdl, qc_num_cp_total, num_cp, ATTR_SRC_HYPFS) ||
	    qc_set_attr_int(hdl, qc_num_ifl_total, num_ifl, ATTR_SRC_HYPFS))
		rc = -3;

out:
	qc_debug_indent_dec();

	return rc;
}

static int qc_read_file(struct qc_handle *hdl, const char *fpath, char *buf, int buflen) {
	FILE *file;
	int rc = 0;

	qc_debug(hdl, "Read file %s\n", fpath);
	file = fopen(fpath, "r");
	if (file) {
		memset(buf, 0, buflen);
		rc = fread(buf, 1, buflen, file);
		fclose(file);
	} else
		qc_debug(hdl, "Could not open file %s: %s\n", fpath, strerror(errno));

	return rc;
}

static int qc_read_diag_file(struct qc_handle *hdl, const char *dbgfs, struct hypfs_priv *priv) {
	long buflen = sizeof(struct dfs_diag_hdr);
	struct dfs_diag_hdr *hdr;
	int fh, i = 0, rc = 0;
	char *fpath = NULL;
	ssize_t lrc;

	if ((fpath = qc_get_path(hdl, dbgfs, priv->diag)) == NULL)
		goto out_fail;
	qc_debug(hdl, "Read in file '%s'\n", fpath);
	// file content needs to be read in one(!) go
	for (i = 0; i < 10; ++i) {
		fh = open(fpath, O_RDONLY);
		if (fh == -1) {
			qc_debug(hdl, "Error: Failed to open file '%s'\n", fpath);
			goto out_fail;
		}
		priv->data = malloc(buflen);
		if (!priv->data) {
			qc_debug(hdl, "Error: Failed to allocate '%ld' Bytes for file content\n",
											buflen);
			goto out_fail;
		}
		lrc = read(fh, priv->data, buflen);
		close(fh);
		if (lrc == -1) {
			qc_debug(hdl, "Error: Failed to read '%ld' Bytes from '%s'\n", buflen, priv->diag);
			close(fh);
			goto out_fail;
		}
		hdr = (struct dfs_diag_hdr*)priv->data;
		if ((buflen = sizeof(struct dfs_diag_hdr) + hdr->len) == lrc) {
			priv->len = lrc;
			break;
		}
		free(priv->data);
		priv->data = NULL;
	}
	if (i >= 10) {
		qc_debug(hdl, "Error: Tried %d times, still no consistent content "
			"- giving up\n", i + 1);
		rc = 1;
	}
	goto out;

out_fail:
	free(priv->data);
	priv->data = NULL;
	rc = -1;
out:
	free(fpath);

	return rc;
}

/* Returns handle to top-layer z/VM instance along with its name. */
static struct qc_handle *qc_get_zvm_hdl(struct qc_handle *hdl, const char **s) {
	int *i;

	while (hdl->next)
		hdl = hdl->next;

	i = qc_get_attr_value_int(hdl, qc_layer_type_num);
	if (!i) {
		qc_debug(hdl, "Error: Attr 'qc_layer_type' not defined at layer %d\n", hdl->layer_no);
		return NULL;
	}
	if (*i != QC_LAYER_TYPE_ZVM_GUEST) {
		qc_debug(hdl, "Error: Layer type is '%d', expected %d\n", *i, QC_LAYER_TYPE_ZVM_GUEST);
		return NULL;
	}
	*s = qc_get_attr_value_string(hdl, qc_layer_name);
	if (!*s) {
		qc_debug(hdl, "Error: Required attr 'qc_layer_name' at layer %d not defined\n", hdl->layer_no);
		return NULL;
	}

	return hdl;
}

static int qc_fill_in_hypfs_zvm_values(struct qc_handle *hdl, const char *hypfs) {
	int fplen, cpu_count, cap_num, rc = 0;
	char str_buf[STR_BUF_SIZE], *fpath = NULL, *cap = NULL;
	int dedicated = -1; /* can be 0 or 1, if set; remains -1, if not set */
	const char *s;

	qc_debug(hdl, "Add z/VM values from textual hypfs API\n");
	qc_debug_indent_inc();
	if ((hdl = qc_get_zvm_hdl(hdl, &s)) == NULL) {
		rc = -1;
		goto out;
	}
	fplen = strlen(hypfs) + strlen("/systems/") + strlen(s)	+ strlen("/cpus/");
	fpath = malloc(fplen + strlen("dedicated") + 1);  // longest string possible
	if (!fpath) {
		qc_debug(hdl, "Error: Could not allocate systems path\n");
		rc = -2;
		goto out;
	}

	/* read capping off/soft/hard */
	sprintf(fpath, "%s/systems/%s/cpus/capped", hypfs, s);	// fill string 1st time only,
					// overwrite filename in all successive occasions
	if (qc_read_file(hdl, fpath, str_buf, STR_BUF_SIZE)) {
		if (!strncmp("1", str_buf, 1)) {
			cap_num = QC_CAPPING_SOFT;
			cap = "soft";
		} else if (!strncmp("2", str_buf, 1)) {
			cap_num = QC_CAPPING_HARD;
			cap = "hard";
		} else {
			cap_num = QC_CAPPING_OFF;
			cap = "off";
		}
		if (qc_set_attr_int(hdl, qc_capping_num, cap_num, ATTR_SRC_HYPFS) ||
		    qc_set_attr_string(hdl, qc_capping, cap, strlen(cap), ATTR_SRC_HYPFS)) {
			rc = -3;
			goto out;
		}
	}

	/* if guest dedicated, all cpus are dedicated, update sums */
	strcpy(fpath + fplen, "dedicated");
	if (qc_read_file(hdl, fpath, str_buf, STR_BUF_SIZE)) {
		if (!strncmp("0", str_buf, 1))
			dedicated = 0;
		else if (!strncmp("1", str_buf, 1))
			dedicated = 1;
	}

	strcpy(fpath + fplen, "count");
	if (qc_read_file(hdl, fpath, str_buf, STR_BUF_SIZE)) {
		if (sscanf(str_buf, "%i", &cpu_count) <= 0)
			cpu_count = 0;
	}

	qc_debug(hdl, "Raw data: %d cpus, dedicated=%u, capped=%s\n", cpu_count, dedicated, cap);
	if (cpu_count) {
		/* the dedicated flag tells us, if the guest has got at least one dedicated CPU.
		 * That means, we can only derive information, if no CPU is dedicated
		 * (i.e. all shared) */
		if (dedicated == 0) { /* dedicated flag present and not set */
			if (qc_set_attr_int(hdl, qc_num_cpu_shared, cpu_count, ATTR_SRC_HYPFS) ||
			    qc_set_attr_int(hdl, qc_num_cpu_dedicated, 0, ATTR_SRC_HYPFS)) {
				rc = -4;
				goto out;
			}
		}
	}

out:
	free(fpath);
	qc_debug_indent_dec();

	return rc;
}

// Returns diag data for highest layer z/VM instance in var 'data', with pointer to entire data
// stored in 'buf' (must be free()'d), and updates hdl to point to respective handle.
static int qc_get_zvm_diag_data(struct qc_handle **hdl, iconv_t *cd, struct dfs_diag_hdr *hdr, struct dfs_diag2fc **data) {
	char name[QC_NAME_LEN + 1];
	const char *s;
	int i;

	if ((*hdl = qc_get_zvm_hdl(*hdl, &s)) == NULL)
		return -1;
	qc_debug(*hdl, "Found data for %llu z/VM guest(s)\n", hdr->count);
	for (i = 0, *data = (struct dfs_diag2fc*)(hdr + 1); i < hdr->count; ++i, ++*data) {
		memset(&name, 0, QC_NAME_LEN + 1);
		memcpy(name, (*data)->guest_name, QC_NAME_LEN);
		if (qc_ebcdic_to_ascii(*hdl, cd, name, QC_NAME_LEN) != 0)
			return -2;
		if (strcmp(name, s) == 0)
			return 0;
	}
	qc_debug(*hdl, "Error: No matching data found for z/VM guest '%s'\n", s);
	return -3;
}

static int qc_fill_in_hypfs_zvm_values_bin(struct qc_handle *hdl, iconv_t *cd, struct hypfs_priv *priv) {
	unsigned int dedicated, capped;
	struct dfs_diag2fc *data;
	int rc = 0, cap_num;
	char *cap;

	qc_debug(hdl, "Add z/VM values from binary hypfs API\n");
	qc_debug_indent_inc();
	if ((rc = qc_get_zvm_diag_data(&hdl, cd, (struct dfs_diag_hdr *)priv->data, &data)) != 0)
		goto out;

	// update capping information
	capped = (data->flags & 0x00000006) >> 1;
	dedicated = (data->flags & 0x00000008) >> 3;
	qc_debug(hdl, "Raw data: %d cpus, dedicated=%u, capped=%u\n", data->vcpus, dedicated, capped);
	switch (capped) {
	case 1: cap_num = QC_CAPPING_SOFT;
		cap = "soft";
		break;
	case 2: cap_num = QC_CAPPING_HARD;
		cap = "hard";
		break;
	default:
		cap_num = QC_CAPPING_OFF;
		cap = "off";
	}
	if (qc_set_attr_int(hdl, qc_capping_num, cap_num, ATTR_SRC_HYPFS) ||
	    qc_set_attr_string(hdl, qc_capping, cap, strlen(cap), ATTR_SRC_HYPFS)) {
		rc = -1;
		goto out;
	}

	// update shared cpu counts
	if (dedicated == 0) {
		/* the dedicated flag tells us, if the guest has got at
		 * least one dedicated CPU. That means, we can only
		 * derive information, if no CPU is dedicated (i.e.
		 * all shared) */
		if (qc_set_attr_int(hdl, qc_num_cpu_shared, data->vcpus, ATTR_SRC_HYPFS) ||
		    qc_set_attr_int(hdl, qc_num_cpu_dedicated, 0, ATTR_SRC_HYPFS)) {
			rc = -4;
			goto out;
		}
	}

out:
	qc_debug_indent_dec();

	return rc;
}

/* Retrieve mountpoint of fstype from /etc/mtab.
   Returns 0 on success with malloc'd mountpoint in 'mp', >0 if not found and
   <0 in case of an error. */
static int qc_get_mountpoint(struct qc_handle *hdl, char *fstype, char **mp) {
	struct mntent *mntbuf;
	FILE *mounts;
	char *fname;
	int rc;

	if (qc_dbg_use_dump) {
		// dumped data will look exactly like if on dbgfs or hypfs, so all we need
		// to do is point *mp to the right directory - if the respective data is present,
		// which we check with a simple sanity check
		qc_debug(hdl, "Read hypfs from dump\n");
		if (strcmp(fstype, "s390_hypfs") == 0) {
			if (asprintf(&fname, "%s/hyp", qc_dbg_use_dump) == -1) {
				qc_debug(hdl, "Error: Mem alloc failed, cannot read dump\n");
				return -1;
			}
		} else {
			if (asprintf(&fname, "%s/%s", qc_dbg_use_dump, QC_HYPFS_LPAR) == -1) {
				qc_debug(hdl, "Error: Mem alloc failed, cannot read dump\n");
				return -1;
			}
		}
		rc = access(fname, R_OK);
		free(fname);
		if (rc)
			return 1;
		*mp = strdup(qc_dbg_use_dump);
		return 0;
	}
	qc_debug(hdl, "Locate mount point of %s\n", fstype);
	*mp = NULL;
	mounts = setmntent(_PATH_MOUNTED, "r");
	if (!mounts) {
		qc_debug(hdl, "Error: Failed to open %s\n", _PATH_MOUNTED);
		return -1;
	}
	while ((mntbuf = getmntent(mounts)) != NULL) {
		if (strcmp(mntbuf->mnt_type, fstype) == 0) {
			*mp = strdup(mntbuf->mnt_dir);
			if (!*mp) {
				qc_debug(hdl, "Error: Failed to allocate buffer\n");
				endmntent(mounts);
				return -2;
			}
			break;
		}
	}
	endmntent(mounts);
	if (!*mp) {
		qc_debug(hdl, "%s not mounted according to '%s'\n", fstype, _PATH_MOUNTED);
		return 1;
	}
	qc_debug(hdl, "%s mounted at '%s'\n", fstype, *mp);

	return 0;
}

static int qc_update_hypfs(struct qc_handle *hdl, const char *upath) {
	FILE *file;
	size_t rc;

	qc_debug(hdl, "Update hypfs using %s\n", upath);
	file = fopen(upath, "w");
	if (!file) {
		/* Don't treat as an error in case hypfs is mounted but not accessible.
		   But we assume the remainder of hypfs won't be accessible either, so out we go. */
		qc_debug(hdl, "Warning: Failed to open '%s': %s\n", upath, strerror(errno));
		return 1;
	}
	rc = fwrite("1\n", 1, strlen("1\n"), file);
	fclose(file);
	if (rc < strlen("1\n")) {
		// Could be file access rights preventing us from a proper update
		qc_debug(hdl, "Warning: Failed to write to '%s', rc=%zd\n", upath, rc);
		return 2;
	}

	return 0;
}

static int qc_get_update_mod_time(struct qc_handle *hdl, const char *hypfs, time_t *mtime) {
	char *fpath = NULL;
	struct stat buf;

	if (qc_dbg_use_dump) {
		*mtime = 37;
		return 0;
	}

	qc_debug(hdl, "Retrieve mod time of %s/update\n", hypfs);
	if ((fpath = qc_get_path(hdl, hypfs, "/update")) == NULL)
		return -1;
	if (stat(fpath, &buf)) {
		qc_debug(hdl, "Error: Couldn't stat '%s'\n", fpath);
		free(fpath);
		return -2;
	}
	free(fpath);
	*mtime = buf.st_mtime;
	qc_debug(hdl, "Mod time: %ld\n", *mtime);

	return 0;
}

static int qc_hypfs_open(struct qc_handle *hdl, char **buf) {
	char *dbgfs = NULL, *fpath = NULL;
	struct hypfs_priv *priv;
	int rc = 0;

	qc_debug(hdl, "Retrieve hypfs information\n");
	qc_debug_indent_inc();
	if ((priv = malloc(sizeof(struct hypfs_priv))) == NULL) {
		qc_debug(hdl, "Error: Failed to allocate hypfs_priv\n");
		rc = -1;
		goto out;
	}
	bzero(priv, sizeof(struct hypfs_priv));
	*buf = (char *)priv;

	// check for binary hypfs interface
	if ((rc = qc_get_mountpoint(hdl, "debugfs", &dbgfs)) < 0)
		goto out;
	if (rc == 0) {
		// LPAR diag file is always present if binary interface is available
		if ((fpath = qc_get_path(hdl, dbgfs, QC_HYPFS_LPAR)) == NULL) {
			rc = -2;
			goto out;
		}
		rc = access(fpath, R_OK);
		free(fpath);
		fpath = NULL;
		if (rc == 0) {
			qc_debug(hdl, "Use binary hypfs API\n");
			if ((fpath = qc_get_path(hdl, dbgfs, QC_HYPFS_ZVM)) == NULL) {
				rc = -3;
				goto out;
			}
			if (access(fpath, R_OK) == 0) {
				/* if z/VM diag file exists, the LPAR diag file's content
				   isn't valid, so we're done after handling the z/VM file */
				priv->diag = QC_HYPFS_ZVM;
				if ((rc = qc_read_diag_file(hdl, dbgfs, priv)) != 0)
					goto out;
				priv->avail = HYPFS_AVAIL_BIN_ZVM;
			} else {
				qc_debug(hdl, "No z/VM diag file found, must be an LPAR\n");
				priv->diag = QC_HYPFS_LPAR;
				if ((rc = qc_read_diag_file(hdl, dbgfs, priv)) != 0)
					goto out;
				priv->avail = HYPFS_AVAIL_BIN_LPAR;
			}
		} else {
			qc_debug(hdl, "Binary hypfs API not available: %s\n", strerror(errno));
			rc = 0;
		}
	} else
		rc = 0;
	// Note: The ASCII hypfs has to be parsed on the fly in qc_hypfs_fill_in()

out:
	qc_debug_indent_dec();
	free(dbgfs);
	free(fpath);

	return rc;
}

static void qc_hypfs_close(struct qc_handle *hdl, char *buf) {
	struct hypfs_priv *priv = (struct hypfs_priv *)buf;
	if (priv) {
		free(priv->data);
		free(priv->hypfs);
		free(priv);
	}
}

static int qc_hypfs_process(struct qc_handle *hdl, iconv_t *cd, char *buf) {
	struct hypfs_priv *priv = (struct hypfs_priv *)buf;
	char str_buf[STR_BUF_SIZE] = "";
	time_t mtime, mtime_old;
	char *fpath = NULL;
	FILE *file = NULL;
	int i, rc = 0;

	qc_debug(hdl, "Process hypfs\n");
	qc_debug_indent_inc();
	if (!priv) {
		qc_debug(hdl, "qc_hypfs_fill_in() called with priv==NULL, exiting\n");
		goto out;
	}
	if (priv->avail == HYPFS_AVAIL_BIN_LPAR) {
		rc = qc_fill_in_hypfs_lpar_values_bin(hdl, cd, (__u8 *)priv->data);
		goto out;
	}
	if (priv->avail == HYPFS_AVAIL_BIN_ZVM) {
       		rc = qc_fill_in_hypfs_zvm_values_bin(hdl, cd, priv);
		goto out;
	}

	/* fallback to textual interface */
	qc_debug(hdl, "Use textual hypfs API\n");
	rc = qc_get_mountpoint(hdl, "s390_hypfs", &priv->hypfs);
	if (rc < 0)
		goto out;
	if (rc > 0) {
		rc = 0;		// don't treat non-presence of hypfs as an error
		qc_debug(hdl, "hypfs info not available\n");
		goto out;
	}
	if ((fpath = qc_get_path(hdl, priv->hypfs, "/update")) == NULL)
		goto mem_err;
	rc = qc_update_hypfs(hdl, fpath);
	if (rc < 0)
		goto out;
	if (rc > 0) {
		qc_debug(hdl, "hypfs info not available\n");
		rc = 0;		// don't treat as an error
		goto out;
	}
	free(fpath);
	fpath = NULL;

	/* If we can't get it right within 3 tries, we give up */
	for(i = 0; i < 3; ++i) {
		if (qc_get_update_mod_time(hdl, priv->hypfs, &mtime_old)) {
			rc = -2;
			goto out;
		}

		memset(str_buf, 0, STR_BUF_SIZE);
		if ((fpath = qc_get_path(hdl, priv->hypfs, "/hyp/type")) == NULL)
			goto mem_err;
		if (!qc_read_file(hdl, fpath, str_buf, STR_BUF_SIZE)) {
			qc_debug(hdl, "Error: Failed to open or read '%s'\n", fpath);
			rc = -4;
			goto out;
		}

		if (!strncmp(str_buf, "LPAR Hypervisor", strlen("LPAR Hypervisor"))) {
			if (qc_fill_in_hypfs_lpar_values(hdl, priv->hypfs)) {
				rc = -6;
				goto out;
			}
			priv->avail = HYPFS_AVAIL_ASCII_LPAR;
		} else if (!strncmp("z/VM Hypervisor", str_buf, strlen("z/VM Hypervisor"))) {
			if (qc_fill_in_hypfs_zvm_values(hdl, priv->hypfs)) {
				rc = -7;
				goto out;
			}
			priv->avail = HYPFS_AVAIL_ASCII_ZVM;
		} else {
			qc_debug(hdl, "Error: Unhandled hypervisor '%s', ignored\n", str_buf);
			rc = 0;
			goto out;
		}

		if (qc_get_update_mod_time(hdl, priv->hypfs, &mtime)) {
			rc = -8;
			goto out;
		}
		if (mtime == mtime_old)
			goto out;
	}

	/* Ideally we'd clear any data possibly collected in an invalid attempt
	   (mod_time != buf.st_mtime). However, that's complicated, as we'd have to
	   revert to previously filled in values from other sources - which we
	   currently can't */
	qc_debug(hdl, "Error: Failed to get consistent data from hypfs\n");
	rc = -9;
	goto out;

mem_err:
	qc_debug(hdl, "Error: Memory allocation error\n");
	rc = -10;
out:
	free(fpath);
	if (file)
		fclose(file);
	qc_debug_indent_dec();

	return rc;
}

struct qc_data_src hypfs = {qc_hypfs_open,
			    qc_hypfs_process,
			    qc_hypfs_dump,
			    qc_hypfs_close,
			    NULL,
			    NULL};

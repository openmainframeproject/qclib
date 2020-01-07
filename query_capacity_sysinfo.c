/* Copyright IBM Corp. 2013, 2015 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "query_capacity_int.h"
#include "query_capacity_data.h"


#define MAX(a,b) ((a)>(b)?a:b)

static const char *qc_sysinfo_delim = "\n";


static void qc_sysinfo_dump(struct qc_handle *hdl, char *sysinfo) {
	char *path;
	FILE *f;

	qc_debug(hdl, "Dump sysinfo\n");
	qc_debug_indent_inc();
	if (!sysinfo) {
		// /proc/sysinfo is guaranteed to exist - if not, something went wrong
		qc_debug(hdl, "Error: Failed to dump sysinfo, as sysinfo == NULL\n");
		qc_mark_dump_incomplete(hdl, "sysinfo");
		qc_debug_indent_dec();
		return;
	}
	if (asprintf(&path, "%s/sysinfo", qc_dbg_dump_dir) == -1) {
		qc_debug(hdl, "Error: Mem alloc failure, cannot dump sysinfo\n");
		qc_mark_dump_incomplete(hdl, "sysinfo");
		qc_debug_indent_dec();
		return;
	}
	f = fopen(path, "w");
	if (f) {
		if (fprintf(f, "%s", sysinfo) >= 0) {
			qc_debug(hdl, "sysinfo dumped to '%s'\n", path);
		} else {
			qc_debug(hdl, "Error: Failed to write dump to '%s'\n", path);
			qc_mark_dump_incomplete(hdl, "sysinfo");
		}
		fclose(f);
	} else {
		qc_debug(hdl, "Error: Failed to open %s to write sysinfo dump\n", path);
		qc_mark_dump_incomplete(hdl, "sysinfo");
	}
	free(path);
	qc_debug_indent_dec();

	return;
}

static int qc_sysinfo_open(struct qc_handle *hdl, char **sysinfo) {
	char *fname = NULL;
	ssize_t lrc = 1, sysinfo_sz;
	struct stat buf;
	int fd;

	qc_debug(hdl, "Retrieve sysinfo\n");
	qc_debug_indent_inc();
	*sysinfo = NULL;
	if (qc_dbg_use_dump) {
		qc_debug(hdl, "Read sysinfo from dump\n");
		if (asprintf(&fname, "%s/sysinfo", qc_dbg_use_dump) == -1) {
			qc_debug(hdl, "Error: Mem alloc failed, cannot open dump\n");
			goto out_early;
		}
		if (stat(fname, &buf)) {
			qc_debug(hdl, "Error: Failed to stat file '%s'\n", fname);
			goto out_early;
		}
		sysinfo_sz = buf.st_size + 1;
	} else {
		qc_debug(hdl, "Read sysinfo from /proc/sysinfo\n");
		sysinfo_sz = 4096;
	}

	for (lrc = sysinfo_sz; lrc >= sysinfo_sz; sysinfo_sz *= 2, lrc *= 2) {
		fd = open(fname ? fname : "/proc/sysinfo", O_RDONLY);
		if (!fd) {
			qc_debug(hdl, "Error: Failed to open file '%s': %s\n",
				 fname ? fname : "/proc/sysinfo", strerror(errno));
			goto out;
		}

		free(*sysinfo);
		qc_debug(hdl, "Read sysinfo using buffer size %zu\n", sysinfo_sz);
		*sysinfo = malloc(sysinfo_sz);
		if (!*sysinfo) {
			qc_debug(hdl, "Error: Failed to alloc buffer for sysinfo file\n");
			goto out;
		}
		lrc = read(fd, *sysinfo, sysinfo_sz);
		if (lrc == -1) {
			qc_debug(hdl, "Error: Failed to read %s file: %s\n",
				 fname ? fname : "/proc/sysinfo", strerror(errno));
			free(*sysinfo);
			*sysinfo = NULL;
			goto out;
		}
		(*sysinfo)[lrc] = '\0';
		close(fd);
	}
	goto out_early;

out:
	close(fd);

out_early:
	free(fname);
	qc_debug(hdl, "Done reading sysinfo, sysinfo=%p'\n", *sysinfo);
	qc_debug_indent_dec();

	return *sysinfo == NULL;
}

static int qc_sysinfo_lgm_check(struct qc_handle *hdl, const char *sysinfo) {
	char *lsysinfo = NULL;
	int rc = 0;

	// Live Guest Migration check: If we were migrated, /proc/sysinfo will have changed
	qc_debug(hdl, "Run LGM check\n");
	qc_debug_indent_inc();
	if (qc_sysinfo_open(hdl, &lsysinfo)) {
		qc_debug(hdl, "Error: Failed to open /proc/sysinfo\n");
		rc = 1;
		goto out;
	}
	if (strcmp(lsysinfo, sysinfo)) {
		qc_debug(hdl, "/proc/sysinfo content changed, LGM took place\n");
		rc = 2;
		goto out;
	}
	qc_debug(hdl, "/proc/sysinfo still consistent, no LGM detected\n");
	rc = 0;

out:
	qc_debug_indent_dec();
	free(lsysinfo);

	return rc;
}

static void qc_sysinfo_close(struct qc_handle *hdl, char *sysinfo) {
	free(sysinfo);
}

/* Whenever we're using strtok() to parse sysinfo, we're messing up the string, since
   strtok() will insert '\0's, so this function will create a fresh copy to work on. */
static char *qc_copy_sysinfo(struct qc_handle *hdl, char *sysinfo) {
	char *rc;

	if ((rc = strdup(sysinfo)) == NULL)
		qc_debug(hdl, "Error: Failed to create copy of qc_sysconfig\n");

	return rc;
}

/* Returns 0, if no VM detected in sysinfo. returns 1, if one VM layer
 * detected, etc.
 * Returns <0 in case of an error. */
static int qc_get_sysinfo_highest_vm_layer(struct qc_handle *hdl, char *sysinfo) {
	char *line = NULL, *start, *sysi = NULL;
	int layer = -1, i;

	qc_debug_indent_inc();
	if ((start = sysi = qc_copy_sysinfo(hdl, sysinfo)) == NULL) {
		layer = -3;
		goto out;
	}

	while ((line = strtok(start, qc_sysinfo_delim)) != NULL) {
		start = NULL;
		if (sscanf(line, "VM%02d", &i) > 0)
			layer = MAX(layer, i);
	}
	qc_debug(hdl, "Found VM on layer %d\n", layer + 1);
out:
	qc_debug_indent_dec();
	free(sysi);

	return layer + 1;
}

/* Returns 0 if cp ID was found, !=0 otherwise */
static int qc_get_cp_id(struct qc_handle *hdl, int layer, char *cp_id, size_t cp_id_len, char *sysinfo) {
	char buf_cp[] = "VMxx Control Program: %16[^\001]s\n";
	char *line, *start, *sysi = NULL;
	int rc = 0;

	/* make sure sscanf() later on cannot overflow */
	if (cp_id_len < 17) {
		qc_debug(hdl, "Internal Error: Called qc_get_cp_id() with cp_id_len=%zd (<17)\n",
				cp_id_len);
		return -1;
	}
	buf_cp[2] = '0' + layer/10;
	buf_cp[3] = '0' + layer%10;
	if ((start = sysi = qc_copy_sysinfo(hdl, sysinfo)) == NULL)
		return -2;
	while ((line = strtok(start, qc_sysinfo_delim)) != NULL && !rc) {
		start = NULL;
		if ((rc = sscanf(line, buf_cp, cp_id)) > 0)
			break;
	}
	free(sysi);

	return !rc;
}

#define QC_PARSE_SYSINFO_VM_LINE_STR(name, strlen, var) \
	if (sscanf(line, buf_##name, string_buffer) > 0) { \
		if (qc_set_attr_string(hdl, var, string_buffer, strlen, ATTR_SRC_SYSINFO)) { \
			rc = -10; \
			goto out; \
		} \
		if ((line = strtok(start, qc_sysinfo_delim)) == NULL) \
			continue; \
	}
#define QC_PARSE_SYSINFO_VM_LINE_NUM(hdl, name, var) \
	if (sscanf(line, buf_##name, &int_buffer) > 0) { \
		if (qc_set_attr_int(hdl, var, int_buffer, ATTR_SRC_SYSINFO)) { \
			rc = -11; \
			goto out; \
		} \
		if ((line = strtok(start, qc_sysinfo_delim)) == NULL) \
			continue; \
	}
static int qc_fill_in_sysinfo_values_vm(struct qc_handle *hdl, int layers, char *sysinfo) {
	char buf_name[]     = "VMxx Name: %8[^\n]";	// 8 << STR_BUF_SIZE (prevents overflow in sscanf())
	char buf_ext_name[] = "VMxx Extended Name: %256[^\n]";	// 256 < STR_BUF_SIZE (prevents overflow in sscanf())
	char buf_uuid[]     = "VMxx UUID: %36s";	// 36 << STR_BUF_SIZE (prevents overflow in sscanf())
	char buf_cpu_tot[]  = "VMxx CPUs Total: %i";
	char buf_cpu_conf[] = "VMxx CPUs Configured: %i";
	char buf_cpu_stby[] = "VMxx CPUs Standby: %i";
	char buf_cpu_resv[] = "VMxx CPUs Reserved: %i";
	char buf_adj[]      = "VMxx Adjustment: %i";
	char *line = NULL, *start, c, *sysi = NULL;
	char string_buffer[STR_BUF_SIZE];
	struct qc_handle *guesthdl = NULL, *hosthdl = NULL;
	int i, j, rc = 0, int_buffer, guesttype, hosttype;

	qc_debug(hdl, "Retrieve /proc/sysinfo information for VM\n");
	qc_debug_indent_inc();
	for (i = layers - 1; i >= 0; i--) {
		if (qc_get_cp_id(hdl, i, string_buffer, STR_BUF_SIZE, sysinfo)) {
			qc_debug(hdl, "Error: Failed to retrieve CP ID from /proc/sysinfo\n");
			rc = -1;
			goto out;
		}
		if (!strncmp(string_buffer, "z/VM", strlen("z/VM"))) {
			hosttype = QC_LAYER_TYPE_ZVM_HYPERVISOR;
			guesttype = QC_LAYER_TYPE_ZVM_GUEST;
			qc_debug(hdl, "Layer %2d: z/VM-host\n", hdl->layer_no + 1);
			qc_debug(hdl, "Layer %2d: z/VM-guest\n", hdl->layer_no + 2);
		} else if (!strncmp(string_buffer, "KVM/Linux", strlen("KVM/Linux"))) {
			hosttype = QC_LAYER_TYPE_KVM_HYPERVISOR;
			guesttype = QC_LAYER_TYPE_KVM_GUEST;
			qc_debug(hdl, "Layer %2d: KVM-host\n", hdl->layer_no + 1);
			qc_debug(hdl, "Layer %2d: KVM-guest\n", hdl->layer_no + 2);
		} else {
			qc_debug(hdl, "Error: Unsupported virtualization environment "
					"encountered: '%s'\n", string_buffer);
			rc = -2;
			goto out;
		}
		if (qc_new_handle(hdl, &hosthdl, hdl->layer_no + 1, hosttype)) {
			rc = -3;
			goto out;
		}
		hdl->next = hosthdl;
		if (qc_new_handle(hosthdl, &guesthdl, hdl->layer_no + 2, guesttype)) {
			rc = -4;
			goto out;
		}
		hosthdl->next = guesthdl;
		if (qc_set_attr_string(hosthdl, qc_control_program_id, string_buffer, 16, ATTR_SRC_SYSINFO)) {
			rc = -5;
			goto out;
		}
		hdl = guesthdl;
		guesthdl = NULL;

		if ((start = sysi = qc_copy_sysinfo(hdl, sysinfo)) == NULL) {
			rc = -6;
			goto out;
		}
		while ((line = strtok(start, qc_sysinfo_delim)) != NULL) {
			start = NULL;
			for (j = 2, c = '0' + i/10; j <= 3; ++j, c = '0' + i%10) {
				buf_name[j] = buf_ext_name[j] = buf_uuid[j] = buf_cpu_tot[j] = c;
				buf_cpu_conf[j] = buf_cpu_stby[j] = buf_cpu_resv[j] = buf_adj[j] = c;
			}
			/* scanf VM.. Name: %s -> strncpy into layer name */
			QC_PARSE_SYSINFO_VM_LINE_STR(name, 8, qc_layer_name);
			QC_PARSE_SYSINFO_VM_LINE_NUM(hosthdl, adj, qc_adjustment);
			QC_PARSE_SYSINFO_VM_LINE_NUM(hdl, cpu_tot, qc_num_cpu_total);
			QC_PARSE_SYSINFO_VM_LINE_NUM(hdl, cpu_conf, qc_num_cpu_configured);
			QC_PARSE_SYSINFO_VM_LINE_NUM(hdl, cpu_stby, qc_num_cpu_standby);
			QC_PARSE_SYSINFO_VM_LINE_NUM(hdl, cpu_resv, qc_num_cpu_reserved);
			QC_PARSE_SYSINFO_VM_LINE_STR(ext_name, 256, qc_layer_extended_name);	// KVM only
			QC_PARSE_SYSINFO_VM_LINE_STR(uuid, 36, qc_layer_uuid);			// KVM only
		}
		free(sysi);
	}
out:
	qc_debug_indent_dec();

	return rc;
}

/* Note: The character arrays have been zeroed out, so we strncpy the
 * actual length, and the next byte is a zero in any case */
#define QC_SYSINFO_PARSE_LINE_STR(str, strlen, id) \
	if (sscanf(line, str, str_buf) > 0 && qc_set_attr_string(hdl, id, str_buf, strlen, ATTR_SRC_SYSINFO)) \
		return -10;
#define QC_SYSINFO_PARSE_LINE_INT(str, id) \
	if (sscanf(line, str, &int_buf) > 0) { \
		if (qc_set_attr_int(hdl, id, int_buf, ATTR_SRC_SYSINFO)) \
			return -11; \
		if ((line = strtok(start, qc_sysinfo_delim)) == NULL) \
			break; \
	}
#define QC_SYSINFO_PARSE_LINE_FLOAT(str, id) \
	if (sscanf(line, str, &float_buf) > 0) { \
		if (qc_set_attr_float(hdl, id, float_buf, ATTR_SRC_SYSINFO)) \
			return -12; \
		if ((line = strtok(start, qc_sysinfo_delim)) == NULL) \
			break; \
	}
static int qc_derive_part_char_num(struct qc_handle *hdl) {
	const char *del = " ";
	char *str, *p, src;
	int pchars = 0;

	if (qc_is_attr_set_string(hdl, qc_partition_char, &src)) {
		str = qc_get_attr_value_string(hdl, qc_partition_char);
		for (p = strtok(str, del); p; p = strtok(NULL, del)) {
			if (!strncmp(p, "Shared", strlen("Shared")))
				pchars |= QC_PART_CHAR_SHARED;
			else if (!strncmp(p, "Dedicated", strlen("Dedicated")))
				pchars |= QC_PART_CHAR_DEDICATED;
			else if (!strncmp(p, "Limited", strlen("Limited")))
				pchars |= QC_PART_CHAR_LIMITED;
			else {
				qc_debug(hdl, "Error: Encountered unknown partition "
					"characteristics '%s' in string '%s'\n", p, str);
				return -12;
			}
		}
		qc_debug(hdl, "Derived qc_partition_char_num from '%s' to be %d\n", str, pchars);
		if (qc_set_attr_int(hdl, qc_partition_char_num, pchars, ATTR_SRC_SYSINFO))
			return -13;
	}

	return 0;
}

static int qc_fill_in_sysinfo_values_cec(struct qc_handle *hdl, char *sysinfo) {
	char *line = NULL, *start, *sysi = NULL;
	char str_buf[STR_BUF_SIZE];	// large enough for all sscanf() calls by far
	int int_buf, rc = 0;
	float float_buf;

	qc_debug(hdl, "Retrieve /proc/sysinfo information for CEC\n");
	qc_debug_indent_inc();
	if (qc_set_attr_int(hdl, qc_layer_type_num, QC_LAYER_TYPE_CEC, ATTR_SRC_SYSINFO) ||
	    qc_set_attr_int(hdl, qc_layer_category_num, QC_LAYER_CAT_HOST, ATTR_SRC_SYSINFO) ||
	    qc_set_attr_string(hdl, qc_layer_type, "CEC", sizeof("CEC"), ATTR_SRC_SYSINFO) ||
	    qc_set_attr_string(hdl, qc_layer_category, "HOST", sizeof("HOST"), ATTR_SRC_SYSINFO)) {
		rc = -1;
		goto out;
	}
	if ((start = sysi = qc_copy_sysinfo(hdl, sysinfo)) == NULL) {
		rc = -2;
		goto out;
	}
	while ((line = strtok(start, qc_sysinfo_delim)) != NULL) {
		start = NULL;
		QC_SYSINFO_PARSE_LINE_STR("Manufacturer: %16s\n", 16, qc_manufacturer);
		QC_SYSINFO_PARSE_LINE_STR("Type: %4s\n", 4, qc_type);
		QC_SYSINFO_PARSE_LINE_STR("Model: %16s\n", 16, qc_model_capacity);
		QC_SYSINFO_PARSE_LINE_STR("Model: %*s %16s\n", 16, qc_model);
		QC_SYSINFO_PARSE_LINE_STR("Sequence Code: %16s\n", 16, qc_sequence_code);
		QC_SYSINFO_PARSE_LINE_STR("Plant: %4s\n", 4, qc_plant);
		QC_SYSINFO_PARSE_LINE_INT("Capacity Adj. Ind.: %i\n", qc_capacity_adjustment_indication);
		QC_SYSINFO_PARSE_LINE_INT("Capacity Ch. Reason: %i\n", qc_capacity_change_reason);
		QC_SYSINFO_PARSE_LINE_INT("CPUs Total: %i\n", qc_num_cpu_total);
		QC_SYSINFO_PARSE_LINE_INT("CPUs Configured: %i\n", qc_num_cpu_configured);
		QC_SYSINFO_PARSE_LINE_INT("CPUs Standby: %i\n", qc_num_cpu_standby);
		QC_SYSINFO_PARSE_LINE_INT("CPUs Reserved: %i\n", qc_num_cpu_reserved);
		QC_SYSINFO_PARSE_LINE_FLOAT("Capability: %f\n", qc_capability);
		QC_SYSINFO_PARSE_LINE_FLOAT("Secondary Capability: %f\n", qc_secondary_capability);
	}

out:
	qc_debug_indent_dec();
	free(sysi);

	return rc;
}

static int qc_fill_in_sysinfo_values_lpar(struct qc_handle *hdl, char *sysinfo) {
	char *line = NULL, *start, *sysi = NULL;
	char str_buf[STR_BUF_SIZE];	// large enough for all sscanf() calls by far
	int int_buf, rc = 0;

	qc_debug(hdl, "Retrieve /proc/sysinfo information for LPAR\n");
	qc_debug_indent_inc();

	if ((start = sysi = qc_copy_sysinfo(hdl, sysinfo)) == NULL) {
		rc = -1;
		goto out;
	}
	while ((line = strtok(start, qc_sysinfo_delim)) != NULL) {
		start = NULL;
		QC_SYSINFO_PARSE_LINE_INT("LPAR Number: %i\n", qc_partition_number);
		QC_SYSINFO_PARSE_LINE_STR("LPAR Characteristics: %25[^\n]s\n", 25, qc_partition_char);
		QC_SYSINFO_PARSE_LINE_STR("LPAR Name: %8s\n", 8, qc_layer_name);
		QC_SYSINFO_PARSE_LINE_INT("LPAR Adjustment: %i\n", qc_adjustment);
		QC_SYSINFO_PARSE_LINE_INT("LPAR CPUs Total: %i\n", qc_num_cpu_total);
		QC_SYSINFO_PARSE_LINE_INT("LPAR CPUs Configured: %i\n", qc_num_cpu_configured);
		QC_SYSINFO_PARSE_LINE_INT("LPAR CPUs Standby: %i\n", qc_num_cpu_standby);
		QC_SYSINFO_PARSE_LINE_INT("LPAR CPUs Reserved: %i\n", qc_num_cpu_reserved);
		QC_SYSINFO_PARSE_LINE_INT("LPAR CPUs Dedicated: %i\n", qc_num_cpu_dedicated);
		QC_SYSINFO_PARSE_LINE_INT("LPAR CPUs Shared: %i\n", qc_num_cpu_shared);
	}
	rc = qc_derive_part_char_num(hdl);

out:
	qc_debug_indent_dec();
	free(sysi);

	return rc;
}

static int qc_sysinfo_process(struct qc_handle *hdl, iconv_t *cd, char *sysinfo) {
	struct qc_handle *lparhdl = hdl->next;
	int vmlayers, rc = 0;

	qc_debug(hdl, "Process sysinfo\n");
	qc_debug_indent_inc();
	if (!sysinfo) {
		qc_debug(hdl, "qc_sysinfo_fill_in() called with priv==0\n");
		goto out;
	}
	if (qc_fill_in_sysinfo_values_cec(hdl, sysinfo)) {
		rc = -1;
		goto out;
	}
	if (qc_fill_in_sysinfo_values_lpar(lparhdl, sysinfo)) {
		rc = -2;
		goto out;
	}
	vmlayers = qc_get_sysinfo_highest_vm_layer(hdl, sysinfo);
	if (vmlayers > 100 || vmlayers < 0) {
		qc_debug(hdl, "Error: Invalid number of VM layers: %d\n", vmlayers);
		rc = -3;
		goto out;
	}
	if (qc_fill_in_sysinfo_values_vm(lparhdl, vmlayers, sysinfo)) {
		rc = -4;
		goto out;
	}

out:
	qc_debug_indent_dec();
	return rc;
}

struct qc_data_src sysinfo = {qc_sysinfo_open,
			      qc_sysinfo_process,
			      qc_sysinfo_dump,
			      qc_sysinfo_close,
			      qc_sysinfo_lgm_check,
			      NULL};

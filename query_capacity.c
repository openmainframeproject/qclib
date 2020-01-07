/* IBM Corp. 2013, 2015 */

//_GNU_SOURCE used for getline and posix_memalign
#define _GNU_SOURCE

#include <unistd.h>
#include <sys/stat.h>

#include "query_capacity_int.h"
#include "query_capacity_data.h"


long  qc_dbg_level;
FILE *qc_dbg_file;
char *qc_dbg_dump_dir;
int   qc_dbg_indent;
char *qc_dbg_use_dump;
static char	    *qc_dbg_file_name;
static long	     qc_dbg_autodump;
static unsigned int  qc_dbg_dump_idx;


/* Update dbg_level from environment variable */
static void qc_update_dbg_level(void) {
	char *s, *end;

	s = getenv("QC_DEBUG");
	if (s) {
		qc_dbg_level = strtol(s, &end, 10);
		if (end == s || qc_dbg_level < 0)
			qc_dbg_level = 0;
	}
	s = getenv("QC_USE_DUMP");
	// if qc_dbg_use_dump is NULL, then there's nothing we can do about it
	if (s) {
		free(qc_dbg_use_dump);
		qc_dbg_use_dump = strdup(s);
	}
	s = getenv("QC_AUTODUMP");
	if (s) {
		qc_dbg_autodump = strtol(s, &end, 10);
		if (end == s || qc_dbg_autodump < 0)
			qc_dbg_autodump = 0;
	}
}

static void qc_debug_deinit(void *hdl) {
	qc_update_dbg_level();
	if (qc_dbg_level <= 0 && qc_dbg_autodump <= 0 && qc_dbg_file) {
		qc_dbg_level = 1;	// temporarily set, or qc_debug won't print anything
		qc_debug(hdl, "Log level set to %ld, closing\n", qc_dbg_level);
		qc_dbg_level = 0;
		fclose(qc_dbg_file);
		qc_dbg_file = NULL;
		free(qc_dbg_dump_dir);
		qc_dbg_dump_dir = NULL;
		free(qc_dbg_file_name);
		qc_dbg_file_name = NULL;
		qc_dbg_dump_idx = 0;
		qc_dbg_autodump = 0;
	}
	free(qc_dbg_use_dump);
	qc_dbg_use_dump = NULL;
}

#define QC_DBGFILE		"/tmp/qclib-XXXXXX"
static int qc_debug_file_init(void) {
	int fd;

	if (!qc_dbg_file_name) {
		qc_dbg_file_name = strdup(QC_DBGFILE);
		if (!qc_dbg_file_name)
			goto out_err;
		fd = mkstemp(qc_dbg_file_name);
		if (fd == -1)
			goto out_err;
		qc_dbg_file = fdopen(fd, "w");
		if (!qc_dbg_file) {
			close(fd);
			goto out_err;
		}
	}

	return 0;

out_err:
	free(qc_dbg_file_name);
	qc_dbg_file_name = NULL;

	return -1;
}

static int qc_debug_open_dump_dir(struct qc_handle *hdl) {
	int i;

	if (!qc_dbg_file_name && qc_debug_file_init())
			return -1;
	for (i = 0, ++qc_dbg_dump_idx; i < 100; ++i, ++qc_dbg_dump_idx) {
		free(qc_dbg_dump_dir);
		qc_dbg_dump_dir = NULL;
		if (asprintf(&qc_dbg_dump_dir, "%s.dump-%u", qc_dbg_file_name,
				qc_dbg_dump_idx) == -1) {
			qc_debug(hdl, "Error: Mem alloc error\n");
			goto out_err;
		}
		if (mkdir(qc_dbg_dump_dir, S_IRWXU) == 0)
			break;
		qc_debug(hdl, "Warning: Could not create dir '%s': %s\n", qc_dbg_dump_dir,
									strerror(errno));
	}
	if (i == 100)
		goto out_err;
	qc_debug(hdl, "Created directory '%s' for all dumps\n", qc_dbg_dump_dir);

	return 0;

out_err:
	qc_debug(hdl, "Error: Could not create directory for dump, better luck maybe next time...\n");
	free(qc_dbg_dump_dir);
	qc_dbg_dump_dir = NULL;

	return -1;
}

static void qc_debug_close_dump_dir(struct qc_handle *hdl) {
	free(qc_dbg_dump_dir);
	qc_dbg_dump_dir = NULL;
}

#define QC_DUMP_INCOMPLETE	"INCOMPLETE_DUMP.txt"
/* Opens a log file for debug messages if env var QC_DEBUG is >0. Note that the file is only
   closed in qc_close_configuration() when qc_dbg_level is <=0, so that it's left up to the user
   to decide whether a single file is used all the time or individual files created for each
   invocation of the library. */
static int qc_debug_init(void) {
	static int init = 0;
	char *path = NULL;
	int rc = 0;

	if (!init) {
		// use a static initializer, as a shared library's init won't work for static libs
		qc_dbg_indent = 0;
		qc_dbg_file = NULL;
		qc_dbg_file_name = NULL;
		qc_dbg_level = 0;
		qc_dbg_dump_dir = NULL;
		qc_dbg_use_dump = NULL;
		qc_dbg_dump_idx = 0;
		qc_dbg_autodump = 0;
		init = 1;
	}
	qc_update_dbg_level();
	if (qc_dbg_level > 0 && !qc_dbg_file) {
		// open the log file - failure to do so is not a fatal error
		if (qc_debug_file_init())
			goto out_err;
		qc_debug(NULL, "Log level set to %ld\n", qc_dbg_level);
	}
	if (qc_dbg_use_dump) {
		// usage of dump file requested - any error in here is fatal
		if (access(qc_dbg_use_dump, R_OK | X_OK) == -1) {
			qc_debug(NULL, "Error: Dump usage requested, but path '%s' "
					"not accessible: %s\n",	qc_dbg_use_dump, strerror(errno));
			free(qc_dbg_use_dump);
			qc_dbg_use_dump = NULL;
			rc = 2;
			goto out_err;
		}
		// Check for marker indicating incomplete dump
		if (asprintf(&path, "%s/%s", qc_dbg_use_dump, QC_DUMP_INCOMPLETE) == -1) {
			qc_debug(NULL, "Error: Mem alloc failed");
			rc = 3;
			goto out_err;
		}
		if (!access(path, R_OK)) {
			qc_debug(NULL, "Error: Dump at %s is incomplete, cannot use\n", qc_dbg_dump_dir);
			qc_debug(NULL, "       See content of %s for list of missing components\n",
											path);
			rc = 4;
			goto out_err;
		}
		free(path);
		path = NULL;
		qc_debug(NULL, "Running with dump in '%s'\n", qc_dbg_use_dump);
	}

	return 0;

out_err:
	// Nothing we can do about this except to disable debug messages to prevent further damage
	free(qc_dbg_dump_dir);
	qc_dbg_dump_dir = NULL;
	qc_dbg_level = 0;
	qc_dbg_file = NULL;
	free(path);

	return rc;
}

void qc_debug_indent_inc(void) {
	qc_dbg_indent += 2;
}

void qc_debug_indent_dec(void) {
	qc_dbg_indent -= 2;
}

void qc_mark_dump_incomplete(struct qc_handle *hdl, char *missing_component) {
	int rc;
	char *cmd;

	if (asprintf(&cmd, "echo %s >> %s/%s", missing_component, qc_dbg_dump_dir,
								QC_DUMP_INCOMPLETE) == -1) {
		qc_debug(hdl, "Error: Failed to alloc mem to indicate dump as incomplete\n");
		return;
	}
	if ((rc = system(cmd)) != 0)
		qc_debug(hdl, "Error: Failed to exec command to indicate dump as incomplete, "
										"rc=%d\n", rc);
	free(cmd);
}

/* Convert EBCDIC input to ASCII in place, removing trailing whitespace */
int qc_ebcdic_to_ascii(struct qc_handle *hdl, iconv_t *cd, char *inbuf, size_t insz) {
	char *outbuf, *outbuf_start, *inbuf_start = inbuf;
	size_t len, outsz = insz, insz_orig, outsz_orig;
	int rc = 0;

	if (!(outbuf_start = malloc(outsz))) {
		qc_debug(hdl, "Error: Failed to alloc tmp buffer of size %zd for iconv\n", outsz);
		rc = -1;
		goto out;
	}
	outbuf = outbuf_start;
	insz_orig = insz;
	outsz_orig = outsz;
	len = iconv(*cd, &inbuf, &insz, &outbuf, &outsz);
	if (len == (size_t)(-1)) {
		qc_debug(hdl, "Error: iconv conversion failed: %s\n", strerror(errno));
		rc = -2;
		goto out;
	}

	// remove trailing whitespace
	for (len = 0; len < outsz_orig - outsz; ++len)
		if (outbuf_start[len] == ' ') {
			outbuf_start[len] = '\0';
			len++;
			break;
		}

	if (len > insz_orig) {
		qc_debug(hdl, "Error: iconv result exceeds target buffer (%zd > %zd)\n",
				len, insz);
		rc = -3;
		goto out;
	}
	memcpy(inbuf_start, outbuf_start, len);

out:
       	free(outbuf_start);

	return rc;
}

// De-alloc hdl, leaving out the actual handle
static void qc_hdl_reinit(struct qc_handle *hdl) {
	struct qc_handle *ptr = hdl, *arg = hdl;

	while (ptr) {
		free(ptr->layer);
		free(ptr->attr_present);
		free(ptr->src);
		hdl = ptr->next;
		if (ptr == arg) {
			memset(ptr, 0, sizeof(struct qc_handle));
			ptr->root = ptr;
		} else
			free(ptr);
		ptr = hdl;
	}
}

void qc_close(void *hdl) {
	qc_debug(hdl, "qc_close()\n");
	qc_debug_indent_inc();

	qc_debug_deinit(hdl);
	qc_hdl_reinit(hdl);
	free(hdl);

	qc_debug_indent_dec();
}

int qc_get_num_layers(void *config, int *rc) {
	struct qc_handle *hdl = config;

	qc_debug(hdl, "qc_get_num_layers()\n");
	qc_debug_indent_inc();
	if (!hdl) {
		qc_debug_indent_dec();
		*rc = -EFAULT;
		return *rc;
	}
	while (hdl->next)
		hdl = hdl->next;
	qc_debug(hdl, "Return %d layers\n", hdl->layer_no + 1);
	*rc = 0;
	qc_debug_indent_dec();

	return hdl->layer_no + 1;
}

#ifdef CONFIG_CONSISTENCY_CHECK
#define ATTR_UNDEF	4096
/** Verifies that a + b (+ c) <= d (c is optional) for the respective int-attributes holds true.
 * @param equals If set, we verify using '=', not '<='
 */
static int qc_verify(struct qc_handle *hdl, enum qc_attr_id a, enum qc_attr_id b, enum qc_attr_id c, enum qc_attr_id d, int equals) {
	int *val_a, *val_b, *val_c, *val_d;

	// We assume that non-presence of a value is due to...non-presence, as opposed to an error (since that would have been reported previously)
	if ((val_a = qc_get_attr_value_int(hdl, a)) == NULL ||
	    (val_b = qc_get_attr_value_int(hdl, b)) == NULL ||
	    (val_d = qc_get_attr_value_int(hdl, d)) == NULL)
		return 0;
	if (c < ATTR_UNDEF) {
		if ((val_c = qc_get_attr_value_int(hdl, c)) == NULL)
			return 0;
		if ((equals && *val_a + *val_b + *val_c != *val_d) || (!equals && *val_a + *val_b + *val_c > *val_d)) {
			qc_debug(hdl, "Warning: Consistency check '%s + %s + %s %s %s' failed at layer %d (%s/%s): %d + %d + %d %s %d\n",
				qc_attr_id_to_char(hdl, a), qc_attr_id_to_char(hdl, b),	qc_attr_id_to_char(hdl, c), (equals ? "=" : "<="),
				qc_attr_id_to_char(hdl, d), hdl->layer_no, qc_get_attr_value_string(hdl, qc_layer_type),
				qc_get_attr_value_string(hdl, qc_layer_category), *val_a, *val_b, *val_c, (equals ? "!=" : ">"), *val_d);
			return 1;
		}
	} else if ((equals && *val_a + *val_b != *val_d) || (!equals && *val_a + *val_b > *val_d)) {
		qc_debug(hdl, "Warning: Consistency check '%s + %s %s %s' failed at layer %d (%s/%s): %d + %d %s %d\n",
			qc_attr_id_to_char(hdl, a), qc_attr_id_to_char(hdl, b), (equals ? "=" : "<="), qc_attr_id_to_char(hdl, d),
			hdl->layer_no, qc_get_attr_value_string(hdl, qc_layer_type), qc_get_attr_value_string(hdl, qc_layer_category),
			*val_a, *val_b, (equals ? "!=" : ">"), *val_d);
		return 2;
	}

	return 0;
}

// Check consistency of data across data sources, as well as consistency of data within each data source.
// Returns 0 in case of success, <0 for errors, and >0 in case the data is inconsistent.
static int qc_consistency_check(struct qc_handle *hdl) {
	int *etype, rc;

	qc_debug(hdl, "Run consistency check\n");
	qc_debug_indent_inc();
	for (; hdl; hdl = hdl->next) {
		if ((etype = qc_get_attr_value_int(hdl, qc_layer_type_num)) == NULL) {
			rc = -1;
			goto out;
		}

		switch (*etype) {
		case QC_LAYER_TYPE_CEC:
			if ((rc = qc_verify(hdl, qc_num_cpu_dedicated,  qc_num_cpu_shared,  ATTR_UNDEF,	    qc_num_cpu_total, 0)) || // FIXME: should be 1
			    (rc = qc_verify(hdl, qc_num_cpu_configured, qc_num_cpu_standby, qc_num_cpu_reserved, qc_num_cpu_total, 1)) ||
			    (rc = qc_verify(hdl, qc_num_cp_total,	qc_num_ifl_total,   ATTR_UNDEF,	    qc_num_cpu_total, 0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_dedicated,   qc_num_cp_shared,   ATTR_UNDEF,	    qc_num_cp_total,  1)) ||
			    (rc = qc_verify(hdl, qc_num_ifl_dedicated,  qc_num_ifl_shared,  ATTR_UNDEF,	    qc_num_ifl_total, 1)))
				goto out;
			break;
		case QC_LAYER_TYPE_LPAR:
			// FIXME: First check disabled, since cpu_total only counts CPs which gives us false positives
			if (//(rc = qc_verify(hdl, i, qc_num_cpu_dedicated, qc_num_cpu_shared, qc_num_cpu_total, 1)) ||
			    (rc = qc_verify(hdl, qc_num_cpu_configured, qc_num_cpu_reserved,	qc_num_cpu_standby, qc_num_cpu_total, 1)) ||
			    (rc = qc_verify(hdl, qc_num_cp_dedicated,   qc_num_cp_shared,	ATTR_UNDEF,	    qc_num_cp_total,  1)) ||
			    (rc = qc_verify(hdl, qc_num_ifl_dedicated,  qc_num_ifl_shared,	ATTR_UNDEF,	    qc_num_ifl_total, 1)))
				goto out;
			break;
		case QC_LAYER_TYPE_ZVM_HYPERVISOR:
			if ((rc = qc_verify(hdl, qc_num_cpu_dedicated, qc_num_cpu_shared,	ATTR_UNDEF, qc_num_cpu_total,	  1)) ||
			    (rc = qc_verify(hdl, qc_num_cp_total,      qc_num_ifl_total,	ATTR_UNDEF, qc_num_cpu_total,	  0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_dedicated,  qc_num_ifl_dedicated,	ATTR_UNDEF, qc_num_cpu_dedicated, 0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_shared,     qc_num_ifl_shared,	ATTR_UNDEF, qc_num_cpu_shared,	  0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_dedicated,  qc_num_cp_shared,	ATTR_UNDEF, qc_num_cp_total,	  1)) ||
			    (rc = qc_verify(hdl, qc_num_ifl_dedicated, qc_num_ifl_shared,	ATTR_UNDEF, qc_num_ifl_total,	  1)))
				goto out;
			break;
		case QC_LAYER_TYPE_ZVM_GUEST:
			if ((rc = qc_verify(hdl, qc_num_cpu_dedicated,  qc_num_cpu_shared,	ATTR_UNDEF,	    qc_num_cpu_total,	  1)) ||
			    (rc = qc_verify(hdl, qc_num_cpu_configured, qc_num_cpu_reserved,	qc_num_cpu_standby, qc_num_cpu_total,	  1)) ||
			    (rc = qc_verify(hdl, qc_num_cp_total,       qc_num_ifl_total,	ATTR_UNDEF,	    qc_num_cpu_total,	  0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_dedicated,   qc_num_ifl_dedicated,	ATTR_UNDEF,	    qc_num_cpu_dedicated, 0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_shared,      qc_num_ifl_shared,	ATTR_UNDEF,	    qc_num_cpu_shared,	  0)) ||
			    (rc = qc_verify(hdl, qc_num_cp_dedicated,   qc_num_cp_shared,	ATTR_UNDEF,	    qc_num_cp_total,	  1)) ||
			    (rc = qc_verify(hdl, qc_num_ifl_dedicated,  qc_num_ifl_shared,	ATTR_UNDEF,	    qc_num_ifl_total,	  1)))
				goto out;
			break;
		case QC_LAYER_TYPE_KVM_GUEST:
			if ((rc = qc_verify(hdl, qc_num_cpu_configured, qc_num_cpu_reserved,	qc_num_cpu_standby, qc_num_cpu_total, 1)))
				goto out;
			break;
		default: break;
		}
	}

out:
	qc_debug_indent_dec();

	return rc;
}
#endif

static void *_qc_open(struct qc_handle *hdl, iconv_t *cd, int *rc) {
	// sysinfo needs to be handled first, or our LGM check later on will have loopholes
	struct qc_data_src *src, *sources[] = {&sysinfo, &hypfs, &sthyi, NULL};
	struct qc_handle *lparhdl;
	int i;

	qc_debug(hdl, "_qc_open()\n");
	qc_debug_indent_inc();
	*rc = 0;
	if (qc_new_handle(NULL, &hdl, 0, QC_LAYER_TYPE_CEC) ||
	    qc_new_handle(hdl, &lparhdl, 1, QC_LAYER_TYPE_LPAR)) {
		*rc = -1;
		goto out;
	}
	hdl->next = lparhdl;
	lparhdl->root = hdl->root;

	// open all data sources
	for (i = 0; (src = sources[i]) != NULL; i++)
		if (src->open(hdl, &src->priv))
			*rc = -2;	// don't exit on error immediately, so we collect all data for a dump later on
	if (*rc)
		goto out;

	// verify that we weren't migrated
	if ((*rc = sysinfo.lgm_check(hdl, sysinfo.priv)) != 0)
		goto out;

	// process data sources
	for (i = 0; (src = sources[i]) != NULL; i++) {
		// Return values >0 will be left as is and passed back to caller
		if ((*rc = src->process(hdl, cd, src->priv)) < 0) {
			*rc = -3;	// match errors to a value that we can identify
			goto out;
		}
		if (*rc)
			goto out;
	}

	if (qc_dbg_level > 0) {
		qc_debug(hdl, "Final layers overview:\n");
		qc_debug_indent_inc();
		for (lparhdl = hdl; lparhdl; lparhdl = lparhdl->next)
			qc_debug(hdl, "Layer %2i: %s %s\n", lparhdl->layer_no, qc_get_attr_value_string(lparhdl, qc_layer_type),
				 qc_get_attr_value_string(lparhdl, qc_layer_category));
		qc_debug_indent_dec();
	}

out:
	// Possibly dump all data sources
	if (qc_dbg_level > 1 || (qc_dbg_autodump && *rc < 0)) {
		qc_debug(hdl, "Create dump\n");
		qc_debug_indent_inc();
		qc_debug_open_dump_dir(hdl);	// get a new dump directory
		for (i = 0; (src = sources[i]) != NULL; i++)
			src->dump(hdl, src->priv);
		qc_debug_close_dump_dir(hdl);
		qc_debug_indent_dec();
	}

	// Close all data sources
	for (i = 0; (src = sources[i]) != NULL; i++)
		src->close(hdl, src->priv);
	qc_debug(hdl, "Return rc=%d\n", *rc);
	qc_debug_indent_dec();

	return hdl;
}

void *qc_open(int *rc) {
	struct qc_handle *hdl = NULL;
	int cd_init = 0;
	iconv_t cd;
	int i;

	*rc = 0;
	if (qc_debug_init()) {
		*rc = -1;
		goto out;
	}
	qc_debug(hdl, "qc_open()\n");
	qc_debug_indent_inc();

	cd = iconv_open("ISO8859-1", "IBM-1047");
	if (cd == (iconv_t)-1) {
		qc_debug(hdl, "Error: iconv setup failed: %s\n", strerror(errno));
		*rc = -2;
		goto out;
	}
	cd_init = 1;

	/* Since we retrieve data from multiple sources, CPU hotplugging provides a chance for
	 * inconsistent data. If we detect that, we retry up to a total of 3 times before
	 * giving up. */
	for (i = 0; i < 3; ++i) {
		if (i > 0) {
			qc_debug(hdl, "Warning: Sanity check failed, retry %d\n", i);
			qc_hdl_reinit(hdl);
		}
		hdl = _qc_open(hdl, &cd, rc);
		if (*rc
#ifdef CONFIG_CONSISTENCY_CHECK
		|| (*rc = qc_consistency_check(hdl)) <= 0
#endif
		)
			break;
	}
	if (*rc > 0) {
		qc_debug(hdl, "Warning: Unable to retrieve consistent data, giving up\n");
	}

out:
	if (cd_init)
		iconv_close(cd);
	qc_debug(hdl, "Return %p, rc=%d\n", *rc ? NULL : hdl, *rc);
	qc_debug_indent_dec();
	if (*rc) {
		qc_close(hdl);
		hdl = NULL;
	}

	return hdl;
}

static struct qc_handle *qc_get_layer_handle(void *config, int layer) {
	struct qc_handle *hdl = config;

	do {
		if (!hdl)
			return NULL;
		if (hdl->layer_no == layer)
			return hdl;
		hdl = hdl->next;
	} while (hdl);

	return NULL;
}

static int qc_is_attr_id_valid(enum qc_attr_id id) {
	return id >= 0 && id <= qc_type;
}

int qc_get_attribute_string(void *cfg, enum qc_attr_id id, int layer, const char **value) {
	struct qc_handle *hdl = qc_get_layer_handle(cfg, layer);
	char src;
	int rc;

	*value = NULL;
	qc_debug(cfg, "qc_get_attribute_string(attr=%d, layer=%d)\n", id, layer);
	qc_debug_indent_inc();
	if (!hdl) {
		rc = -1;
		goto out;
	}
	if (!qc_is_attr_id_valid(id)) {
		rc = -2;
		goto out;
	}
	if ((*value = qc_get_attr_value_string(hdl, id))) {
		qc_debug(cfg, "Attr '%s' from '%c' res=%s\n", qc_attr_id_to_char(cfg, id), src, *value);
		rc = 1;
		goto out;
	}
	if (qc_is_attr_set_string(hdl, id, &src) <= 0) {
		qc_debug(cfg, "Attr '%s' not defined\n", qc_attr_id_to_char(cfg, id));
		rc = 0;
		goto out;
	}
	rc = -3;

out:
	qc_debug(cfg, "Return value='%s', rc=%d\n", *value, rc);
	qc_debug_indent_dec();
	return rc;
}

int qc_get_attribute_int(void *cfg, enum qc_attr_id id, int layer, int *value) {
	struct qc_handle *hdl = qc_get_layer_handle(cfg, layer);
	void *ptr = NULL;
	char src;
	int rc;

	*value = -EINVAL;
	qc_debug(cfg, "qc_get_attribute_int(attr=%d, layer=%d)\n", id, layer);
	qc_debug_indent_inc();
	if (!hdl) {
		rc = -1;
		goto out;
	}
	if (!qc_is_attr_id_valid(id)) {
		rc = -2;
		goto out;
	}
	if ((ptr = qc_get_attr_value_int(hdl, id))) {
		qc_debug(cfg, "Attr '%s' from '%c' res=%d\n", qc_attr_id_to_char(cfg, id), src, *(int *)ptr);
		rc = 1;
		goto out;
	}
	// Attribute value not set - let's figure out why
	if (qc_is_attr_set_int(hdl, id, &src) <= 0) {
		qc_debug(cfg, "Attr '%s' not defined\n", qc_attr_id_to_char(cfg, id));
		rc = 0;
		goto out;
	}
	rc = -3;

out:
	if (ptr)
		*value = *(int *)ptr;
	qc_debug(cfg, "Return value=%d, rc=%d\n", *value, rc);
	qc_debug_indent_dec();

	return rc;
}


int qc_get_attribute_float(void *cfg, enum qc_attr_id id, int layer, float *value) {
	struct qc_handle *hdl = qc_get_layer_handle(cfg, layer);
	void *ptr = NULL;
	char src;
	int rc;

	*value = -EINVAL;
	qc_debug(cfg, "qc_get_attribute_float(attr=%d, layer=%d)\n", id, layer);
	qc_debug_indent_inc();
	if (!hdl) {
		rc = -1;
		goto out;
	}
	if (!qc_is_attr_id_valid(id)) {
		rc = -2;
		goto out;
	}
	if ((ptr = qc_get_attr_value_float(hdl, id))) {
		qc_debug(cfg, "Attr '%s' from '%c' res=%f\n", qc_attr_id_to_char(cfg, id), src, *(float *)ptr);
		rc = 1;
		goto out;
	}
	// Attribute value not set - let's figure out why
	if (qc_is_attr_set_float(hdl, id, &src) <= 0) {
		qc_debug(cfg, "Attr '%s' not defined\n", qc_attr_id_to_char(cfg, id));
		rc = 0;
		goto out;
	}
	rc = -3;

out:
	if (ptr)
		*value = *(float *)ptr;
	qc_debug(cfg, "Return value=%f, rc=%d\n", *value, rc);
	qc_debug_indent_dec();

	return rc;
}

/* Copyright IBM Corp. 2013, 2015 */

#ifndef QUERY_CAPACITY_INT
#define QUERY_CAPACITY_INT

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <iconv.h>

#include "query_capacity.h"


/* Miscellaneous structures and constants */
#define STR_BUF_SIZE		257

#define ATTR_SRC_SYSINFO	'S'
#define ATTR_SRC_HYPFS		'H'
#define ATTR_SRC_STHYI		'V'
#define ATTR_SRC_UNDEF		0

struct qc_handle {
	void		 *layer;	// holds a copy of the respective *_values struct (see below),
					// and is filled by looking up the offset via the respective *_attrs table
	struct qc_attr	 *attr_list;
	int 		  layer_no;
	int 		 *attr_present;	// array indicating whether attributes are set
	char		 *src;		// array indicating the source of the attribute's value, see ATTR_SRC_*
	struct qc_handle *next;
	struct qc_handle *root;		// points to top handle
};

struct qc_data_src {
	int  (*open)(struct qc_handle *, char **);
	int  (*process)(struct qc_handle *, iconv_t *, char *);
	void (*dump)(struct qc_handle *, char *);
	void (*close)(struct qc_handle *, char *);
	int  (*lgm_check)(struct qc_handle *, const char *);
	char *priv;
};

extern struct qc_data_src sysinfo, hypfs, sthyi;

/* Utility functions */
int qc_ebcdic_to_ascii(struct qc_handle *hdl, iconv_t *cd, char *inbuf, size_t insz);
int qc_is_nonempty_ebcdic(struct qc_handle *hdl, const unsigned char *buf, unsigned int buflen, iconv_t *cd);
int qc_new_handle(struct qc_handle *hdl, struct qc_handle **tgthdl, int layer_no, int layer_type);
struct qc_handle *qc_get_root_handle(struct qc_handle *hdl);
struct qc_handle *qc_get_prev_handle(struct qc_handle *hdl);


/* Debugging-related functions and variables */
extern long  qc_dbg_level;
extern FILE *qc_dbg_file;
extern char *qc_dbg_dump_dir;
extern char *qc_dbg_use_dump;
extern int   qc_dbg_indent;
void qc_debug_indent_inc();
void qc_debug_indent_dec();
void qc_mark_dump_incomplete(struct qc_handle *hdl, char *missing_component);

/* Build Customization */
#define CONFIG_DEBUG_TIMESTAMPS
#define CONFIG_CONSISTENCY_CHECK

#ifdef CONFIG_DEBUG_TIMESTAMPS
#define qc_debug(hdl, arg, ...)	if (qc_dbg_level > 0) { \
					time_t t; \
					struct tm *tm; \
					time(&t); \
					tm = localtime(&t); \
					fprintf(qc_dbg_file, "%02d/%02d,%02d:%02d:%02d,%-10p: %*s" arg, \
					tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, qc_get_root_handle(hdl), qc_dbg_indent, "", ##__VA_ARGS__); \
				}
#else
#define qc_debug(hdl, arg, ...)	if (qc_dbg_level > 0) { \
					fprintf(qc_dbg_file, "%-10p: %*s" arg, qc_get_root_handle(hdl), qc_dbg_indent, "", ##__VA_ARGS__); \
				}
#endif
#endif

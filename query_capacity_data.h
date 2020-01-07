/* Copyright IBM Corp. 2013, 2015 */

#ifndef QUERY_CAPACITY_DATA
#define QUERY_CAPACITY_DATA

//#include <stddef.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <errno.h>

#include "query_capacity_int.h"


/* Functions to set and get attributes */
int qc_set_attr_int(struct qc_handle *hdl, enum qc_attr_id id, int val, char src);
int qc_set_attr_float(struct qc_handle *hdl, enum qc_attr_id id, float val, char src);
int qc_set_attr_string(struct qc_handle *hdl, enum qc_attr_id id, char *val, unsigned int val_len, char src);
int qc_set_attr_ebcdic_string(struct qc_handle *hdl, enum qc_attr_id id, unsigned char *val, unsigned int val_len,
												iconv_t *cd, char src);
int qc_is_attr_set_int(struct qc_handle *hdl, enum qc_attr_id id, char *src);
int qc_is_attr_set_float(struct qc_handle *hdl, enum qc_attr_id id, char *src);
int qc_is_attr_set_string(struct qc_handle *hdl, enum qc_attr_id id, char *src);

const char *qc_attr_id_to_char(struct qc_handle *hdl, enum qc_attr_id id);

int   *qc_get_attr_value_int(struct qc_handle *hdl, enum qc_attr_id id);
float *qc_get_attr_value_float(struct qc_handle *hdl, enum qc_attr_id id);
char  *qc_get_attr_value_string(struct qc_handle *hdl, enum qc_attr_id id);
#endif

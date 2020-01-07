/* IBM Corp. 2013, 2015 */

//_GNU_SOURCE used for getline and posix_memalign
#define _GNU_SOURCE

#include <unistd.h>
#include <sys/stat.h>

#include "query_capacity_data.h"


/*
 * Below are the structures that define the attributes. The attribute is
 * reference as an enum, see documentation in query_capacity.h.
 *
 * Note that strings (char pointers) carry the trailing zero byte.
 */

#define QC_LAYER_TYPE_LEN 20
#define QC_LAYER_CAT_LEN   6

struct qc_cec_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	char layer_name[9];
	char manufacturer[17];
	char type[5];
	char model_capacity[17];
	char model[17];
	char sequence_code[17];
	char plant[5];
	int num_cpu_total;
	int num_cpu_configured;
	int num_cpu_standby;
	int num_cpu_reserved;
	int num_cpu_dedicated;
	int num_cpu_shared;
	int num_cp_total;
	int num_cp_dedicated;
	int num_cp_shared;
	int num_ifl_total;
	int num_ifl_dedicated;
	int num_ifl_shared;
	float capability;
	float secondary_capability;
	int capacity_adjustment_indication;
	int capacity_change_reason;
};

struct qc_lpar_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	int partition_number;
	char partition_char[26];
	int partition_char_num;
	char layer_name[9];
	int adjustment;
	int num_cpu_total;
	int num_cpu_configured;
	int num_cpu_standby;
	int num_cpu_reserved;
	int num_cpu_dedicated;
	int num_cpu_shared;
	int num_cp_total;
	int num_cp_dedicated;
	int num_cp_shared;
	int num_ifl_total;
	int num_ifl_dedicated;
	int num_ifl_shared;
	int cp_absolute_capping;
	int ifl_absolute_capping;
	int cp_weight_capping;
	int ifl_weight_capping;
};

struct qc_zvm_pool_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	char layer_name[9];
	int cp_limithard_cap;
	int cp_capacity_cap;
	int ifl_limithard_cap;
	int ifl_capacity_cap;
	int cp_capped_capacity;
	int ifl_capped_capacity;
};

struct qc_zvm_hypervisor_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	char layer_name[9];
	char cluster_name[9];
	char control_program_id[17];
	int adjustment;
	int hardlimit_consumption;
	int num_cpu_total;
	int num_cpu_dedicated;
	int num_cpu_shared;
	int num_cp_total;
	int num_cp_dedicated;
	int num_cp_shared;
	int num_ifl_total;
	int num_ifl_dedicated;
	int num_ifl_shared;
};

struct qc_zvm_guest_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	char layer_name[9];
	char system_identifier[9];
	char cluster_name[9];
	char control_program_id[17];
	int adjustment;
	char capping[5];
	int capping_num;
	int num_cpu_total;
	int num_cpu_configured;
	int num_cpu_standby;
	int num_cpu_reserved;
	int num_cpu_dedicated;
	int num_cpu_shared;
	int num_cp_total;
	int num_cp_dedicated;
	int num_cp_shared;
	int num_ifl_total;
	int num_ifl_dedicated;
	int num_ifl_shared;
	int mobility_eligible;
	int has_multiple_cpu_types;
	int cp_dispatch_limithard;
	int ifl_dispatch_limithard;
	int cp_dispatch_type;
	int ifl_dispatch_type;
	int cp_capped_capacity;
	int ifl_capped_capacity;
	char pool_name[9];
	int pool_cp_limithard_cap;
	int pool_cp_capacity_cap;
	int pool_ifl_limithard_cap;
	int pool_ifl_capacity_cap;
	int pool_cp_capped_capacity;
	int pool_ifl_capped_capacity;
	int hardlimit_consumption;
	int hyp_cpu_total;
	int hyp_cpu_dedicated;
	int hyp_cpu_shared;
	int hyp_cp_total;
	int hyp_cp_dedicated;
	int hyp_cp_shared;
	int hyp_ifl_total;
	int hyp_ifl_dedicated;
	int hyp_ifl_shared;
};

struct qc_kvm_hypervisor_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	char control_program_id[17];
	int adjustment;
};

struct qc_kvm_guest_values {
	int layer_type_num;
	int layer_category_num;
	char layer_type[QC_LAYER_TYPE_LEN];
	char layer_category[QC_LAYER_CAT_LEN];
	char layer_name[9];
	char layer_extended_name[257];
	char layer_uuid[37];
	int num_cpu_total;
	int num_cpu_configured;
	int num_cpu_standby;
	int num_cpu_reserved;
};

enum qc_data_type {
	string,
	integer,
	floatingpoint
};

struct qc_attr {
	enum qc_attr_id id;
	enum qc_data_type type;
	int offset;
};

static struct qc_attr cec_attrs[]  =  {
	{qc_layer_type_num, integer, offsetof(struct qc_cec_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_cec_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_cec_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_cec_values, layer_category)},
	{qc_layer_name, string, offsetof(struct qc_cec_values, layer_name)},
	{qc_manufacturer, string, offsetof(struct qc_cec_values, manufacturer)},
	{qc_type, string, offsetof(struct qc_cec_values, type)},
	{qc_model_capacity, string, offsetof(struct qc_cec_values, model_capacity)},
	{qc_model, string, offsetof(struct qc_cec_values, model)},
	{qc_sequence_code, string, offsetof(struct qc_cec_values, sequence_code)},
	{qc_plant, string, offsetof(struct qc_cec_values, plant)},
	{qc_num_cpu_total, integer, offsetof(struct qc_cec_values, num_cpu_total)},
	{qc_num_cpu_configured, integer, offsetof(struct qc_cec_values, num_cpu_configured)},
	{qc_num_cpu_standby, integer, offsetof(struct qc_cec_values, num_cpu_standby)},
	{qc_num_cpu_reserved, integer, offsetof(struct qc_cec_values, num_cpu_reserved)},
	{qc_num_cpu_dedicated, integer, offsetof(struct qc_cec_values, num_cpu_dedicated)},
	{qc_num_cpu_shared, integer, offsetof(struct qc_cec_values, num_cpu_shared)},
	{qc_num_cp_total, integer, offsetof(struct qc_cec_values, num_cp_total)},
	{qc_num_cp_dedicated, integer, offsetof(struct qc_cec_values, num_cp_dedicated)},
	{qc_num_cp_shared, integer, offsetof(struct qc_cec_values, num_cp_shared)},
	{qc_num_ifl_total, integer, offsetof(struct qc_cec_values, num_ifl_total)},
	{qc_num_ifl_dedicated, integer, offsetof(struct qc_cec_values, num_ifl_dedicated)},
	{qc_num_ifl_shared, integer, offsetof(struct qc_cec_values, num_ifl_shared)},
	{qc_capability, floatingpoint, offsetof(struct qc_cec_values, capability)},
	{qc_secondary_capability, floatingpoint, offsetof(struct qc_cec_values, secondary_capability)},
	{qc_capacity_adjustment_indication, integer, offsetof(struct qc_cec_values, capacity_adjustment_indication)},
	{qc_capacity_change_reason, integer, offsetof(struct qc_cec_values, capacity_change_reason)},
	{-1, string, -1}
};

static struct qc_attr lpar_attrs[] = {
	{qc_layer_type_num, integer, offsetof(struct qc_lpar_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_lpar_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_lpar_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_lpar_values, layer_category)},
	{qc_partition_number, integer, offsetof(struct qc_lpar_values, partition_number)},
	{qc_partition_char, string, offsetof(struct qc_lpar_values, partition_char)},
	{qc_partition_char_num, integer, offsetof(struct qc_lpar_values, partition_char_num)},
	{qc_layer_name, string, offsetof(struct qc_lpar_values, layer_name)},
	{qc_adjustment, integer, offsetof(struct qc_lpar_values, adjustment)},
	{qc_num_cpu_total, integer, offsetof(struct qc_lpar_values, num_cpu_total)},
	{qc_num_cpu_configured, integer, offsetof(struct qc_lpar_values, num_cpu_configured)},
	{qc_num_cpu_standby, integer, offsetof(struct qc_lpar_values, num_cpu_standby)},
	{qc_num_cpu_reserved, integer, offsetof(struct qc_lpar_values, num_cpu_reserved)},
	{qc_num_cpu_dedicated, integer, offsetof(struct qc_lpar_values, num_cpu_dedicated)},
	{qc_num_cpu_shared, integer, offsetof(struct qc_lpar_values, num_cpu_shared)},
	{qc_num_cp_total, integer, offsetof(struct qc_lpar_values, num_cp_total)},
	{qc_num_cp_dedicated, integer, offsetof(struct qc_lpar_values, num_cp_dedicated)},
	{qc_num_cp_shared, integer, offsetof(struct qc_lpar_values, num_cp_shared)},
	{qc_num_ifl_total, integer, offsetof(struct qc_lpar_values, num_ifl_total)},
	{qc_num_ifl_dedicated, integer, offsetof(struct qc_lpar_values, num_ifl_dedicated)},
	{qc_num_ifl_shared, integer, offsetof(struct qc_lpar_values, num_ifl_shared)},
	{qc_cp_absolute_capping, integer, offsetof(struct qc_lpar_values, cp_absolute_capping)},
	{qc_ifl_absolute_capping, integer, offsetof(struct qc_lpar_values, ifl_absolute_capping)},
	{qc_cp_weight_capping, integer, offsetof(struct qc_lpar_values, cp_weight_capping)},
	{qc_ifl_weight_capping, integer, offsetof(struct qc_lpar_values, ifl_weight_capping)},
	{-1, string, -1}
};

static struct qc_attr zvm_hv_attrs[] = {
	{qc_layer_type_num, integer, offsetof(struct qc_zvm_hypervisor_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_zvm_hypervisor_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_zvm_hypervisor_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_zvm_hypervisor_values, layer_category)},
	{qc_layer_name, string, offsetof(struct qc_zvm_hypervisor_values, layer_name)},
	{qc_cluster_name, string, offsetof(struct qc_zvm_hypervisor_values, cluster_name)},
	{qc_control_program_id, string, offsetof(struct qc_zvm_hypervisor_values, control_program_id)},
	{qc_adjustment, integer, offsetof(struct qc_zvm_hypervisor_values, adjustment)},
	{qc_hardlimit_consumption, integer, offsetof(struct qc_zvm_hypervisor_values, hardlimit_consumption)},
	{qc_num_cpu_total, integer, offsetof(struct qc_zvm_hypervisor_values, num_cpu_total)},
	{qc_num_cpu_dedicated, integer, offsetof(struct qc_zvm_hypervisor_values, num_cpu_dedicated)},
	{qc_num_cpu_shared, integer, offsetof(struct qc_zvm_hypervisor_values, num_cpu_shared)},
	{qc_num_cp_total, integer, offsetof(struct qc_zvm_hypervisor_values, num_cp_total)},
	{qc_num_cp_dedicated, integer, offsetof(struct qc_zvm_hypervisor_values, num_cp_dedicated)},
	{qc_num_cp_shared, integer, offsetof(struct qc_zvm_hypervisor_values, num_cp_shared)},
	{qc_num_ifl_total, integer, offsetof(struct qc_zvm_hypervisor_values, num_ifl_total)},
	{qc_num_ifl_dedicated, integer, offsetof(struct qc_zvm_hypervisor_values, num_ifl_dedicated)},
	{qc_num_ifl_shared, integer, offsetof(struct qc_zvm_hypervisor_values, num_ifl_shared)},
	{-1, string, -1}
};

static struct qc_attr kvm_hv_attrs[] = {
	{qc_layer_type_num, integer, offsetof(struct qc_kvm_hypervisor_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_kvm_hypervisor_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_kvm_hypervisor_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_kvm_hypervisor_values, layer_category)},
	{qc_control_program_id, string, offsetof(struct qc_kvm_hypervisor_values, control_program_id)},
	{qc_adjustment, integer, offsetof(struct qc_kvm_hypervisor_values, adjustment)},
	{-1, string, -1}
};

static struct qc_attr zvm_pool_attrs[] = {
	{qc_layer_type_num, integer, offsetof(struct qc_zvm_pool_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_zvm_pool_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_zvm_pool_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_zvm_pool_values, layer_category)},
	{qc_layer_name, string, offsetof(struct qc_zvm_pool_values, layer_name)},
	{qc_cp_limithard_cap, integer, offsetof(struct qc_zvm_pool_values, cp_limithard_cap)},
	{qc_cp_capacity_cap, integer, offsetof(struct qc_zvm_pool_values, cp_capacity_cap)},
	{qc_ifl_limithard_cap, integer, offsetof(struct qc_zvm_pool_values, ifl_limithard_cap)},
	{qc_ifl_capacity_cap, integer, offsetof(struct qc_zvm_pool_values, ifl_capacity_cap)},
	{qc_cp_capped_capacity, integer, offsetof(struct qc_zvm_pool_values, cp_capped_capacity)},
	{qc_ifl_capped_capacity, integer, offsetof(struct qc_zvm_pool_values, ifl_capped_capacity)},
	{-1, string, -1}
};

static struct qc_attr zvm_guest_attrs[] = {
	{qc_layer_type_num, integer, offsetof(struct qc_zvm_guest_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_zvm_guest_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_zvm_guest_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_zvm_guest_values, layer_category)},
	{qc_layer_name, string, offsetof(struct qc_zvm_guest_values, layer_name)},
	{qc_capping, string, offsetof(struct qc_zvm_guest_values, capping)},
	{qc_capping_num, integer, offsetof(struct qc_zvm_guest_values, capping_num)},
	{qc_num_cpu_total, integer, offsetof(struct qc_zvm_guest_values, num_cpu_total)},
	{qc_num_cpu_configured, integer, offsetof(struct qc_zvm_guest_values, num_cpu_configured)},
	{qc_num_cpu_standby, integer, offsetof(struct qc_zvm_guest_values, num_cpu_standby)},
	{qc_num_cpu_reserved, integer, offsetof(struct qc_zvm_guest_values, num_cpu_reserved)},
	{qc_num_cpu_dedicated, integer, offsetof(struct qc_zvm_guest_values, num_cpu_dedicated)},
	{qc_num_cpu_shared, integer, offsetof(struct qc_zvm_guest_values, num_cpu_shared)},
	{qc_num_cp_total, integer, offsetof(struct qc_zvm_guest_values, num_cp_total)},
	{qc_num_cp_dedicated, integer, offsetof(struct qc_zvm_guest_values, num_cp_dedicated)},
	{qc_num_cp_shared, integer, offsetof(struct qc_zvm_guest_values, num_cp_shared)},
	{qc_num_ifl_total, integer, offsetof(struct qc_zvm_guest_values, num_ifl_total)},
	{qc_num_ifl_dedicated, integer, offsetof(struct qc_zvm_guest_values, num_ifl_dedicated)},
	{qc_num_ifl_shared, integer, offsetof(struct qc_zvm_guest_values, num_ifl_shared)},
	{qc_mobility_eligible, integer, offsetof(struct qc_zvm_guest_values, mobility_eligible)},
	{qc_has_multiple_cpu_types, integer, offsetof(struct qc_zvm_guest_values, has_multiple_cpu_types)},
	{qc_cp_dispatch_limithard, integer, offsetof(struct qc_zvm_guest_values, cp_dispatch_limithard)},
	{qc_cp_capped_capacity, integer, offsetof(struct qc_zvm_guest_values, cp_capped_capacity)},
	{qc_ifl_dispatch_limithard, integer, offsetof(struct qc_zvm_guest_values, ifl_dispatch_limithard)},
	{qc_ifl_capped_capacity, integer, offsetof(struct qc_zvm_guest_values, ifl_capped_capacity)},
	{qc_cp_dispatch_type, integer, offsetof(struct qc_zvm_guest_values, cp_dispatch_type)},
	{qc_ifl_dispatch_type, integer, offsetof(struct qc_zvm_guest_values, ifl_dispatch_type)},
	{-1, string, -1}
};

static struct qc_attr kvm_guest_attrs[] = {
	{qc_layer_type_num, integer, offsetof(struct qc_kvm_guest_values, layer_type_num)},
	{qc_layer_category_num, integer, offsetof(struct qc_kvm_guest_values, layer_category_num)},
	{qc_layer_type, string, offsetof(struct qc_kvm_guest_values, layer_type)},
	{qc_layer_category, string, offsetof(struct qc_kvm_guest_values, layer_category)},
	{qc_layer_name, string, offsetof(struct qc_kvm_guest_values, layer_name)},
	{qc_layer_extended_name, string, offsetof(struct qc_kvm_guest_values, layer_extended_name)},
	{qc_layer_uuid, string, offsetof(struct qc_kvm_guest_values, layer_uuid)},
	{qc_num_cpu_total, integer, offsetof(struct qc_kvm_guest_values, num_cpu_total)},
	{qc_num_cpu_configured, integer, offsetof(struct qc_kvm_guest_values, num_cpu_configured)},
	{qc_num_cpu_standby, integer, offsetof(struct qc_kvm_guest_values, num_cpu_standby)},
	{qc_num_cpu_reserved, integer, offsetof(struct qc_kvm_guest_values, num_cpu_reserved)},
	{-1, string, -1}
};


const char *qc_attr_id_to_char(struct qc_handle *hdl, enum qc_attr_id id) {
	switch (id) {
	case qc_layer_type_num: return "layer_type_num";
	case qc_layer_category_num: return "layer_category_num";
	case qc_layer_type: return "layer_type";
	case qc_layer_category: return "layer_category";
	case qc_layer_name: return "layer_name";
	case qc_layer_extended_name: return "layer_extended_name";
	case qc_layer_uuid: return "layer_uuid";
	case qc_manufacturer: return "manufacturer";
	case qc_type: return "type";
	case qc_model_capacity: return "model_capacity";
	case qc_model: return "model";
	case qc_sequence_code: return "sequence_code";
	case qc_plant: return "plant";
	case qc_num_cpu_total: return "num_cpu_total";
	case qc_num_cpu_configured: return "num_cpu_configured";
	case qc_num_cpu_standby: return "num_cpu_standby";
	case qc_num_cpu_reserved: return "num_cpu_reserved";
	case qc_num_cpu_dedicated: return "num_cpu_dedicated";
	case qc_num_cpu_shared: return "num_cpu_shared";
	case qc_num_cp_total: return "num_cp_total";
	case qc_num_cp_dedicated: return "num_cp_dedicated";
	case qc_num_cp_shared: return "num_cp_shared";
	case qc_num_ifl_total: return "num_ifl_total";
	case qc_num_ifl_dedicated: return "num_ifl_dedicated";
	case qc_num_ifl_shared: return "num_ifl_shared";
	case qc_capability: return "capability";
	case qc_secondary_capability: return "secondary_capability";
	case qc_capacity_adjustment_indication: return "capacity_adjustment_indication";
	case qc_capacity_change_reason: return "capacity_change_reason";
	case qc_partition_number: return "partition_number";
	case qc_partition_char: return "partition_char";
	case qc_partition_char_num: return "partition_char_num";
	case qc_adjustment: return "adjustment";
	case qc_cp_absolute_capping: return "cp_absolute_capping";
	case qc_ifl_absolute_capping: return "ifl_absolute_capping";
	case qc_cp_weight_capping: return "cp_weight_capping";
	case qc_ifl_weight_capping: return "ifl_weight_capping";
	case qc_cluster_name: return "cluster_name";
	case qc_control_program_id: return "control_program_id";
	case qc_hardlimit_consumption: return "hardlimit_consumption";
	case qc_cp_limithard_cap: return "pool_cp_limithard_cap";
	case qc_cp_capacity_cap: return "pool_cp_capacity_cap";
	case qc_ifl_limithard_cap: return "pool_ifl_limithard_cap";
	case qc_ifl_capacity_cap: return "pool_ifl_capacity_cap";
	case qc_capping: return "capping";
	case qc_capping_num: return "capping_num";
	case qc_mobility_eligible: return "mobility_eligible";
	case qc_has_multiple_cpu_types: return "has_multiple_cpu_types";
	case qc_cp_dispatch_limithard: return "cp_dispatch_limithard";
	case qc_ifl_dispatch_limithard: return "ifl_dispatch_limithard";
	case qc_cp_dispatch_type: return "cp_dispatch_type";
	case qc_ifl_dispatch_type: return "ifl_dispatch_type";
	case qc_cp_capped_capacity: return "cp_capped_capacity";
	case qc_ifl_capped_capacity: return "ifl_capped_capacity";
	default: break;
	}
	qc_debug(hdl, "Error: Cannot convert unknown attribute '%d' to char*\n", id);

	return NULL;
}

// 'hdl' is for error reporting, as 'tgthdl' might not be part of the pointer lists yet
int qc_new_handle(struct qc_handle *hdl, struct qc_handle **tgthdl, int layer_no,
		  int layer_type_num) {
	int num_attrs, layer_category_num;
	char *layer_type, *layer_category;
	struct qc_attr *attrs;
	size_t layer_sz;

	switch (layer_type_num) {
	case QC_LAYER_TYPE_CEC:
		layer_sz = sizeof(struct qc_cec_values);
		attrs = cec_attrs;
		layer_category_num = QC_LAYER_CAT_HOST;
		layer_category = "HOST";
		layer_type = "CEC";
		break;
	case QC_LAYER_TYPE_LPAR:
		layer_sz = sizeof(struct qc_lpar_values);
		attrs = lpar_attrs;
		layer_category_num = QC_LAYER_CAT_GUEST;
		layer_category = "GUEST";
		layer_type = "LPAR";
		break;
	case QC_LAYER_TYPE_ZVM_HYPERVISOR:
		layer_sz = sizeof(struct qc_zvm_hypervisor_values);
		attrs = zvm_hv_attrs;
		layer_category_num = QC_LAYER_CAT_HOST;
		layer_category = "HOST";
		layer_type = "z/VM-hypervisor";
		break;
	case QC_LAYER_TYPE_ZVM_CPU_POOL:
		layer_sz = sizeof(struct qc_zvm_pool_values);
		attrs = zvm_pool_attrs;
		layer_category_num = QC_LAYER_CAT_POOL;
		layer_category = "POOL";
		layer_type = "z/VM-CPU-pool";
		break;
	case QC_LAYER_TYPE_ZVM_GUEST:
		layer_sz = sizeof(struct qc_zvm_guest_values);
		attrs = zvm_guest_attrs;
		layer_category_num = QC_LAYER_CAT_GUEST;
		layer_category = "GUEST";
		layer_type = "z/VM-guest";
		break;
	case QC_LAYER_TYPE_KVM_HYPERVISOR:
		layer_sz = sizeof(struct qc_kvm_hypervisor_values);
		attrs = kvm_hv_attrs;
		layer_category_num = QC_LAYER_CAT_HOST;
		layer_category = "HOST";
		layer_type = "KVM-hypervisor";
		break;
	case QC_LAYER_TYPE_KVM_GUEST:
		layer_sz = sizeof(struct qc_kvm_guest_values);
		attrs = kvm_guest_attrs;
		layer_category_num = QC_LAYER_CAT_GUEST;
		layer_category = "GUEST";
		layer_type =  "KVM-guest";
		break;
	default:
		qc_debug(hdl, "Error: Unhandled layer type in qc_new_handle()\n");
		return -1;
	}

	// determine number of attributes
	for (num_attrs = 0; attrs[num_attrs].offset >= 0; ++num_attrs);
	num_attrs++;

	if (hdl || *tgthdl == NULL) {
		// Possibly reuse existing handle when alloc'ing the cec layer.
		// Otherwise we'd change the handle which serves as an identified in
		// our log output, which could be confusing.
		*tgthdl = malloc(sizeof(struct qc_handle));
		if (!*tgthdl) {
			qc_debug(hdl, "Error: Failed to allocate handle\n");
			return -2;
		}
	}
	memset(*tgthdl, 0, sizeof(struct qc_handle));
	(*tgthdl)->layer_no = layer_no;
	(*tgthdl)->attr_list = attrs;
	if (hdl)
		(*tgthdl)->root = hdl->root;
	else
		(*tgthdl)->root = *tgthdl;
	(*tgthdl)->layer = malloc(layer_sz);
	if (!(*tgthdl)->layer) {
		qc_debug(hdl, "Error: Failed to allocate layer\n");
		free(*tgthdl);
		*tgthdl = NULL;
		return -3;
	}
	memset((*tgthdl)->layer, 0, layer_sz);
	(*tgthdl)->attr_present = calloc(num_attrs, sizeof(int));
	(*tgthdl)->src = calloc(num_attrs, sizeof(int));
	if (!(*tgthdl)->attr_present || !(*tgthdl)->src) {
		qc_debug(hdl, "Error: Failed to allocate attr_present array\n");
		free((*tgthdl)->layer);
		free(*tgthdl);
		*tgthdl = NULL;
		return -4;
	}
	if (qc_set_attr_int(*tgthdl, qc_layer_type_num, layer_type_num, ATTR_SRC_UNDEF) ||
	    qc_set_attr_int(*tgthdl, qc_layer_category_num, layer_category_num, ATTR_SRC_UNDEF) ||
	    qc_set_attr_string(*tgthdl, qc_layer_type, layer_type, strlen(layer_type), ATTR_SRC_UNDEF) ||
	    qc_set_attr_string(*tgthdl, qc_layer_category, layer_category, strlen(layer_category), ATTR_SRC_UNDEF))
		return -5;

	return 0;
}

static char *qc_set_attr(struct qc_handle *hdl, enum qc_attr_id id, enum qc_data_type type, char src) {
	struct qc_attr *attr_list = hdl->attr_list;
	int count;

	for (count = 0; attr_list[count].offset >= 0; ++count) {
		if (attr_list[count].id == id && attr_list[count].type == type) {
			hdl->attr_present[count] = 1;
			hdl->src[count] = src;
			return (char *)hdl->layer + attr_list[count].offset;
		}
	}
	qc_debug(hdl, "Error: Failed to set attr=%s\n", qc_attr_id_to_char(hdl, id));

	return NULL;
}

// Sets attribute 'id' in layer as pointed to by 'hdl'
int qc_set_attr_int(struct qc_handle *hdl, enum qc_attr_id id, int val, char src) {
	int *ptr;

	if ((ptr = (int *)qc_set_attr(hdl, id, integer, src)) == NULL)
		return -1;
	*ptr = val;

	return 0;
}

// Sets attribute 'id' in layer as pointed to by 'hdl'
int qc_set_attr_float(struct qc_handle *hdl, enum qc_attr_id id, float val, char src) {
	float *ptr;

	if ((ptr = (float *)qc_set_attr(hdl, id, floatingpoint, src)) == NULL)
		return -1;
	*ptr = val;

	return 0;
}

// Sets attribute 'id' in layer as pointed to by 'hdl'
int qc_set_attr_string(struct qc_handle *hdl, enum qc_attr_id id, char *str, unsigned int str_len, char src) {
	char *ptr;

	if ((ptr = qc_set_attr(hdl, id, string, src)) == NULL)
		return -1;
	ptr[str_len] = '\0';
	strncpy(ptr, str, str_len);

	return 0;
}

// Sets attribute 'id' in layer as pointed to by 'hdl'
int qc_set_attr_ebcdic_string(struct qc_handle *hdl, enum qc_attr_id id, unsigned char *str,
			      unsigned int str_len, iconv_t *cd, char src) {
	char *buf;

	if (qc_set_attr_string(hdl, id, (char *)str, str_len, src))
		return -1;
	buf = qc_get_attr_value_string(hdl, id);

	return qc_ebcdic_to_ascii(hdl, cd, buf, str_len);
}

// Certain parts assume that empty strings might also consist of spaces
// Returns >0 if not empty, 0 if empty, and <0 for errors
int qc_is_nonempty_ebcdic(struct qc_handle *hdl, const unsigned char *buf, unsigned int buflen, iconv_t *cd) {
	char str[9];	// suffices for all users of this function

	if (*buf == '\0')
		return 0;
	if (buflen > sizeof(str)) {
		qc_debug(hdl, "Error: Insufficient static buffer length\n");
		return -1;
	}
	memcpy(str, buf, buflen);
	if (qc_ebcdic_to_ascii(hdl, cd, str, sizeof(str)))
		return -2;

	return *str != '\0';
}

// Sets attribute 'id' in layer as pointed to by 'hdl'
static int qc_is_attr_set(struct qc_handle *hdl, enum qc_attr_id id, enum qc_data_type type, char *src) {
	struct qc_attr *attr_list = hdl->attr_list;
	int count = 0;

	while (attr_list[count].offset >= 0) {
		if (attr_list[count].id == id && attr_list[count].type == type) {
			*src = hdl->src[count];
			return hdl->attr_present[count];
		}
		count++;
	}

	return 0;
}

int qc_is_attr_set_int(struct qc_handle *hdl, enum qc_attr_id id, char *src) {
	return qc_is_attr_set(hdl, id, integer, src);
}

int qc_is_attr_set_float(struct qc_handle *hdl, enum qc_attr_id id, char *src) {
	return qc_is_attr_set(hdl, id, floatingpoint, src);
}

int qc_is_attr_set_string(struct qc_handle *hdl, enum qc_attr_id id, char *src) {
	return qc_is_attr_set(hdl, id, string, src);
}

struct qc_handle *qc_get_root_handle(struct qc_handle *hdl) {
	return hdl ? hdl->root : hdl;
}

struct qc_handle *qc_get_prev_handle(struct qc_handle *hdl) {
	struct qc_handle *prev_hdl = NULL;

	for (prev_hdl = hdl->root; prev_hdl->next != NULL; prev_hdl = prev_hdl->next)
		if (prev_hdl->next == hdl)
			return prev_hdl;
	qc_debug(hdl, "Error: Couldn't find handle pointing at layer %d handle\n", hdl->layer_no);

	return NULL;
}

/// Retrieve value of attribute 'id' of layer pointed at by 'hdl'
static void *qc_get_attr_value(struct qc_handle *hdl, enum qc_attr_id id, enum qc_data_type type) {
	struct qc_attr *attr_list = hdl->attr_list;
	int count;

	for (count = 0; attr_list[count].offset >= 0; ++count) {
		if (attr_list[count].id == id && attr_list[count].type == type) {
			if (!hdl->attr_present[count])
				return NULL;
			return (char *)hdl->layer + attr_list[count].offset;
		}
	}

	return NULL;
}

int *qc_get_attr_value_int(struct qc_handle *hdl, enum qc_attr_id id) {
	return (int *)qc_get_attr_value(hdl, id, integer);
}

float *qc_get_attr_value_float(struct qc_handle *hdl, enum qc_attr_id id) {
	return (float *)qc_get_attr_value(hdl, id, floatingpoint);
}

char *qc_get_attr_value_string(struct qc_handle *hdl, enum qc_attr_id id) {
	return (char *)qc_get_attr_value(hdl, id, string);
}

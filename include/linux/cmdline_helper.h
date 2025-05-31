#ifndef CMDLINE_HELPER_H
#define CMDLINE_HELPER_H

#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/moduleparam.h>

enum cmdline_value_type {
	CMDLINE_TYPE_INT,
	CMDLINE_TYPE_BOOL,
	CMDLINE_TYPE_STRING,
};

struct cmdline_param_map {
	const char *param_name;	   // The parameter name on the command line (after the prefix).
	void *target_var_ptr;		 // Pointer to the module's internal variable to update.
	enum cmdline_value_type type; // The data type of the target variable.
	size_t string_max_len;		// For CMDLINE_TYPE_STRING, max length of the char array buffer
								  // pointed to by target_var_ptr.
};

/*
 * @brief Parses kernel command line parameters with a given prefix into C variables.
 *
 * Scans the command line for "prefix.name=value" patterns. If 'name' matches
 * an entry in @map, its 'value' is parsed and stored in the corresponding
 * C variable pointed to by @map.target_var_ptr.
 *
 * @param log_source_id Identifier for log messages (e.g., "my_driver").
 * @param cmdline_target_prefix The expected prefix (e.g., "vendor.display."). "" for no prefix.
 * @param map Array defining which 'name's to look for and where to store values.
 * @param map_size Number of entries in @map.
 */
void parse_cmdline_params(
	const char *log_source_id,
	const char *cmdline_target_prefix,
	const struct cmdline_param_map *map,
	int map_size
);

#endif // CMDLINE_HELPER_H
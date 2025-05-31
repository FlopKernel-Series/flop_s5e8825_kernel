#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/moduleparam.h>

#include <linux/cmdline_helper.h>

/**
 * parse_cmdline_params - Parses parameters from the kernel command line
 * and assigns to variable map.
 */
void parse_cmdline_params(
	const char *log_source_id,
	const char *cmdline_target_prefix,
	const struct cmdline_param_map *map,
	int map_size)
{
	char *cmdline_copy = NULL;
	char *temp_ptr, *token;
	size_t prefix_len;

	if (!log_source_id || !cmdline_target_prefix || !map) {
		pr_err("Invalid arguments to parse_cmdline_params\n");
		return;
	}
	prefix_len = strlen(cmdline_target_prefix);

	cmdline_copy = kstrdup(saved_command_line, GFP_KERNEL);
	if (!cmdline_copy) {
		pr_err("%s: Failed to kstrdup saved_command_line for parsing\n", log_source_id);
		return;
	}
	temp_ptr = cmdline_copy;

	pr_info("%s: Checking for command line parameters with prefix '%s'\n", log_source_id, cmdline_target_prefix);

	while ((token = strsep(&temp_ptr, " ")) != NULL) {
		if (strlen(token) >= prefix_len &&
			strncmp(token, cmdline_target_prefix, prefix_len) == 0) {
			
			char *param_and_val = token + prefix_len;
			char *equals_ptr = strchr(param_and_val, '=');
			char *value_str;
			size_t name_len;
			int i;

			if (!equals_ptr) {
				// Not "name=value", could be a boolean flag without "=value" or just a keyword.
				// Current logic expects "name=value".
				continue;
			}

			name_len = equals_ptr - param_and_val;
			value_str = equals_ptr + 1;

			for (i = 0; i < map_size; i++) {
				// Using map[i].param_name now
				if (strlen(map[i].param_name) == name_len &&
					map[i].target_var_ptr && 
					strncmp(param_and_val, map[i].param_name, name_len) == 0) {
					
					int ret = 0;

					// Using map[i].type (which is enum cmdline_value_type)
					switch (map[i].type) {
					case CMDLINE_TYPE_INT:
						ret = kstrtoint(value_str, 0, (int *)map[i].target_var_ptr);
						if (ret == 0) {
							pr_info("%s: Applied '%s%s' = %d\n", log_source_id, cmdline_target_prefix, map[i].param_name, *(int *)map[i].target_var_ptr);
						}
						break;
					case CMDLINE_TYPE_BOOL:
						ret = kstrtobool(value_str, (bool *)map[i].target_var_ptr);
						if (ret == 0) {
							pr_info("%s: Applied '%s%s' = %s\n", log_source_id, cmdline_target_prefix, map[i].param_name, *(bool *)map[i].target_var_ptr ? "true" : "false");
						}
						break;
					case CMDLINE_TYPE_STRING:
						if (map[i].string_max_len > 0) {
							strncpy((char *)map[i].target_var_ptr, value_str, map[i].string_max_len - 1);
							((char*)map[i].target_var_ptr)[map[i].string_max_len - 1] = '\0'; // Ensure null termination
							pr_info("%s: Applied '%s%s' = \"%s\"\n", log_source_id, cmdline_target_prefix, map[i].param_name, (char *)map[i].target_var_ptr);
						} else {
							pr_warn("%s: String param %s%s has no buffer defined (string_max_len=0)\n", log_source_id, cmdline_target_prefix, map[i].param_name);
							ret = -EINVAL;
						}
						break;
					default:
						pr_warn("%s: Unknown type for param %s%s\n", log_source_id, cmdline_target_prefix, map[i].param_name);
						ret = -EINVAL;
					}

					if (ret != 0) {
						pr_warn("%s: Failed to parse value for param %s%s (value: '%s', error: %d)\n",
								log_source_id, cmdline_target_prefix, map[i].param_name, value_str, ret);
					}
					goto next_token; // Found and processed, move to next cmdline token
				}
			}
		}
next_token:;
	}

	kfree(cmdline_copy);
}
EXPORT_SYMBOL_GPL(parse_cmdline_params);

static int __init cmdline_helper_module_init(void)
{
	// Just initialize it to make its symbols available.
	pr_info("Command Line Helper initialized.\n");
	return 0;
}

static void __exit cmdline_helper_module_exit(void)
{
	pr_info("Command Line Helper exited.\n");
}

module_init(cmdline_helper_module_init);
module_exit(cmdline_helper_module_exit);

MODULE_AUTHOR("Flopster101 <nahuelgomez329@gmail.com>");
MODULE_DESCRIPTION("Kernel Command Line Parameter Parsing Helper");
MODULE_LICENSE("GPL");

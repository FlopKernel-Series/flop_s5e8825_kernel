#ifndef __SHUB_PRESSURE_FACTORY_H_
#define __SHUB_PRESSURE_FACTORY_H_

#include <linux/device.h>
#include <linux/types.h>


struct pressure_factory_chipset_funcs {
	ssize_t (*temperature_show)(char *name);
	ssize_t (*esn_show)(char *name);
};

struct pressure_factory_chipset_funcs *get_pressure_bmp580_chipset_func(char *name);
struct pressure_factory_chipset_funcs *get_pressure_lps22df_chipset_func(char *name);
struct pressure_factory_chipset_funcs *get_pressure_lps22hh_chipset_func(char *name);
struct pressure_factory_chipset_funcs *get_pressure_lps25h_chipset_func(char *name);

#endif

#include "../../pressure.h"
#include "../../../utility/shub_utility.h"
#include "../../../sensormanager/shub_sensor_type.h"
#include "../../../sensormanager/shub_sensor_manager.h"
#include "../../../sensormanager/shub_sensor.h"

#define LPS22HH_NAME	"LPS22HHTR"

static int init_pressure_lps22hh(int type)
{
	struct pressure_data *data = (struct pressure_data *)(get_sensor(type)->data);

	shub_infof("");

	data->convert_coef = 4096;

	return 0;
}


struct sensor_chipset_init_funcs pressure_lps22hh_func = {
	.init = init_pressure_lps22hh,
};

struct sensor_chipset_init_funcs *get_pressure_lps22hh_function_pointer(char *name)
{
	if (strcmp(name, LPS22HH_NAME) != 0)
		return NULL;

	return &pressure_lps22hh_func;
}

#include "fingerprint_common.h"
#include <linux/of.h>
#include <linux/platform_data/spi-mt65xx.h>


void spi_get_ctrldata(struct spi_device *spi)
{
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	struct device_node *np, *data_np = NULL;
	u32 tckdly = 0;

	np = spi->dev.of_node;
	if (!np) {
		pr_err("%s : device node not founded\n", __func__);
		return;
	}

	data_np = of_get_child_by_name(np, "controller-data");
	if (!data_np) {
		pr_err("%s : controller-data not founded\n", __func__);
		return;
	}

	of_property_read_u32(data_np, "mediatek,tckdly", &tckdly);
	((struct mtk_chip_config *)spi->controller_data)->tick_delay = tckdly;

	of_node_put(data_np);
	pr_info("%s done\n", __func__);
#endif
}

int spi_clk_register(struct spi_clk_setting *clk_setting, struct device *dev)
{
	return 0;
}

int spi_clk_unregister(struct spi_clk_setting *clk_setting)
{
	return 0;
}

int spi_clk_enable(struct spi_clk_setting *clk_setting)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	if (!clk_setting->enabled_clk) {
		__pm_stay_awake(clk_setting->spi_wake_lock);
		clk_setting->enabled_clk = true;
	}
#endif
	return 0;
}

int spi_clk_disable(struct spi_clk_setting *clk_setting)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	if (clk_setting->enabled_clk) {
		__pm_relax(clk_setting->spi_wake_lock);
		clk_setting->enabled_clk = false;
	}
#endif

	return 0;
}

int cpu_speedup_enable(struct boosting_config *boosting)
{
	return 0;
}

int cpu_speedup_disable(struct boosting_config *boosting)
{
	return 0;
}

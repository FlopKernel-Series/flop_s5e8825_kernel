#ifndef __KSU_H_SUS_SU
#define __KSU_H_SUS_SU

#if defined(CONFIG_KSU_NEXT)
#include "../../drivers/kernelsu/kernel/core_hook.h"
#elif defined(CONFIG_KSU_SUKI)
#include "../../drivers/sukisu/kernel/core_hook.h"
#endif

int sus_su_fifo_init(int *maj_dev_num, char *drv_path);
int sus_su_fifo_exit(int *maj_dev_num, char *drv_path);

#endif

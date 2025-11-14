# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux kernel panel device drivers.
#

subdir-ccflags-$(CONFIG_SEC_KUNIT) += \
	-I$(srctree)/$(src)/.. \
	-Wframe-larger-than=8192 \
	-Wno-unused-variable \
	-Wunused-function \
	-Wno-missing-braces \
	-Wno-implicit-fallthrough \
	-Wformat

subdir-ccflags-y += -I$(srctree)/$(src)/../oled_common


subdir-ccflags-y += -I$(srctree)/$(src)/../oled_common

mcd-panel-s6e8fc3_a26x-y += s6e8fc3.o s6e8fc3_a26x.o
obj-$(CONFIG_USDM_PANEL_S6E8FC3) += mcd-panel-s6e8fc3_a26x.o


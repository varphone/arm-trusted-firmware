/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <platform_def.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/mmio.h>

/* Table of regions to map using the MMU.  */
const mmap_region_t plat_mmap[] = {
	/* for TF text, RO, RW */
	MAP_REGION_FLAT(DRAM_NS_BASE, DRAM_NS_SIZE,
			MT_MEMORY | MT_RW | MT_NS),
	MAP_REGION_FLAT(TEE_SEC_MEM_BASE, TEE_SEC_MEM_SIZE,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(DEVICE_BASE, DEVICE_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	{ 0 }

};

/*******************************************************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/

/* Define EL3 variants of the function initialising the MMU */
void plat_configure_mmu_el3(unsigned long total_base,
			    unsigned long total_size,
			    unsigned long ro_start,
			    unsigned long ro_limit,
			    unsigned long coh_start,
			    unsigned long coh_limit)
{
	mmap_add_region(total_base, total_base,
			total_size,
			MT_MEMORY | MT_RW | MT_SECURE);
	mmap_add_region(ro_start, ro_start,
			ro_limit - ro_start,
			MT_MEMORY | MT_RO | MT_SECURE);
	mmap_add_region(coh_start, coh_start,
			coh_limit - coh_start,
			MT_DEVICE | MT_RW | MT_SECURE);
	mmap_add(plat_mmap);
	init_xlat_tables();

	enable_mmu_el3(0);
}

enum sec_attr_ctrl0 {
	SEC_FLAG_GPU    = BIT(0),
	SEC_FLAG_DSP    = BIT(1),
	SEC_FLAG_TDE    = BIT(2),
	SEC_FLAG_GPIO   = BIT(3) | BIT(4) | BIT(5) | BIT(6),
	SEC_FLAG_JPGD   = BIT(7),
	SEC_FLAG_PGD    = BIT(8),
	SEC_FLAG_VEDU   = BIT(10),
	SEC_FLAG_AIAO   = BIT(12),
	SEC_FLAG_GEN    = BIT(13) | BIT(14),
	SEC_FLAG_SHA1   = BIT(15),
	SEC_FLAG_TIMER  = BIT(17) | BIT(18),
	SEC_FLAG_I2C    = BIT(20),
	SEC_FLAG_IR     = BIT(21),
	SEC_FLAG_USB    = BIT(23),
	SEC_FLAG_DEGI2C = BIT(25),
	SEC_FLAG_MCU    = BIT(26) | BIT(27),
	SEC_FLAG_DDRT   = BIT(28),
	SEC_FLAG_VICAP  = BIT(29),
};

#if  DISABLE_TEE == 1

#define VENDOR_SEC_CTRL0_FLAG (SEC_FLAG_DSP  | SEC_FLAG_TDE    | SEC_FLAG_GPIO | SEC_FLAG_JPGD |	\
			     SEC_FLAG_PGD  | SEC_FLAG_VEDU   | SEC_FLAG_AIAO | SEC_FLAG_GEN  |	\
			     SEC_FLAG_SHA1 | SEC_FLAG_TIMER  | SEC_FLAG_I2C  | SEC_FLAG_IR   |	\
			     SEC_FLAG_USB  | SEC_FLAG_DEGI2C | SEC_FLAG_MCU  | SEC_FLAG_DDRT |	\
			     SEC_FLAG_VICAP)
#else

#define VENDOR_SEC_CTRL0_FLAG (SEC_FLAG_DSP | SEC_FLAG_TDE  | SEC_FLAG_GPIO | SEC_FLAG_JPGD   |	\
			     SEC_FLAG_PGD | SEC_FLAG_VEDU | SEC_FLAG_AIAO | SEC_FLAG_SHA1   |	\
			     SEC_FLAG_I2C | SEC_FLAG_IR   |  SEC_FLAG_USB | SEC_FLAG_DEGI2C |	\
			     SEC_FLAG_VICAP)
#endif


unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}


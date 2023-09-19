/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <lib/mmio.h>
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <common/debug.h>
#ifdef FIQ_SUPPORT
#include <plat_private.h>
#endif

/* GIC600-specific register offsets */
#define GICR_PWRR	0x24

/* GICR_PWRR fields */
#define PWRR_RDPD_SHIFT		0
#define PWRR_RDGPD_SHIFT	2
#define PWRR_RDGPO_SHIFT	3

#define PWRR_RDGPD	(1 << PWRR_RDGPD_SHIFT)
#define PWRR_RDGPO	(1 << PWRR_RDGPO_SHIFT)

/* Values to write to GICR_PWRR register to power redistributor */
#define PWRR_ON		(0 << PWRR_RDPD_SHIFT)
#define PWRR_OFF	(1 << PWRR_RDPD_SHIFT)

/* GIC600-specific accessor functions */
static void gicr_write_pwrr(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICR_PWRR, val);
}

static uint32_t gicr_read_pwrr(uintptr_t base)
{
	return mmio_read_32(base + GICR_PWRR);
}

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 * the way the GICv3 driver is initialised and used.
 *****************************************************************************/
#pragma weak plat_gic_driver_init
#pragma weak plat_gic_init
#pragma weak plat_gic_cpuif_enable
#pragma weak plat_gic_cpuif_disable
#pragma weak plat_gic_pcpu_init

/* The GICv3 driver only needs to be initialized in EL3 */
uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];
#ifdef FIQ_SUPPORT
static const interrupt_prop_t g_g0_interrupt_props[] = {
	INTR_PROP_DESC(PERI_WDT5_S, GIC_HIGHEST_SEC_PRIORITY, INTR_GROUP0, GIC_INTR_CFG_LEVEL),
	INTR_PROP_DESC(FIQ_SGI_S, GIC_HIGHEST_SEC_PRIORITY, INTR_GROUP0, GIC_INTR_CFG_LEVEL),
};
#endif

const gicv3_driver_data_t gic_data = {
	.gicd_base = PLAT_VENDOR_GICD_BASE,
	.gicr_base = PLAT_VENDOR_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
#ifdef FIQ_SUPPORT
	.interrupt_props = g_g0_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(g_g0_interrupt_props),
#else
	.interrupt_props_num = 0,
#endif
};

void plat_gic_driver_init(void)
{
	/*
	 * The GICv3 driver is initialized in EL3 and does not need
	 * to be initialized again in SEL1. This is because the S-EL1
	 * can use GIC system registers to manage interrupts and does
	 * not need GIC interface base addresses to be configured.
	 */
#if IMAGE_BL31
	gicv3_driver_init(&gic_data);
#endif
}

/******************************************************************************
 * ARM common helper to initialize the GIC. Only invoked by BL31
 *****************************************************************************/
void plat_gic_init(void)
{
	unsigned int cpu = plat_my_core_pos();
	/* Power on redistributor */
	gicr_write_pwrr(PLAT_VENDOR_GICR_BASE + 0x20000 * cpu, PWRR_ON);

	/* Wait until the power on state is reflected */
	while (gicr_read_pwrr(PLAT_VENDOR_GICR_BASE + 0x20000 * cpu) & PWRR_RDGPO);

	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());
}
#ifdef FIQ_SUPPORT
uintptr_t get_gicr_base(int cpu_id)
{
	uintptr_t gicr_base = gic_data.rdistif_base_addrs[cpu_id];
	return gicr_base;
}
#endif

/******************************************************************************
 * ARM common helper to enable the GIC CPU interface
 *****************************************************************************/
void plat_gic_cpuif_enable(void)
{
	unsigned int cpu = plat_my_core_pos();
	/* Power on redistributor */
	gicr_write_pwrr(PLAT_VENDOR_GICR_BASE + 0x20000 * cpu, PWRR_ON);

	/* Wait until the power on state is reflected */
	while (gicr_read_pwrr(PLAT_VENDOR_GICR_BASE + 0x20000 * cpu) & PWRR_RDGPO);

	gicv3_cpuif_enable(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to disable the GIC CPU interface
 *****************************************************************************/
void plat_gic_cpuif_disable(void)
{
	gicv3_cpuif_disable(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to initialize the per-cpu redistributor interface in GICv3
 *****************************************************************************/
void plat_gic_pcpu_init(void)
{
	gicv3_rdistif_init(plat_my_core_pos());
}
#ifdef FIQ_SUPPORT
/*******************************************************************************
 * When the Nmi Interrupt happen, write the context of ELR_EL3,
 * and exit the exception through eret to the OS handler.
 ******************************************************************************/
interrupt_handler_table_t g_interrupt_handler_table_g0s[] = {
	{PERI_WDT5_S, nmi_exception_handler, NULL},
	{FIQ_SGI_S, nmi_exception_handler, NULL},
	{INTR_ID_UNAVAILABLE, NULL, NULL}};

uint64_t el3_interrupt_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	uint32_t gic_interrupt_id;
	interrupt_type_handler_t interrupt_handler = NULL;
	interrupt_handler_table_t *interrupt_handler_table = NULL;

#if ARM_GIC_ARCH == 2
	gic_interrupt_id = gicc_read_IAR(GICC_BASE);
#else
	gic_interrupt_id = read_icc_iar0_el1();
#endif

	interrupt_handler_table = g_interrupt_handler_table_g0s;
	for (; interrupt_handler_table->source != INTR_ID_UNAVAILABLE;) {
		if (gic_interrupt_id == interrupt_handler_table->source) {
			interrupt_handler = interrupt_handler_table->handler;
			break;
		}
		interrupt_handler_table++;
	}

	if (interrupt_handler != NULL) {
		(void)interrupt_handler(gic_interrupt_id, flags, handle, cookie);
	} else {
		ERROR("[%s]:[%dL] interrupt_handler is NULL!\n\r", __func__, __LINE__);
		goto EXIT;
	}

EXIT:
	/*
	 * A processor writes EIOR to inform the CPU interface
	 * that it has completed the processing of the specified
	 * Group0 interrupts
	 */
#if ARM_GIC_ARCH == 2
	NOTICE("gicc_write_EOIR(GICD_BASE,0x%x)\n", gic_interrupt_id);
	gicc_write_EOIR(GICC_BASE, gic_interrupt_id);
#else
	write_icc_eoir0_el1(gic_interrupt_id);
#endif
	dsb();

	return 0;
}
#endif

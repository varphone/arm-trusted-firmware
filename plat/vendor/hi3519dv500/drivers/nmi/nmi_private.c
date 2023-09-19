/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#ifdef FIQ_SUPPORT
#include <arch_helpers.h>
#include <common/runtime_svc.h>
#include <common/debug.h>
#include <string.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <plat/common/platform.h>
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <plat_private.h>
#include "vendor_def.h"
#include "arm/gic/v3/gicv3_private.h"

#define PRIVATE_NMI_SMC_FIQ_GLUE_ID 0x83000004

#define NMI_STACK_SIZE 0x200

uint64_t nmi_context_addr __attribute__((section("tzfw_coherent_mem")));
uint64_t nmi_transfer_add __attribute__((section("tzfw_coherent_mem")));

static uint64_t checksum(uint8_t *buf, uint64_t size)
{
	uint64_t i;
	uint64_t sum = 0;

	for (i = 0; i < size; i++) {
		sum += (uint64_t) * (buf + i);
	}

	return sum;
}

static uint64_t read_sp_el1(void)
{
	uint64_t val = 0;

	__asm__ volatile("mrs %0, sp_el1" : "=r"(val));

	return val;
}

static void save_context(void *handle, uint32_t linear_id, uint64_t id)
{
	uint32_t i;
	uint32_t cpu_no = linear_id;
	uint64_t *addr_stack = (uint64_t *)(nmi_context_addr + cpu_no * NMI_STACK_SIZE);
	volatile uint64_t *addr = addr_stack;
	uint64_t tmp;

	for (i = 0; i < GP_NUM; i++) {                                          /* Only 31 data definition registers */
		tmp = read_ctx_reg(get_gpregs_ctx(handle), (CTX_GPREG_X0 + 8 * i)); /* 8*i */
		*(addr++) = (tmp);
	}

	tmp = read_sp_el1();
	(*(addr++)) = (tmp);
	tmp = read_ctx_reg(get_el3state_ctx(handle), CTX_ELR_EL3);
	(*(addr++)) = (tmp);
	tmp = read_ctx_reg(get_el3state_ctx(handle), CTX_SPSR_EL3);
	(*(addr++)) = (tmp);
	/* orig_x0 */
	(*(addr++)) = (tmp);
	/* syscallno */
	(*(addr++)) = (tmp);
	tmp = PERI_WDT5_S;
	(*(addr++)) = (tmp);

	/* The size of the structure defined in the OS is 288 bytes, which is the sum of 288 bytes. */
	tmp = checksum((uint8_t *)addr_stack, 288);
	(*(addr++)) = (tmp);
	return;
}

static void nmi_trigger_sgi_to_cores(int sgi_num)
{
	uint64_t sgi_val;

	/* Raise SGI to PE specified by its affinity */
	sgi_val = GICV3_SGIR_VALUE(0, 0, 0, sgi_num, 1, 0);
	/*
	 * Ensure that any shared variable updates depending on out of band
	 * interrupt trigger are observed before raising SGI.
	 */
	dsbishst();
	write_icc_sgi0r_el1(sgi_val);
	isb();
}

uint64_t nmi_exception_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	uint32_t linear_id = plat_my_core_pos();
	uint32_t spsr_el3;

	if (id == PERI_WDT5_S) {
		gicv3_disable_interrupt(PERI_WDT5_S, 0);
	}
	dsb();
	isb();

	if (nmi_transfer_add) {
		if (handle == NULL) {
			ERROR("[%s]:[%dL] handle NULL!!! \n\r", __func__, __LINE__);
			return TF_ERROR;
		}

		save_context(handle, linear_id, (uint64_t)id);
		write_ctx_reg(get_el3state_ctx(handle), CTX_ELR_EL3, nmi_transfer_add);

		/* If the state is EL0 before entering EL3, it needs to return to EL1 to execute the processing function.
		 * Here, it is determined whether the state is EL0 before entering EL3,If the value is 0, switch to EL1.
		 * In addition, the interrupt bit needs to be changed to mask to prevent interrupt generation.
		 */
		spsr_el3 = read_ctx_reg(get_el3state_ctx(handle), CTX_SPSR_EL3);
		if ((0x0f & spsr_el3) == 0) {
			spsr_el3 |= 0x5;
			write_ctx_reg(get_el3state_ctx(handle), CTX_SPSR_EL3, spsr_el3);
		}

		spsr_el3 |= 0x02c0; /* not set A bit */
		write_ctx_reg(get_el3state_ctx(handle), CTX_SPSR_EL3, spsr_el3);
	}

	if (id == PERI_WDT5_S) {
		nmi_trigger_sgi_to_cores(FIQ_SGI_S);
	}
	dsb();
	isb();

	return TF_OK;
}

static void configer_sgi(uintptr_t gicd_base, uint32_t intr_num)
{
	gicr_clr_igroupr(gicd_base, intr_num);

	gicr_clr_igrpmodr(gicd_base, intr_num);

	gicr_set_icfgr(gicd_base, intr_num, GIC_INTR_CFG_LEVEL);

	gicr_set_ipriorityr(gicd_base, intr_num, GIC_HIGHEST_SEC_PRIORITY);

	gicr_set_isenabler(gicd_base, intr_num);
}

static void configer_spi(uintptr_t gicd_base, uint32_t intr_num)
{
	gicd_clr_igroupr(gicd_base, intr_num);

	gicd_clr_igrpmodr(gicd_base, intr_num);

	gicd_set_icfgr(gicd_base, intr_num, GIC_INTR_CFG_LEVEL);

	gicd_set_ipriorityr(gicd_base, intr_num, GIC_HIGHEST_SEC_PRIORITY);

	gicd_set_isenabler(gicd_base, intr_num);
}

#define CPU_NUM 2

static void gicr_reinit(void)
{
	for (int i = 0; i < CPU_NUM; i++) {
		uintptr_t gicr_base = get_gicr_base(i);
		configer_sgi(gicr_base, FIQ_SGI_S);
	}
}

static uintptr_t nmi_private_handler(uint32_t fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4,
	void *cookie, void *handle, u_register_t flags)
{
	configer_spi(VENDOR_GICD_BASE, PERI_WDT5_S);
	gicr_reinit();

	dsb();
	isb();
	switch (fid) {
	case PRIVATE_NMI_SMC_FIQ_GLUE_ID:
		INFO("Private SMC Call: 0x%x \n", fid);
		INFO("scr el3 value: 0x%lx \n", read_scr_el3());
		if ((void *)x1 != NULL) {
			nmi_transfer_add = x1;
		} else {
			INFO("nmi_transfer_add is NULL\n");
		}

		if ((void *)x2 != NULL) {
			nmi_context_addr = x2;
		} else {
			INFO("nmi_context_addr is NULL\n");
		}

		SMC_RET0(x1)
	/* fall-through */
	default:
		NOTICE("Unimplemented Private SMC Call: 0x%x \n", fid);
		SMC_RET1(handle, SMC_UNK)
	}
}

DECLARE_RT_SVC(Private, OEN_OEM_START, OEN_OEM_END, SMC_TYPE_FAST, NULL, nmi_private_handler);
#endif

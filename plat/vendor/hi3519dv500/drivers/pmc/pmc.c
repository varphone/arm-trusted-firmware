/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <arch_helpers.h>
#include <assert.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <plat_private.h>
#include <lib/psci/psci.h>

static const uintptr_t cpu_crg[PLATFORM_MAX_CPUS_PER_CLUSTER] = {
	CPU_CLST0_CORE0_SET,
	CPU_CLST0_CORE1_SET,
};

static inline void set_pwr_domain_val(uintptr_t addr, bool status)
{
	unsigned long val = mmio_read_32(addr);
	if (status) {
		val |= (0x1 << 4);
		val &= (~0x3); /* enable core */
	} else {
		val &= ~(0x1 << 4);
		val |= 0x3; /* disable core and reset */
	}
	mmio_write_32(addr, val);
}

static void set_core_pwr_domain(uint32_t cpu, bool status)
{
	uintptr_t addr = cpu_crg[cpu];
	set_pwr_domain_val(addr, status);
}

static void wait_core_pstate_ready(uint32_t cpu, uint32_t max_times)
{
	uint32_t i = 0;
	uint32_t reg = MISC_REG_CPU_CTRL4 + cpu * CPU_CTRL_RANGE;
	do {
		volatile uint32_t pchn1_sys_corepstate = mmio_read_32(reg);
		pchn1_sys_corepstate &= CPU_PSTATE_MASK;

		if (pchn1_sys_corepstate != 0) {
			VERBOSE("[%s, %d]warn: core%u pstate not ready\r\n", __func__, __LINE__, cpu);
		} else {
			VERBOSE("[%s, %d]core%u pstate is ready, i=%u\r\n", __func__, __LINE__, cpu, i);
			break;
		}
		i++;
	} while (i < max_times);
}

static int cores_pwr_domain_on(unsigned long mpidr, uint64_t entrypoint)
{
	uint32_t cpu = plat_core_pos_by_mpidr(mpidr);
	if (cpu > PLATFORM_MAX_CPUS_PER_CLUSTER) {
		ERROR("invalid cpu(%u)\n", cpu);
		return -1;
	}
	INFO("ATF [%s, %d]cpu=%u, entrypoint=0x%lx\n", __func__, __LINE__, cpu, entrypoint);
	wait_core_pstate_ready(cpu, 100);	/* 100 for the max retry times */
	set_core_pwr_domain(cpu, false);

	mmio_write_32(REG_PERI_CPU_RVBARADDR, entrypoint >> 2);	 /* psci_entrypoint */
	set_core_pwr_domain(cpu, true);
	return 0;
}

static int cores_pwr_domain_on_finish(void)
{
	INFO("[%s][%d]\n", __func__, __LINE__);
	return 0;
}

static int sys_pwr_domain_resume(void)
{
	INFO("[%s][%d]\n", __func__, __LINE__);
	return 0;
}

static int sys_pwr_domain_suspend(void)
{
	INFO("[%s][%d]\n", __func__, __LINE__);
	return 0;
}

static void __dead2 sys_system_reset(void)
{
	/* Any value to this reg will reset the cpu */
	mmio_write_32(SYS_CTRL_BASE + REG_SC_SYSRES, 0x12345678);

	/* we shouldn't get to this point */
	panic();
}

static struct pm_ops_cb pm_ops = {
	.cores_pwr_dm_on = cores_pwr_domain_on,
	.cores_pwr_dm_on_finish = cores_pwr_domain_on_finish,
	.sys_pwr_dm_suspend = sys_pwr_domain_suspend,
	.sys_pwr_dm_resume = sys_pwr_domain_resume,
	.sys_gbl_soft_reset = sys_system_reset,
};

void plat_pmc_init(void)
{
	plat_setup_pm_ops(&pm_ops);
}

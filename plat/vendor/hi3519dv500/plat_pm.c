/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <context.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <lib/psci/psci.h>
#include <plat_private.h>

extern uintptr_t sec_entry_point;

static struct pm_ops_cb *pm_ops;

/*******************************************************************************
 * Handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
static void cluster_pwrdwn_common(void)
{
}

/*******************************************************************************
 * This handler is called by the PSCI implementation during the `SYSTEM_SUSPEND`
 * call to get the `power_state` parameter. This allows the platform to encode
 * the appropriate State-ID field within the `power_state` parameter which can
 * be utilized in `pwr_domain_suspend()` to suspend to system affinity level.
******************************************************************************/
void get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	/* lower affinities use PLAT_MAX_OFF_STATE */
	for (int i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

/*******************************************************************************
 * Handler called when an affinity instance is about to enter standby.
 ******************************************************************************/
void cpu_standby(plat_local_state_t cpu_state)
{
	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
#ifdef FIQ_SUPPORT
	unsigned long scr;
	unsigned int coreid;

	if (cpu_state != PLAT_MAX_RET_STATE) {
		ERROR("[%s]: cpu_state is %u, error! \n\r", __func__, cpu_state);
		return;
	}

	scr = read_scr_el3();

	/* Enable Physical IRQ and FIQ to wake the CPU. */
	write_scr_el3(scr | SCR_IRQ_BIT | SCR_FIQ_BIT);
	wfi();

	coreid = plat_my_core_pos();

	INFO("[%s] Core %d has been wakeup. \n\r", __func__, coreid);

	/*
	 * Restore SCR to the original value, synchronisazion of
	 * scr_el3 is done by eret while el3_exit to save some
	 * execution cycles.
	 */
	write_scr_el3(scr);
	dsb();
#else
	dsb();
	wfi();
#endif
}

/*******************************************************************************
 * Handler called when an affinity instance is about to be turned on. The
 * level and mpidr determine the affinity instance.
 ******************************************************************************/
int pwr_domain_on(u_register_t mpidr)
{
	if (pm_ops && pm_ops->cores_pwr_dm_on)
		pm_ops->cores_pwr_dm_on(mpidr, sec_entry_point);

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void pwr_domain_off(const psci_power_state_t *target_state)
{
	plat_gic_cpuif_disable();
	if (target_state->pwr_domain_state[PLAT_CLUSTER_PWR_LVL] == PLAT_MAX_OFF_STATE)
		cluster_pwrdwn_common();
}

/*******************************************************************************
 * Handler called when called when a power domain is about to be suspended. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void pwr_domain_suspend(const psci_power_state_t *target_state)
{
	plat_gic_driver_init();
	plat_gic_cpuif_disable();
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
void pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	/* Enable the gic cpu interface */
	plat_gic_pcpu_init();
	/* Program the gic per-cpu distributor or re-distributor interface */
	plat_gic_cpuif_enable();
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on after
 * having been suspended earlier. The target_state encodes the low power state
 * that each level has woken up from.
 ******************************************************************************/
void pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
}

void __dead2 pwr_domain_pwr_down_wfi(const psci_power_state_t *target_state)
{
	while (1)
		wfi();
}

/*******************************************************************************
 * Handler called when the system wants to be powered off
 ******************************************************************************/
void __dead2 system_off(void)
{
	if (pm_ops && pm_ops->sys_gbl_soft_reset)
		(void)pm_ops->sys_gbl_soft_reset();

	panic();
}

/*******************************************************************************
 * Handler called when the system wants to be restarted.
 ******************************************************************************/
void __dead2 system_reset(void)
{
	if (pm_ops && pm_ops->sys_gbl_soft_reset)
		(void)pm_ops->sys_gbl_soft_reset();

	panic();
}

/*******************************************************************************
 * Handler called to check the validity of the power state parameter.
 ******************************************************************************/
int32_t validate_power_state(unsigned int power_state,
				  psci_power_state_t *req_state)
{
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);
	int i;

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	for (i = 0; i <= pwr_lvl; i++)
		req_state->pwr_domain_state[i] =
			PLAT_MAX_OFF_STATE;

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Platform handler called to check the validity of the non secure entrypoint.
 ******************************************************************************/
int validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Export the platform handlers to enable psci to invoke them
 ******************************************************************************/
static const plat_psci_ops_t plat_psci_ops = {
	.cpu_standby			= cpu_standby,
	.pwr_domain_on			= pwr_domain_on,
	.pwr_domain_off			= pwr_domain_off,
	.pwr_domain_suspend		= pwr_domain_suspend,
	.pwr_domain_on_finish		= pwr_domain_on_finish,
	.pwr_domain_suspend_finish	= pwr_domain_suspend_finish,
	.pwr_domain_pwr_down_wfi        = pwr_domain_pwr_down_wfi,
	.system_off			= system_off,
	.system_reset			= system_reset,
	.validate_power_state		= validate_power_state,
	.validate_ns_entrypoint		= validate_ns_entrypoint,
	.get_sys_suspend_power_state	= get_sys_suspend_power_state,
};

/*******************************************************************************
 * Export the platform specific power ops and initialize Power Controller
 ******************************************************************************/
int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	/*
	 * Flush entrypoint variable to PoC since it will be
	 * accessed after a reset with the caches turned off.
	 */
	sec_entry_point = sec_entrypoint;
	flush_dcache_range((uintptr_t)&sec_entry_point, sizeof(uintptr_t));
	/*
	 * Initialize PSCI ops struct
	 */
	*psci_ops = &plat_psci_ops;
	return 0;
}

void plat_setup_pm_ops(struct pm_ops_cb *ops)
{
	pm_ops = ops;
}

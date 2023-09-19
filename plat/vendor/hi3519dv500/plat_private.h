/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#ifndef __PLAT_PRIVATE_H__
#define __PLAT_PRIVATE_H__
#ifdef FIQ_SUPPORT
#include <bl31/interrupt_mgmt.h>
#endif

/*******************************************************************************
 * For socs pm ops
 ******************************************************************************/
struct pm_ops_cb {
	int (*cores_pwr_dm_on)(unsigned long mpidr, uint64_t entrypoint);
	int (*cores_pwr_dm_off)(void);
	int (*cores_pwr_dm_on_finish)(void);
	int (*cores_pwr_dm_suspend)(void);
	int (*cores_pwr_dm_resume)(void);
	int (*sys_pwr_dm_suspend)(void);
	int (*sys_pwr_dm_resume)(void);
	void __dead2 (*sys_gbl_soft_reset)(void);
};

/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/
void plat_configure_mmu_el3(unsigned long total_base,
			    unsigned long total_size,
			    unsigned long,
			    unsigned long,
			    unsigned long,
			    unsigned long);


void plat_delay_timer_init(void);

void plat_gic_driver_init(void);
void plat_gic_init(void);
void plat_gic_cpuif_enable(void);
void plat_gic_cpuif_disable(void);
void plat_gic_pcpu_init(void);


void plat_pmc_init(void);
void plat_setup_pm_ops(struct pm_ops_cb *ops);
void plat_cci_enable(void);
#ifdef FIQ_SUPPORT
uintptr_t get_gicr_base(int cpu_id);

/* Pointer of fun, which init sth before register the gic */
typedef void (*INITFUN_BEFORE_REGISTER)(void);
typedef struct gic_register_para {
	uint32_t source;
	interrupt_type_handler_t handler;
	INITFUN_BEFORE_REGISTER init_fun;
} interrupt_handler_table_t;

uint64_t el3_interrupt_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t nmi_exception_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
#endif
#endif /* __PLAT_PRIVATE_H__ */

/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#ifndef __PLAT_DEF_H__
#define __PLAT_DEF_H__

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
#define DEVICE_BASE 			0x00000000
#define DEVICE_SIZE 			0x40000000

#define DRAM_NS_BASE			0x40000000
#define DRAM_NS_SIZE			0x40000000

#define TEE_SEC_MEM_BASE		0x90000000
#define TEE_SEC_MEM_SIZE		0x10000000

#define FCM_CLUSTER_REG_BASE		0x11020000
#define	CPU_CTRL_RANGE			0x10
#define	CPU_CTRL2			0x4118
#define	CPU_CTRL4			0x4120
#define	MISC_REG_CPU_CTRL2		(FCM_CLUSTER_REG_BASE + CPU_CTRL2)
#define	MISC_REG_CPU_CTRL4		(FCM_CLUSTER_REG_BASE + CPU_CTRL4)
#define	MISC_REG_ENABLE_CPU0		MISC_REG_CPU_CTRL2
#define CPU_HW_STATE_MACHINE		(1 << 6)
#define	CPU_PSTATE_MASK			((1 << 6) - 1)
/*******************************************************************************
 * UART related constants
 ******************************************************************************/
#define VENDOR_UART0_BASE		(0x11040000)
#define VENDOR_BAUDRATE			(115200)
#define VENDOR_UART_CLOCK		(24000000)

/*******************************************************************************
 * IPC related constants
 ******************************************************************************/
#define IPC_REG_BASE		0x1103E000U
#define IPC_SEC_REG_BASE	0x1103F000U
#define IPC_SET_REG		(IPC_REG_BASE + 0x000)
#define IPC_CLEAR_REG		(IPC_REG_BASE + 0x004)
#define IPC_STATUS_REG		(IPC_REG_BASE + 0x008)
#define IPC_INT_MASK_REG	(IPC_REG_BASE + 0x00C)
#define IPC_SHARE_REG_BASE	(IPC_REG_BASE + 0x020)
#define IPC_SHARE_MAX_REG	16
#define IPC_INT_MAX		9

#define IPC_CMD_ACK		0xA0
#define IPC_CMD_START_BL31	0xA1
#define IPC_CMD_START_TEEIMG	0xA2
#define IPC_CMD_START_KERNEL	0xA3
#define IPC_CMD_NEED_BL31	0xB1
#define IPC_CMD_NEED_TEEIMG	0xB2
#define IPC_CMD_NEED_KERNEL	0xB3

#define IPC_NODE_CORE0		0
#define IPC_NODE_CORE1		1

/*******************************************************************************
 * System counter frequency related constants
 ******************************************************************************/
#define SYS_COUNTER_FREQ_IN_TICKS	25000000
#define SYS_COUNTER_FREQ_IN_MHZ		25

/* Base MTK_platform compatible GIC memory map */
#define VENDOR_GICD_BASE		(0x12400000) /*  */
#define VENDOR_GICR_BASE		(0x12440000) /*  */

#define REG_PERI_CPU_RVBARADDR		0x11024204

/* cci registers */
#define CLUSTER0_CPU_CTRL		0x1102411C
#define CLUSTER1_CPU_CTRL		0x11024120

#define SYS_CTRL_BASE	0x11020000
#define REG_SC_SYSRES	0x0004

#define CPU_CLST0_CORE0_SET		0x11012048
#define CPU_CLST0_CORE1_SET		0x1101204c
#ifdef FIQ_SUPPORT
/* wdg interrupt used as fiq */
#define PERI_WDT5_S 35
/* used for trigger interrupt to other cores */
#define FIQ_SGI_S 14
#define GP_NUM 31

#define TF_ERROR 0xffffffffull
#define TF_OK 0
#endif
#endif /* __PLAT_DEF_H__ */

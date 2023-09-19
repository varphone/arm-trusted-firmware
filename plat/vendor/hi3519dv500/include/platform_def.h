/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include <vendor_def.h>

#define DEBUG_XLAT_TABLE 0

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

/* Size of cacheable stacks, May need fix */
#if DEBUG_XLAT_TABLE
#define PLATFORM_STACK_SIZE 0x800
#elif IMAGE_BL1
#define PLATFORM_STACK_SIZE 0x440
#elif IMAGE_BL2
#define PLATFORM_STACK_SIZE 0x400
#elif IMAGE_BL31
#define PLATFORM_STACK_SIZE 0x800
#elif IMAGE_BL32
#define PLATFORM_STACK_SIZE 0x440
#endif

/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_CLUSTER_COUNT		1
#define PLATFORM_MAX_CPUS_PER_CLUSTER	2

#define PLAT_CLUSTER_PWR_LVL		MPIDR_AFFLVL1
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL1
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * PLATFORM_MAX_CPUS_PER_CLUSTER)
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CORE_COUNT + PLATFORM_CLUSTER_COUNT + 1)
/*******************************************************************************
 * Platform power states
 ******************************************************************************/
#define PLAT_MAX_OFF_STATE	2
#define PLAT_MAX_RET_STATE	1

/*******************************************************************************
 * BL31 specific defines.
 ******************************************************************************/
#define BL31_SIZE			0x20000

// Modify this configuration according to the system framework
#define BL31_BASE 			0x40010000

#define BL31_LIMIT			(BL31_BASE + BL31_SIZE - 1)

#define ADDR_SPACE_SIZE			(1Ull << 34)
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 34)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)
#define MAX_XLAT_TABLES			4
#define MAX_MMAP_REGIONS		16


/*
 * Define GICD and GICC and GICR base
 */
#define PLAT_VENDOR_GICD_BASE	VENDOR_GICD_BASE
#define PLAT_VENDOR_GICR_BASE	VENDOR_GICR_BASE

/*
 * Define a list of Group 1 Secure and Group 0 interrupts as per GICv3
 * terminology. On a GICv2 system or mode, the lists will be merged and treated
 * as Group 0 interrupts.
 */
#define PLAT_VENDOR_G1S_IRQS	VENDOR_G1S_IRQS
#define PLAT_VENDOR_G0_IRQS	VENDOR_G0_IRQS

#define PLAT_VENDOR_UART_BASE	VENDOR_UART0_BASE
#define PLAT_VENDOR_UART_CLOCK	VENDOR_UART_CLOCK
#define PLAT_VENDOR_UART_BAUDRATE	VENDOR_BAUDRATE

#define PLAT_VENDOR_PRIMARY_CPU		0x0
#define PLAT_VENDOR_MPIDR_CPU_BIT		0x8

#define REG_SYS_CNT_BASE		(0x11050000)
#define REG_SYS_CNT_FREQ		0x20

#endif /* __PLATFORM_DEF_H__ */

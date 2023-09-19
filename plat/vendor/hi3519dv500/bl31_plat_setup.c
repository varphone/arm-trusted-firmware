/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <assert.h>
#include <common/bl_common.h>
#include <plat/common/common_def.h>
#include <drivers/console.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <drivers/arm/pl011.h>
#include <plat_private.h>
#include <plat/common/platform.h>
#ifdef FIQ_SUPPORT
#include <drivers/arm/gicv3.h>
#endif
#include "ipc.h"

static entry_point_info_t bl33_image_ep_info, bl32_image_ep_info;
static int bl33_ep_info_ready = 0;

static void ipc_send_ack(int node)
{
	struct ipc_share_msg msg;
	msg.cmd = IPC_CMD_ACK;
	msg.len = 0;
	ipc_send_msg(node, &msg);
}

static void ipc_send_nack(int node, uint32_t cmd)
{
	struct ipc_share_msg msg;
	msg.cmd = cmd;
	msg.len = 0;
	ipc_send_msg(node, &msg);
}

static void bl31_setup_bl33_image_ep_info()
{
	struct ipc_share_msg msg;
	uintptr_t bl33_base;
	uintptr_t fdt_base;

	NOTICE("IPC: waiting for kernel");

	do {
		int ret = ipc_recv_msg(IPC_NODE_CORE0, &msg, 1);
		if (ret <= 0) {
			WARN("IPC: core0 got invalid message\r\n");
			continue;
		}
		if (msg.cmd != IPC_CMD_START_KERNEL) {
			ipc_send_nack(IPC_NODE_CORE1, IPC_CMD_NEED_KERNEL);
			continue;
		}
		bl33_base = ((uint64_t)msg.buf[0] << 32) | msg.buf[1];
		fdt_base = ((uint64_t)msg.buf[2] << 32) | msg.buf[3];
		ipc_send_ack(IPC_NODE_CORE1);
		break;
	} while (1);

	bl33_image_ep_info.pc = bl33_base;
	bl33_image_ep_info.args.arg0 = fdt_base;
	bl33_image_ep_info.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type.
 ******************************************************************************/
#if DISABLE_TEE == 1
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	return &bl33_image_ep_info;
}
#else
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	if (type == NON_SECURE) {
		if (!bl33_ep_info_ready) {
			bl31_setup_bl33_image_ep_info();
			bl33_ep_info_ready = 1;
		}
		return &bl33_image_ep_info;
	}

	if (type == SECURE)
		return &bl32_image_ep_info;

	return NULL;
}
#endif

/*******************************************************************************
 * Perform any BL31 specific platform actions. Populate the BL33 and BL32 image
 * info.
 ******************************************************************************/
void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	static console_t console;
	uintptr_t bl32_ep = (uintptr_t)arg0;
	/*
	 * Configure the UART port to be used as the console
	 */
	(void)console_pl011_register(PLAT_VENDOR_UART_BASE,
				     PLAT_VENDOR_UART_CLOCK,
				     PLAT_VENDOR_UART_BAUDRATE, &console);
	/* Initialise crash console */
	plat_crash_console_init();

	bl32_image_ep_info.pc = bl32_ep;
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);
}

/*******************************************************************************
 * Initialize the gic, configure the SCR.
 ******************************************************************************/
#if DISABLE_TEE == 1
void bl31_platform_setup(void)
{
	plat_pmc_init();
	plat_gic_driver_init();
	plat_gic_init();
	plat_delay_timer_init();
}
#else
void bl31_platform_setup(void)
{
	plat_delay_timer_init();
	/* Initialize the gic cpu and distributor interfaces */
	plat_gic_driver_init();
	plat_gic_init();
	plat_pmc_init();
}
#endif

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	plat_configure_mmu_el3(BL_CODE_BASE,
			       (BL_COHERENT_RAM_END - BL_CODE_BASE),
			       BL_CODE_BASE,
			       BL_CODE_END,
			       BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
}
#ifdef FIQ_SUPPORT
void bl31_plat_runtime_setup(void)
{
	uint64_t rc;
	uint32_t flags = 0;

	set_interrupt_rm_flag(flags, NON_SECURE);
	rc = register_interrupt_type_handler(INTR_TYPE_EL3, el3_interrupt_handler, flags);
	if (rc) {
		ERROR("register interrupt error!\n");
		return;
	}

	return;
}
#endif
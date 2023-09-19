/*
 * Copyright (c) 2020-2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <arch_helpers.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>

#define TRNG_FIFO_DATA 0x10000100

u_register_t plat_get_stack_protector_canary(void)
{
	u_register_t seed;

	seed = mmio_read_32(TRNG_FIFO_DATA);

	return seed ^ read_cntpct_el0();
}
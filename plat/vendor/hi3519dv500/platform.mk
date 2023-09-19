#
# Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of ARM nor the names of its contributors may be used
# to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

$(eval $(call add_define,CONFIG_BL31_BASE))
$(eval $(call add_define,CONFIG_FPGA))

VENDOR_PLAT		:=	plat/vendor
VENDOR_PLAT_SOC		:=	${VENDOR_PLAT}/${PLAT}

PLAT_INCLUDES		:=	-I${VENDOR_PLAT}/				\
				-I${VENDOR_PLAT_SOC}/				\
				-I${VENDOR_PLAT_SOC}/include/		\
				-I${VENDOR_PLAT_SOC}/include/		\
				-Idrivers
				
GICV3_SUPPORT_GIC600	:=	1
include drivers/arm/gic/v3/gicv3.mk

VENDOR_GIC_SOURCES	:= 	$(GICV3_SOURCES)				\
				plat/common/plat_gicv3.c			\
				$(VENDOR_PLAT)/common/vendor_gicv3.c

# Enable stack protection
ENABLE_STACK_PROTECTOR      :=  strong

ifneq ($(ENABLE_STACK_PROTECTOR), 0)
VENDOR_GIC_SOURCES	+=	$(VENDOR_PLAT)/common/vendor_stack_protector.c
endif

PLAT_BL_COMMON_SOURCES	:=	lib/xlat_tables_v2/xlat_tables_context.c	\
				lib/xlat_tables_v2/xlat_tables_utils.c		\
				lib/xlat_tables_v2/xlat_tables_core.c		\
				lib/xlat_tables_v2/aarch64/enable_mmu.S	\
				lib/xlat_tables_v2/aarch64/xlat_tables_arch.c	\
				plat/common/plat_psci_common.c

BL31_SOURCES		+=	$(VENDOR_GIC_SOURCES)				\
				drivers/arm/pl011/aarch64/pl011_console.S		\
				drivers/delay_timer/delay_timer.c		\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a55.S			\
				${VENDOR_PLAT_SOC}/aarch64/plat_helpers.S		\
				${VENDOR_PLAT_SOC}/aarch64/platform_common.c	\
				${VENDOR_PLAT_SOC}/drivers/pmc/pmc.c		\
				${VENDOR_PLAT_SOC}/drivers/ipc/ipc.c		\
				${VENDOR_PLAT_SOC}/bl31_plat_setup.c		\
				${VENDOR_PLAT_SOC}/plat_pm.c			\
				${VENDOR_PLAT_SOC}/plat_topology.c		\
				${VENDOR_PLAT_SOC}/plat_delay_timer.c

BL31_SOURCES	+= ${VENDOR_PLAT_SOC}/drivers/nmi/nmi_private.c

DEFINES := -DFIQ_SUPPORT
ENABLE_PLAT_COMPAT      := 0
CTX_INCLUDE_FPREGS	:= 1
NEED_BL33		:= yes
MULTI_CONSOLE_API   := 1
#ERRATA_A53_855873 := 1
PROGRAMMABLE_RESET_ADDRESS := 1

# Do not enable SVE
ENABLE_SVE_FOR_NS		:=	0

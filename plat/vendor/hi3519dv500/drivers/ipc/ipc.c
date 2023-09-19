/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include "ipc.h"

#define	ipc_info(fmt, args...)		INFO("Info:[%s, %d] "fmt, __func__, __LINE__, ##args)
#define	ipc_err(fmt, args...)		ERROR("Error:[%s, %d] "fmt, __func__, __LINE__, ##args)

static inline uint32_t readl(uintptr_t addr)
{
	return mmio_read_32(addr);
}

static inline void writel(uint32_t val, uintptr_t addr)
{
	mmio_write_32(addr, val);
}

static int ipc_send_msg_int(uint32_t node)
{
	if (node > IPC_INT_MAX) {
		ipc_err("invalid int_no[0x%x]\n", node);
		return -1;
	}
	writel(node, IPC_SET_REG);
	return 0;
}

static void ipc_msg_received(uint32_t node)
{
	writel(node, IPC_CLEAR_REG);
}

static int ipc_has_msg(uint32_t node)
{
	uint32_t status = readl(IPC_STATUS_REG);
	status &= (1 << node);
	return status != 0;
}

int ipc_send_msg(uint32_t node, const struct ipc_share_msg *msg)
{
	uint32_t i;
	uint32_t xor = 0;

	if (node >= IPC_INT_MAX) {
		ipc_err("invalid node[0x%x]\n", node);
		return -1;
	}
	if (msg == NULL) {
		ipc_err("msg is NULL\n");
		return -1;
	}
	if (msg->len > IPC_DATA_BUF_SIZE) {
		ipc_err("invalid msg->len[0x%x]\n", msg->len);
		return -1;
	}
	writel(msg->cmd, IPC_CMD_REG);
	xor ^= msg->cmd;
	writel(msg->len, IPC_LEN_REG);
	xor ^= msg->len;
	for (i = 0; i < msg->len; i++) {
		uintptr_t addr = ipc_data_reg(i);
		writel(msg->buf[i], addr);
		xor ^= msg->buf[i];
	}
	writel(xor, IPC_XOR_REG);
	ipc_send_msg_int(node);
	return 0;
}

static int do_ipc_recv_msg(struct ipc_share_msg *msg)
{
	uint32_t i;
	uint32_t xor = 0;

	if (msg == NULL) {
		ipc_err("msg is NULL\n");
		return -1;
	}
	msg->cmd = readl(IPC_CMD_REG);
	xor ^= msg->cmd;
	msg->len = readl(IPC_LEN_REG);
	xor ^= msg->len;

	if (msg->len > IPC_DATA_BUF_SIZE) {
		ipc_err("invalid msg->len[0x%x]\n", msg->len);
		return -1;
	}

	for (i = 0; i < msg->len; i++) {
		uintptr_t addr = ipc_data_reg(i);
		msg->buf[i] = readl(addr);
		xor ^= msg->buf[i];
	}
	msg->xor = readl(IPC_XOR_REG);
	if (msg->xor != xor) {
		ipc_err("verify failed, xor[0x%x], msg->xor[0x%x]\n", xor, msg->len);
		return -1;
	}
	return 0;
}

/*
 * ipc_recv_msg - Receive message from specific CPU.
 *
 * @node: The node to receive message
 * @msg: Point to the ojecct to received message
 * @block: Blocked or not
 * Return: The length of message or an error code.
 */
int ipc_recv_msg(uint32_t node, struct ipc_share_msg *msg, int block)
{
	int ret;

	if (msg == NULL) {
		ipc_err("msg is NULL\n");
		return -1;
	}

	if (node >= IPC_INT_MAX) {
		ipc_err("invalid node[0x%x]\n", node);
		return -1;
	}

	if (block)
		while (!ipc_has_msg(node));

	ret = do_ipc_recv_msg(msg);
	ipc_msg_received(node);
	if (ret != 0) {
		ipc_err("Failed to receive ipc message\n");
		return ret;
	}
	return (int)msg->len;
}

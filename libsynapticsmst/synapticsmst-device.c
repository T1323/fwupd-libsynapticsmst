/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2016 Mario Limonciello <mario.limonciello@dell.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "config.h"
#include <string.h>
#include <glib-object.h>

#include "synapticsmst-device.h"
#include "synapticsmst-common.h"

#define BLOCK_UNIT         64

typedef struct
{
	SynapticsMSTDeviceKind	 kind;
	gchar			*devfs_node;
	gchar			*version;
	SynapticsMSTDeviceBoardID boardID;
	gchar           *chipID;
} SynapticsMSTDevicePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SynapticsMSTDevice, synapticsmst_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (synapticsmst_device_get_instance_private (o))

/**
 * synapticsmst_device_kind_from_string:
 * @kind: the string.
 *
 * Converts the text representation to an enumerated value.
 *
 * Returns: (transfer full): a #SynapticsMSTDeviceKind, or %SYNAPTICSMST_DEVICE_KIND_UNKNOWN for unknown.
 *
 * Since: 0.1.0
 **/
SynapticsMSTDeviceKind
synapticsmst_device_kind_from_string (const gchar *kind)
{
	if (g_strcmp0 (kind, "DIRECT") == 0)
		return SYNAPTICSMST_DEVICE_KIND_DIRECT;
	if (g_strcmp0 (kind, "REMOTE") == 0)
		return SYNAPTICSMST_DEVICE_KIND_REMOTE;
	return SYNAPTICSMST_DEVICE_KIND_UNKNOWN;
}

/**
 * synapticsmst_device_kind_to_string:
 * @kind: the #SynapticsMSTDeviceKind.
 *
 * Converts the enumerated value to an text representation.
 *
 * Returns: string version of @kind
 *
 * Since: 0.1.0
 **/
const gchar *
synapticsmst_device_kind_to_string (SynapticsMSTDeviceKind kind)
{
	if (kind == SYNAPTICSMST_DEVICE_KIND_DIRECT)
		return "DIRECT";
	if (kind == SYNAPTICSMST_DEVICE_KIND_REMOTE)
		return "REMOTE";
	return NULL;
}

const gchar	*
synapticsmst_device_boardID_to_string (SynapticsMSTDeviceBoardID boardID)
{
	if (boardID == SYNAPTICSMST_DEVICE_BOARDID_SYNA_EVB)
		return "SYNA evb baord";
	if (boardID == SYNAPTICSMST_DEVICE_BOARDID_X6)
		return "Dell X6 Dock";
	if (boardID == SYNAPTICSMST_DEVICE_BOARDID_X7)
		return "Dell X7 Dock";
	if (boardID == SYNAPTICSMST_DEVICE_BOARDID_TRINITY_WIRE)
		return "Dell Trinity Wire Dock";
	if (boardID == SYNAPTICSMST_DEVICE_BOARDID_TRINITY_WIRELESS)
		return "Dell Trinity Wireless Dock";
	return NULL;
}

static void
synapticsmst_device_finalize (GObject *object)
{
	SynapticsMSTDevice *device = SYNAPTICSMST_DEVICE (object);
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);

	g_free (priv->version);
	g_free (priv->devfs_node);
	g_free (priv->chipID);
	G_OBJECT_CLASS (synapticsmst_device_parent_class)->finalize (object);
}

static void
synapticsmst_device_init (SynapticsMSTDevice *device)
{
}

static void
synapticsmst_device_class_init (SynapticsMSTDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = synapticsmst_device_finalize;
}

/**
 * synapticsmst_device_get_kind:
 * @device: a #SynapticsMSTDevice instance.
 *
 * Gets the device kind.
 *
 * Returns: the #SynapticsMSTDeviceKind
 *
 * Since: 0.1.0
 **/
SynapticsMSTDeviceKind
synapticsmst_device_get_kind (SynapticsMSTDevice *device)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);
	return priv->kind;
}

SynapticsMSTDeviceBoardID
synapticsmst_device_get_boardID (SynapticsMSTDevice *device)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);
	return priv->boardID;
}

gboolean
synapticsmst_device_enable_remote_control (SynapticsMSTDevice *device, GError **error)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);
	const gchar *sc = "PRIUS";

	if (synapticsmst_common_rc_set_command(UPDC_ENABLE_RC, 5, 0, (unsigned char*)sc)) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to enable MST remote control!\n");
		return FALSE;
	}
	else {
		return TRUE;
	}
}

gboolean
synapticsmst_device_disable_remote_control (SynapticsMSTDevice *device, GError **error)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);

	if (synapticsmst_common_rc_set_command(UPDC_DISABLE_RC, 0, 0, (unsigned char*)NULL)) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to disable MST remote control!\n");
		return FALSE;
	}
	else {
		return TRUE;
	}
}

gboolean
synapticsmst_device_enumerate_device (SynapticsMSTDevice *device, GError **error)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);
	
	guint8 byte[16];
	
	if (synapticsmst_common_open_aux_node(priv->devfs_node)) {
		guint8 nRet;
		// enable remote control
		if (!synapticsmst_device_enable_remote_control(device, error)) {
			return FALSE;
		}

		// read firmware version
		nRet = synapticsmst_common_read_dpcd(REG_FIRMWARE_VERSIOIN, (int *)byte, 3);
		if (nRet) {
			g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to read dpcd from device!\n");
			return FALSE;
		}
		priv->version = g_strdup_printf("v%1d.%02d.%03d", byte[0], byte[1], byte[2]);

		// read board ID
		synapticsmst_common_rc_get_command(UPDC_READ_FROM_EEPROM, 2, ADDR_CUSTOMER_ID, byte);
		if (nRet) {
			g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to read from EEPROM of device!\n");
			return FALSE;
		}
		if (byte[0] == 0x01) {
			priv->boardID = (byte[0] << 8) | (byte[1]);
		}
		else if (byte[0] == 0x00) {
			priv->boardID = (byte[0] << 8) | (byte[1]); // remove this when release
		}
		else {
			priv->boardID == 0;
		}

		// read board chipID
		synapticsmst_common_read_dpcd(REG_CHIP_ID, (int *)byte, 2);
		if (nRet) {
			g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to read dpcd from device!\n");
			return FALSE;
		}
		priv->chipID = g_strdup_printf("VMM%02x%02x", byte[0], byte[1]);

		// disable remote control and close aux node
		synapticsmst_device_disable_remote_control(device,error);
		synapticsmst_common_close_aux_node();
	}
	else {
		//g_print("fail to open aux node %s\n", priv->devfs_node);
		g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to open device in DP Aux Node %d\n", synapticsmst_device_get_aux_node_to_int(device));
		return FALSE;
	}

	return TRUE;
}

const gchar *
synapticsmst_device_get_version (SynapticsMSTDevice *device)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);
	return priv->version;
}

const gchar *
synapticsmst_device_get_chipID (SynapticsMSTDevice *device)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);
	return priv->chipID;
}

gboolean
synapticsmst_device_get_flash_checksum (SynapticsMSTDevice *device, int length, int offset, unsigned char *checksum, GError **error)
{
	guint8 nRet;
	if (synapticsmst_common_rc_special_get_command(UPDC_CAL_EEPROM_CHECKSUM, length, offset, NULL, 4, checksum)) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to get flash checksum!\n");
		return FALSE;
	}
	else {
		return TRUE;
	}
}

gboolean
synapticsmst_device_write_firmware (SynapticsMSTDevice *device, GBytes *fw, GError **error)
{
	const guint8 *payload_data;
	guint32 payload_len;
	guint32 code_size = 0;
	guint32 checksum = 0;
	guint32 offset = 0;
	guint32 write_loops = 0;
	guint32 data_to_write = 0;
	guint8 percentage = 0;
	guint8 nRet = 0;
		
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);

	// get firmware data and check size
	payload_data = g_bytes_get_data(fw, NULL);
	payload_len = g_bytes_get_size(fw);
	if (payload_len > 0x10000 || payload_len == 0) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : invalid file size!\n");
		return FALSE;
	}

	// check firmware content
	for (guint8 i=0; i<128; i++) {
		checksum += *(payload_data + i);
	}
	if (checksum & 0xFF) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : EDID checksum error!\n");
		return FALSE;
	}

	checksum = 0;
	offset = 128;
	for (guint8 i=0; i<128; i++) {
		checksum += *(payload_data + offset + i);
	}
	if (checksum & 0xFF) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : EDID checksum error!\n");
		return FALSE;
	}

	checksum = 0;
	offset = 0x100;
	for (guint16 i=0; i<256; i++) {
		checksum += *(payload_data + offset + i);
	}
	if (checksum & 0xFF) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : configuration checksum error!\n");
		return FALSE;
	}

	checksum = 0;
	offset = 0x200;
	for (guint16 i=0; i<256; i++) {
		checksum += *(payload_data + offset + i);
	}
	if (checksum & 0xFF) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : configuration checksum error!\n");
		return FALSE;
	}

	checksum = 0;
	offset = 0x400;
	code_size = (*(payload_data + offset) << 8) + *(payload_data + offset + 1);
	if (code_size >= 0xFFFF) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : invalid firmware size!\n");
		return FALSE;
	}
	for (guint32 i=0; i<(code_size + 17); i++) {
		checksum += *(payload_data + offset + i);
	}
	if (checksum & 0xFF) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : firmware checksum error!\n");
		return FALSE;
	}

	// check firmware and board ID again
	guint16 tmp = (*(payload_data + 0x10E) << 8) + *(payload_data + 0x10F);
	if (tmp != synapticsmst_device_get_boardID(device)) {
		g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : board ID mismatch!\n");
		return FALSE;
	}

	if (synapticsmst_common_open_aux_node(priv->devfs_node)) {
		// enable remote control
		if (!synapticsmst_device_enable_remote_control(device, error)) {
			return FALSE;
		}

		// erase SPI flash
		guint16 erase_ctrl = 0xFFFF;
		if (synapticsmst_common_rc_set_command(UPDC_FLASH_ERASE, 2, 0, (guint8 *)&erase_ctrl)) {
			g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : can't erase flash!\n");
			return FALSE;
		}

		// update firmware
		//g_print ("Writing FW to %s\n", priv->devfs_node);
		write_loops = (payload_len / BLOCK_UNIT);
		data_to_write = payload_len;
		nRet = 0;
		offset = 0;

		if (payload_len % BLOCK_UNIT) {
			write_loops++;
		}

		g_print("updating... 0%%");

		for (guint32 i=0; i<write_loops; i++) {
			guint8 length = BLOCK_UNIT;
			if (data_to_write < BLOCK_UNIT) {
				length = data_to_write;
			}

			nRet = synapticsmst_common_rc_set_command(UPDC_WRITE_TO_EEPROM, length, offset, (payload_data + offset));
			if (nRet) {
				nRet = synapticsmst_common_rc_set_command(UPDC_WRITE_TO_EEPROM, length, offset, (payload_data + offset)); // repeat once
			}

			if (nRet) {
				break;
			}

			offset += length;
			data_to_write -= length;
			percentage = i * 100 / (write_loops - 1);
			g_print("\rupdating... %d%%", percentage);
		}
		g_print("\n");

		if (nRet) {
			g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : can't write flash at offset 0x%04x!\n", offset);
		}
		else {
		// check data just written
			checksum = 0;
			for (guint32 i=0; i<payload_len; i++) {
				checksum += *(payload_data + i);
			}

			guint32 flash_checksum = 0;
			if (synapticsmst_device_get_flash_checksum(device, payload_len, 0, &flash_checksum, error)) {
				if (checksum != flash_checksum) {
					nRet = -1;
					g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : checksum mismatch!\n");
				}
			}
			else {
				nRet = -1;
			}

		}

		// disable remote control and close aux node
		synapticsmst_device_disable_remote_control(device, error);
		synapticsmst_common_close_aux_node();
	}
	else {
		g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Failed to flash firmware : can't open DP Aux node %d!\n", synapticsmst_device_get_aux_node_to_int(device));
		return FALSE;
	}

	if (nRet) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

/**
 * synapticsmst_device_new:
 *
 * Creates a new #SynapticsMSTDevice.
 *
 * Returns: (transfer full): a #SynapticsMSTDevice
 *
 * Since: 0.1.0
 **/
SynapticsMSTDevice *
synapticsmst_device_new (SynapticsMSTDeviceKind kind, const gchar* aux_node)
{
	SynapticsMSTDevice *device;

	device = g_object_new (SYNAPTICSMST_TYPE_DEVICE, NULL);
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);

	priv->devfs_node = aux_node;
	priv->kind = kind;
	priv->version = NULL;

	return SYNAPTICSMST_DEVICE (device);
}

const gchar *
synapticsmst_device_get_aux_node (guint8 index)
{
	if (index == 0) {
		return "/dev/drm_dp_aux0";
	}
	else if (index == 1) {
		return "/dev/drm_dp_aux1";
	}
	else if (index == 2) {
		return "/dev/drm_dp_aux2";
	}
	else {
		return "";
	}
}

guint8
synapticsmst_device_get_aux_node_to_int (SynapticsMSTDevice *device)
{
	SynapticsMSTDevicePrivate *priv = GET_PRIVATE (device);

	if (g_strcmp0(priv->devfs_node, "/dev/drm_dp_aux0") == 0) {
		return 0;
	}
	if (g_strcmp0(priv->devfs_node, "/dev/drm_dp_aux1") == 0) {
		return 1;
	}
	if (g_strcmp0(priv->devfs_node, "/dev/drm_dp_aux2") == 0) {
		return 2;
	}
	return 0xFF;
}

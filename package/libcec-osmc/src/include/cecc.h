#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#ifndef CECEXPORTS_C_H_
#define CECEXPORTS_C_H_

#include "cectypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_initialise(CEC::libcec_configuration *configuration);
#else
extern DECLSPEC int cec_initialise(libcec_configuration *configuration);
#endif

extern DECLSPEC void cec_destroy(void);

extern DECLSPEC int cec_open(const char *strPort, uint32_t iTimeout);

extern DECLSPEC void cec_close(void);

#ifdef __cplusplus
extern DECLSPEC int cec_enable_callbacks(void *cbParam, CEC::ICECCallbacks *callbacks);
#else
extern DECLSPEC int cec_enable_callbacks(void *cbParam, ICECCallbacks *callbacks);
#endif

#ifdef __cplusplus
extern DECLSPEC int8_t cec_find_adapters(CEC::cec_adapter *deviceList, uint8_t iBufSize, const char *strDevicePath);
#else
extern DECLSPEC int8_t cec_find_adapters(cec_adapter *deviceList, uint8_t iBufSize, const char *strDevicePath);
#endif

extern DECLSPEC int cec_ping_adapters(void);

extern DECLSPEC int cec_start_bootloader(void);

#ifdef __cplusplus
extern DECLSPEC int cec_power_on_devices(CEC::cec_logical_address address);
#else
extern DECLSPEC int cec_power_on_devices(cec_logical_address address);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_standby_devices(CEC::cec_logical_address address);
#else
extern DECLSPEC int cec_standby_devices(cec_logical_address address);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_active_source(CEC::cec_device_type type);
#else
extern DECLSPEC int cec_set_active_source(cec_device_type type);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_deck_control_mode(CEC::cec_deck_control_mode mode, int bSendUpdate);
#else
extern DECLSPEC int cec_set_deck_control_mode(cec_deck_control_mode mode, int bSendUpdate);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_deck_info(CEC::cec_deck_info info, int bSendUpdate);
#else
extern DECLSPEC int cec_set_deck_info(cec_deck_info info, int bSendUpdate);
#endif

extern DECLSPEC int cec_set_inactive_view(void);

#ifdef __cplusplus
extern DECLSPEC int cec_set_menu_state(CEC::cec_menu_state state, int bSendUpdate);
#else
extern DECLSPEC int cec_set_menu_state(cec_menu_state state, int bSendUpdate);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_transmit(const CEC::cec_command *data);
#else
extern DECLSPEC int cec_transmit(const cec_command *data);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_logical_address(CEC::cec_logical_address iLogicalAddress);
#else
extern DECLSPEC int cec_set_logical_address(cec_logical_address iLogicalAddress);
#endif

extern DECLSPEC int cec_set_physical_address(uint16_t iPhysicalAddress);

#ifdef __cplusplus
extern DECLSPEC int cec_set_osd_string(CEC::cec_logical_address iLogicalAddress, CEC::cec_display_control duration, const char *strMessage);
#else
extern DECLSPEC int cec_set_osd_string(cec_logical_address iLogicalAddress, cec_display_control duration, const char *strMessage);
#endif

extern DECLSPEC int cec_switch_monitoring(int bEnable);

#ifdef __cplusplus
extern DECLSPEC CEC::cec_version cec_get_device_cec_version(CEC::cec_logical_address iLogicalAddress);
#else
extern DECLSPEC cec_version cec_get_device_cec_version(cec_logical_address iLogicalAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_get_device_menu_language(CEC::cec_logical_address iLogicalAddress, CEC::cec_menu_language *language);
#else
extern DECLSPEC int cec_get_device_menu_language(cec_logical_address iLogicalAddress, cec_menu_language *language);
#endif

#ifdef __cplusplus
extern DECLSPEC uint64_t cec_get_device_vendor_id(CEC::cec_logical_address iLogicalAddress);
#else
extern DECLSPEC uint64_t cec_get_device_vendor_id(cec_logical_address iLogicalAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC uint16_t cec_get_device_physical_address(CEC::cec_logical_address iLogicalAddress);
#else
extern DECLSPEC uint16_t cec_get_device_physical_address(cec_logical_address iLogicalAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC CEC::cec_logical_address cec_get_active_source(void);
#else
extern DECLSPEC cec_logical_address cec_get_active_source(void);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_is_active_source(CEC::cec_logical_address iAddress);
#else
extern DECLSPEC int cec_is_active_source(cec_logical_address iAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC CEC::cec_power_status cec_get_device_power_status(CEC::cec_logical_address iLogicalAddress);
#else
extern DECLSPEC cec_power_status cec_get_device_power_status(cec_logical_address iLogicalAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_poll_device(CEC::cec_logical_address iLogicalAddress);
#else
extern DECLSPEC int cec_poll_device(cec_logical_address iLogicalAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC CEC::cec_logical_addresses cec_get_active_devices(void);
#else
extern DECLSPEC cec_logical_addresses cec_get_active_devices(void);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_is_active_device(CEC::cec_logical_address iAddress);
#else
extern DECLSPEC int cec_is_active_device(cec_logical_address iAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_is_active_device_type(CEC::cec_device_type type);
#else
extern DECLSPEC int cec_is_active_device_type(cec_device_type type);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_hdmi_port(CEC::cec_logical_address iBaseDevice, uint8_t iPort);
#else
extern DECLSPEC int cec_set_hdmi_port(cec_logical_address iBaseDevice, uint8_t iPort);
#endif

extern DECLSPEC int cec_volume_up(int bSendRelease);

extern DECLSPEC int cec_volume_down(int bSendRelease);

extern DECLSPEC int cec_mute_audio(int bSendRelease);

#ifdef __cplusplus
extern DECLSPEC int cec_send_keypress(CEC::cec_logical_address iDestination, CEC::cec_user_control_code key, int bWait);
#else
extern DECLSPEC int cec_send_keypress(cec_logical_address iDestination, cec_user_control_code key, int bWait);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_send_key_release(CEC::cec_logical_address iDestination, int bWait);
#else
extern DECLSPEC int cec_send_key_release(cec_logical_address iDestination, int bWait);
#endif

#ifdef __cplusplus
extern DECLSPEC CEC::cec_osd_name cec_get_device_osd_name(CEC::cec_logical_address iAddress);
#else
extern DECLSPEC cec_osd_name cec_get_device_osd_name(cec_logical_address iAddress);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_stream_path_logical(CEC::cec_logical_address iAddress);
#else
extern DECLSPEC int cec_set_stream_path_logical(cec_logical_address iAddress);
#endif

extern DECLSPEC int cec_set_stream_path_physical(uint16_t iPhysicalAddress);

#ifdef __cplusplus
extern DECLSPEC CEC::cec_logical_addresses cec_get_logical_addresses(void);
#else
extern DECLSPEC cec_logical_addresses cec_get_logical_addresses(void);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_get_current_configuration(CEC::libcec_configuration *configuration);
#else
extern DECLSPEC int cec_get_current_configuration(libcec_configuration *configuration);
#endif

extern DECLSPEC int cec_can_persist_configuration(void);

#ifdef __cplusplus
extern DECLSPEC int cec_persist_configuration(CEC::libcec_configuration *configuration);
#else
extern DECLSPEC int cec_persist_configuration(libcec_configuration *configuration);
#endif

#ifdef __cplusplus
extern DECLSPEC int cec_set_configuration(const CEC::libcec_configuration *configuration);
#else
extern DECLSPEC int cec_set_configuration(const libcec_configuration *configuration);
#endif

extern DECLSPEC void cec_rescan_devices(void);

extern DECLSPEC int cec_is_libcec_active_source(void);

#ifdef __cplusplus
extern DECLSPEC int cec_get_device_information(const char *strPort, CEC::libcec_configuration *config, uint32_t iTimeoutMs);
#else
extern DECLSPEC int cec_get_device_information(const char *strPort, libcec_configuration *config, uint32_t iTimeoutMs);
#endif

extern DECLSPEC const char * cec_get_lib_info(void);

extern DECLSPEC void cec_init_video_standalone(void);

extern DECLSPEC uint16_t cec_get_adapter_vendor_id(void);

extern DECLSPEC uint16_t cec_get_adapter_product_id(void);

extern DECLSPEC uint8_t cec_audio_toggle_mute(void);

extern DECLSPEC uint8_t cec_audio_mute(void);

extern DECLSPEC uint8_t cec_audio_unmute(void);

extern DECLSPEC uint8_t cec_audio_get_status(void);

#ifdef __cplusplus
extern DECLSPEC int8_t cec_detect_adapters(CEC::cec_adapter_descriptor *deviceList, uint8_t iBufSize, const char *strDevicePath, int bQuickScan);
#else
extern DECLSPEC int8_t cec_detect_adapters(cec_adapter_descriptor *deviceList, uint8_t iBufSize, const char *strDevicePath, int bQuickScan);
#endif

#ifdef __cplusplus
};
#endif

#endif /* CECEXPORTS_C_H_ */

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

#include "env.h"
#include "USBCECAdapterCommands.h"

#include "USBCECAdapterMessage.h"
#include "USBCECAdapterCommunication.h"
#include "lib/LibCEC.h"
#include "lib/CECProcessor.h"
#include "lib/CECTypeUtils.h"
#include "lib/platform/util/util.h"
#include <stdio.h>

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_comm->m_callback->GetLib()
#define ToString(p) CCECTypeUtils::ToString(p)

CUSBCECAdapterCommands::CUSBCECAdapterCommands(CUSBCECAdapterCommunication *comm) :
    m_comm(comm),
    m_bSettingsRetrieved(false),
    m_bSettingAutoEnabled(false),
    m_settingCecVersion(CEC_VERSION_UNKNOWN),
    m_iSettingLAMask(0),
    m_bNeedsWrite(false),
    m_iBuildDate(CEC_FW_BUILD_UNKNOWN),
    m_bControlledMode(false),
    m_adapterType(P8_ADAPTERTYPE_UNKNOWN)
{
  m_persistedConfiguration.Clear();
}

cec_datapacket CUSBCECAdapterCommands::RequestSetting(cec_adapter_messagecode msgCode)
{
  cec_datapacket retVal;
  retVal.Clear();

  CCECAdapterMessage params;
  CCECAdapterMessage *message = m_comm->SendCommand(msgCode, params);
  if (message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED)
  {
    retVal = message->response;
    retVal.Shift(2); // shift out start and msgcode
    retVal.size -= 1; // remove end
  }

  DELETE_AND_NULL(message);
  return retVal;
}

uint16_t CUSBCECAdapterCommands::RequestFirmwareVersion(void)
{
  m_persistedConfiguration.iFirmwareVersion = CEC_FW_VERSION_UNKNOWN;
  unsigned int iFwVersionTry(0);

  while (m_persistedConfiguration.iFirmwareVersion == CEC_FW_VERSION_UNKNOWN && iFwVersionTry++ < 3)
  {
#ifdef CEC_DEBUGGING
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting the firmware version");
#endif
    cec_datapacket response = RequestSetting(MSGCODE_FIRMWARE_VERSION);
    if (response.size == 2)
      m_persistedConfiguration.iFirmwareVersion = (response[0] << 8 | response[1]);
    else
    {
      LIB_CEC->AddLog(CEC_LOG_WARNING, "the adapter did not respond with a correct firmware version (try %d, size = %d)", iFwVersionTry, response.size);
      CEvent::Sleep(500);
    }
  }

  if (m_persistedConfiguration.iFirmwareVersion == CEC_FW_VERSION_UNKNOWN)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "defaulting to firmware version 1");
    m_persistedConfiguration.iFirmwareVersion = 1;
  }

  // firmware versions < 2 don't have an autonomous mode
  if (m_persistedConfiguration.iFirmwareVersion < 2)
    m_bControlledMode = true;

  return m_persistedConfiguration.iFirmwareVersion;
}

bool CUSBCECAdapterCommands::RequestSettingAutoEnabled(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting autonomous mode setting");
#endif

  cec_datapacket response = RequestSetting(MSGCODE_GET_AUTO_ENABLED);
  if (response.size == 1)
  {
    m_bSettingAutoEnabled = response[0] == 1;
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted autonomous mode setting: '%s'", m_bSettingAutoEnabled ? "enabled" : "disabled");
    return true;
  }
  return false;
}

bool CUSBCECAdapterCommands::RequestSettingCECVersion(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting CEC version setting");
#endif

  cec_datapacket response = RequestSetting(MSGCODE_GET_HDMI_VERSION);
  if (response.size == 1)
  {
    m_settingCecVersion = (cec_version)response[0];
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted CEC version setting: '%s'", ToString(m_settingCecVersion));
    return true;
  }
  return false;
}

p8_cec_adapter_type CUSBCECAdapterCommands::RequestAdapterType(void)
{
  if (m_adapterType == P8_ADAPTERTYPE_UNKNOWN)
  {
#ifdef CEC_DEBUGGING
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting adapter type");
#endif

    cec_datapacket response = RequestSetting(MSGCODE_GET_ADAPTER_TYPE);
    if (response.size == 1)
      m_adapterType = (p8_cec_adapter_type)response[0];
  }
  return m_adapterType;
}

uint32_t CUSBCECAdapterCommands::RequestBuildDate(void)
{
  if (m_iBuildDate == CEC_FW_BUILD_UNKNOWN)
  {
#ifdef CEC_DEBUGGING
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting firmware build date");
#endif

    cec_datapacket response = RequestSetting(MSGCODE_GET_BUILDDATE);
    if (response.size == 4)
      m_iBuildDate = (uint32_t)response[0] << 24 | (uint32_t)response[1] << 16 | (uint32_t)response[2] << 8 | (uint32_t)response[3];
  }
  return m_iBuildDate;
}

bool CUSBCECAdapterCommands::RequestSettingDefaultLogicalAddress(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting default logical address setting");
#endif

  cec_datapacket response = RequestSetting(MSGCODE_GET_DEFAULT_LOGICAL_ADDRESS);
  if (response.size == 1)
  {
    m_persistedConfiguration.logicalAddresses.primary = (cec_logical_address)response[0];
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted logical address setting: '%s'", ToString(m_persistedConfiguration.logicalAddresses.primary));
    return true;
  }
  return false;
}

bool CUSBCECAdapterCommands::RequestSettingDeviceType(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting device type setting");
#endif
  m_persistedConfiguration.deviceTypes.Clear();

  cec_datapacket response = RequestSetting(MSGCODE_GET_DEVICE_TYPE);
  if (response.size == 1)
  {
    m_persistedConfiguration.deviceTypes.Add((cec_device_type)response[0]);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted device type setting: '%s'", ToString((cec_device_type)response[0]));
    return true;
  }
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "no persisted device type setting");
  return false;
}

bool CUSBCECAdapterCommands::RequestSettingLogicalAddressMask(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting logical address mask setting");
#endif

  cec_datapacket response = RequestSetting(MSGCODE_GET_LOGICAL_ADDRESS_MASK);
  if (response.size == 2)
  {
    m_iSettingLAMask = ((uint16_t)response[0] << 8) | ((uint16_t)response[1]);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted logical address mask setting: '%x'", m_iSettingLAMask);
    return true;
  }
  return false;
}

bool CUSBCECAdapterCommands::RequestSettingOSDName(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting OSD name setting");
#endif

  memset(m_persistedConfiguration.strDeviceName, 0, 13);
  cec_datapacket response = RequestSetting(MSGCODE_GET_OSD_NAME);
  if (response.size == 0)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "no persisted device name setting");
    return false;
  }

  char buf[14];
  for (uint8_t iPtr = 0; iPtr < response.size && iPtr < 13; iPtr++)
    buf[iPtr] = (char)response[iPtr];
  buf[response.size] = 0;

  snprintf(m_persistedConfiguration.strDeviceName, 13, "%s", buf);
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted device name setting: '%s'", buf);
  return true;
}

bool CUSBCECAdapterCommands::RequestSettingPhysicalAddress(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "requesting physical address setting");
#endif

  cec_datapacket response = RequestSetting(MSGCODE_GET_PHYSICAL_ADDRESS);
  if (response.size == 2)
  {
    m_persistedConfiguration.iPhysicalAddress = ((uint16_t)response[0] << 8) | ((uint16_t)response[1]);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using persisted physical address setting: '%4x'", m_persistedConfiguration.iPhysicalAddress);
    return true;
  }
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "no persisted physical address setting");
  return false;
}

bool CUSBCECAdapterCommands::SetSettingAutoEnabled(bool enabled)
{
  bool bReturn(false);

  /* check whether this value was changed */
  {
    CLockObject lock(m_mutex);
    if (m_bSettingAutoEnabled == enabled)
      return bReturn;
    m_bNeedsWrite = true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "turning autonomous mode %s", enabled ? "on" : "off");

  CCECAdapterMessage params;
  params.PushEscaped(enabled ? 1 : 0);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_AUTO_ENABLED, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_bSettingAutoEnabled = enabled;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::SetSettingDeviceType(cec_device_type type)
{
  bool bReturn(false);

  /* check whether this value was changed */
  {
    CLockObject lock(m_mutex);
    if (m_persistedConfiguration.deviceTypes.types[0] == type)
      return bReturn;
    m_bNeedsWrite = true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the device type to %X (previous: %X)", (uint8_t)type, (uint8_t)m_persistedConfiguration.deviceTypes.types[0]);

  CCECAdapterMessage params;
  params.PushEscaped((uint8_t)type);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_DEVICE_TYPE, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_persistedConfiguration.deviceTypes.types[0] = type;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::SetSettingDefaultLogicalAddress(cec_logical_address address)
{
  bool bReturn(false);

  /* check whether this value was changed */
  {
    CLockObject lock(m_mutex);
    if (m_persistedConfiguration.logicalAddresses.primary == address)
      return bReturn;
    m_bNeedsWrite = true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the default logical address to %X (previous: %X)", (uint8_t)address, (uint8_t)m_persistedConfiguration.logicalAddresses.primary);

  CCECAdapterMessage params;
  params.PushEscaped((uint8_t)address);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_DEFAULT_LOGICAL_ADDRESS, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_persistedConfiguration.logicalAddresses.primary = address;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::SetSettingLogicalAddressMask(uint16_t iMask)
{
  bool bReturn(false);

  /* check whether this value was changed */
  {
    CLockObject lock(m_mutex);
    if (m_iSettingLAMask == iMask)
      return bReturn;
    m_bNeedsWrite = true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the logical address mask to %2X (previous: %2X)", iMask, m_iSettingLAMask);

  CCECAdapterMessage params;
  params.PushEscaped(iMask >> 8);
  params.PushEscaped((uint8_t)iMask);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_LOGICAL_ADDRESS_MASK, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_iSettingLAMask = iMask;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::SetSettingPhysicalAddress(uint16_t iPhysicalAddress)
{
  bool bReturn(false);

  /* check whether this value was changed */
  {
    CLockObject lock(m_mutex);
    if (m_persistedConfiguration.iPhysicalAddress == iPhysicalAddress)
      return bReturn;
    m_bNeedsWrite = true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the physical address to %04X (previous: %04X)", iPhysicalAddress, m_persistedConfiguration.iPhysicalAddress);

  CCECAdapterMessage params;
  params.PushEscaped(iPhysicalAddress >> 8);
  params.PushEscaped((uint8_t)iPhysicalAddress);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_PHYSICAL_ADDRESS, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_persistedConfiguration.iPhysicalAddress = iPhysicalAddress;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::SetSettingCECVersion(cec_version version)
{
  bool bReturn(false);

  /* check whether this value was changed */
  {
    CLockObject lock(m_mutex);
    if (m_settingCecVersion == version)
      return bReturn;
    m_bNeedsWrite = true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the CEC version to %s (previous: %s)", ToString(version), ToString(m_settingCecVersion));

  CCECAdapterMessage params;
  params.PushEscaped((uint8_t)version);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_HDMI_VERSION, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_settingCecVersion = version;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::SetSettingOSDName(const char *strOSDName)
{
  bool bReturn(false);

  /* check whether this value was changed */
  if (!strcmp(m_persistedConfiguration.strDeviceName, strOSDName))
    return bReturn;

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the OSD name to %s (previous: %s)", strOSDName, m_persistedConfiguration.strDeviceName);

  CCECAdapterMessage params;
  for (size_t iPtr = 0; iPtr < strlen(strOSDName); iPtr++)
    params.PushEscaped(strOSDName[iPtr]);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_OSD_NAME, params);
  bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
    snprintf(m_persistedConfiguration.strDeviceName, 13, "%s", strOSDName);

  return bReturn;
}

bool CUSBCECAdapterCommands::WriteEEPROM(void)
{
  {
    CLockObject lock(m_mutex);
    if (!m_bNeedsWrite)
      return true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "writing settings in the EEPROM");

  CCECAdapterMessage params;
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_WRITE_EEPROM, params);
  bool bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_bNeedsWrite = false;
  }

  return bReturn;
}

bool CUSBCECAdapterCommands::PersistConfiguration(const libcec_configuration &configuration)
{
  bool bReturn(false);
  if (m_persistedConfiguration.iFirmwareVersion < 2)
    return bReturn;

  if (!RequestSettings())
    return bReturn;

  bReturn |= SetSettingAutoEnabled(true);
  bReturn |= SetSettingDeviceType(CLibCEC::GetType(configuration.logicalAddresses.primary));
  bReturn |= SetSettingDefaultLogicalAddress(configuration.logicalAddresses.primary);
  bReturn |= SetSettingLogicalAddressMask(CLibCEC::GetMaskForType(configuration.logicalAddresses.primary));
  bReturn |= SetSettingPhysicalAddress(configuration.iPhysicalAddress);
  bReturn |= SetSettingCECVersion(configuration.clientVersion >= CEC_CLIENT_VERSION_1_8_0 ? configuration.cecVersion : CEC_VERSION_1_4);
  bReturn |= SetSettingOSDName(configuration.strDeviceName);

  return bReturn;
}

bool CUSBCECAdapterCommands::RequestSettings(void)
{
  if (m_persistedConfiguration.iFirmwareVersion < 2)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - firmware version %d does not have any eeprom settings", __FUNCTION__, m_persistedConfiguration.iFirmwareVersion);
    // settings can only be persisted with firmware v2+
    return false;
  }

  if (m_bSettingsRetrieved)
    return true;

  bool bReturn(true);
  bReturn &= RequestSettingAutoEnabled();
  bReturn &= RequestSettingCECVersion();
  bReturn &= RequestSettingDefaultLogicalAddress();
  bReturn &= RequestSettingDeviceType();
  bReturn &= RequestSettingLogicalAddressMask();
  bReturn &= RequestSettingOSDName();
  bReturn &= RequestSettingPhysicalAddress();

  // don't read the following settings:
  // - auto enabled (always enabled)
  // - default logical address (autodetected)
  // - logical address mask (autodetected)
  // - CEC version (1.3a)

  // TODO to be added to the firmware:
  // - base device (4 bits)
  // - HDMI port number (4 bits)
  // - TV vendor id (12 bits)
  // - wake devices (8 bits)
  // - standby devices (8 bits)
  // - use TV menu language (1 bit)
  // - activate source (1 bit)
  // - power off screensaver (1 bit)
  // - power off on standby (1 bit)
  // - send inactive source (1 bit)

  m_bSettingsRetrieved = true;

  return bReturn;
}

bool CUSBCECAdapterCommands::GetConfiguration(libcec_configuration &configuration)
{
  // get the settings from the eeprom if needed
  if (!RequestSettings())
    return false;

  // copy the settings
  configuration.iFirmwareVersion = m_persistedConfiguration.iFirmwareVersion;
  configuration.deviceTypes      = m_persistedConfiguration.deviceTypes;
  configuration.iPhysicalAddress = m_persistedConfiguration.iPhysicalAddress;
  snprintf(configuration.strDeviceName, 13, "%s", m_persistedConfiguration.strDeviceName);

  return true;
}

bool CUSBCECAdapterCommands::PingAdapter(void)
{
#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "sending ping");
#endif

  CCECAdapterMessage params;
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_PING, params);
  bool bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);
  return bReturn;
}

bool CUSBCECAdapterCommands::SetAckMask(uint16_t iMask)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting ackmask to %2x", iMask);

  CCECAdapterMessage params;
  params.PushEscaped(iMask >> 8);
  params.PushEscaped((uint8_t)iMask);
  CCECAdapterMessage *message  = m_comm->SendCommand(MSGCODE_SET_ACK_MASK, params);
  bool bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);
  return bReturn;
}

void CUSBCECAdapterCommands::SetActiveSource(bool bSetTo, bool bClientUnregistered)
{
  if (bClientUnregistered) return;
  if (m_persistedConfiguration.iFirmwareVersion >= 3)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "marking the adapter as %s source", bSetTo ? "active" : "inactive");
    CCECAdapterMessage params;
    params.PushEscaped(bSetTo ? 1 : 0);
    CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_ACTIVE_SOURCE, params);
    DELETE_AND_NULL(message);
  }
}

bool CUSBCECAdapterCommands::StartBootloader(void)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "starting the bootloader");

  CCECAdapterMessage params;
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_START_BOOTLOADER, params);
  bool bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);
  return bReturn;
}

bool CUSBCECAdapterCommands::SetLineTimeout(uint8_t iTimeout)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting the line timeout to %d", iTimeout);
  CCECAdapterMessage params;
  params.PushEscaped(iTimeout);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_TRANSMIT_IDLETIME, params);
  bool bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);
  return bReturn;
}

bool CUSBCECAdapterCommands::SetControlledMode(bool controlled)
{
  {
    CLockObject lock(m_mutex);
    if (m_bControlledMode == controlled)
      return true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "turning controlled mode %s", controlled ? "on" : "off");

  CCECAdapterMessage params;
  params.PushEscaped(controlled ? 1 : 0);
  CCECAdapterMessage *message = m_comm->SendCommand(MSGCODE_SET_CONTROLLED, params);
  bool bReturn = message && message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED;
  DELETE_AND_NULL(message);

  if (bReturn)
  {
    CLockObject lock(m_mutex);
    m_bControlledMode = controlled;
  }

  return bReturn;
}

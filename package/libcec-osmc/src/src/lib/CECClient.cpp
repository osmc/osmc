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
#include "CECClient.h"

#include "CECProcessor.h"
#include "LibCEC.h"
#include "CECTypeUtils.h"
#include "devices/CECPlaybackDevice.h"
#include "devices/CECAudioSystem.h"
#include "devices/CECTV.h"
#include "implementations/CECCommandHandler.h"

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_processor->GetLib()
#define ToString(x) CCECTypeUtils::ToString(x)

CCECClient::CCECClient(CCECProcessor *processor, const libcec_configuration &configuration) :
    m_processor(processor),
    m_bInitialised(false),
    m_bRegistered(false),
    m_iCurrentButton(CEC_USER_CONTROL_CODE_UNKNOWN),
    m_buttontime(0),
    m_iPreventForwardingPowerOffCommand(0),
    m_iLastKeypressTime(0)
{
  m_lastKeypress.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;
  m_lastKeypress.duration = 0;
  m_configuration.Clear();
  // set the initial configuration
  SetConfiguration(configuration);
}

CCECClient::~CCECClient(void)
{
  // unregister the client
  if (m_processor && IsRegistered())
    m_processor->UnregisterClient(this);
}

bool CCECClient::IsInitialised(void)
{
  CLockObject lock(m_mutex);
  return m_bInitialised && m_processor;
}

void CCECClient::SetInitialised(bool bSetTo)
{
  CLockObject lock(m_mutex);
  m_bInitialised = bSetTo;
}

bool CCECClient::IsRegistered(void)
{
  CLockObject lock(m_mutex);
  return m_bRegistered && m_processor;
}

void CCECClient::SetRegistered(bool bSetTo)
{
  CLockObject lock(m_mutex);
  m_bRegistered = bSetTo;
}

bool CCECClient::OnRegister(void)
{
  // return false if already initialised
  if (IsInitialised())
    return true;

  // get all device we control
  CECDEVICEVEC devices;
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);

  // return false when no devices were found
  if (devices.empty())
  {
    LIB_CEC->AddLog(CEC_LOG_WARNING, "cannot find the primary device (logical address %x)", GetPrimaryLogicalAdddress());
    return false;
  }

  // mark as initialised
  SetInitialised(true);

  // configure all devices
  for (CECDEVICEVEC::iterator it = devices.begin(); it != devices.end(); it++)
  {
    // only set our OSD name for the primary device
    if ((*it)->GetLogicalAddress() == GetPrimaryLogicalAdddress())
      (*it)->SetOSDName(m_configuration.strDeviceName);

    // set the default menu language for devices we control
    (*it)->SetMenuLanguage(m_configuration.strDeviceLanguage);
  }

  // set the physical address
  SetPhysicalAddress(m_configuration);

  // make the primary device the active source if the option is set
  if (m_configuration.bActivateSource == 1)
    GetPrimaryDevice()->ActivateSource(500);

  return true;
}

bool CCECClient::SetHDMIPort(const cec_logical_address iBaseDevice, const uint8_t iPort, bool bForce /* = false */)
{
  bool bReturn(false);

  // limit the HDMI port range to 1-15
  if (iPort < CEC_MIN_HDMI_PORTNUMBER ||
      iPort > CEC_MAX_HDMI_PORTNUMBER)
    return bReturn;

  LIB_CEC->AddLog(CEC_LOG_NOTICE, "setting HDMI port to %d on device %s (%d)", iPort, ToString(iBaseDevice), (int)iBaseDevice);

  // update the configuration
  {
    CLockObject lock(m_mutex);
    m_configuration.baseDevice = iBaseDevice;
    m_configuration.iHDMIPort  = iPort;
  }

  // don't continue if the connection isn't opened
  if (!m_processor->CECInitialised() && !bForce)
    return true;

  // get the PA of the base device
  uint16_t iPhysicalAddress(CEC_INVALID_PHYSICAL_ADDRESS);
  CCECBusDevice *baseDevice = m_processor->GetDevice(iBaseDevice);
  if (baseDevice)
    iPhysicalAddress = baseDevice->GetPhysicalAddress(GetPrimaryLogicalAdddress());

  // add our port number
  if (iPhysicalAddress <= CEC_MAX_PHYSICAL_ADDRESS)
  {
    if (iPhysicalAddress == 0)
      iPhysicalAddress += 0x1000 * iPort;
    else if (iPhysicalAddress % 0x1000 == 0)
      iPhysicalAddress += 0x100 * iPort;
    else if (iPhysicalAddress % 0x100 == 0)
      iPhysicalAddress += 0x10 * iPort;
    else if (iPhysicalAddress % 0x10 == 0)
      iPhysicalAddress += iPort;

    bReturn = true;
  }

  // set the default address when something went wrong
  if (!bReturn)
  {
    uint16_t iEepromAddress = m_processor->GetPhysicalAddressFromEeprom();
    if (CLibCEC::IsValidPhysicalAddress(iEepromAddress))
    {
      LIB_CEC->AddLog(CEC_LOG_WARNING, "failed to set the physical address to %04X, setting it to the value that was persisted in the eeprom, %04X", iPhysicalAddress, iEepromAddress);
      iPhysicalAddress = iEepromAddress;
      bReturn = true;
    }
    else
    {
      LIB_CEC->AddLog(CEC_LOG_WARNING, "failed to set the physical address to %04X, setting it to the default value %04X", iPhysicalAddress, CEC_DEFAULT_PHYSICAL_ADDRESS);
      iPhysicalAddress = CEC_DEFAULT_PHYSICAL_ADDRESS;
    }
  }

  // and set the address
  SetDevicePhysicalAddress(iPhysicalAddress);

  CallbackConfigurationChanged(m_configuration);

  return bReturn;
}

void CCECClient::ResetPhysicalAddress(void)
{
  SetPhysicalAddress(CEC_DEFAULT_PHYSICAL_ADDRESS);
}

void CCECClient::SetPhysicalAddress(const libcec_configuration &configuration)
{
  bool bPASet(false);

  // override the physical address from configuration.iPhysicalAddress if it's set
  if (!bPASet && CLibCEC::IsValidPhysicalAddress(configuration.iPhysicalAddress))
    bPASet = SetPhysicalAddress(configuration.iPhysicalAddress);

  // try to autodetect the address
  if (!bPASet && m_processor->CECInitialised())
  {
    bPASet = AutodetectPhysicalAddress();
    m_configuration.bAutodetectAddress = bPASet ? 1 : 0;
  }

  // use the base device + hdmi port settings
  if (!bPASet)
    bPASet = SetHDMIPort(configuration.baseDevice, configuration.iHDMIPort);

  // reset to defaults if something went wrong
  if (!bPASet)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - resetting HDMI port and base device to defaults", __FUNCTION__);
    m_configuration.baseDevice = CECDEVICE_UNKNOWN;
    m_configuration.iHDMIPort  = CEC_HDMI_PORTNUMBER_NONE;
  }
}

bool CCECClient::SetPhysicalAddress(const uint16_t iPhysicalAddress)
{
  // update the configuration
  bool bChanged(true);
  {
    CLockObject lock(m_mutex);
    if (m_configuration.iPhysicalAddress == iPhysicalAddress)
      bChanged = false;
    else
      m_configuration.iPhysicalAddress = iPhysicalAddress;
  }
  if (!bChanged)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "physical address unchanged (%04X)", iPhysicalAddress);
    return true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting physical address to '%04X'", iPhysicalAddress);

  // set the physical address for each device
  SetDevicePhysicalAddress(iPhysicalAddress);

  // and send back the updated configuration
  CallbackConfigurationChanged(m_configuration);

  return true;
}

void CCECClient::SetSupportedDeviceTypes(void)
{
  cec_device_type_list types;
  types.Clear();

  // get the command handler for the tv
  CCECCommandHandler *tvHandler = m_processor->GetTV()->GetHandler();
  if (!tvHandler)
    return;

  // check all device types
  for (uint8_t iPtr = 0; iPtr < 5; iPtr++)
  {
    if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_RESERVED)
      continue;

    // get the supported device type. the handler will replace types it doesn't support by one it does support
    cec_device_type type = tvHandler->GetReplacementDeviceType(m_configuration.deviceTypes.types[iPtr]);
    if (!types.IsSet(type))
      types.Add(type);
  }
  m_processor->GetTV()->MarkHandlerReady();

  // set the new type list
  m_configuration.deviceTypes = types;

  // persist the new configuration
  PersistConfiguration(m_configuration);
}

bool CCECClient::AllocateLogicalAddresses(void)
{
  // reset all previous LAs that were set
  m_configuration.logicalAddresses.Clear();

  // get the supported device types from the command handler of the TV
  SetSupportedDeviceTypes();

  // display an error if no device types are set
  if (m_configuration.deviceTypes.IsEmpty())
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "no device types given");
    return false;
  }

  // check each entry of the list
  for (uint8_t iPtr = 0; iPtr < 5; iPtr++)
  {
    if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_RESERVED)
      continue;

    // find an LA for this type
    cec_logical_address address(CECDEVICE_UNKNOWN);
    if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_RECORDING_DEVICE)
      address = AllocateLogicalAddressRecordingDevice();
    if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_TUNER)
      address = AllocateLogicalAddressTuner();
    if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_PLAYBACK_DEVICE)
      address = AllocateLogicalAddressPlaybackDevice();
    if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
      address = AllocateLogicalAddressAudioSystem();

    // display an error if no LA could be allocated
    if (address == CECDEVICE_UNKNOWN)
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - failed to allocate device '%d', type '%s'", __FUNCTION__, iPtr, ToString(m_configuration.deviceTypes.types[iPtr]));
      return false;
    }

    // display the registered LA
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - device '%d', type '%s', LA '%X'", __FUNCTION__, iPtr, ToString(m_configuration.deviceTypes.types[iPtr]), address);
    m_configuration.logicalAddresses.Set(address);
  }

  // persist the new configuration
  PersistConfiguration(m_configuration);

  return true;
}

cec_logical_address CCECClient::AllocateLogicalAddressRecordingDevice(void)
{
  cec_logical_address retVal(CECDEVICE_UNKNOWN);

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'recording device'");
  if (m_processor->TryLogicalAddress(CECDEVICE_RECORDINGDEVICE1, m_configuration.cecVersion))
    retVal = CECDEVICE_RECORDINGDEVICE1;
  else if (m_processor->TryLogicalAddress(CECDEVICE_RECORDINGDEVICE2, m_configuration.cecVersion))
    retVal = CECDEVICE_RECORDINGDEVICE2;
  else if (m_processor->TryLogicalAddress(CECDEVICE_RECORDINGDEVICE3, m_configuration.cecVersion))
    retVal = CECDEVICE_RECORDINGDEVICE3;

  return retVal;
}

cec_logical_address CCECClient::AllocateLogicalAddressTuner(void)
{
  cec_logical_address retVal(CECDEVICE_UNKNOWN);

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'tuner'");
  if (m_processor->TryLogicalAddress(CECDEVICE_TUNER1, m_configuration.cecVersion))
    retVal = CECDEVICE_TUNER1;
  else if (m_processor->TryLogicalAddress(CECDEVICE_TUNER2, m_configuration.cecVersion))
    retVal = CECDEVICE_TUNER2;
  else if (m_processor->TryLogicalAddress(CECDEVICE_TUNER3, m_configuration.cecVersion))
    retVal = CECDEVICE_TUNER3;
  else if (m_processor->TryLogicalAddress(CECDEVICE_TUNER4, m_configuration.cecVersion))
    retVal = CECDEVICE_TUNER4;

  return retVal;
}

cec_logical_address CCECClient::AllocateLogicalAddressPlaybackDevice(void)
{
  cec_logical_address retVal(CECDEVICE_UNKNOWN);

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'playback device'");
  if (m_processor->TryLogicalAddress(CECDEVICE_PLAYBACKDEVICE1, m_configuration.cecVersion))
    retVal = CECDEVICE_PLAYBACKDEVICE1;
  else if (m_processor->TryLogicalAddress(CECDEVICE_PLAYBACKDEVICE2, m_configuration.cecVersion))
    retVal = CECDEVICE_PLAYBACKDEVICE2;
  else if (m_processor->TryLogicalAddress(CECDEVICE_PLAYBACKDEVICE3, m_configuration.cecVersion))
    retVal = CECDEVICE_PLAYBACKDEVICE3;

  return retVal;
}

cec_logical_address CCECClient::AllocateLogicalAddressAudioSystem(void)
{
  cec_logical_address retVal(CECDEVICE_UNKNOWN);

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'audiosystem'");
  if (m_processor->TryLogicalAddress(CECDEVICE_AUDIOSYSTEM, m_configuration.cecVersion))
    retVal = CECDEVICE_AUDIOSYSTEM;

  return retVal;
}

CCECBusDevice *CCECClient::GetDeviceByType(const cec_device_type type) const
{
  // get all devices that match our logical addresses
  CECDEVICEVEC devices;
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);

  // filter the type we need
  CCECDeviceMap::FilterType(type, devices);

  return devices.empty() ?
      NULL :
      *devices.begin();
}

bool CCECClient::ChangeDeviceType(const cec_device_type from, const cec_device_type to)
{
  if (from == to)
    return true;

  LIB_CEC->AddLog(CEC_LOG_NOTICE, "changing device type '%s' into '%s'", ToString(from), ToString(to));

  {
    CLockObject lock(m_mutex);

    // get the previous device that was allocated
    CCECBusDevice *previousDevice = GetDeviceByType(from);
    if (!previousDevice)
      return false;

    // change the type in the device type list
    bool bChanged(false);
    for (uint8_t iPtr = 0; iPtr < 5; iPtr++)
    {
      if (m_configuration.deviceTypes.types[iPtr] == CEC_DEVICE_TYPE_RESERVED)
        continue;

      if (m_configuration.deviceTypes.types[iPtr] == from)
      {
        bChanged = true;
        m_configuration.deviceTypes.types[iPtr] = to;
      }
      else if (m_configuration.deviceTypes.types[iPtr] == to && bChanged)
      {
        // ensure that dupes are removed
        m_configuration.deviceTypes.types[iPtr] = CEC_DEVICE_TYPE_RESERVED;
      }
    }
  }

  // re-register the client to set the new ackmask
  if (!m_processor->RegisterClient(this))
    return false;

  // persist the new configuration
  PersistConfiguration(m_configuration);

  return true;
}

bool CCECClient::SetLogicalAddress(const cec_logical_address iLogicalAddress)
{
  bool bReturn(true);

  if (GetPrimaryLogicalAdddress() != iLogicalAddress)
  {
    LIB_CEC->AddLog(CEC_LOG_NOTICE, "setting primary logical address to %1x", iLogicalAddress);
    {
      CLockObject lock(m_mutex);
      m_configuration.logicalAddresses.primary = iLogicalAddress;
      m_configuration.logicalAddresses.Set(iLogicalAddress);
    }

    bReturn = m_processor->RegisterClient(this);

    // persist the new configuration
    if (bReturn)
      PersistConfiguration(m_configuration);
  }

  return bReturn;
}

bool CCECClient::Transmit(const cec_command &data, bool bIsReply)
{
  return m_processor ? m_processor->Transmit(data, bIsReply) : false;
}

bool CCECClient::SendPowerOnDevices(const cec_logical_address address /* = CECDEVICE_TV */)
{
  // if the broadcast address if set as destination, read the wakeDevices setting
  if (address == CECDEVICE_BROADCAST)
  {
    CECDEVICEVEC devices;
    m_processor->GetDevices()->GetWakeDevices(m_configuration, devices);
    return m_processor->PowerOnDevices(GetPrimaryLogicalAdddress(), devices);
  }

  return m_processor->PowerOnDevice(GetPrimaryLogicalAdddress(), address);
}

bool CCECClient::SendStandbyDevices(const cec_logical_address address /* = CECDEVICE_BROADCAST */)
{
  // if the broadcast address if set as destination, read the standbyDevices setting
  if (address == CECDEVICE_BROADCAST)
  {
    CECDEVICEVEC devices;
    m_processor->GetDevices()->GetPowerOffDevices(m_configuration, devices);
    return m_processor->StandbyDevices(GetPrimaryLogicalAdddress(), devices);
  }

  return m_processor->StandbyDevice(GetPrimaryLogicalAdddress(), address);
}

bool CCECClient::SendSetActiveSource(const cec_device_type type /* = CEC_DEVICE_TYPE_RESERVED */)
{
  // get the devices that are controlled by us
  CECDEVICEVEC devices;
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);

  // filter out the device that matches the given type
  if (type != CEC_DEVICE_TYPE_RESERVED)
    CCECDeviceMap::FilterType(type, devices);

  // no devices left, re-fetch the list of devices that are controlled by us
  if (devices.empty())
    m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);

  if (!devices.empty())
  {
    // get the first device from the list
    CCECBusDevice *device = *devices.begin();

    // and activate it
    if (!m_processor->CECInitialised())
      device->MarkAsActiveSource();
    else if (device->HasValidPhysicalAddress())
      return device->ActivateSource();
  }

  return false;
}

CCECPlaybackDevice *CCECClient::GetPlaybackDevice(void)
{
  CCECPlaybackDevice *device(NULL);
  CECDEVICEVEC devices;

  // get the playback devices
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);
  CCECDeviceMap::FilterType(CEC_DEVICE_TYPE_PLAYBACK_DEVICE, devices);

  // no matches, get the recording devices
  if (devices.empty())
  {
    m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);
    CCECDeviceMap::FilterType(CEC_DEVICE_TYPE_RECORDING_DEVICE, devices);
  }

  // get the first device that matches, and cast it to CCECPlaybackDevice
  if (!devices.empty())
    device = (*devices.begin())->AsPlaybackDevice();

  return device;
}

cec_logical_address CCECClient::GetPrimaryLogicalAdddress(void)
{
  CLockObject lock(m_mutex);
  return m_configuration.logicalAddresses.primary;
}

CCECBusDevice *CCECClient::GetPrimaryDevice(void)
{
  return m_processor->GetDevice(GetPrimaryLogicalAdddress());
}

bool CCECClient::SendSetDeckControlMode(const cec_deck_control_mode mode, bool bSendUpdate /* = true */)
{
  // find a playback device that we control
  CCECPlaybackDevice *device = GetPlaybackDevice();
  if (device)
  {
    // and set the deck control mode if there is a match
    device->SetDeckControlMode(mode);
    if (bSendUpdate)
      return device->TransmitDeckStatus(CECDEVICE_TV, false);
    return true;
  }

  // no match
  return false;
}

bool CCECClient::SendSetDeckInfo(const cec_deck_info info, bool bSendUpdate /* = true */)
{
  // find a playback device that we control
  CCECPlaybackDevice *device = GetPlaybackDevice();
  if (device)
  {
    // and set the deck status if there is a match
    device->SetDeckStatus(info);
    if (bSendUpdate)
      return device->AsPlaybackDevice()->TransmitDeckStatus(CECDEVICE_TV, false);
    return true;
  }

  // no match
  return false;
}

bool CCECClient::SendSetMenuState(const cec_menu_state state, bool bSendUpdate /* = true */)
{
  CECDEVICEVEC devices;

  // set the menu state for all devices that are controlled by us
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);
  for (CECDEVICEVEC::iterator it = devices.begin(); it != devices.end(); it++)
  {
    (*it)->SetMenuState(state);
    if (bSendUpdate)
      (*it)->TransmitMenuState(CECDEVICE_TV, false);
  }

  return true;
}

bool CCECClient::SendSetInactiveView(void)
{
  CECDEVICEVEC devices;

  // mark all devices that are controlled by us as inactive source
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);
  for (CECDEVICEVEC::iterator it = devices.begin(); it != devices.end(); it++)
  {
    if ((*it)->IsActiveSource())
    {
      (*it)->MarkAsInactiveSource();
      return (*it)->TransmitInactiveSource();
    }
  }

  return true;
}

bool CCECClient::SendSetOSDString(const cec_logical_address iLogicalAddress, const cec_display_control duration, const char *strMessage)
{
  CCECBusDevice *primary = GetPrimaryDevice();
  if (primary)
    return primary->TransmitOSDString(iLogicalAddress, duration, strMessage, false);

  return false;
}

cec_version CCECClient::GetDeviceCecVersion(const cec_logical_address iAddress)
{
  CCECBusDevice *device = m_processor->GetDevice(iAddress);
  if (device)
    return device->GetCecVersion(GetPrimaryLogicalAdddress());
  return CEC_VERSION_UNKNOWN;
}

bool CCECClient::GetDeviceMenuLanguage(const cec_logical_address iAddress, cec_menu_language &language)
{
  CCECBusDevice *device = m_processor->GetDevice(iAddress);
  if (device)
  {
    language = device->GetMenuLanguage(GetPrimaryLogicalAdddress());
    return (strcmp(language.language, "???") != 0);
  }
  return false;
}

cec_osd_name CCECClient::GetDeviceOSDName(const cec_logical_address iAddress)
{
  cec_osd_name retVal;
  retVal.device = iAddress;
  retVal.name[0] = 0;

  CCECBusDevice *device = m_processor->GetDevice(iAddress);
  if (device)
  {
    CStdString strOSDName = device->GetOSDName(GetPrimaryLogicalAdddress());
    snprintf(retVal.name, sizeof(retVal.name), "%s", strOSDName.c_str());
    retVal.device = iAddress;
  }

  return retVal;
}

uint16_t CCECClient::GetDevicePhysicalAddress(const cec_logical_address iAddress)
{
  CCECBusDevice *device = m_processor->GetDevice(iAddress);
  if (device)
    return device->GetPhysicalAddress(GetPrimaryLogicalAdddress());
  return CEC_INVALID_PHYSICAL_ADDRESS;
}

cec_power_status CCECClient::GetDevicePowerStatus(const cec_logical_address iAddress)
{
  CCECBusDevice *device = m_processor->GetDevice(iAddress);
  if (device)
    return device->GetPowerStatus(GetPrimaryLogicalAdddress());
  return CEC_POWER_STATUS_UNKNOWN;
}

uint64_t CCECClient::GetDeviceVendorId(const cec_logical_address iAddress)
{
  CCECBusDevice *device = m_processor->GetDevice(iAddress);
  if (device)
    return device->GetVendorId(GetPrimaryLogicalAdddress());
  return CEC_VENDOR_UNKNOWN;
}

uint8_t CCECClient::SendVolumeUp(bool bSendRelease /* = true */)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();

  return device && audio && audio->IsPresent() ?
      audio->VolumeUp(device->GetLogicalAddress(), bSendRelease) :
      (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CCECClient::SendVolumeDown(bool bSendRelease /* = true */)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();

  return device && audio && audio->IsPresent() ?
      audio->VolumeDown(device->GetLogicalAddress(), bSendRelease) :
      (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CCECClient::SendMuteAudio(void)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();

  return device && audio && audio->IsPresent() ?
      audio->MuteAudio(device->GetLogicalAddress()) :
      (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CCECClient::AudioToggleMute(void)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();

  return device && audio && audio->IsPresent() ?
      audio->MuteAudio(device->GetLogicalAddress()) :
      (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CCECClient::AudioMute(void)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();
  uint8_t iStatus = device && audio && audio->IsPresent() ? audio->GetAudioStatus(device->GetLogicalAddress()) : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
  if ((iStatus & CEC_AUDIO_MUTE_STATUS_MASK) != CEC_AUDIO_MUTE_STATUS_MASK)
    iStatus = audio->MuteAudio(device->GetLogicalAddress());

  return iStatus;
}

uint8_t CCECClient::AudioUnmute(void)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();
  uint8_t iStatus = device && audio && audio->IsPresent() ? audio->GetAudioStatus(device->GetLogicalAddress()) : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
  if ((iStatus & CEC_AUDIO_MUTE_STATUS_MASK) == CEC_AUDIO_MUTE_STATUS_MASK)
    iStatus = audio->MuteAudio(device->GetLogicalAddress());

  return iStatus;
}

uint8_t CCECClient::AudioStatus(void)
{
  CCECBusDevice *device = GetPrimaryDevice();
  CCECAudioSystem *audio = m_processor->GetAudioSystem();
  return device && audio && audio->IsPresent() ? audio->GetAudioStatus(device->GetLogicalAddress()) : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

bool CCECClient::SendKeypress(const cec_logical_address iDestination, const cec_user_control_code key, bool bWait /* = true */)
{
  CCECBusDevice *dest = m_processor->GetDevice(iDestination);

  return dest ?
      dest->TransmitKeypress(GetPrimaryLogicalAdddress(), key, bWait) :
      false;
}

bool CCECClient::SendKeyRelease(const cec_logical_address iDestination, bool bWait /* = true */)
{
  CCECBusDevice *dest = m_processor->GetDevice(iDestination);

  return dest ?
      dest->TransmitKeyRelease(GetPrimaryLogicalAdddress(), bWait) :
      false;
}

bool CCECClient::GetCurrentConfiguration(libcec_configuration &configuration)
{
  CLockObject lock(m_mutex);

  // client version 1.5.0
  snprintf(configuration.strDeviceName, 13, "%s", m_configuration.strDeviceName);
  configuration.deviceTypes               = m_configuration.deviceTypes;
  configuration.bAutodetectAddress        = m_configuration.bAutodetectAddress;
  configuration.iPhysicalAddress          = m_configuration.iPhysicalAddress;
  configuration.baseDevice                = m_configuration.baseDevice;
  configuration.iHDMIPort                 = m_configuration.iHDMIPort;
  configuration.clientVersion             = m_configuration.clientVersion;
  configuration.serverVersion             = m_configuration.serverVersion;
  configuration.tvVendor                  = m_configuration.tvVendor;
  configuration.bGetSettingsFromROM       = m_configuration.bGetSettingsFromROM;
  configuration.bUseTVMenuLanguage        = m_configuration.bUseTVMenuLanguage;
  configuration.bActivateSource           = m_configuration.bActivateSource;
  configuration.wakeDevices               = m_configuration.wakeDevices;
  configuration.powerOffDevices           = m_configuration.powerOffDevices;
  configuration.bPowerOffScreensaver      = m_configuration.bPowerOffScreensaver;
  configuration.bPowerOnScreensaver       = m_configuration.bPowerOnScreensaver;
  configuration.bPowerOffOnStandby        = m_configuration.bPowerOffOnStandby;
  configuration.bSendInactiveSource       = m_configuration.bSendInactiveSource;
  configuration.logicalAddresses          = m_configuration.logicalAddresses;
  configuration.iFirmwareVersion          = m_configuration.iFirmwareVersion;
  configuration.bPowerOffDevicesOnStandby = m_configuration.bPowerOffDevicesOnStandby;
  configuration.bShutdownOnStandby        = m_configuration.bShutdownOnStandby;
  memcpy(configuration.strDeviceLanguage,  m_configuration.strDeviceLanguage, 3);
  configuration.iFirmwareBuildDate        = m_configuration.iFirmwareBuildDate;
  configuration.bMonitorOnly              = m_configuration.bMonitorOnly;
  configuration.cecVersion                = m_configuration.cecVersion;
  configuration.adapterType               = m_configuration.adapterType;

  return true;
}

bool CCECClient::SetConfiguration(const libcec_configuration &configuration)
{
  libcec_configuration defaultSettings;
  bool bIsRunning(m_processor && m_processor->CECInitialised());
  CCECBusDevice *primary = bIsRunning ? GetPrimaryDevice() : NULL;
  uint16_t iPA = primary ? primary->GetCurrentPhysicalAddress() : CEC_INVALID_PHYSICAL_ADDRESS;

  // update the callbacks
  if (configuration.callbacks)
    EnableCallbacks(configuration.callbackParam, configuration.callbacks);

  // update the client version
  SetClientVersion((cec_client_version)configuration.clientVersion);

  // update the OSD name
  CStdString strOSDName(configuration.strDeviceName);
  SetOSDName(strOSDName);

  // update the TV vendor override
  SetTVVendorOverride((cec_vendor_id)configuration.tvVendor);

  // just copy these
  {
    CLockObject lock(m_mutex);
    m_configuration.bUseTVMenuLanguage         = configuration.bUseTVMenuLanguage;
    m_configuration.bActivateSource            = configuration.bActivateSource;
    m_configuration.bGetSettingsFromROM        = configuration.bGetSettingsFromROM;
    m_configuration.wakeDevices                = configuration.wakeDevices;
    m_configuration.powerOffDevices            = configuration.powerOffDevices;
    m_configuration.bPowerOffScreensaver       = configuration.bPowerOffScreensaver;
    m_configuration.bPowerOffOnStandby         = configuration.bPowerOffOnStandby;
    m_configuration.bSendInactiveSource        = configuration.bSendInactiveSource;
    m_configuration.bPowerOffDevicesOnStandby  = configuration.bPowerOffDevicesOnStandby;
    m_configuration.bShutdownOnStandby         = configuration.bShutdownOnStandby;
    memcpy(m_configuration.strDeviceLanguage,   configuration.strDeviceLanguage, 3);
    m_configuration.bMonitorOnly               = configuration.bMonitorOnly;
    m_configuration.cecVersion                 = configuration.cecVersion;
    m_configuration.adapterType                = configuration.adapterType;
    m_configuration.iDoubleTapTimeoutMs        = configuration.iDoubleTapTimeoutMs;
    m_configuration.deviceTypes.Add(configuration.deviceTypes[0]);

    if (m_configuration.clientVersion >= CEC_CLIENT_VERSION_2_0_5)
    {
      m_configuration.comboKey           = configuration.comboKey;
      m_configuration.iComboKeyTimeoutMs = configuration.iComboKeyTimeoutMs;
    }
    else
    {
      m_configuration.comboKey           = defaultSettings.comboKey;
      m_configuration.iComboKeyTimeoutMs = defaultSettings.iComboKeyTimeoutMs;
    }

    if (m_configuration.clientVersion >= CEC_CLIENT_VERSION_2_1_0)
      m_configuration.bPowerOnScreensaver = configuration.bPowerOnScreensaver;
    else
      m_configuration.bPowerOnScreensaver = defaultSettings.bPowerOnScreensaver;
  }

  bool bNeedReinit(false);

  // device types
  if (SetDeviceTypes(configuration.deviceTypes))
  {
    // the device type changed. just copy the rest, and re-register
    {
      CLockObject lock(m_mutex);
      m_configuration.iPhysicalAddress = configuration.iPhysicalAddress;
      m_configuration.baseDevice       = configuration.baseDevice;
      m_configuration.iHDMIPort        = configuration.iHDMIPort;
      bNeedReinit = true;
    }
  }
  else
  {
    // set the physical address
    SetPhysicalAddress(configuration);
  }

  // persist the new configuration
  PersistConfiguration(m_configuration);

  if (!primary)
    primary = GetPrimaryDevice();

  if (bNeedReinit || !primary || primary->GetCurrentPhysicalAddress() != iPA)
  {
    // PA or device type changed
    m_processor->RegisterClient(this);
  }
  else if (primary && configuration.bActivateSource == 1 && bIsRunning && !primary->IsActiveSource())
  {
    // activate the source if we're not already the active source
    primary->ActivateSource();
  }

  return true;
}

void CCECClient::AddCommand(const cec_command &command)
{
  // don't forward the standby opcode more than once every 10 seconds
  if (command.opcode == CEC_OPCODE_STANDBY)
  {
    CLockObject lock(m_mutex);
    if (m_iPreventForwardingPowerOffCommand != 0 &&
        m_iPreventForwardingPowerOffCommand > GetTimeMs())
      return;
    else
      m_iPreventForwardingPowerOffCommand = GetTimeMs() + CEC_FORWARD_STANDBY_MIN_INTERVAL;
  }

  if (command.destination == CECDEVICE_BROADCAST || GetLogicalAddresses().IsSet(command.destination))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, ">> %s (%X) -> %s (%X): %s (%2X)", ToString(command.initiator), command.initiator, ToString(command.destination), command.destination, ToString(command.opcode), command.opcode);
    CallbackAddCommand(command);
  }
}

int CCECClient::MenuStateChanged(const cec_menu_state newState)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, ">> %s: %s", ToString(CEC_OPCODE_MENU_REQUEST), ToString(newState));
  return CallbackMenuStateChanged(newState);
}

void CCECClient::AddKey(bool bSendComboKey /* = false */)
{
  cec_keypress key;
  key.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;

  {
    CLockObject lock(m_mutex);
    if (m_iCurrentButton != CEC_USER_CONTROL_CODE_UNKNOWN)
    {
      key.duration = (unsigned int) (GetTimeMs() - m_buttontime);

      if (key.duration > m_configuration.iComboKeyTimeoutMs ||
          m_configuration.iComboKeyTimeoutMs == 0 ||
          m_iCurrentButton != m_configuration.comboKey ||
          bSendComboKey)
      {
        key.keycode = m_iCurrentButton;

        m_iCurrentButton = CEC_USER_CONTROL_CODE_UNKNOWN;
        m_buttontime = 0;
      }
    }
  }

  if (key.keycode != CEC_USER_CONTROL_CODE_UNKNOWN)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "key released: %s (%1x)", ToString(key.keycode), key.keycode);
    CallbackAddKey(key);
  }
}

void CCECClient::AddKey(const cec_keypress &key)
{
  if (key.keycode > CEC_USER_CONTROL_CODE_MAX &&
      key.keycode < CEC_USER_CONTROL_CODE_SELECT)
  {
    // send back the previous key if there is one
    AddKey();
    return;
  }

  cec_keypress transmitKey(key);
  cec_user_control_code comboKey(m_configuration.clientVersion >= CEC_CLIENT_VERSION_2_0_5 ?
      m_configuration.comboKey : CEC_USER_CONTROL_CODE_STOP);

  {
    CLockObject lock(m_mutex);
    if (m_configuration.iComboKeyTimeoutMs > 0 && m_iCurrentButton == comboKey && key.duration == 0)
    {
      // stop + ok -> exit
      if (key.keycode == CEC_USER_CONTROL_CODE_SELECT)
        transmitKey.keycode = CEC_USER_CONTROL_CODE_EXIT;
      // stop + pause -> root menu
      else if (key.keycode == CEC_USER_CONTROL_CODE_PAUSE)
        transmitKey.keycode = CEC_USER_CONTROL_CODE_ROOT_MENU;
      // stop + play -> dot (which is handled as context menu in xbmc)
      else if (key.keycode == CEC_USER_CONTROL_CODE_PLAY)
        transmitKey.keycode = CEC_USER_CONTROL_CODE_DOT;
      // default, send back the previous key
      else
        AddKey(true);
    }

    if (m_iCurrentButton == key.keycode)
    {
      m_buttontime = GetTimeMs();
    }
    else
    {
      AddKey();
      if (key.duration == 0)
      {
        m_iCurrentButton = transmitKey.keycode;
        m_buttontime = m_iCurrentButton == CEC_USER_CONTROL_CODE_UNKNOWN || key.duration > 0 ? 0 : GetTimeMs();
      }
    }
  }

  if (key.keycode != comboKey || key.duration > 0)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "key pressed: %s (%1x)", ToString(transmitKey.keycode), transmitKey.keycode);
    CallbackAddKey(transmitKey);
  }
}

void CCECClient::SetCurrentButton(const cec_user_control_code iButtonCode)
{
  // push a keypress to the buffer with 0 duration and another with the duration set when released
  cec_keypress key;
  key.duration = 0;
  key.keycode = iButtonCode;

  AddKey(key);
}

void CCECClient::CheckKeypressTimeout(void)
{
  cec_keypress key;

  {
    CLockObject lock(m_mutex);
    uint64_t iNow = GetTimeMs();
    cec_user_control_code comboKey(m_configuration.clientVersion >= CEC_CLIENT_VERSION_2_0_5 ?
        m_configuration.comboKey : CEC_USER_CONTROL_CODE_STOP);
    uint32_t iTimeoutMs(m_configuration.clientVersion >= CEC_CLIENT_VERSION_2_0_5 ?
        m_configuration.iComboKeyTimeoutMs : CEC_DEFAULT_COMBO_TIMEOUT_MS);

    if (m_iCurrentButton != CEC_USER_CONTROL_CODE_UNKNOWN &&
          ((m_iCurrentButton == comboKey && iTimeoutMs > 0 && iNow - m_buttontime > iTimeoutMs) ||
          (m_iCurrentButton != comboKey && iNow - m_buttontime > CEC_BUTTON_TIMEOUT)))
    {
      key.duration = (unsigned int) (iNow - m_buttontime);
      key.keycode = m_iCurrentButton;

      m_iCurrentButton = CEC_USER_CONTROL_CODE_UNKNOWN;
      m_buttontime = 0;
    }
    else
    {
      return;
    }
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "key auto-released: %s (%1x)", ToString(key.keycode), key.keycode);
  CallbackAddKey(key);
}

bool CCECClient::EnableCallbacks(void *cbParam, ICECCallbacks *callbacks)
{
  CLockObject lock(m_cbMutex);
  m_configuration.callbackParam = cbParam;
  m_configuration.callbacks     = callbacks;
  return true;
}

bool CCECClient::PingAdapter(void)
{
  return m_processor ? m_processor->PingAdapter() : false;
}

std::string CCECClient::GetConnectionInfo(void)
{
  CStdString strLog;
  strLog.Format("libCEC version = %s, client version = %s, firmware version = %d", ToString((cec_server_version)m_configuration.serverVersion), ToString((cec_client_version)m_configuration.clientVersion), m_configuration.iFirmwareVersion);
  if (m_configuration.iFirmwareBuildDate != CEC_FW_BUILD_UNKNOWN)
  {
    time_t buildTime = (time_t)m_configuration.iFirmwareBuildDate;
    strLog.AppendFormat(", firmware build date: %s", asctime(gmtime(&buildTime)));
    strLog = strLog.substr(0, strLog.length() > 0 ? (size_t)(strLog.length() - 1) : 0); // strip \n added by asctime
    strLog.append(" +0000");
  }

  // log the addresses that are being used
  if (!m_configuration.logicalAddresses.IsEmpty())
  {
    strLog.append(", logical address(es) = ");
    CECDEVICEVEC devices;
    m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);
    for (CECDEVICEVEC::iterator it = devices.begin(); it != devices.end(); it++)
      strLog.AppendFormat("%s (%X) ", (*it)->GetLogicalAddressName(), (*it)->GetLogicalAddress());
  }

  if (!CLibCEC::IsValidPhysicalAddress(m_configuration.iPhysicalAddress))
    strLog.AppendFormat(", base device: %s (%X), HDMI port number: %d", ToString(m_configuration.baseDevice), m_configuration.baseDevice, m_configuration.iHDMIPort);
  uint16_t iPhysicalAddress = GetPrimaryDevice()->GetPhysicalAddress(GetLogicalAddresses().primary, false);
  strLog.AppendFormat(", physical address: %x.%x.%x.%x", (iPhysicalAddress >> 12) & 0xF, (iPhysicalAddress >> 8) & 0xF, (iPhysicalAddress >> 4) & 0xF, iPhysicalAddress & 0xF);

  strLog.AppendFormat(", %s", LIB_CEC->GetLibInfo());

  std::string strReturn(strLog.c_str());
  return strReturn;
}

void CCECClient::SetTVVendorOverride(const cec_vendor_id id)
{
  {
    CLockObject lock(m_mutex);
    m_configuration.tvVendor = id;
  }

  if (id != CEC_VENDOR_UNKNOWN)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - vendor id '%s'", __FUNCTION__, ToString(id));

    CCECBusDevice *tv = m_processor ? m_processor->GetTV() : NULL;
    if (tv)
      tv->SetVendorId((uint64_t)id);
  }

  // persist the new configuration
  PersistConfiguration(m_configuration);
}

cec_vendor_id CCECClient::GetTVVendorOverride(void)
{
  CLockObject lock(m_mutex);
  return (cec_vendor_id)m_configuration.tvVendor;
}

void CCECClient::SetOSDName(const std::string &strDeviceName)
{
  {
    CLockObject lock(m_mutex);
    snprintf(m_configuration.strDeviceName, 13, "%s", strDeviceName.c_str());
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - using OSD name '%s'", __FUNCTION__, strDeviceName.c_str());

  CCECBusDevice *primary = GetPrimaryDevice();
  if (primary && !primary->GetCurrentOSDName().Equals(strDeviceName.c_str()))
  {
    primary->SetOSDName(strDeviceName);
    if (m_processor && m_processor->CECInitialised())
      primary->TransmitOSDName(CECDEVICE_TV, false);
  }

  // persist the new configuration
  PersistConfiguration(m_configuration);
}

std::string CCECClient::GetOSDName(void)
{
  CLockObject lock(m_mutex);
  std::string strOSDName(m_configuration.strDeviceName);
  return strOSDName;
}

void CCECClient::SetWakeDevices(const cec_logical_addresses &addresses)
{
  {
    CLockObject lock(m_mutex);
    m_configuration.wakeDevices = addresses;
  }
  // persist the new configuration
  PersistConfiguration(m_configuration);
}

cec_logical_addresses CCECClient::GetWakeDevices(void)
{
  CLockObject lock(m_mutex);
  return m_configuration.wakeDevices;
}

bool CCECClient::AutodetectPhysicalAddress(void)
{
  bool bPhysicalAutodetected(false);
  uint16_t iPhysicalAddress = m_processor ? m_processor->GetDetectedPhysicalAddress() : CEC_INVALID_PHYSICAL_ADDRESS;

  if (CLibCEC::IsValidPhysicalAddress(iPhysicalAddress))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - autodetected physical address '%04X'", __FUNCTION__, iPhysicalAddress);

    CLockObject lock(m_mutex);
    m_configuration.iPhysicalAddress = iPhysicalAddress;
    m_configuration.iHDMIPort        = CEC_HDMI_PORTNUMBER_NONE;
    m_configuration.baseDevice       = CECDEVICE_UNKNOWN;
    bPhysicalAutodetected            = true;
  }

  SetDevicePhysicalAddress(iPhysicalAddress);

  return bPhysicalAutodetected;
}

void CCECClient::SetClientVersion(const cec_client_version version)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - using client version '%s'", __FUNCTION__, ToString(version));

  CLockObject lock(m_mutex);
  m_configuration.clientVersion = (uint32_t)version;
}

cec_client_version CCECClient::GetClientVersion(void)
{
  CLockObject lock(m_mutex);
  return (cec_client_version)m_configuration.clientVersion;
}

bool CCECClient::SetDeviceTypes(const cec_device_type_list &deviceTypes)
{
  bool bNeedReinit(false);

  {
    CLockObject lock(m_mutex);
    bNeedReinit = m_processor && m_processor->CECInitialised() &&
        (m_configuration.deviceTypes != deviceTypes);
    m_configuration.deviceTypes = deviceTypes;
  }

  // persist the new configuration
  PersistConfiguration(m_configuration);

  if (bNeedReinit)
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - using primary device type '%s'", __FUNCTION__, ToString(deviceTypes[0]));

  return bNeedReinit;
}

cec_device_type_list CCECClient::GetDeviceTypes(void)
{
  cec_device_type_list retVal;
  CLockObject lock(m_mutex);
  retVal = m_configuration.deviceTypes;
  return retVal;
}

bool CCECClient::SetDevicePhysicalAddress(const uint16_t iPhysicalAddress)
{
  if (!CLibCEC::IsValidPhysicalAddress(iPhysicalAddress))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - not setting invalid physical address %04x", __FUNCTION__, iPhysicalAddress);
    return false;
  }

  // reconfigure all devices
  cec_logical_address reactivateSource(CECDEVICE_UNKNOWN);
  CECDEVICEVEC devices;
  m_processor->GetDevices()->GetByLogicalAddresses(devices, m_configuration.logicalAddresses);
  for (CECDEVICEVEC::iterator it = devices.begin(); it != devices.end(); it++)
  {
    // if this device was the active source, reactivate it afterwards
    if ((*it)->IsActiveSource())
      reactivateSource = (*it)->GetLogicalAddress();

    // mark the device as inactive source
    if (IsInitialised())
      (*it)->MarkAsInactiveSource();

    // set the new physical address
    (*it)->SetPhysicalAddress(iPhysicalAddress);

    // and transmit it
    if (IsInitialised())
      (*it)->TransmitPhysicalAddress(false);
  }

  // reactivate the previous active source
  if (reactivateSource != CECDEVICE_UNKNOWN &&
      m_processor->CECInitialised() &&
      IsInitialised())
  {
    CCECBusDevice *device = m_processor->GetDevice(reactivateSource);
    if (device)
      device->ActivateSource();
  }

  // persist the new configuration
  PersistConfiguration(m_configuration);

  return true;
}

bool CCECClient::SwitchMonitoring(bool bEnable)
{
  LIB_CEC->AddLog(CEC_LOG_NOTICE, "== %s monitoring mode ==", bEnable ? "enabling" : "disabling");

  if (m_processor)
  {
    m_processor->SwitchMonitoring(bEnable);
    m_configuration.bMonitorOnly = bEnable;
    return bEnable ? true: m_processor->RegisterClient(this);
  }

  return false;
}

bool CCECClient::PollDevice(const cec_logical_address iAddress)
{
  // try to find the primary device
  CCECBusDevice *primary = GetPrimaryDevice();
  // poll the destination, with the primary as source
  if (primary)
    return primary->TransmitPoll(iAddress, true);

  return m_processor ? m_processor->PollDevice(iAddress) : false;
}

cec_logical_addresses CCECClient::GetActiveDevices(void)
{
  CECDEVICEVEC activeDevices;
  if (m_processor)
    m_processor->GetDevices()->GetActive(activeDevices);
  return CCECDeviceMap::ToLogicalAddresses(activeDevices);
}

bool CCECClient::IsActiveDevice(const cec_logical_address iAddress)
{
  cec_logical_addresses activeDevices = GetActiveDevices();
  return activeDevices.IsSet(iAddress);
}

bool CCECClient::IsActiveDeviceType(const cec_device_type type)
{
  CECDEVICEVEC activeDevices;
  if (m_processor)
    m_processor->GetDevices()->GetActive(activeDevices);
  CCECDeviceMap::FilterType(type, activeDevices);
  return !activeDevices.empty();
}

cec_logical_address CCECClient::GetActiveSource(void)
{
  return m_processor ? m_processor->GetActiveSource() : CECDEVICE_UNKNOWN;
}

bool CCECClient::IsActiveSource(const cec_logical_address iAddress)
{
  return m_processor ? m_processor->IsActiveSource(iAddress) : false;
}

bool CCECClient::SetStreamPath(const cec_logical_address iAddress)
{
  uint16_t iPhysicalAddress = GetDevicePhysicalAddress(iAddress);
  if (iPhysicalAddress != CEC_INVALID_PHYSICAL_ADDRESS)
    return SetStreamPath(iPhysicalAddress);
  return false;
}

bool CCECClient::SetStreamPath(const uint16_t iPhysicalAddress)
{
  bool bReturn(false);

  CCECBusDevice *device = GetDeviceByType(CEC_DEVICE_TYPE_TV);
  if (device)
  {
    device->SetStreamPath(iPhysicalAddress);
    bReturn = device->GetHandler()->TransmitSetStreamPath(iPhysicalAddress, false);
    device->MarkHandlerReady();
  }
  else
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "only the TV is allowed to send CEC_OPCODE_SET_STREAM_PATH");
  }

  return bReturn;
}

cec_logical_addresses CCECClient::GetLogicalAddresses(void)
{
  cec_logical_addresses addresses;
  CLockObject lock(m_mutex);
  addresses = m_configuration.logicalAddresses;
  return addresses;
}

bool CCECClient::CanPersistConfiguration(void)
{
  return m_processor ? m_processor->CanPersistConfiguration() : false;
}

bool CCECClient::PersistConfiguration(const libcec_configuration &configuration)
{
  return m_processor && IsRegistered() ?
      m_processor->PersistConfiguration(configuration) :
      false;
}

void CCECClient::RescanActiveDevices(void)
{
  if (m_processor)
    m_processor->RescanActiveDevices();
}

bool CCECClient::IsLibCECActiveSource(void)
{
  bool bReturn(false);
  if (m_processor)
  {
    cec_logical_address activeSource = m_processor->GetActiveSource();
    CCECBusDevice *device = m_processor->GetDevice(activeSource);
    if (device)
      bReturn = device->IsHandledByLibCEC() && !device->GetHandler()->ActiveSourcePending();
  }
  return bReturn;
}

void CCECClient::SourceActivated(const cec_logical_address logicalAddress)
{
  LIB_CEC->AddLog(CEC_LOG_NOTICE, ">> source activated: %s (%x)", ToString(logicalAddress), logicalAddress);
  CallbackSourceActivated(true, logicalAddress);
}

void CCECClient::SourceDeactivated(const cec_logical_address logicalAddress)
{
  LIB_CEC->AddLog(CEC_LOG_NOTICE, ">> source deactivated: %s (%x)", ToString(logicalAddress), logicalAddress);
  CallbackSourceActivated(false, logicalAddress);
}

void CCECClient::CallbackAddCommand(const cec_command &command)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks && m_configuration.callbacks->CBCecCommand)
    m_configuration.callbacks->CBCecCommand(m_configuration.callbackParam, command);
}

void CCECClient::CallbackAddKey(const cec_keypress &key)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks && m_configuration.callbacks->CBCecKeyPress)
  {
    // prevent double taps
    int64_t now = GetTimeMs();
    if (m_lastKeypress.keycode != key.keycode ||
        key.duration > 0 ||
        now - m_iLastKeypressTime >= m_configuration.iDoubleTapTimeoutMs)
    {
      // no double tap
      if (key.duration == 0)
        m_iLastKeypressTime = now;
      m_lastKeypress = key;
      m_configuration.callbacks->CBCecKeyPress(m_configuration.callbackParam, key);
    }
  }
}

void CCECClient::CallbackAddLog(const cec_log_message &message)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks && m_configuration.callbacks->CBCecLogMessage)
    m_configuration.callbacks->CBCecLogMessage(m_configuration.callbackParam, message);
}

void CCECClient::CallbackConfigurationChanged(const libcec_configuration &config)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks &&
      m_configuration.callbacks->CBCecConfigurationChanged &&
      m_processor->CECInitialised())
    m_configuration.callbacks->CBCecConfigurationChanged(m_configuration.callbackParam, config);
}

void CCECClient::CallbackSourceActivated(bool bActivated, const cec_logical_address logicalAddress)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks &&
      m_configuration.callbacks->CBCecSourceActivated)
    m_configuration.callbacks->CBCecSourceActivated(m_configuration.callbackParam, logicalAddress, bActivated ? 1 : 0);
}

void CCECClient::CallbackAlert(const libcec_alert type, const libcec_parameter &param)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks &&
      m_configuration.callbacks->CBCecAlert)
    m_configuration.callbacks->CBCecAlert(m_configuration.callbackParam, type, param);
}

int CCECClient::CallbackMenuStateChanged(const cec_menu_state newState)
{
  CLockObject lock(m_cbMutex);
  if (m_configuration.callbacks &&
      m_configuration.callbacks->CBCecMenuStateChanged)
    return m_configuration.callbacks->CBCecMenuStateChanged(m_configuration.callbackParam, newState);
  return 0;
}

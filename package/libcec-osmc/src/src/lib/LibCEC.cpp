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
#include "LibCEC.h"

#include "adapter/AdapterFactory.h"
#include "adapter/AdapterCommunication.h"
#include "CECProcessor.h"
#include "devices/CECAudioSystem.h"
#include "devices/CECBusDevice.h"
#include "devices/CECPlaybackDevice.h"
#include "devices/CECTV.h"
#include "platform/util/timeutils.h"
#include "platform/util/StdString.h"
#include "platform/util/util.h"

#include "CECClient.h"

using namespace std;
using namespace CEC;
using namespace PLATFORM;

CLibCEC::CLibCEC(void) :
    m_iStartTime(GetTimeMs()),
    m_client(NULL)
{
  m_cec = new CCECProcessor(this);
}

CLibCEC::~CLibCEC(void)
{
  // unregister all clients client
  UnregisterClients();

  // delete the adapter connection
  DELETE_AND_NULL(m_cec);
}

bool CLibCEC::Open(const char *strPort, uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */)
{
  if (!m_cec || !strPort)
    return false;

  // open a new connection
  if (!m_cec->Start(strPort, CEC_SERIAL_DEFAULT_BAUDRATE, iTimeoutMs))
  {
    AddLog(CEC_LOG_ERROR, "could not start CEC communications");
    return false;
  }

  // register all clients
  for (vector<CCECClient *>::iterator it = m_clients.begin(); it != m_clients.end(); it++)
  {
    if (!m_cec->RegisterClient(*it))
    {
      AddLog(CEC_LOG_ERROR, "failed to register a CEC client");
      return false;
    }
  }

  return true;
}

void CLibCEC::Close(void)
{
  if (!m_cec)
    return;

  // unregister all clients
  m_cec->UnregisterClients();

  // close the connection
  m_cec->Close();
}

int8_t CLibCEC::FindAdapters(cec_adapter *deviceList, uint8_t iBufSize, const char *strDevicePath /* = NULL */)
{
  return CAdapterFactory(this).FindAdapters(deviceList, iBufSize, strDevicePath);
}

bool CLibCEC::StartBootloader(void)
{
  return m_cec ? m_cec->StartBootloader() : false;
}

bool CLibCEC::PingAdapter(void)
{
  return m_client ? m_client->PingAdapter() : false;
}

bool CLibCEC::EnableCallbacks(void *cbParam, ICECCallbacks *callbacks)
{
  return m_client ? m_client->EnableCallbacks(cbParam, callbacks) : false;
}

bool CLibCEC::GetCurrentConfiguration(libcec_configuration *configuration)
{
  return m_client ? m_client->GetCurrentConfiguration(*configuration) : false;
}

bool CLibCEC::SetConfiguration(const libcec_configuration *configuration)
{
  return m_client ? m_client->SetConfiguration(*configuration) : false;
}

bool CLibCEC::CanPersistConfiguration(void)
{
  return m_client ? m_client->CanPersistConfiguration() : false;
}

bool CLibCEC::PersistConfiguration(libcec_configuration *configuration)
{
  return m_client ? m_client->PersistConfiguration(*configuration) : false;
}

void CLibCEC::RescanActiveDevices(void)
{
  if (m_client)
    m_client->RescanActiveDevices();
}

bool CLibCEC::IsLibCECActiveSource(void)
{
  return m_client ? m_client->IsLibCECActiveSource() : false;
}

bool CLibCEC::Transmit(const cec_command &data)
{
  return m_client ? m_client->Transmit(data, false) : false;
}

bool CLibCEC::SetLogicalAddress(cec_logical_address iLogicalAddress)
{
  return m_client ? m_client->SetLogicalAddress(iLogicalAddress) : false;
}

bool CLibCEC::SetPhysicalAddress(uint16_t iPhysicalAddress /* = CEC_DEFAULT_PHYSICAL_ADDRESS */)
{
  return m_client ? m_client->SetPhysicalAddress(iPhysicalAddress) : false;
}

bool CLibCEC::SetHDMIPort(cec_logical_address iBaseDevice, uint8_t iPort /* = CEC_DEFAULT_HDMI_PORT */)
{
  return m_client ? m_client->SetHDMIPort(iBaseDevice, iPort) : false;
}

bool CLibCEC::PowerOnDevices(cec_logical_address address /* = CECDEVICE_TV */)
{
  return m_client ? m_client->SendPowerOnDevices(address) : false;
}

bool CLibCEC::StandbyDevices(cec_logical_address address /* = CECDEVICE_BROADCAST */)
{
  return m_client ? m_client->SendStandbyDevices(address) : false;
}

bool CLibCEC::SetActiveSource(cec_device_type type /* = CEC_DEVICE_TYPE_RESERVED */)
{
  return m_client ? m_client->SendSetActiveSource(type) : false;
}

bool CLibCEC::SetDeckControlMode(cec_deck_control_mode mode, bool bSendUpdate /* = true */)
{
  return m_client ? m_client->SendSetDeckControlMode(mode, bSendUpdate) : false;
}

bool CLibCEC::SetDeckInfo(cec_deck_info info, bool bSendUpdate /* = true */)
{
  return m_client ? m_client->SendSetDeckInfo(info, bSendUpdate) : false;
}

bool CLibCEC::SetInactiveView(void)
{
  return m_client ? m_client->SendSetInactiveView() : false;
}

bool CLibCEC::SetMenuState(cec_menu_state state, bool bSendUpdate /* = true */)
{
  return m_client ? m_client->SendSetMenuState(state, bSendUpdate) : false;
}

bool CLibCEC::SetOSDString(cec_logical_address iLogicalAddress, cec_display_control duration, const char *strMessage)
{
  return m_client ? m_client->SendSetOSDString(iLogicalAddress, duration, strMessage) : false;
}

bool CLibCEC::SwitchMonitoring(bool bEnable)
{
  return m_client ? m_client->SwitchMonitoring(bEnable) : false;
}

cec_version CLibCEC::GetDeviceCecVersion(cec_logical_address iAddress)
{
  return m_client ? m_client->GetDeviceCecVersion(iAddress) : CEC_VERSION_UNKNOWN;
}

bool CLibCEC::GetDeviceMenuLanguage(cec_logical_address iAddress, cec_menu_language *language)
{
  return m_client ? m_client->GetDeviceMenuLanguage(iAddress, *language) : false;
}

uint64_t CLibCEC::GetDeviceVendorId(cec_logical_address iAddress)
{
  return m_client ? m_client->GetDeviceVendorId(iAddress) : (uint64_t)CEC_VENDOR_UNKNOWN;
}

uint16_t CLibCEC::GetDevicePhysicalAddress(cec_logical_address iAddress)
{
  return m_client ? m_client->GetDevicePhysicalAddress(iAddress) : CEC_INVALID_PHYSICAL_ADDRESS;
}

cec_power_status CLibCEC::GetDevicePowerStatus(cec_logical_address iAddress)
{
  return m_client ? m_client->GetDevicePowerStatus(iAddress) : CEC_POWER_STATUS_UNKNOWN;
}

bool CLibCEC::PollDevice(cec_logical_address iAddress)
{
  return m_client ? m_client->PollDevice(iAddress) : false;
}

cec_logical_addresses CLibCEC::GetActiveDevices(void)
{
  cec_logical_addresses addresses;
  addresses.Clear();
  if (m_client)
    addresses = m_client->GetActiveDevices();
  return addresses;
}

bool CLibCEC::IsActiveDevice(cec_logical_address iAddress)
{
  return m_client ? m_client->IsActiveDevice(iAddress) : false;
}

bool CLibCEC::IsActiveDeviceType(cec_device_type type)
{
  return m_client ? m_client->IsActiveDeviceType(type) : false;
}

uint8_t CLibCEC::VolumeUp(bool bSendRelease /* = true */)
{
  return m_client ? m_client->SendVolumeUp(bSendRelease) : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CLibCEC::VolumeDown(bool bSendRelease /* = true */)
{
  return m_client ? m_client->SendVolumeDown(bSendRelease) : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CLibCEC::MuteAudio(bool UNUSED(bSendRelease) /* = true */)
{
  AddLog(CEC_LOG_WARNING, "deprecated function called: %s", __FUNCTION__);
  return m_client ? m_client->SendMuteAudio() : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

bool CLibCEC::SendKeypress(cec_logical_address iDestination, cec_user_control_code key, bool bWait /* = true */)
{
  return m_client ? m_client->SendKeypress(iDestination, key, bWait) : false;
}

bool CLibCEC::SendKeyRelease(cec_logical_address iDestination, bool bWait /* = true */)
{
  return m_client ? m_client->SendKeyRelease(iDestination, bWait) : false;
}

cec_osd_name CLibCEC::GetDeviceOSDName(cec_logical_address iAddress)
{
  cec_osd_name retVal;
  retVal.device = CECDEVICE_UNKNOWN;
  memset(retVal.name, 0, 14);

  if (m_client)
    retVal = m_client->GetDeviceOSDName(iAddress);
  return retVal;
}

cec_logical_address CLibCEC::GetActiveSource(void)
{
  return m_client ? m_client->GetActiveSource() : CECDEVICE_UNKNOWN;
}

bool CLibCEC::IsActiveSource(cec_logical_address iAddress)
{
  return m_client ? m_client->IsActiveSource(iAddress) : false;
}
bool CLibCEC::SetStreamPath(cec_logical_address iAddress)
{
  return m_client ? m_client->SetStreamPath(iAddress) : false;
}

bool CLibCEC::SetStreamPath(uint16_t iPhysicalAddress)
{
  return m_client ? m_client->SetStreamPath(iPhysicalAddress) : false;
}

cec_logical_addresses CLibCEC::GetLogicalAddresses(void)
{
  cec_logical_addresses addresses;
  addresses.Clear();
  if (m_client)
    addresses = m_client->GetLogicalAddresses();
  return addresses;
}

cec_device_type CLibCEC::GetType(cec_logical_address address)
{
  return CCECTypeUtils::GetType(address);
}

uint16_t CLibCEC::GetMaskForType(cec_logical_address address)
{
  return CCECTypeUtils::GetMaskForType(address);
}

uint16_t CLibCEC::GetMaskForType(cec_device_type type)
{
  return CCECTypeUtils::GetMaskForType(type);
}

bool CLibCEC::IsValidPhysicalAddress(uint16_t iPhysicalAddress)
{
  return iPhysicalAddress >= CEC_MIN_PHYSICAL_ADDRESS &&
         iPhysicalAddress <= CEC_MAX_PHYSICAL_ADDRESS;
}

void CLibCEC::CheckKeypressTimeout(void)
{
  // check all clients
  for (vector<CCECClient *>::iterator it = m_clients.begin(); it != m_clients.end(); it++)
    (*it)->CheckKeypressTimeout();
}

void CLibCEC::AddLog(const cec_log_level level, const char *strFormat, ...)
{
  CStdString strLog;

  // format the message
  va_list argList;
  va_start(argList, strFormat);
  strLog.FormatV(strFormat, argList);
  va_end(argList);

  cec_log_message message;
  message.level = level;
  message.time = GetTimeMs() - m_iStartTime;
  snprintf(message.message, sizeof(message.message), "%s", strLog.c_str());

  // send the message to all clients
  for (vector<CCECClient *>::iterator it = m_clients.begin(); it != m_clients.end(); it++)
    (*it)->AddLog(message);
}

void CLibCEC::AddCommand(const cec_command &command)
{
  // send the command to all clients
  for (vector<CCECClient *>::iterator it = m_clients.begin(); it != m_clients.end(); it++)
    (*it)->AddCommand(command);
}

void CLibCEC::Alert(const libcec_alert type, const libcec_parameter &param)
{
  // send the alert to all clients
  for (vector<CCECClient *>::iterator it = m_clients.begin(); it != m_clients.end(); it++)
    (*it)->Alert(type, param);
}

CCECClient *CLibCEC::RegisterClient(libcec_configuration &configuration)
{
  if (!m_cec)
    return NULL;

  // create a new client instance
  CCECClient *newClient = new CCECClient(m_cec, configuration);
  if (!newClient)
    return NULL;
  m_clients.push_back(newClient);

  // if the default client isn't set, set it
  if (!m_client)
    m_client = newClient;

  // register the new client
  if (m_cec->CECInitialised())
    m_cec->RegisterClient(newClient);

  return newClient;
}

void CLibCEC::UnregisterClients(void)
{
  if (m_cec && m_cec->IsRunning())
    m_cec->UnregisterClients();

  m_clients.clear();

  DELETE_AND_NULL(m_client);
}

void * CECInitialise(libcec_configuration *configuration)
{
  if (!configuration)
    return NULL;

  // create a new libCEC instance
  CLibCEC *lib = new CLibCEC;

  // register a new client
  CCECClient *client(NULL);
  if (lib && configuration)
    client = lib->RegisterClient(*configuration);

  // update the current configuration
  if (client)
    client->GetCurrentConfiguration(*configuration);

  // ensure that the correct server version is set
  configuration->serverVersion = LIBCEC_VERSION_CURRENT;

  return static_cast< void* > (lib);
}

void * CECInit(const char *strDeviceName, CEC::cec_device_type_list types)
{
  libcec_configuration configuration; configuration.Clear();

  // client version < 1.5.0
  snprintf(configuration.strDeviceName, 13, "%s", strDeviceName);
  configuration.deviceTypes      = types;
  configuration.iPhysicalAddress = CEC_INVALID_PHYSICAL_ADDRESS;

  if (configuration.deviceTypes.IsEmpty())
    configuration.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);

  return CECInitialise(&configuration);
}

bool CECStartBootloader(void)
{
  bool bReturn(false);
  cec_adapter deviceList[1];
  if (CAdapterFactory(NULL).FindAdapters(deviceList, 1, 0) > 0)
  {
    CAdapterFactory factory(NULL);
    IAdapterCommunication *comm = factory.GetInstance(deviceList[0].comm);
    if (comm)
    {
      CTimeout timeout(CEC_DEFAULT_CONNECT_TIMEOUT);
      while (timeout.TimeLeft() > 0 &&
          (bReturn = comm->Open(timeout.TimeLeft() / CEC_CONNECT_TRIES, true)) == false)
      {
        comm->Close();
        CEvent::Sleep(500);
      }
      if (comm->IsOpen())
        bReturn = comm->StartBootloader();

      delete comm;
    }
  }

  return bReturn;
}

void CECDestroy(CEC::ICECAdapter *instance)
{
  DELETE_AND_NULL(instance);
}

bool CLibCEC::GetDeviceInformation(const char *strPort, libcec_configuration *config, uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */)
{
  if (m_cec->IsRunning())
    return false;

  return m_cec->GetDeviceInformation(strPort, config, iTimeoutMs);
}

const char *CLibCEC::GetLibInfo(void)
{
#ifndef LIB_INFO
#ifdef _WIN32
#define FEATURES "'P8 USB' 'P8 USB detect'"
#ifdef _WIN64
#define HOST_TYPE "Windows (x64)"
#else
#define HOST_TYPE "Windows (x86)"
#endif
#else
#define HOST_TYPE "unknown"
#define FEATURES "unknown"
#endif

  return "host: " HOST_TYPE ", features: " FEATURES ", compiled: " __DATE__;
#else
  return LIB_INFO;
#endif
}

void CLibCEC::InitVideoStandalone(void)
{
  CAdapterFactory::InitVideoStandalone();
}
uint16_t CLibCEC::GetAdapterVendorId(void) const
{
  return m_cec && m_cec->IsRunning() ? m_cec->GetAdapterVendorId() : 0;
}

uint16_t CLibCEC::GetAdapterProductId(void) const
{
  return m_cec && m_cec->IsRunning() ? m_cec->GetAdapterProductId() : 0;
}

uint8_t CLibCEC::AudioToggleMute(void)
{
  return m_client ? m_client->AudioToggleMute() : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CLibCEC::AudioMute(void)
{
  return m_client ? m_client->AudioMute() : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CLibCEC::AudioUnmute(void)
{
  return m_client ? m_client->AudioUnmute() : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

uint8_t CLibCEC::AudioStatus(void)
{
  return m_client ? m_client->AudioStatus() : (uint8_t)CEC_AUDIO_VOLUME_STATUS_UNKNOWN;
}

int8_t CLibCEC::DetectAdapters(cec_adapter_descriptor *deviceList, uint8_t iBufSize, const char *strDevicePath /* = NULL */, bool bQuickScan /* = false */)
{
  int8_t iAdaptersFound = CAdapterFactory(this).DetectAdapters(deviceList, iBufSize, strDevicePath);
  if (!bQuickScan)
  {
    for (int8_t iPtr = 0; iPtr < iAdaptersFound; iPtr++)
    {
      libcec_configuration config;
      GetDeviceInformation(deviceList[iPtr].strComName, &config);
      deviceList[iPtr].iFirmwareVersion   = config.iFirmwareVersion;
      deviceList[iPtr].iPhysicalAddress   = config.iPhysicalAddress;
      deviceList[iPtr].iFirmwareBuildDate = config.iFirmwareBuildDate;
      deviceList[iPtr].adapterType        = config.adapterType;
    }
  }
  return iAdaptersFound;
}

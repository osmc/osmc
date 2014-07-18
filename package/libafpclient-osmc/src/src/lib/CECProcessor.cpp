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
#include "CECProcessor.h"

#include "adapter/AdapterFactory.h"
#include "devices/CECBusDevice.h"
#include "devices/CECAudioSystem.h"
#include "devices/CECPlaybackDevice.h"
#include "devices/CECRecordingDevice.h"
#include "devices/CECTuner.h"
#include "devices/CECTV.h"
#include "implementations/CECCommandHandler.h"
#include "LibCEC.h"
#include "CECClient.h"
#include "CECTypeUtils.h"
#include "platform/util/timeutils.h"
#include "platform/util/util.h"

using namespace CEC;
using namespace std;
using namespace PLATFORM;

#define CEC_PROCESSOR_SIGNAL_WAIT_TIME 1000
#define ACTIVE_SOURCE_CHECK_INTERVAL   500
#define TV_PRESENT_CHECK_INTERVAL      30000

#define ToString(x) CCECTypeUtils::ToString(x)

CCECStandbyProtection::CCECStandbyProtection(CCECProcessor* processor) :
    m_processor(processor) {}
CCECStandbyProtection::~CCECStandbyProtection(void) {}

void* CCECStandbyProtection::Process(void)
{
  int64_t last = GetTimeMs();
  int64_t next;
  while (!IsStopped())
  {
    PLATFORM::CEvent::Sleep(1000);

    next = GetTimeMs();

    // reset the connection if the clock changed
    if (next < last || next - last > 10000)
    {
      libcec_parameter param;
      param.paramData = NULL; param.paramType = CEC_PARAMETER_TYPE_UNKOWN;
      m_processor->GetLib()->Alert(CEC_ALERT_CONNECTION_LOST, param);
      break;
    }

    last = next;
  }
  return NULL;
}

CCECProcessor::CCECProcessor(CLibCEC *libcec) :
    m_bInitialised(false),
    m_communication(NULL),
    m_libcec(libcec),
    m_iStandardLineTimeout(3),
    m_iRetryLineTimeout(3),
    m_iLastTransmission(0),
    m_bMonitor(true),
    m_addrAllocator(NULL),
    m_bStallCommunication(false),
    m_connCheck(NULL)
{
  m_busDevices = new CCECDeviceMap(this);
}

CCECProcessor::~CCECProcessor(void)
{
  m_bStallCommunication = false;
  DELETE_AND_NULL(m_addrAllocator);
  Close();
  DELETE_AND_NULL(m_busDevices);
}

bool CCECProcessor::Start(const char *strPort, uint16_t iBaudRate /* = CEC_SERIAL_DEFAULT_BAUDRATE */, uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */)
{
  CLockObject lock(m_mutex);
  // open a connection
  if (!OpenConnection(strPort, iBaudRate, iTimeoutMs))
    return false;

  // create the processor thread
  if (!IsRunning())
  {
    if (!CreateThread())
    {
      m_libcec->AddLog(CEC_LOG_ERROR, "could not create a processor thread");
      return false;
    }
  }

  return true;
}

void CCECProcessor::Close(void)
{
  // mark as uninitialised
  SetCECInitialised(false);

  // stop the processor
  DELETE_AND_NULL(m_connCheck);
  StopThread(-1);
  m_inBuffer.Broadcast();
  StopThread();

  // close the connection
  CLockObject lock(m_mutex);
  DELETE_AND_NULL(m_communication);
}

void CCECProcessor::ResetMembers(void)
{
  // close the connection
  DELETE_AND_NULL(m_communication);

  // reset the other members to the initial state
  m_iStandardLineTimeout = 3;
  m_iRetryLineTimeout = 3;
  m_iLastTransmission = 0;
  m_busDevices->ResetDeviceStatus();
}

bool CCECProcessor::OpenConnection(const char *strPort, uint16_t iBaudRate, uint32_t iTimeoutMs, bool bStartListening /* = true */)
{
  bool bReturn(false);
  CTimeout timeout(iTimeoutMs > 0 ? iTimeoutMs : CEC_DEFAULT_TRANSMIT_WAIT);

  // ensure that a previous connection is closed
  Close();

  // reset all member to the initial state
  ResetMembers();

  // check whether the Close() method deleted any previous connection
  if (m_communication)
  {
    m_libcec->AddLog(CEC_LOG_ERROR, "previous connection could not be closed");
    return bReturn;
  }

  // create a new connection
  m_communication = CAdapterFactory(this->m_libcec).GetInstance(strPort, iBaudRate);

  // open a new connection
  unsigned iConnectTry(0);
  while (timeout.TimeLeft() > 0 && (bReturn = m_communication->Open((timeout.TimeLeft() / CEC_CONNECT_TRIES), false, bStartListening)) == false)
  {
    m_libcec->AddLog(CEC_LOG_ERROR, "could not open a connection (try %d)", ++iConnectTry);
    m_communication->Close();
    CEvent::Sleep(CEC_DEFAULT_CONNECT_RETRY_WAIT);
  }

  m_libcec->AddLog(CEC_LOG_NOTICE, "connection opened");

  // mark as initialised
  SetCECInitialised(true);

  return bReturn;
}

bool CCECProcessor::CECInitialised(void)
{
  CLockObject lock(m_threadMutex);
  return m_bInitialised;
}

void CCECProcessor::SetCECInitialised(bool bSetTo /* = true */)
{
  {
    CLockObject lock(m_mutex);
    m_bInitialised = bSetTo;
  }
  if (!bSetTo)
    UnregisterClients();
}

bool CCECProcessor::TryLogicalAddress(cec_logical_address address, cec_version libCECSpecVersion /* = CEC_VERSION_1_4 */)
{
  // find the device
  CCECBusDevice *device = m_busDevices->At(address);
  if (device)
  {
    // check if it's already marked as present or used
    if (device->IsPresent() || device->IsHandledByLibCEC())
      return false;

    // poll the LA if not
    return device->TryLogicalAddress(libCECSpecVersion);
  }

  return false;
}

void CCECProcessor::ReplaceHandlers(void)
{
  if (!CECInitialised())
    return;

  // check each device
  for (CECDEVICEMAP::iterator it = m_busDevices->Begin(); it != m_busDevices->End(); it++)
    it->second->ReplaceHandler(true);
}

bool CCECProcessor::OnCommandReceived(const cec_command &command)
{
  return m_inBuffer.Push(command);
}

void *CCECProcessor::Process(void)
{
  m_libcec->AddLog(CEC_LOG_DEBUG, "processor thread started");

  if (!m_connCheck)
    m_connCheck = new CCECStandbyProtection(this);
  m_connCheck->CreateThread();

  cec_command command; command.Clear();
  CTimeout activeSourceCheck(ACTIVE_SOURCE_CHECK_INTERVAL);
  CTimeout tvPresentCheck(TV_PRESENT_CHECK_INTERVAL);

  // as long as we're not being stopped and the connection is open
  while (!IsStopped() && m_communication->IsOpen())
  {
    // wait for a new incoming command, and process it
    if (m_inBuffer.Pop(command, CEC_PROCESSOR_SIGNAL_WAIT_TIME))
      ProcessCommand(command);

    if (CECInitialised() && !IsStopped())
    {
      // check clients for keypress timeouts
      m_libcec->CheckKeypressTimeout();

      // check if we need to replace handlers
      ReplaceHandlers();

      // check whether we need to activate a source, if it failed before
      if (activeSourceCheck.TimeLeft() == 0)
      {
        if (CECInitialised())
          TransmitPendingActiveSourceCommands();
        activeSourceCheck.Init(ACTIVE_SOURCE_CHECK_INTERVAL);
      }

      // check whether the TV is present and responding
      if (tvPresentCheck.TimeLeft() == 0)
      {
        CCECClient *primary = GetPrimaryClient();
        // only check whether the tv responds to polls when a client is connected and not in monitoring mode
        if (primary && primary->GetConfiguration()->bMonitorOnly != 1)
        {
          if (!m_busDevices->At(CECDEVICE_TV)->IsPresent())
          {
            libcec_parameter param;
            param.paramType = CEC_PARAMETER_TYPE_STRING;
            param.paramData = (void*)"TV does not respond to CEC polls";
            primary->Alert(CEC_ALERT_TV_POLL_FAILED, param);
          }
        }
        tvPresentCheck.Init(TV_PRESENT_CHECK_INTERVAL);
      }
    }
  }

  return NULL;
}

bool CCECProcessor::ActivateSource(uint16_t iStreamPath)
{
  bool bReturn(false);

  // find the device with the given PA
  CCECBusDevice *device = GetDeviceByPhysicalAddress(iStreamPath);
  // and make it the active source when found
  if (device)
    bReturn = device->ActivateSource();
  else
    m_libcec->AddLog(CEC_LOG_DEBUG, "device with PA '%04x' not found", iStreamPath);

  return bReturn;
}

void CCECProcessor::SetActiveSource(bool bSetTo, bool bClientUnregistered)
{
  if (m_communication)
    m_communication->SetActiveSource(bSetTo, bClientUnregistered);
}

void CCECProcessor::SetStandardLineTimeout(uint8_t iTimeout)
{
  CLockObject lock(m_mutex);
  m_iStandardLineTimeout = iTimeout;
}

uint8_t CCECProcessor::GetStandardLineTimeout(void)
{
  CLockObject lock(m_mutex);
  return m_iStandardLineTimeout;
}

void CCECProcessor::SetRetryLineTimeout(uint8_t iTimeout)
{
  CLockObject lock(m_mutex);
  m_iRetryLineTimeout = iTimeout;
}

uint8_t CCECProcessor::GetRetryLineTimeout(void)
{
  CLockObject lock(m_mutex);
  return m_iRetryLineTimeout;
}

bool CCECProcessor::PhysicalAddressInUse(uint16_t iPhysicalAddress)
{
  CCECBusDevice *device = GetDeviceByPhysicalAddress(iPhysicalAddress);
  return device != NULL;
}

void CCECProcessor::LogOutput(const cec_command &data)
{
  CStdString strTx;

  // initiator and destination
  strTx.Format("<< %02x", ((uint8_t)data.initiator << 4) + (uint8_t)data.destination);

  // append the opcode
  if (data.opcode_set)
      strTx.AppendFormat(":%02x", (uint8_t)data.opcode);

  // append the parameters
  for (uint8_t iPtr = 0; iPtr < data.parameters.size; iPtr++)
    strTx.AppendFormat(":%02x", data.parameters[iPtr]);

  // and log it
  m_libcec->AddLog(CEC_LOG_TRAFFIC, strTx.c_str());
}

bool CCECProcessor::PollDevice(cec_logical_address iAddress)
{
  // try to find the primary device
  CCECBusDevice *primary = GetPrimaryDevice();
  // poll the destination, with the primary as source
  if (primary)
    return primary->TransmitPoll(iAddress, true);

  CCECBusDevice *device = m_busDevices->At(CECDEVICE_UNREGISTERED);
  if (device)
    return device->TransmitPoll(iAddress, true);

  return false;
}

CCECBusDevice *CCECProcessor::GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress, bool bSuppressUpdate /* = true */)
{
  return m_busDevices ?
      m_busDevices->GetDeviceByPhysicalAddress(iPhysicalAddress, bSuppressUpdate) :
      NULL;
}

CCECBusDevice *CCECProcessor::GetDevice(cec_logical_address address) const
{
  return m_busDevices ?
      m_busDevices->At(address) :
      NULL;
}

cec_logical_address CCECProcessor::GetActiveSource(bool bRequestActiveSource /* = true */)
{
  // get the device that is marked as active source from the device map
  CCECBusDevice *activeSource = m_busDevices->GetActiveSource();
  if (activeSource)
    return activeSource->GetLogicalAddress();

  if (bRequestActiveSource)
  {
    // request the active source from the bus
    CCECBusDevice *primary = GetPrimaryDevice();
    if (primary)
    {
      primary->RequestActiveSource();
      return GetActiveSource(false);
    }
  }

  // unknown or none
  return CECDEVICE_UNKNOWN;
}

bool CCECProcessor::IsActiveSource(cec_logical_address iAddress)
{
  CCECBusDevice *device = m_busDevices->At(iAddress);
  return device && device->IsActiveSource();
}

bool CCECProcessor::Transmit(const cec_command &data, bool bIsReply)
{
  cec_command transmitData(data);
  uint8_t iMaxTries(0);
  bool bRetry(true);
  uint8_t iTries(0);

  // get the current timeout setting
  uint8_t iLineTimeout(GetStandardLineTimeout());

  // reset the state of this message to 'unknown'
  cec_adapter_message_state adapterState = ADAPTER_MESSAGE_STATE_UNKNOWN;

  CLockObject lock(m_mutex);
  if (!m_communication)
    return false;

  if (!m_communication->SupportsSourceLogicalAddress(transmitData.initiator))
  {
    if (transmitData.initiator == CECDEVICE_UNREGISTERED && m_communication->SupportsSourceLogicalAddress(CECDEVICE_FREEUSE))
    {
      m_libcec->AddLog(CEC_LOG_DEBUG, "initiator '%s' is not supported by the CEC adapter. using '%s' instead", ToString(transmitData.initiator), ToString(CECDEVICE_FREEUSE));
      transmitData.initiator = CECDEVICE_FREEUSE;
    }
    else
    {
      m_libcec->AddLog(CEC_LOG_DEBUG, "initiator '%s' is not supported by the CEC adapter", ToString(transmitData.initiator));
      return false;
    }
  }

  LogOutput(transmitData);

  // find the initiator device
  CCECBusDevice *initiator = m_busDevices->At(transmitData.initiator);
  if (!initiator)
  {
    m_libcec->AddLog(CEC_LOG_WARNING, "invalid initiator");
    return false;
  }

  // find the destination device, if it's not the broadcast address
  if (transmitData.destination != CECDEVICE_BROADCAST)
  {
    // check if the device is marked as handled by libCEC
    CCECBusDevice *destination = m_busDevices->At(transmitData.destination);
    if (destination && destination->IsHandledByLibCEC())
    {
      // and reject the command if it's trying to send data to a device that is handled by libCEC
      m_libcec->AddLog(CEC_LOG_WARNING, "not sending data to myself!");
      return false;
    }
  }

  // wait until we finished allocating a new LA if it got lost
  lock.Unlock();
  while (m_bStallCommunication) Sleep(5);
  lock.Lock();

  m_iLastTransmission = GetTimeMs();
  // set the number of tries
  iMaxTries = initiator->GetHandler()->GetTransmitRetries() + 1;
  initiator->MarkHandlerReady();

  // and try to send the command
  while (bRetry && ++iTries < iMaxTries)
  {
    if (initiator->IsUnsupportedFeature(transmitData.opcode))
      return false;

    adapterState = !IsStopped() && m_communication && m_communication->IsOpen() ?
        m_communication->Write(transmitData, bRetry, iLineTimeout, bIsReply) :
        ADAPTER_MESSAGE_STATE_ERROR;
    iLineTimeout = m_iRetryLineTimeout;
  }

  return bIsReply ?
      adapterState == ADAPTER_MESSAGE_STATE_SENT_ACKED || adapterState == ADAPTER_MESSAGE_STATE_SENT || adapterState == ADAPTER_MESSAGE_STATE_WAITING_TO_BE_SENT :
      adapterState == ADAPTER_MESSAGE_STATE_SENT_ACKED;
}

void CCECProcessor::TransmitAbort(cec_logical_address source, cec_logical_address destination, cec_opcode opcode, cec_abort_reason reason /* = CEC_ABORT_REASON_UNRECOGNIZED_OPCODE */)
{
  m_libcec->AddLog(CEC_LOG_DEBUG, "<< transmitting abort message");

  cec_command command;
  cec_command::Format(command, source, destination, CEC_OPCODE_FEATURE_ABORT);
  command.parameters.PushBack((uint8_t)opcode);
  command.parameters.PushBack((uint8_t)reason);

  Transmit(command, true);
}

void CCECProcessor::ProcessCommand(const cec_command &command)
{
  // log the command
  m_libcec->AddLog(CEC_LOG_TRAFFIC, ToString(command).c_str());

  // find the initiator
  CCECBusDevice *device = m_busDevices->At(command.initiator);

  if (device)
    device->HandleCommand(command);
}

bool CCECProcessor::IsPresentDevice(cec_logical_address address)
{
  CCECBusDevice *device = m_busDevices->At(address);
  return device && device->GetStatus() == CEC_DEVICE_STATUS_PRESENT;
}

bool CCECProcessor::IsPresentDeviceType(cec_device_type type)
{
  CECDEVICEVEC devices;
  m_busDevices->GetByType(type, devices);
  CCECDeviceMap::FilterActive(devices);
  return !devices.empty();
}

uint16_t CCECProcessor::GetDetectedPhysicalAddress(void) const
{
  return m_communication ? m_communication->GetPhysicalAddress() : CEC_INVALID_PHYSICAL_ADDRESS;
}

bool CCECProcessor::ClearLogicalAddresses(void)
{
  cec_logical_addresses addresses; addresses.Clear();
  return SetLogicalAddresses(addresses);
}

bool CCECProcessor::SetLogicalAddresses(const cec_logical_addresses &addresses)
{
  return m_communication ? m_communication->SetLogicalAddresses(addresses) : false;
}

bool CCECProcessor::StandbyDevices(const cec_logical_address initiator, const CECDEVICEVEC &devices)
{
  bool bReturn(true);
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
    bReturn &= (*it)->Standby(initiator);
  return bReturn;
}

bool CCECProcessor::StandbyDevice(const cec_logical_address initiator, cec_logical_address address)
{
  CCECBusDevice *device = m_busDevices->At(address);
  return device ? device->Standby(initiator) : false;
}

bool CCECProcessor::PowerOnDevices(const cec_logical_address initiator, const CECDEVICEVEC &devices)
{
  bool bReturn(true);
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
    bReturn &= (*it)->PowerOn(initiator);
  return bReturn;
}

bool CCECProcessor::PowerOnDevice(const cec_logical_address initiator, cec_logical_address address)
{
  CCECBusDevice *device = m_busDevices->At(address);
  return device ? device->PowerOn(initiator) : false;
}

bool CCECProcessor::StartBootloader(const char *strPort /* = NULL */)
{
  bool bReturn(false);
  // open a connection if no connection has been opened
  if (!m_communication && strPort)
  {
    CAdapterFactory factory(this->m_libcec);
    IAdapterCommunication *comm = factory.GetInstance(strPort);
    CTimeout timeout(CEC_DEFAULT_CONNECT_TIMEOUT);
    int iConnectTry(0);
    while (timeout.TimeLeft() > 0 && (bReturn = comm->Open(timeout.TimeLeft() / CEC_CONNECT_TRIES, true)) == false)
    {
      m_libcec->AddLog(CEC_LOG_ERROR, "could not open a connection (try %d)", ++iConnectTry);
      comm->Close();
      Sleep(CEC_DEFAULT_TRANSMIT_RETRY_WAIT);
    }
    if (comm->IsOpen())
    {
      bReturn = comm->StartBootloader();
      DELETE_AND_NULL(comm);
    }
    return bReturn;
  }
  else
  {
    m_communication->StartBootloader();
    Close();
    bReturn = true;
  }

  return bReturn;
}

bool CCECProcessor::PingAdapter(void)
{
  return m_communication->PingAdapter();
}

void CCECProcessor::HandlePoll(cec_logical_address initiator, cec_logical_address destination)
{
  CCECBusDevice *device = m_busDevices->At(destination);
  if (device)
    device->HandlePollFrom(initiator);
}

bool CCECProcessor::HandleReceiveFailed(cec_logical_address initiator)
{
  CCECBusDevice *device = m_busDevices->At(initiator);
  return !device || !device->HandleReceiveFailed();
}

bool CCECProcessor::CanPersistConfiguration(void)
{
  return m_communication ? m_communication->GetFirmwareVersion() >= 2 : false;
}

bool CCECProcessor::PersistConfiguration(const libcec_configuration &configuration)
{
  libcec_configuration persistConfiguration = configuration;
  if (!CLibCEC::IsValidPhysicalAddress(configuration.iPhysicalAddress))
  {
    CCECBusDevice *device = GetPrimaryDevice();
    if (device)
      persistConfiguration.iPhysicalAddress = device->GetCurrentPhysicalAddress();
  }

  return m_communication ? m_communication->PersistConfiguration(persistConfiguration) : false;
}

void CCECProcessor::RescanActiveDevices(void)
{
  for (CECDEVICEMAP::iterator it = m_busDevices->Begin(); it != m_busDevices->End(); it++)
    it->second->GetStatus(true);
}

bool CCECProcessor::GetDeviceInformation(const char *strPort, libcec_configuration *config, uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */)
{
  if (!OpenConnection(strPort, CEC_SERIAL_DEFAULT_BAUDRATE, iTimeoutMs, false))
    return false;

  config->iFirmwareVersion   = m_communication->GetFirmwareVersion();
  config->iPhysicalAddress   = m_communication->GetPhysicalAddress();
  config->iFirmwareBuildDate = m_communication->GetFirmwareBuildDate();
  config->adapterType        = m_communication->GetAdapterType();

  Close();

  return true;
}

bool CCECProcessor::TransmitPendingActiveSourceCommands(void)
{
  bool bReturn(true);
  for (CECDEVICEMAP::iterator it = m_busDevices->Begin(); it != m_busDevices->End(); it++)
    bReturn &= it->second->TransmitPendingActiveSourceCommands();
  return bReturn;
}

CCECTV *CCECProcessor::GetTV(void) const
{
  return CCECBusDevice::AsTV(m_busDevices->At(CECDEVICE_TV));
}

CCECAudioSystem *CCECProcessor::GetAudioSystem(void) const
{
  return CCECBusDevice::AsAudioSystem(m_busDevices->At(CECDEVICE_AUDIOSYSTEM));
}

CCECPlaybackDevice *CCECProcessor::GetPlaybackDevice(cec_logical_address address) const
{
  return CCECBusDevice::AsPlaybackDevice(m_busDevices->At(address));
}

CCECRecordingDevice *CCECProcessor::GetRecordingDevice(cec_logical_address address) const
{
  return CCECBusDevice::AsRecordingDevice(m_busDevices->At(address));
}

CCECTuner *CCECProcessor::GetTuner(cec_logical_address address) const
{
  return CCECBusDevice::AsTuner(m_busDevices->At(address));
}

bool CCECProcessor::AllocateLogicalAddresses(CCECClient* client)
{
  libcec_configuration &configuration = *client->GetConfiguration();

  // mark as unregistered
  client->SetRegistered(false);

  // unregister this client from the old addresses
  CECDEVICEVEC devices;
  m_busDevices->GetByLogicalAddresses(devices, configuration.logicalAddresses);
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
  {
    // remove client entry
    CLockObject lock(m_mutex);
    m_clients.erase((*it)->GetLogicalAddress());
  }

  // find logical addresses for this client
  if (!client->AllocateLogicalAddresses())
  {
    m_libcec->AddLog(CEC_LOG_ERROR, "failed to find a free logical address for the client");
    return false;
  }

  // register this client on the new addresses
  devices.clear();
  m_busDevices->GetByLogicalAddresses(devices, configuration.logicalAddresses);
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
  {
    // set the physical address of the device at this LA
    if (CLibCEC::IsValidPhysicalAddress(configuration.iPhysicalAddress))
      (*it)->SetPhysicalAddress(configuration.iPhysicalAddress);

    // replace a previous client
    CLockObject lock(m_mutex);
    m_clients.erase((*it)->GetLogicalAddress());
    m_clients.insert(make_pair((*it)->GetLogicalAddress(), client));
  }

  // set the new ackmask
  SetLogicalAddresses(GetLogicalAddresses());

  // resume outgoing communication
  m_bStallCommunication = false;

  return true;
}

uint16_t CCECProcessor::GetPhysicalAddressFromEeprom(void)
{
  libcec_configuration config; config.Clear();
  if (m_communication)
    m_communication->GetConfiguration(config);
  return config.iPhysicalAddress;
}

bool CCECProcessor::RegisterClient(CCECClient *client)
{
  if (!client)
    return false;

  libcec_configuration &configuration = *client->GetConfiguration();

  if (configuration.clientVersion < CEC_CLIENT_VERSION_2_0_0)
  {
    m_libcec->AddLog(CEC_LOG_ERROR, "failed to register a new CEC client: client version %s is no longer supported", ToString((cec_client_version)configuration.clientVersion));
    return false;
  }

  if (configuration.bMonitorOnly == 1)
    return true;

  if (!CECInitialised())
  {
    m_libcec->AddLog(CEC_LOG_ERROR, "failed to register a new CEC client: CEC processor is not initialised");
    return false;
  }

  // unregister the client first if it's already been marked as registered
  if (client->IsRegistered())
    UnregisterClient(client);

  // ensure that controlled mode is enabled
  m_communication->SetControlledMode(true);
  m_bMonitor = false;

  // source logical address for requests
  cec_logical_address sourceAddress(CECDEVICE_UNREGISTERED);
  if (!m_communication->SupportsSourceLogicalAddress(CECDEVICE_UNREGISTERED))
  {
    if (m_communication->SupportsSourceLogicalAddress(CECDEVICE_FREEUSE))
      sourceAddress = CECDEVICE_FREEUSE;
    else
    {
      m_libcec->AddLog(CEC_LOG_ERROR, "failed to register a new CEC client: both unregistered and free use are not supported by the device");
      return false;
    }
  }

  // ensure that we know the vendor id of the TV
  CCECBusDevice *tv = GetTV();
  cec_vendor_id tvVendor(tv->GetVendorId(sourceAddress));

  // wait until the handler is replaced, to avoid double registrations
  if (tvVendor != CEC_VENDOR_UNKNOWN &&
      CCECCommandHandler::HasSpecificHandler(tvVendor))
  {
    while (!tv->ReplaceHandler(false))
      CEvent::Sleep(5);
  }

  // get the configuration from the client
  m_libcec->AddLog(CEC_LOG_NOTICE, "registering new CEC client - v%s", ToString((cec_client_version)configuration.clientVersion));

  // get the current ackmask, so we can restore it if polling fails
  cec_logical_addresses previousMask = GetLogicalAddresses();

  // mark as uninitialised
  client->SetInitialised(false);

  // find logical addresses for this client
  if (!AllocateLogicalAddresses(client))
  {
    m_libcec->AddLog(CEC_LOG_ERROR, "failed to register the new CEC client - cannot allocate the requested device types");
    SetLogicalAddresses(previousMask);
    return false;
  }

  // get the settings from the rom
  if (configuration.bGetSettingsFromROM == 1)
  {
    libcec_configuration config; config.Clear();
    m_communication->GetConfiguration(config);

    CLockObject lock(m_mutex);
    if (!config.deviceTypes.IsEmpty())
      configuration.deviceTypes = config.deviceTypes;
    if (CLibCEC::IsValidPhysicalAddress(config.iPhysicalAddress))
      configuration.iPhysicalAddress = config.iPhysicalAddress;
    snprintf(configuration.strDeviceName, 13, "%s", config.strDeviceName);
  }

  // set the firmware version and build date
  configuration.serverVersion      = LIBCEC_VERSION_CURRENT;
  configuration.iFirmwareVersion   = m_communication->GetFirmwareVersion();
  configuration.iFirmwareBuildDate = m_communication->GetFirmwareBuildDate();
  configuration.adapterType        = m_communication->GetAdapterType();

  // mark the client as registered
  client->SetRegistered(true);

  sourceAddress = client->GetPrimaryLogicalAdddress();

  // initialise the client
  bool bReturn = client->OnRegister();

  // log the new registration
  CStdString strLog;
  strLog.Format("%s: %s", bReturn ? "CEC client registered" : "failed to register the CEC client", client->GetConnectionInfo().c_str());
  m_libcec->AddLog(bReturn ? CEC_LOG_NOTICE : CEC_LOG_ERROR, strLog);

  // display a warning if the firmware can be upgraded
  if (bReturn && !IsRunningLatestFirmware())
  {
    const char *strUpgradeMessage = "The firmware of this adapter can be upgraded. Please visit http://blog.pulse-eight.com/ for more information.";
    m_libcec->AddLog(CEC_LOG_WARNING, strUpgradeMessage);
    libcec_parameter param;
    param.paramData = (void*)strUpgradeMessage; param.paramType = CEC_PARAMETER_TYPE_STRING;
    client->Alert(CEC_ALERT_SERVICE_DEVICE, param);
  }

  // ensure that the command handler for the TV is initialised
  if (bReturn)
  {
    CCECCommandHandler *handler = GetTV()->GetHandler();
    if (handler)
      handler->InitHandler();
    GetTV()->MarkHandlerReady();
  }

  // report our OSD name to the TV, since some TVs don't request it
  client->GetPrimaryDevice()->TransmitOSDName(CECDEVICE_TV, false);

  // request the power status of the TV
  tv->RequestPowerStatus(sourceAddress, true, true);

  return bReturn;
}

bool CCECProcessor::UnregisterClient(CCECClient *client)
{
  if (!client)
    return false;

  if (client->IsRegistered())
    m_libcec->AddLog(CEC_LOG_NOTICE, "unregistering client: %s", client->GetConnectionInfo().c_str());

  // notify the client that it will be unregistered
  client->OnUnregister();

  {
    CLockObject lock(m_mutex);
    // find all devices that match the LA's of this client
    CECDEVICEVEC devices;
    m_busDevices->GetByLogicalAddresses(devices, client->GetConfiguration()->logicalAddresses);
    for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
    {
      // find the client
      map<cec_logical_address, CCECClient *>::iterator entry = m_clients.find((*it)->GetLogicalAddress());
      // unregister the client
      if (entry != m_clients.end())
        m_clients.erase(entry);

      // reset the device status
      (*it)->ResetDeviceStatus(true);
    }
  }

  // set the new ackmask
  cec_logical_addresses addresses = GetLogicalAddresses();
  if (SetLogicalAddresses(addresses))
  {
    // no more clients left, disable controlled mode
    if (addresses.IsEmpty() && !m_bMonitor)
      m_communication->SetControlledMode(false);

    return true;
  }

  return false;
}

void CCECProcessor::UnregisterClients(void)
{
  m_libcec->AddLog(CEC_LOG_DEBUG, "unregistering all CEC clients");

  vector<CCECClient *> clients = m_libcec->GetClients();
  for (vector<CCECClient *>::iterator client = clients.begin(); client != clients.end(); client++)
    UnregisterClient(*client);

  CLockObject lock(m_mutex);
  m_clients.clear();
}

CCECClient *CCECProcessor::GetClient(const cec_logical_address address)
{
  CLockObject lock(m_mutex);
  map<cec_logical_address, CCECClient *>::const_iterator client = m_clients.find(address);
  if (client != m_clients.end())
    return client->second;
  return NULL;
}

CCECClient *CCECProcessor::GetPrimaryClient(void)
{
  CLockObject lock(m_mutex);
  map<cec_logical_address, CCECClient *>::const_iterator client = m_clients.begin();
  if (client != m_clients.end())
    return client->second;
  return NULL;
}

CCECBusDevice *CCECProcessor::GetPrimaryDevice(void)
{
  return m_busDevices->At(GetLogicalAddress());
}

cec_logical_address CCECProcessor::GetLogicalAddress(void)
{
  cec_logical_addresses addresses = GetLogicalAddresses();
  return addresses.primary;
}

cec_logical_addresses CCECProcessor::GetLogicalAddresses(void)
{
  CLockObject lock(m_mutex);
  cec_logical_addresses addresses;
  addresses.Clear();
  for (map<cec_logical_address, CCECClient *>::const_iterator client = m_clients.begin(); client != m_clients.end(); client++)
    addresses.Set(client->first);

  return addresses;
}

bool CCECProcessor::IsHandledByLibCEC(const cec_logical_address address) const
{
  CCECBusDevice *device = GetDevice(address);
  return device && device->IsHandledByLibCEC();
}

bool CCECProcessor::IsRunningLatestFirmware(void)
{
  return m_communication && m_communication->IsOpen() ?
      m_communication->IsRunningLatestFirmware() :
      true;
}

void CCECProcessor::SwitchMonitoring(bool bSwitchTo)
{
  {
    CLockObject lock(m_mutex);
    m_bMonitor = bSwitchTo;
  }
  if (bSwitchTo)
    UnregisterClients();
}

void CCECProcessor::HandleLogicalAddressLost(cec_logical_address oldAddress)
{
  // stall outgoing messages until we know our new LA
  m_bStallCommunication = true;

  m_libcec->AddLog(CEC_LOG_NOTICE, "logical address %x was taken by another device, allocating a new address", oldAddress);
  CCECClient* client = GetClient(oldAddress);
  if (!client)
    client = GetPrimaryClient();
  if (client)
  {
    if (m_addrAllocator)
      while (m_addrAllocator->IsRunning()) Sleep(5);
    delete m_addrAllocator;

    m_addrAllocator = new CCECAllocateLogicalAddress(this, client);
    m_addrAllocator->CreateThread();
  }
}

void CCECProcessor::HandlePhysicalAddressChanged(uint16_t iNewAddress)
{
  m_libcec->AddLog(CEC_LOG_NOTICE, "physical address changed to %04x", iNewAddress);
  CCECClient* client = GetPrimaryClient();
  if (client)
    client->SetPhysicalAddress(iNewAddress);
}

uint16_t CCECProcessor::GetAdapterVendorId(void) const
{
  return m_communication ? m_communication->GetAdapterVendorId() : 0;
}

uint16_t CCECProcessor::GetAdapterProductId(void) const
{
  return m_communication ? m_communication->GetAdapterProductId() : 0;
}

CCECAllocateLogicalAddress::CCECAllocateLogicalAddress(CCECProcessor* processor, CCECClient* client) :
    m_processor(processor),
    m_client(client) { }

void* CCECAllocateLogicalAddress::Process(void)
{
  m_processor->AllocateLogicalAddresses(m_client);
  return NULL;
}

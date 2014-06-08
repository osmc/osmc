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
#include "USBCECAdapterCommunication.h"

#include "USBCECAdapterCommands.h"
#include "USBCECAdapterMessageQueue.h"
#include "USBCECAdapterMessage.h"
#include "USBCECAdapterDetection.h"
#include "lib/platform/sockets/serialport.h"
#include "lib/platform/util/timeutils.h"
#include "lib/platform/util/util.h"
#include "lib/platform/util/edid.h"
#include "lib/platform/adl/adl-edid.h"
#include "lib/platform/nvidia/nv-edid.h"
#include "lib/LibCEC.h"
#include "lib/CECProcessor.h"

using namespace std;
using namespace CEC;
using namespace PLATFORM;

#define CEC_ADAPTER_PING_TIMEOUT          15000
#define CEC_ADAPTER_EEPROM_WRITE_INTERVAL 30000
#define CEC_ADAPTER_EEPROM_WRITE_RETRY    5000

// firmware version 3
#define CEC_LATEST_ADAPTER_FW_VERSION 3
// firmware date Thu Nov 15 11:09:45 2012
#define CEC_LATEST_ADAPTER_FW_DATE    0x50a4cd79

#define CEC_FW_DATE_EXTENDED_RESPONSE 0x501a4b0c
#define CEC_FW_DATE_DESCRIPTOR2       0x5045dbf5

#define LIB_CEC m_callback->GetLib()

CUSBCECAdapterCommunication::CUSBCECAdapterCommunication(IAdapterCommunicationCallback *callback, const char *strPort, uint16_t iBaudRate /* = CEC_SERIAL_DEFAULT_BAUDRATE */) :
    IAdapterCommunication(callback),
    m_port(NULL),
    m_iLineTimeout(0),
    m_lastPollDestination(CECDEVICE_UNKNOWN),
    m_bInitialised(false),
    m_pingThread(NULL),
    m_eepromWriteThread(NULL),
    m_commands(NULL),
    m_adapterMessageQueue(NULL)
{
  m_logicalAddresses.Clear();
  for (unsigned int iPtr = CECDEVICE_TV; iPtr < CECDEVICE_BROADCAST; iPtr++)
    m_bWaitingForAck[iPtr] = false;
  m_port = new CSerialPort(strPort, iBaudRate);
  m_commands = new CUSBCECAdapterCommands(this);
}

CUSBCECAdapterCommunication::~CUSBCECAdapterCommunication(void)
{
  Close();
  DELETE_AND_NULL(m_commands);
  DELETE_AND_NULL(m_adapterMessageQueue);
  DELETE_AND_NULL(m_port);
}

void CUSBCECAdapterCommunication::ResetMessageQueue(void)
{
  DELETE_AND_NULL(m_adapterMessageQueue);
  m_adapterMessageQueue = new CCECAdapterMessageQueue(this);
  m_adapterMessageQueue->CreateThread();
}

bool CUSBCECAdapterCommunication::Open(uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */, bool bSkipChecks /* = false */, bool bStartListening /* = true */)
{
  bool bConnectionOpened(false);
  {
    CLockObject lock(m_mutex);

    /* we need the port settings here */
    if (!m_port)
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "port is NULL");
      return bConnectionOpened;
    }

    /* return true when the port is already open */
    if (IsOpen())
    {
      LIB_CEC->AddLog(CEC_LOG_WARNING, "port is already open");
      return true;
    }

    ResetMessageQueue();

    /* try to open the connection */
    CStdString strError;
    CTimeout timeout(iTimeoutMs);
    while (!bConnectionOpened && timeout.TimeLeft() > 0)
    {
      if ((bConnectionOpened = m_port->Open(timeout.TimeLeft())) == false)
      {
        strError.Format("error opening serial port '%s': %s", m_port->GetName().c_str(), m_port->GetError().c_str());
        Sleep(250);
      }
      /* and retry every 250ms until the timeout passed */
    }

    /* return false when we couldn't connect */
    if (!bConnectionOpened)
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, strError);

      if (m_port->GetErrorNumber() == EACCES)
      {
        libcec_parameter param;
        param.paramType = CEC_PARAMETER_TYPE_STRING;
        param.paramData = (void*)"No permission to open the device";
        LIB_CEC->Alert(CEC_ALERT_PERMISSION_ERROR, param);
      }
      else if (m_port->GetErrorNumber() == EBUSY)
      {
        libcec_parameter param;
        param.paramType = CEC_PARAMETER_TYPE_STRING;
        param.paramData = (void*)"The serial port is busy. Only one program can access the device directly.";
        LIB_CEC->Alert(CEC_ALERT_PORT_BUSY, param);
      }
      return false;
    }

    LIB_CEC->AddLog(CEC_LOG_DEBUG, "connection opened, clearing any previous input and waiting for active transmissions to end before starting");
    ClearInputBytes();
  }

  // always start by setting the ackmask to 0, to clear previous values
  cec_logical_addresses addresses; addresses.Clear();
  SetLogicalAddresses(addresses);

  if (!CreateThread())
  {
    bConnectionOpened = false;
    LIB_CEC->AddLog(CEC_LOG_ERROR, "could not create a communication thread");
  }
  else if (!bSkipChecks && !CheckAdapter())
  {
    bConnectionOpened = false;
    LIB_CEC->AddLog(CEC_LOG_ERROR, "the adapter failed to pass basic checks");
  }
  else if (bStartListening)
  {
    /* start the eeprom write thread, that handles all eeprom writes async */
    m_eepromWriteThread = new CAdapterEepromWriteThread(this);
    if (!m_eepromWriteThread->CreateThread())
    {
      bConnectionOpened = false;
      LIB_CEC->AddLog(CEC_LOG_ERROR, "could not create the eeprom write thread");
    }
    else
    {
      /* start a ping thread, that will ping the adapter every 15 seconds
         if it doesn't receive any ping for 30 seconds, it'll switch to auto mode */
      m_pingThread = new CAdapterPingThread(this, CEC_ADAPTER_PING_TIMEOUT);
      if (m_pingThread->CreateThread())
      {
        bConnectionOpened = true;
      }
      else
      {
        bConnectionOpened = false;
        LIB_CEC->AddLog(CEC_LOG_ERROR, "could not create a ping thread");
      }
    }
  }

  if (!bConnectionOpened || !bStartListening)
    StopThread(0);

  return bConnectionOpened;
}

void CUSBCECAdapterCommunication::Close(void)
{
  /* stop the reader thread */
  StopThread(0);

  CLockObject lock(m_mutex);

  /* set the ackmask to 0 before closing the connection */
  if (IsOpen() && m_port->GetErrorNumber() == 0)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - closing the connection", __FUNCTION__);
    cec_logical_addresses addresses; addresses.Clear();
    SetLogicalAddresses(addresses);
    if (m_commands->GetFirmwareVersion() >= 2)
      SetControlledMode(false);
  }

  m_adapterMessageQueue->Clear();

  /* stop and delete the write thread */
  if (m_eepromWriteThread)
    m_eepromWriteThread->Stop();
  DELETE_AND_NULL(m_eepromWriteThread);

  /* stop and delete the ping thread */
  DELETE_AND_NULL(m_pingThread);

  /* close and delete the com port connection */
  if (m_port)
    m_port->Close();
}

cec_adapter_message_state CUSBCECAdapterCommunication::Write(const cec_command &data, bool &bRetry, uint8_t iLineTimeout, bool bIsReply)
{
  cec_adapter_message_state retVal(ADAPTER_MESSAGE_STATE_UNKNOWN);
  if (!IsRunning())
    return retVal;

  CCECAdapterMessage *output = new CCECAdapterMessage(data, iLineTimeout);
  output->bFireAndForget = bIsReply;

  /* mark as waiting for an ack from the destination */
  MarkAsWaiting(data.destination);

  /* send the message */
  if (bIsReply)
  {
    retVal = m_adapterMessageQueue->Write(output) ?
        ADAPTER_MESSAGE_STATE_WAITING_TO_BE_SENT : ADAPTER_MESSAGE_STATE_ERROR;
  }
  else
  {
    bRetry = (!m_adapterMessageQueue->Write(output) || output->NeedsRetry()) && output->transmit_timeout > 0;
    if (bRetry)
      Sleep(CEC_DEFAULT_TRANSMIT_RETRY_WAIT);
    retVal = output->state;

    delete output;
  }
  return retVal;
}

void *CUSBCECAdapterCommunication::Process(void)
{
  CCECAdapterMessage msg;
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "communication thread started");

  while (!IsStopped())
  {
    /* read from the serial port */
    if (!ReadFromDevice(50, 5))
    {
      libcec_parameter param;
      param.paramData = NULL; param.paramType = CEC_PARAMETER_TYPE_UNKOWN;
      LIB_CEC->Alert(CEC_ALERT_CONNECTION_LOST, param);

      break;
    }

    /* TODO sleep 5 ms so other threads can get a lock */
    if (!IsStopped())
      Sleep(5);
  }

  m_adapterMessageQueue->Clear();
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "communication thread ended");
  return NULL;
}

bool CUSBCECAdapterCommunication::HandlePoll(const CCECAdapterMessage &msg)
{
  bool bIsError(msg.IsError());
  cec_adapter_messagecode messageCode(msg.Message());
  CLockObject lock(m_mutex);

  if (messageCode == MSGCODE_FRAME_START && msg.IsACK())
  {
    m_lastPollDestination = msg.Destination();
    if (msg.Destination() < CECDEVICE_BROADCAST)
    {
      CLockObject waitingLock(m_waitingMutex);
      if (!m_bWaitingForAck[msg.Destination()] && !msg.IsEOM())
      {
        if (m_callback)
          m_callback->HandlePoll(msg.Initiator(), msg.Destination());
      }
      else
        m_bWaitingForAck[msg.Destination()] = false;
    }
  }
  else if (messageCode == MSGCODE_RECEIVE_FAILED)
  {
    /* hack to suppress warnings when an LG is polling */
    if (m_lastPollDestination != CECDEVICE_UNKNOWN)
      bIsError = m_callback->HandleReceiveFailed(m_lastPollDestination);
  }

  return bIsError;
}

void CUSBCECAdapterCommunication::MarkAsWaiting(const cec_logical_address dest)
{
  /* mark as waiting for an ack from the destination */
  if (dest < CECDEVICE_BROADCAST)
  {
    CLockObject waitingLock(m_waitingMutex);
    m_bWaitingForAck[dest] = true;
  }
}

void CUSBCECAdapterCommunication::ClearInputBytes(uint32_t iTimeout /* = CEC_CLEAR_INPUT_DEFAULT_WAIT */)
{
  CTimeout timeout(iTimeout);
  uint8_t buff[1024];
  ssize_t iBytesRead(0);
  bool bGotMsgEnd(true);

  while (timeout.TimeLeft() > 0 && ((iBytesRead = m_port->Read(buff, 1024, 5)) > 0 || !bGotMsgEnd))
  {
    bGotMsgEnd = false;
    /* if something was received, wait for MSGEND */
    for (ssize_t iPtr = 0; iPtr < iBytesRead; iPtr++)
      bGotMsgEnd = buff[iPtr] == MSGEND;
  }
}

bool CUSBCECAdapterCommunication::SetLineTimeout(uint8_t iTimeout)
{
  bool bReturn(true);
  bool bChanged(false);

  /* only send the command if the timeout changed */
  {
    CLockObject lock(m_mutex);
    bChanged = (m_iLineTimeout != iTimeout);
    m_iLineTimeout = iTimeout;
  }

  if (bChanged)
    bReturn = m_commands->SetLineTimeout(iTimeout);

  return bReturn;
}

bool CUSBCECAdapterCommunication::WriteToDevice(CCECAdapterMessage *message)
{
  CLockObject adapterLock(m_mutex);
  if (!IsOpen())
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "error writing command '%s' to serial port '%s': the connection is closed", CCECAdapterMessage::ToString(message->Message()), m_port->GetName().c_str());
    message->state = ADAPTER_MESSAGE_STATE_ERROR;
    return false;
  }

  /* write the message */
  if (m_port->Write(message->packet.data, message->Size()) != (ssize_t) message->Size())
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "error writing command '%s' to serial port '%s': %s", CCECAdapterMessage::ToString(message->Message()), m_port->GetName().c_str(), m_port->GetError().c_str());
    message->state = ADAPTER_MESSAGE_STATE_ERROR;
    // let the higher level close the port
    return false;
  }

#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "command '%s' sent", message->IsTranmission() ? "CEC transmission" : CCECAdapterMessage::ToString(message->Message()));
#endif
  message->state = ADAPTER_MESSAGE_STATE_SENT;
  return true;
}

bool CUSBCECAdapterCommunication::ReadFromDevice(uint32_t iTimeout, size_t iSize /* = 256 */)
{
  ssize_t iBytesRead(0);
  uint8_t buff[256];
  if (iSize > 256)
    iSize = 256;

  /* read from the serial port */
  {
    CLockObject lock(m_mutex);
    if (!IsOpen())
      return false;

    do {
      /* retry Read() if it was interrupted */
      iBytesRead = m_port->Read(buff, sizeof(uint8_t) * iSize, iTimeout);
    } while(m_port->GetErrorNumber() == EINTR);


    if (m_port->GetErrorNumber())
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "error reading from serial port: %s", m_port->GetError().c_str());
      // let the higher level close the port
      return false;
    }
  }

  if (iBytesRead < 0 || iBytesRead > 256)
    return false;
  else if (iBytesRead > 0)
  {
    /* add the data to the current frame */
    m_adapterMessageQueue->AddData(buff, iBytesRead);
  }

  return true;
}

CCECAdapterMessage *CUSBCECAdapterCommunication::SendCommand(cec_adapter_messagecode msgCode, CCECAdapterMessage &params, bool bIsRetry /* = false */)
{
  if (!IsOpen() || !m_adapterMessageQueue)
    return NULL;

  /* create the adapter message for this command */
  CCECAdapterMessage *output = new CCECAdapterMessage;
  output->PushBack(MSGSTART);
  output->PushEscaped((uint8_t)msgCode);
  output->Append(params);
  output->PushBack(MSGEND);

  /* write the command */
  if (!m_adapterMessageQueue->Write(output))
  {
    // this will trigger an alert in the reader thread
    if (output->state == ADAPTER_MESSAGE_STATE_ERROR)
      m_port->Close();
    return output;
  }
  else
  {
    if (!bIsRetry && output->Reply() == MSGCODE_COMMAND_REJECTED && msgCode != MSGCODE_SET_CONTROLLED &&
        msgCode != MSGCODE_GET_BUILDDATE /* same messagecode value had a different meaning in older fw builds */)
    {
      /* if the controller reported that the command was rejected, and we didn't send the command
         to set controlled mode, then the controller probably switched to auto mode. set controlled
         mode and retry */
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "setting controlled mode and retrying");
      delete output;
      if (SetControlledMode(true))
        return SendCommand(msgCode, params, true);
    }
  }

  return output;
}

bool CUSBCECAdapterCommunication::CheckAdapter(uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */)
{
  bool bReturn(false);
  CTimeout timeout(iTimeoutMs > 0 ? iTimeoutMs : CEC_DEFAULT_TRANSMIT_WAIT);

  /* try to ping the adapter */
  bool bPinged(false);
  unsigned iPingTry(0);
  while (timeout.TimeLeft() > 0 && (bPinged = PingAdapter()) == false)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "the adapter did not respond correctly to a ping (try %d)", ++iPingTry);
    CEvent::Sleep(500);
  }

  /* try to read the firmware version */
  if (bPinged && timeout.TimeLeft() > 0 && m_commands->RequestFirmwareVersion() >= 2)
  {
    /* try to set controlled mode for v2+ firmwares */
    unsigned iControlledTry(0);
    bool bControlled(false);
    while (timeout.TimeLeft() > 0 && (bControlled = SetControlledMode(true)) == false)
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "the adapter did not respond correctly to setting controlled mode (try %d)", ++iControlledTry);
      CEvent::Sleep(500);
    }
    bReturn = bControlled;
  }
  else
    bReturn = true;

  if (m_commands->GetFirmwareVersion() >= 2)
  {
    /* try to read the build date */
    m_commands->RequestBuildDate();

    /* try to read the adapter type */
    m_commands->RequestAdapterType();
  }

  SetInitialised(bReturn);
  return bReturn;
}

bool CUSBCECAdapterCommunication::IsOpen(void)
{
  /* thread is not being stopped, the port is open and the thread is running */
  return !IsStopped() && m_port->IsOpen() && IsRunning();
}

std::string CUSBCECAdapterCommunication::GetError(void) const
{
  return m_port->GetError();
}

void CUSBCECAdapterCommunication::SetInitialised(bool bSetTo /* = true */)
{
  CLockObject lock(m_mutex);
  m_bInitialised = bSetTo;
}

bool CUSBCECAdapterCommunication::IsInitialised(void)
{
  CLockObject lock(m_mutex);
  return m_bInitialised;
}

bool CUSBCECAdapterCommunication::StartBootloader(void)
{
  if (m_port->IsOpen() && m_commands->StartBootloader())
  {
    m_port->Close();
    return true;
  }
  return false;
}

bool CUSBCECAdapterCommunication::SetLogicalAddresses(const cec_logical_addresses &addresses)
{
  {
    CLockObject lock(m_mutex);
    if (m_logicalAddresses == addresses)
      return true;
  }

  if (IsOpen() && m_commands->SetAckMask(addresses.AckMask()))
  {
    CLockObject lock(m_mutex);
    m_logicalAddresses = addresses;
    return true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "couldn't change the ackmask: the connection is closed");
  return false;
}

cec_logical_addresses CUSBCECAdapterCommunication::GetLogicalAddresses(void)
{
  cec_logical_addresses addresses;
  CLockObject lock(m_mutex);
  addresses = m_logicalAddresses;
  return addresses;
}

bool CUSBCECAdapterCommunication::PingAdapter(void)
{
  return IsOpen() ? m_commands->PingAdapter() : false;
}

uint16_t CUSBCECAdapterCommunication::GetFirmwareVersion(void)
{
  return m_commands ? m_commands->GetFirmwareVersion() : CEC_FW_VERSION_UNKNOWN;
}

uint32_t CUSBCECAdapterCommunication::GetFirmwareBuildDate(void)
{
  uint32_t iBuildDate(0);
  if (m_commands)
    iBuildDate = m_commands->GetPersistedBuildDate();
  if (iBuildDate == 0 && IsOpen())
    iBuildDate = m_commands->RequestBuildDate();

  return iBuildDate;
}

cec_adapter_type CUSBCECAdapterCommunication::GetAdapterType(void)
{
  cec_adapter_type type(ADAPTERTYPE_UNKNOWN);
  if (m_commands)
    type = (cec_adapter_type)m_commands->GetPersistedAdapterType();
  if (type == ADAPTERTYPE_UNKNOWN && IsOpen())
    type = (cec_adapter_type)m_commands->RequestAdapterType();

  return type;
}

bool CUSBCECAdapterCommunication::ProvidesExtendedResponse(void)
{
  uint32_t iBuildDate(0);
  if (m_commands)
    iBuildDate = m_commands->GetPersistedBuildDate();

  return iBuildDate >= CEC_FW_DATE_EXTENDED_RESPONSE;
}

uint16_t CUSBCECAdapterCommunication::GetAdapterVendorId(void) const
{
  return CEC_VID;
}

uint16_t CUSBCECAdapterCommunication::GetAdapterProductId(void) const
{
  uint32_t iBuildDate(0);
  if (m_commands)
    iBuildDate = m_commands->GetPersistedBuildDate();

  return iBuildDate >= CEC_FW_DATE_DESCRIPTOR2 ? CEC_PID2 : CEC_PID;
}

void CUSBCECAdapterCommunication::SetActiveSource(bool bSetTo, bool bClientUnregistered)
{
  if (m_commands)
    m_commands->SetActiveSource(bSetTo, bClientUnregistered);
}

bool CUSBCECAdapterCommunication::IsRunningLatestFirmware(void)
{
  return GetFirmwareBuildDate() >= CEC_LATEST_ADAPTER_FW_DATE &&
      GetFirmwareVersion() >= CEC_LATEST_ADAPTER_FW_VERSION;
}

bool CUSBCECAdapterCommunication::PersistConfiguration(const libcec_configuration &configuration)
{
  return IsOpen() ?
      m_commands->PersistConfiguration(configuration) && m_eepromWriteThread->Write() :
      false;
}

bool CUSBCECAdapterCommunication::GetConfiguration(libcec_configuration &configuration)
{
  return IsOpen() ? m_commands->GetConfiguration(configuration) : false;
}

std::string CUSBCECAdapterCommunication::GetPortName(void)
{
  return m_port->GetName();
}

bool CUSBCECAdapterCommunication::SetControlledMode(bool controlled)
{
  return IsOpen() ? m_commands->SetControlledMode(controlled) : false;
}

uint16_t CUSBCECAdapterCommunication::GetPhysicalAddress(void)
{
  uint16_t iPA(0);

  // try to get the PA from ADL
#if defined(HAS_ADL_EDID_PARSER)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - trying to get the physical address via ADL", __FUNCTION__);
    CADLEdidParser adl;
    iPA = adl.GetPhysicalAddress();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - ADL returned physical address %04x", __FUNCTION__, iPA);
  }
#endif

  // try to get the PA from the nvidia driver
#if defined(HAS_NVIDIA_EDID_PARSER)
  if (iPA == 0)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - trying to get the physical address via nvidia driver", __FUNCTION__);
    CNVEdidParser nv;
    iPA = nv.GetPhysicalAddress();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - nvidia driver returned physical address %04x", __FUNCTION__, iPA);
  }
#endif

  // try to get the PA from the OS
  if (iPA == 0)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - trying to get the physical address from the OS", __FUNCTION__);
    iPA = CEDIDParser::GetPhysicalAddress();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - OS returned physical address %04x", __FUNCTION__, iPA);
  }

  return iPA;
}

void *CAdapterPingThread::Process(void)
{
  while (!IsStopped())
  {
    if (m_timeout.TimeLeft() == 0)
    {
      /* reinit the timeout */
      m_timeout.Init(CEC_ADAPTER_PING_TIMEOUT);

      /* send a ping to the adapter */
      bool bPinged(false);
      int iFailedCounter(0);
      while (!bPinged && iFailedCounter < 3)
      {
        if (!m_com->PingAdapter())
        {
          /* sleep and retry */
          Sleep(CEC_DEFAULT_TRANSMIT_RETRY_WAIT);
          ++iFailedCounter;
        }
        else
        {
          bPinged = true;
        }
      }

      if (iFailedCounter == 3)
      {
        /* failed to ping the adapter 3 times in a row. something must be wrong with the connection */
        m_com->LIB_CEC->AddLog(CEC_LOG_ERROR, "failed to ping the adapter 3 times in a row. closing the connection.");
        m_com->StopThread(false);

        libcec_parameter param;
        param.paramData = NULL; param.paramType = CEC_PARAMETER_TYPE_UNKOWN;
        m_com->LIB_CEC->Alert(CEC_ALERT_CONNECTION_LOST, param);

        break;
      }
    }

    Sleep(5);
  }
  return NULL;
}

void CAdapterEepromWriteThread::Stop(void)
{
  StopThread(-1);
  {
    CLockObject lock(m_mutex);
    if (m_iScheduleEepromWrite > 0)
      m_com->LIB_CEC->AddLog(CEC_LOG_WARNING, "write thread stopped while a write was queued");
    m_bWrite = true;
    m_condition.Signal();
  }
  StopThread();
}

void *CAdapterEepromWriteThread::Process(void)
{
  while (!IsStopped())
  {
    CLockObject lock(m_mutex);
    if ((m_iScheduleEepromWrite > 0 && m_iScheduleEepromWrite < GetTimeMs()) ||
        m_condition.Wait(m_mutex, m_bWrite, 100))
    {
      if (IsStopped())
        break;
      m_bWrite = false;
      if (m_com->m_commands->WriteEEPROM())
      {
        m_iLastEepromWrite = GetTimeMs();
        m_iScheduleEepromWrite = 0;
      }
      else
      {
        m_iScheduleEepromWrite = GetTimeMs() + CEC_ADAPTER_EEPROM_WRITE_RETRY;
      }
    }
  }
  return NULL;
}

bool CAdapterEepromWriteThread::Write(void)
{
  CLockObject lock(m_mutex);
  if (m_iScheduleEepromWrite == 0)
  {
    int64_t iNow = GetTimeMs();
    if (m_iLastEepromWrite + CEC_ADAPTER_EEPROM_WRITE_INTERVAL > iNow)
    {
      m_com->LIB_CEC->AddLog(CEC_LOG_DEBUG, "delaying eeprom write by %ld ms", m_iLastEepromWrite + CEC_ADAPTER_EEPROM_WRITE_INTERVAL - iNow);
      m_iScheduleEepromWrite = m_iLastEepromWrite + CEC_ADAPTER_EEPROM_WRITE_INTERVAL;
    }
    else
    {
      m_bWrite = true;
      m_condition.Signal();
    }
  }
  return true;
}

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

#if defined(HAVE_TDA995X_API)
#include "TDA995xCECAdapterCommunication.h"

#include "lib/CECTypeUtils.h"
#include "lib/LibCEC.h"
#include "lib/platform/sockets/cdevsocket.h"
#include "lib/platform/util/StdString.h"
#include "lib/platform/util/buffer.h"

extern "C" {
#define __cec_h__
#include <../comps/tmdlHdmiCEC/inc/tmdlHdmiCEC_Types.h>
#include <../tda998x_ioctl.h>
}

using namespace std;
using namespace CEC;
using namespace PLATFORM;

#include "AdapterMessageQueue.h"

#define LIB_CEC m_callback->GetLib()

// these are defined in nxp private header file
#define CEC_MSG_SUCCESS                 0x00	/*Message transmisson Succeed*/
#define CEC_CSP_OFF_STATE               0x80	/*CSP in Off State*/
#define CEC_BAD_REQ_SERVICE             0x81	/*Bad .req service*/
#define CEC_MSG_FAIL_UNABLE_TO_ACCESS	0x82	/*Message transmisson failed: Unable to access CEC line*/
#define CEC_MSG_FAIL_ARBITRATION_ERROR	0x83	/*Message transmisson failed: Arbitration error*/
#define CEC_MSG_FAIL_BIT_TIMMING_ERROR	0x84	/*Message transmisson failed: Bit timming error*/
#define CEC_MSG_FAIL_DEST_NOT_ACK       0x85	/*Message transmisson failed: Destination Address not aknowledged*/
#define CEC_MSG_FAIL_DATA_NOT_ACK       0x86	/*Message transmisson failed: Databyte not acknowledged*/


CTDA995xCECAdapterCommunication::CTDA995xCECAdapterCommunication(IAdapterCommunicationCallback *callback) :
    IAdapterCommunication(callback),
    m_bLogicalAddressChanged(false)
{ 
  CLockObject lock(m_mutex);

  m_iNextMessage = 0;
  m_logicalAddresses.Clear();
  m_dev = new CCDevSocket(CEC_TDA995x_PATH);
}


CTDA995xCECAdapterCommunication::~CTDA995xCECAdapterCommunication(void)
{
  Close();

  CLockObject lock(m_mutex);
  delete m_dev;
  m_dev = 0;
}


bool CTDA995xCECAdapterCommunication::IsOpen(void)
{
  return IsInitialised() && m_dev->IsOpen();
}

    
bool CTDA995xCECAdapterCommunication::Open(uint32_t iTimeoutMs, bool UNUSED(bSkipChecks), bool bStartListening)
{
  if (m_dev->Open(iTimeoutMs))
  {
    unsigned char raw_mode = 0xff;
    
    if (m_dev->Ioctl(CEC_IOCTL_GET_RAW_MODE, &raw_mode) == 0)
    {
      raw_mode = 1;
      if (m_dev->Ioctl(CEC_IOCTL_SET_RAW_MODE, &raw_mode) == 0)
      {
        if (!bStartListening || CreateThread())
          return true;
      }
      else
      {
        LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_SET_RAW_MODE failed !", __func__);
      }

      raw_mode = 0;
      m_dev->Ioctl(CEC_IOCTL_SET_RAW_MODE, &raw_mode);
    }
    else
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, 
        "%s: CEC_IOCTL_GET_RAW_MODE not supported. Please update your kernel.", __func__);
    }

    m_dev->Close();
  }

  return false;
}


void CTDA995xCECAdapterCommunication::Close(void)
{
  StopThread(0);

  unsigned char raw_mode = 0;
  m_dev->Ioctl(CEC_IOCTL_SET_RAW_MODE, &raw_mode);

  m_dev->Close();
}


std::string CTDA995xCECAdapterCommunication::GetError(void) const
{
  std::string strError(m_strError);
  return strError;
}


cec_adapter_message_state CTDA995xCECAdapterCommunication::Write(
  const cec_command &data, bool &UNUSED(bRetry), uint8_t UNUSED(iLineTimeout), bool UNUSED(bIsReply))
{
  cec_frame frame;
  CAdapterMessageQueueEntry *entry;
  cec_adapter_message_state rc = ADAPTER_MESSAGE_STATE_ERROR;

  if ((size_t)data.parameters.size + data.opcode_set > sizeof(frame.data))
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: data size too large !", __func__);
    return ADAPTER_MESSAGE_STATE_ERROR;
  }
  
  frame.size    = 0;
  frame.service = 0;
  frame.addr    = (data.initiator << 4) | (data.destination & 0x0f);

  if (data.opcode_set)
  {
    frame.data[0] = data.opcode;
    frame.size++;

    memcpy(&frame.data[frame.size], data.parameters.data, data.parameters.size);
    frame.size += data.parameters.size;
  }
  
  frame.size += 3;

  entry = new CAdapterMessageQueueEntry(data);
  
  m_messageMutex.Lock();
  uint32_t msgKey = ++m_iNextMessage;
  m_messages.insert(make_pair(msgKey, entry));
 
  if (m_dev->Write((char *)&frame, sizeof(frame)) == sizeof(frame))
  {
    m_messageMutex.Unlock();

    if (entry->Wait(CEC_DEFAULT_TRANSMIT_WAIT))
    {
      uint32_t status = entry->Result();
     
      if (status == CEC_MSG_FAIL_DEST_NOT_ACK)
        rc = ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
      else if (status == CEC_MSG_SUCCESS)
        rc = ADAPTER_MESSAGE_STATE_SENT_ACKED;
    }
    else
      LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: command timed out !", __func__);
    
    m_messageMutex.Lock();
  }
  else
     LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: write failed !", __func__);

  m_messages.erase(msgKey);
  m_messageMutex.Unlock();

  delete entry;

  return rc;
}


uint16_t CTDA995xCECAdapterCommunication::GetFirmwareVersion(void)
{
  cec_sw_version  vers = { 0 };

  m_dev->Ioctl(CEC_IOCTL_GET_SW_VERSION, &vers);
  
  return vers.majorVersionNr;
}


cec_vendor_id CTDA995xCECAdapterCommunication::GetVendorId(void)
{
  cec_raw_info info;
 
  if (m_dev->Ioctl(CEC_IOCTL_GET_RAW_INFO, &info) != 0)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_GET_RAW_INFO failed !", __func__);
    return CEC_VENDOR_LG; 
  }
  
  return cec_vendor_id(info.VendorID);
}


uint16_t CTDA995xCECAdapterCommunication::GetPhysicalAddress(void)
{
  cec_raw_info info;
 
  if (m_dev->Ioctl(CEC_IOCTL_GET_RAW_INFO, &info) != 0)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_GET_RAW_INFO failed !", __func__);
    return CEC_INVALID_PHYSICAL_ADDRESS; 
  }
  
  return info.PhysicalAddress;
}


cec_logical_addresses CTDA995xCECAdapterCommunication::GetLogicalAddresses(void)
{
  CLockObject lock(m_mutex);

  if (m_bLogicalAddressChanged || m_logicalAddresses.IsEmpty() )
  {
    cec_raw_info info;

    m_logicalAddresses.Clear();

    if (m_dev->Ioctl(CEC_IOCTL_GET_RAW_INFO, &info) != 0)
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_GET_RAW_INFO failed !", __func__);
    }
    else if (info.LogicalAddress != CECDEVICE_UNREGISTERED)
    {
      m_logicalAddresses.Set(cec_logical_address(info.LogicalAddress));
      
      for (int la = CECDEVICE_TV; la < CECDEVICE_BROADCAST; la++)
      {
        m_logicalAddresses.Set(cec_logical_address(la));  
      }
    }

    m_bLogicalAddressChanged = false;
  }

  return m_logicalAddresses;
}


bool CTDA995xCECAdapterCommunication::SetLogicalAddresses(const cec_logical_addresses &addresses)
{
  unsigned char log_addr = addresses.primary;
  
  if (m_dev->Ioctl(CEC_IOCTL_RX_ADDR, &log_addr) != 0)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_RX_ADDR failed !", __func__);
    return false;
  }

  cec_rx_mask all_addresses;
  
  all_addresses.SwitchOn  = addresses.AckMask() & 0x7fff;
  all_addresses.SwitchOff = ~all_addresses.SwitchOn;
  
  if (all_addresses.SwitchOn != (1 << addresses.primary) &&
      m_dev->Ioctl(CEC_IOCTL_SET_RX_ADDR_MASK, &all_addresses) != 0)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_SET_RX_ADDR_MASK failed !", __func__);
    return false;
  }
  
  m_bLogicalAddressChanged = true;
  
  return true;
}


void CTDA995xCECAdapterCommunication::HandleLogicalAddressLost(cec_logical_address UNUSED(oldAddress))
{
  unsigned char log_addr = CECDEVICE_BROADCAST;

  if (m_dev->Ioctl(CEC_IOCTL_RX_ADDR, &log_addr) != 0)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_RX_ADDR failed !", __func__);
  }
}


void *CTDA995xCECAdapterCommunication::Process(void)
{
  bool bHandled;
  cec_frame frame;
  uint32_t opcode, status;
  cec_logical_address initiator, destination;

  while (!IsStopped())
  {
    if (m_dev->Read((char *)&frame, sizeof(frame), 500) == sizeof(frame))
    {
      initiator = cec_logical_address(frame.addr >> 4);
      destination = cec_logical_address(frame.addr & 0x0f);
      
      if (frame.service == CEC_RX_PKT)
      {
        cec_command cmd;

        cec_command::Format(
          cmd, initiator, destination,
          ( frame.size > 3 ) ? cec_opcode(frame.data[0]) : CEC_OPCODE_NONE);

        for( uint8_t i = 1; i < frame.size-3; i++ )
          cmd.parameters.PushBack(frame.data[i]);

        if (!IsStopped())
          m_callback->OnCommandReceived(cmd);
      }
      else if (frame.service == CEC_ACK_PKT)
      {
        bHandled = false;
        status = ( frame.size > 3 ) ? frame.data[0] : 255;
        opcode = ( frame.size > 4 ) ? frame.data[1] : (uint32_t)CEC_OPCODE_NONE;

        m_messageMutex.Lock();
        for (map<uint32_t, CAdapterMessageQueueEntry *>::iterator it = m_messages.begin(); 
             !bHandled && it != m_messages.end(); it++)
        {
          bHandled = it->second->CheckMatch(opcode, initiator, destination, status);
        }
        m_messageMutex.Unlock();
      
        if (!bHandled)
          LIB_CEC->AddLog(CEC_LOG_WARNING, "%s: unhandled response received !", __func__);
      }
    }
  }

  return 0;
}

#endif	// HAVE_TDA995X_API

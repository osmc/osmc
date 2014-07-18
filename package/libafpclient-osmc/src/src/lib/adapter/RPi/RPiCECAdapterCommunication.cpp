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

#if defined(HAVE_RPI_API)
#include "RPiCECAdapterCommunication.h"

extern "C" {
#include <bcm_host.h>
}

#include "lib/CECTypeUtils.h"
#include "lib/LibCEC.h"
#include "lib/platform/util/StdString.h"
#include "RPiCECAdapterMessageQueue.h"

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC m_callback->GetLib()

static bool g_bHostInited = false;

// callback for the RPi CEC service
void rpi_cec_callback(void *callback_data, uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
  if (callback_data)
    static_cast<CRPiCECAdapterCommunication *>(callback_data)->OnDataReceived(p0, p1, p2, p3, p4);
}

// callback for the TV service
void rpi_tv_callback(void *callback_data, uint32_t reason, uint32_t p0, uint32_t p1)
{
  if (callback_data)
    static_cast<CRPiCECAdapterCommunication *>(callback_data)->OnTVServiceCallback(reason, p0, p1);
}

CRPiCECAdapterCommunication::CRPiCECAdapterCommunication(IAdapterCommunicationCallback *callback) :
    IAdapterCommunication(callback),
    m_logicalAddress(CECDEVICE_UNKNOWN),
    m_bLogicalAddressChanged(false),
    m_previousLogicalAddress(CECDEVICE_FREEUSE),
    m_bLogicalAddressRegistered(false)
{
  m_queue = new CRPiCECAdapterMessageQueue(this);
}

CRPiCECAdapterCommunication::~CRPiCECAdapterCommunication(void)
{
  delete(m_queue);
  Close();
}

const char *ToString(const VC_CEC_ERROR_T error)
{
  switch(error)
  {
  case VC_CEC_SUCCESS:
    return "success";
  case VC_CEC_ERROR_NO_ACK:
    return "no ack";
  case VC_CEC_ERROR_SHUTDOWN:
    return "shutdown";
  case VC_CEC_ERROR_BUSY:
    return "device is busy";
  case VC_CEC_ERROR_NO_LA:
    return "no logical address";
  case VC_CEC_ERROR_NO_PA:
    return "no physical address";
  case VC_CEC_ERROR_NO_TOPO:
    return "no topology";
  case VC_CEC_ERROR_INVALID_FOLLOWER:
    return "invalid follower";
  case VC_CEC_ERROR_INVALID_ARGUMENT:
    return "invalid arg";
  default:
    return "unknown";
  }
}

bool CRPiCECAdapterCommunication::IsInitialised(void)
{
  CLockObject lock(m_mutex);
  return m_bInitialised;
}

void CRPiCECAdapterCommunication::OnTVServiceCallback(uint32_t reason, uint32_t UNUSED(p0), uint32_t UNUSED(p1))
{
  switch(reason)
  {
  case VC_HDMI_ATTACHED:
  {
    uint16_t iNewAddress = GetPhysicalAddress();
    m_callback->HandlePhysicalAddressChanged(iNewAddress);
    break;
  }
  case VC_HDMI_UNPLUGGED:
  case VC_HDMI_DVI:
  case VC_HDMI_HDMI:
  case VC_HDMI_HDCP_UNAUTH:
  case VC_HDMI_HDCP_AUTH:
  case VC_HDMI_HDCP_KEY_DOWNLOAD:
  case VC_HDMI_HDCP_SRM_DOWNLOAD:
  default:
     break;
  }
}

void CRPiCECAdapterCommunication::OnDataReceived(uint32_t header, uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3)
{
  VC_CEC_NOTIFY_T reason = (VC_CEC_NOTIFY_T)CEC_CB_REASON(header);

#ifdef CEC_DEBUGGING
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "received data: header:%08X p0:%08X p1:%08X p2:%08X p3:%08X reason:%x", header, p0, p1, p2, p3, reason);
#endif

  switch (reason)
  {
  case VC_CEC_RX:
    // CEC data received
    {
      // translate into a VC_CEC_MESSAGE_T
      VC_CEC_MESSAGE_T message;
      vc_cec_param2message(header, p0, p1, p2, p3, &message);

      // translate to a cec_command
      cec_command command;
      cec_command::Format(command,
          (cec_logical_address)message.initiator,
          (cec_logical_address)message.follower,
          (cec_opcode)CEC_CB_OPCODE(p0));

      // copy parameters
      for (uint8_t iPtr = 1; iPtr < message.length; iPtr++)
        command.PushBack(message.payload[iPtr]);

      // send to libCEC
      m_callback->OnCommandReceived(command);
    }
    break;
  case VC_CEC_TX:
    {
      // handle response to a command that was sent earlier
      m_queue->MessageReceived((cec_opcode)CEC_CB_OPCODE(p0), (cec_logical_address)CEC_CB_INITIATOR(p0), (cec_logical_address)CEC_CB_FOLLOWER(p0), CEC_CB_RC(header));
    }
    break;
  case VC_CEC_BUTTON_PRESSED:
  case VC_CEC_REMOTE_PRESSED:
    {
      // translate into a cec_command
      cec_command command;
      cec_command::Format(command,
                          (cec_logical_address)CEC_CB_INITIATOR(p0),
                          (cec_logical_address)CEC_CB_FOLLOWER(p0),
                          reason == VC_CEC_BUTTON_PRESSED ? CEC_OPCODE_USER_CONTROL_PRESSED : CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN);
      command.parameters.PushBack((uint8_t)CEC_CB_OPERAND1(p0));

      // send to libCEC
      m_callback->OnCommandReceived(command);
    }
    break;
  case VC_CEC_BUTTON_RELEASE:
  case VC_CEC_REMOTE_RELEASE:
    {
      // translate into a cec_command
      cec_command command;
      cec_command::Format(command,
                          (cec_logical_address)CEC_CB_INITIATOR(p0),
                          (cec_logical_address)CEC_CB_FOLLOWER(p0),
                          reason == VC_CEC_BUTTON_PRESSED ? CEC_OPCODE_USER_CONTROL_RELEASE : CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP);
      command.parameters.PushBack((uint8_t)CEC_CB_OPERAND1(p0));

      // send to libCEC
      m_callback->OnCommandReceived(command);
    }
    break;
  case VC_CEC_LOGICAL_ADDR:
    {
      CLockObject lock(m_mutex);
      m_previousLogicalAddress = m_logicalAddress;
      if (CEC_CB_RC(header) == VCHIQ_SUCCESS)
      {
        m_bLogicalAddressChanged = true;
        m_logicalAddress = (cec_logical_address)(p0 & 0xF);
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "logical address changed to %s (%x)", LIB_CEC->ToString(m_logicalAddress), m_logicalAddress);
      }
      else
      {
        m_logicalAddress = CECDEVICE_FREEUSE;
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "failed to change the logical address, reset to %s (%x)", LIB_CEC->ToString(m_logicalAddress), m_logicalAddress);
      }
      m_logicalAddressCondition.Signal();
    }
    break;
  case VC_CEC_LOGICAL_ADDR_LOST:
    {
      // the logical address was taken by another device
      cec_logical_address previousAddress = m_logicalAddress == CECDEVICE_BROADCAST ? m_previousLogicalAddress : m_logicalAddress;
      m_logicalAddress = CECDEVICE_UNKNOWN;

      // notify libCEC that we lost our LA when the connection was initialised
      bool bNotify(false);
      {
        CLockObject lock(m_mutex);
        bNotify = m_bInitialised && m_bLogicalAddressRegistered;
      }
      if (bNotify)
        m_callback->HandleLogicalAddressLost(previousAddress);
    }
    break;
  case VC_CEC_TOPOLOGY:
    break;
  default:
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "ignoring unknown reason %x", reason);
    break;
  }
}

int CRPiCECAdapterCommunication::InitHostCEC(void)
{
  VCHIQ_INSTANCE_T vchiq_instance;
  int iResult;

  if ((iResult = vchiq_initialise(&vchiq_instance)) != VCHIQ_SUCCESS)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - vchiq_initialise failed (%d)", __FUNCTION__, iResult);
    CStdString strError;
    strError.Format("%s - vchiq_initialise failed (%d)", __FUNCTION__, iResult);
    m_strError = strError;
    return iResult;
  }
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - vchiq_initialise succeeded", __FUNCTION__);

  if ((iResult = vchi_initialise(&m_vchi_instance)) != VCHIQ_SUCCESS)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - vchi_initialise failed (%d)", __FUNCTION__, iResult);
    CStdString strError;
    strError.Format("%s - vchi_initialise failed (%d)", __FUNCTION__, iResult);
    m_strError = strError;
    return iResult;
  }
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - vchi_initialise succeeded", __FUNCTION__);

  vchiq_instance = (VCHIQ_INSTANCE_T)m_vchi_instance;

  m_vchi_connection = vchi_create_connection(single_get_func_table(),
      vchi_mphi_message_driver_func_table());

  if ((iResult = vchi_connect(&m_vchi_connection, 1, m_vchi_instance)) != VCHIQ_SUCCESS)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - vchi_connect failed (%d)", __FUNCTION__, iResult);
    CStdString strError;
    strError.Format("%s - vchi_connect failed (%d)", __FUNCTION__, iResult);
    m_strError = strError;
    return iResult;
  }
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - vchi_connect succeeded", __FUNCTION__);

  return VCHIQ_SUCCESS;
}

bool CRPiCECAdapterCommunication::Open(uint32_t iTimeoutMs /* = CEC_DEFAULT_CONNECT_TIMEOUT */, bool UNUSED(bSkipChecks) /* = false */, bool bStartListening)
{
  Close();

  if (InitHostCEC() != VCHIQ_SUCCESS)
    return false;

  if (bStartListening)
  {
    // enable passive mode
    vc_cec_set_passive(true);

    // register the callbacks
    vc_cec_register_callback(rpi_cec_callback, (void*)this);
    vc_tv_register_callback(rpi_tv_callback, (void*)this);

    // release previous LA
    vc_cec_release_logical_address();
    if (!m_logicalAddressCondition.Wait(m_mutex, m_bLogicalAddressChanged, iTimeoutMs))
    {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "failed to release the previous LA");
      return false;
    }

    // register LA "freeuse"
    if (RegisterLogicalAddress(CECDEVICE_FREEUSE))
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - vc_cec initialised", __FUNCTION__);
      CLockObject lock(m_mutex);
      m_bInitialised = true;
    }
    else
      LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - vc_cec could not be initialised", __FUNCTION__);
  }

  return true;
}

uint16_t CRPiCECAdapterCommunication::GetPhysicalAddress(void)
{
  uint16_t iPA(CEC_INVALID_PHYSICAL_ADDRESS);
  if (!IsInitialised())
    return iPA;

  if (vc_cec_get_physical_address(&iPA) == VCHIQ_SUCCESS)
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - physical address = %04x", __FUNCTION__, iPA);
  else
  {
    LIB_CEC->AddLog(CEC_LOG_WARNING, "%s - failed to get the physical address", __FUNCTION__);
    iPA = CEC_INVALID_PHYSICAL_ADDRESS;
  }

  return iPA;
}

void CRPiCECAdapterCommunication::Close(void)
{
  {
    CLockObject lock(m_mutex);
    if (m_bInitialised)
      m_bInitialised = false;
    else
      return;
  }
  vc_tv_unregister_callback(rpi_tv_callback);

  UnregisterLogicalAddress();

  // disable passive mode
  vc_cec_set_passive(false);

  if (!g_bHostInited)
  {
    g_bHostInited = false;
    bcm_host_deinit();
  }
}

std::string CRPiCECAdapterCommunication::GetError(void) const
{
  std::string strError(m_strError);
  return strError;
}

cec_adapter_message_state CRPiCECAdapterCommunication::Write(const cec_command &data, bool &UNUSED(bRetry), uint8_t UNUSED(iLineTimeout), bool bIsReply)
{
  // ensure that the source LA is registered
  if (!RegisterLogicalAddress(data.initiator))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "failed to register logical address %s (%X)", CCECTypeUtils::ToString(data.initiator), data.initiator);
    return (data.initiator == data.destination) ? ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED : ADAPTER_MESSAGE_STATE_ERROR;
  }

  if (!data.opcode_set && data.initiator == data.destination)
  {
    // registration of the logical address would have failed
    return ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
  }

  return m_queue->Write(data, bIsReply) ? ADAPTER_MESSAGE_STATE_SENT_ACKED : ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
}

uint16_t CRPiCECAdapterCommunication::GetFirmwareVersion(void)
{
  return VC_CECSERVICE_VER;
}

cec_logical_address CRPiCECAdapterCommunication::GetLogicalAddress(void)
{
  {
    CLockObject lock(m_mutex);
    if (m_logicalAddress != CECDEVICE_UNKNOWN)
      return m_logicalAddress;
  }

  CEC_AllDevices_T address;
  return (vc_cec_get_logical_address(&address) == VCHIQ_SUCCESS) ?
      (cec_logical_address)address : CECDEVICE_UNKNOWN;
}

bool CRPiCECAdapterCommunication::UnregisterLogicalAddress(void)
{
  CLockObject lock(m_mutex);
  if (m_logicalAddress == CECDEVICE_UNKNOWN ||
      m_logicalAddress == CECDEVICE_BROADCAST)
    return true;

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - releasing previous logical address", __FUNCTION__);
  {
    CLockObject lock(m_mutex);
    m_bLogicalAddressRegistered = false;
    m_bLogicalAddressChanged    = false;
  }

  vc_cec_release_logical_address();

  return m_logicalAddressCondition.Wait(m_mutex, m_bLogicalAddressChanged);
}

bool CRPiCECAdapterCommunication::RegisterLogicalAddress(const cec_logical_address address)
{
  {
    CLockObject lock(m_mutex);
    if (m_logicalAddress == address)
      return true;
  }

  if (!UnregisterLogicalAddress())
    return false;

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - registering address %x", __FUNCTION__, address);

  CLockObject lock(m_mutex);
  m_bLogicalAddressChanged = false;
  vc_cec_poll_address((CEC_AllDevices_T)address);

  // register the new LA
  int iRetval = vc_cec_set_logical_address((CEC_AllDevices_T)address, (CEC_DEVICE_TYPE_T)CCECTypeUtils::GetType(address), CEC_VENDOR_ID_BROADCOM);
  if (iRetval != VCHIQ_SUCCESS)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - vc_cec_set_logical_address(%X) returned %s (%d)", __FUNCTION__, address, ToString((VC_CEC_ERROR_T)iRetval), iRetval);
    return false;
  }

  if (m_logicalAddressCondition.Wait(m_mutex, m_bLogicalAddressChanged))
  {
    m_bLogicalAddressRegistered = true;
    return true;
  }
  return false;
}

cec_logical_addresses CRPiCECAdapterCommunication::GetLogicalAddresses(void)
{
  cec_logical_addresses addresses; addresses.Clear();
  cec_logical_address current = GetLogicalAddress();
  if (current != CECDEVICE_UNKNOWN)
    addresses.Set(current);

  return addresses;
}

bool CRPiCECAdapterCommunication::SetLogicalAddresses(const cec_logical_addresses &addresses)
{
  // the current generation RPi only supports 1 LA, so just ensure that the primary address is registered
  return SupportsSourceLogicalAddress(addresses.primary) &&
      RegisterLogicalAddress(addresses.primary);
}

void CRPiCECAdapterCommunication::InitHost(void)
{
  if (!g_bHostInited)
  {
    g_bHostInited = true;
    bcm_host_init();
  }
}

#endif

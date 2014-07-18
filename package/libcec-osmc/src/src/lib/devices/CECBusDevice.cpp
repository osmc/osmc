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
#include "CECBusDevice.h"

#include "lib/CECProcessor.h"
#include "lib/CECClient.h"
#include "lib/implementations/ANCommandHandler.h"
#include "lib/implementations/CECCommandHandler.h"
#include "lib/implementations/SLCommandHandler.h"
#include "lib/implementations/VLCommandHandler.h"
#include "lib/implementations/PHCommandHandler.h"
#include "lib/implementations/RLCommandHandler.h"
#include "lib/implementations/RHCommandHandler.h"
#include "lib/implementations/AQCommandHandler.h"
#include "lib/LibCEC.h"
#include "lib/CECTypeUtils.h"
#include "lib/platform/util/timeutils.h"
#include "lib/platform/util/util.h"

#include "CECAudioSystem.h"
#include "CECPlaybackDevice.h"
#include "CECRecordingDevice.h"
#include "CECTuner.h"
#include "CECTV.h"

using namespace std;
using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_processor->GetLib()
#define ToString(p) CCECTypeUtils::ToString(p)

CResponse::CResponse(cec_opcode opcode) :
    m_opcode(opcode)
{
}

CResponse::~CResponse(void)
{
  Broadcast();
}

bool CResponse::Wait(uint32_t iTimeout)
{
  return m_event.Wait(iTimeout);
}

void CResponse::Broadcast(void)
{
  m_event.Broadcast();
}

CWaitForResponse::CWaitForResponse(void)
{
}

CWaitForResponse::~CWaitForResponse(void)
{
  Clear();
}

void CWaitForResponse::Clear()
{
  PLATFORM::CLockObject lock(m_mutex);
  for (std::map<cec_opcode, CResponse*>::iterator it = m_waitingFor.begin(); it != m_waitingFor.end(); it++)
  {
    it->second->Broadcast();
    delete it->second;
  }
  m_waitingFor.clear();
}

bool CWaitForResponse::Wait(cec_opcode opcode, uint32_t iTimeout)
{
  CResponse *response = GetEvent(opcode);
  return response ? response->Wait(iTimeout) : false;
}

void CWaitForResponse::Received(cec_opcode opcode)
{
  CResponse *response = GetEvent(opcode);
  if (response)
    response->Broadcast();
}

CResponse* CWaitForResponse::GetEvent(cec_opcode opcode)
{
  CResponse *retVal(NULL);
  {
    PLATFORM::CLockObject lock(m_mutex);
    std::map<cec_opcode, CResponse*>::iterator it = m_waitingFor.find(opcode);
    if (it != m_waitingFor.end())
    {
      retVal = it->second;
    }
    else
    {
      retVal = new CResponse(opcode);
      m_waitingFor[opcode] = retVal;
    }
    return retVal;
  }
}

CCECBusDevice::CCECBusDevice(CCECProcessor *processor, cec_logical_address iLogicalAddress, uint16_t iPhysicalAddress /* = CEC_INVALID_PHYSICAL_ADDRESS */) :
  m_type                  (CEC_DEVICE_TYPE_RESERVED),
  m_iPhysicalAddress      (iPhysicalAddress),
  m_iStreamPath           (CEC_INVALID_PHYSICAL_ADDRESS),
  m_iLogicalAddress       (iLogicalAddress),
  m_powerStatus           (CEC_POWER_STATUS_UNKNOWN),
  m_processor             (processor),
  m_vendor                (CEC_VENDOR_UNKNOWN),
  m_bReplaceHandler       (false),
  m_menuState             (CEC_MENU_STATE_ACTIVATED),
  m_bActiveSource         (false),
  m_iLastActive           (0),
  m_iLastPowerStateUpdate (0),
  m_cecVersion            (CEC_VERSION_UNKNOWN),
  m_deviceStatus          (CEC_DEVICE_STATUS_UNKNOWN),
  m_iHandlerUseCount      (0),
  m_bAwaitingReceiveFailed(false),
  m_bVendorIdRequested    (false),
  m_waitForResponse       (new CWaitForResponse),
  m_bImageViewOnSent      (false)
{
  m_handler = new CCECCommandHandler(this);

  for (unsigned int iPtr = 0; iPtr < 4; iPtr++)
    m_menuLanguage.language[iPtr] = '?';
  m_menuLanguage.language[3] = 0;
  m_menuLanguage.device = iLogicalAddress;

  m_strDeviceName = ToString(m_iLogicalAddress);
}

CCECBusDevice::~CCECBusDevice(void)
{
  DELETE_AND_NULL(m_handler);
  DELETE_AND_NULL(m_waitForResponse);
}

bool CCECBusDevice::ReplaceHandler(bool bActivateSource /* = true */)
{
  if (m_iLogicalAddress == CECDEVICE_BROADCAST)
    return false;

  bool bInitHandler(false);
  {
    CLockObject lock(m_mutex);
    CLockObject handlerLock(m_handlerMutex);
    if (m_iHandlerUseCount > 0)
      return false;

    MarkBusy();

    if (m_vendor != m_handler->GetVendorId())
    {
      if (CCECCommandHandler::HasSpecificHandler(m_vendor))
      {
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "replacing the command handler for device '%s' (%x)", GetLogicalAddressName(), GetLogicalAddress());

        int32_t iTransmitTimeout     = m_handler->m_iTransmitTimeout;
        int32_t iTransmitWait        = m_handler->m_iTransmitWait;
        int8_t  iTransmitRetries     = m_handler->m_iTransmitRetries;
        int64_t iActiveSourcePending = m_handler->m_iActiveSourcePending;

        DELETE_AND_NULL(m_handler);

        switch (m_vendor)
        {
        case CEC_VENDOR_SAMSUNG:
          m_handler = new CANCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        case CEC_VENDOR_LG:
          m_handler = new CSLCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        case CEC_VENDOR_PANASONIC:
          m_handler = new CVLCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        case CEC_VENDOR_PHILIPS:
          m_handler = new CPHCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        case CEC_VENDOR_TOSHIBA:
        case CEC_VENDOR_TOSHIBA2:
          m_handler = new CRLCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        case CEC_VENDOR_ONKYO:
          m_handler = new CRHCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        case CEC_VENDOR_SHARP:
          m_handler = new CAQCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        default:
          m_handler = new CCECCommandHandler(this, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending);
          break;
        }

        m_handler->SetVendorId(m_vendor);
        bInitHandler = true;
      }
    }
  }

  if (bInitHandler)
  {
    CCECBusDevice *primary = GetProcessor()->GetPrimaryDevice();
    if (primary->GetLogicalAddress() != CECDEVICE_UNREGISTERED)
    {
      m_handler->InitHandler();

      if (bActivateSource && IsHandledByLibCEC() && IsActiveSource())
        m_handler->ActivateSource();
    }
  }

  MarkReady();

  return true;
}

CCECCommandHandler *CCECBusDevice::GetHandler(void)
{
  ReplaceHandler(false);
  MarkBusy();
  return m_handler;
}

bool CCECBusDevice::HandleCommand(const cec_command &command)
{
  bool bHandled(false);

  /* update "last active" */
  {
    CLockObject lock(m_mutex);
    m_iLastActive = GetTimeMs();
    MarkBusy();
  }

  /* handle the command */
  bHandled = m_handler->HandleCommand(command);

  /* change status to present */
  if (bHandled && GetLogicalAddress() != CECDEVICE_BROADCAST && command.opcode_set == 1)
  {
    CLockObject lock(m_mutex);
    if (m_deviceStatus != CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC)
    {
      if (m_deviceStatus != CEC_DEVICE_STATUS_PRESENT)
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "device %s (%x) status changed to present after command %s", GetLogicalAddressName(), (uint8_t)GetLogicalAddress(), ToString(command.opcode));
      m_deviceStatus = CEC_DEVICE_STATUS_PRESENT;
    }
  }

  MarkReady();
  return bHandled;
}

const char* CCECBusDevice::GetLogicalAddressName(void) const
{
  return ToString(m_iLogicalAddress);
}

bool CCECBusDevice::IsPresent(void)
{
  CLockObject lock(m_mutex);
  return m_deviceStatus == CEC_DEVICE_STATUS_PRESENT;
}

bool CCECBusDevice::IsHandledByLibCEC(void)
{
  CLockObject lock(m_mutex);
  return m_deviceStatus == CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC;
}

void CCECBusDevice::SetUnsupportedFeature(cec_opcode opcode)
{
  // some commands should never be marked as unsupported
  if (opcode == CEC_OPCODE_VENDOR_COMMAND ||
      opcode == CEC_OPCODE_VENDOR_COMMAND_WITH_ID ||
      opcode == CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN ||
      opcode == CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP ||
      opcode == CEC_OPCODE_ABORT ||
      opcode == CEC_OPCODE_FEATURE_ABORT ||
      opcode == CEC_OPCODE_NONE ||
      opcode == CEC_OPCODE_USER_CONTROL_PRESSED ||
      opcode == CEC_OPCODE_USER_CONTROL_RELEASE)
    return;

  {
    CLockObject lock(m_mutex);
    if (m_unsupportedFeatures.find(opcode) == m_unsupportedFeatures.end())
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "marking opcode '%s' as unsupported feature for device '%s'", ToString(opcode), GetLogicalAddressName());
      m_unsupportedFeatures.insert(opcode);
    }
  }

  // signal threads that are waiting for a reponse
  MarkBusy();
  SignalOpcode(cec_command::GetResponseOpcode(opcode));
  MarkReady();
}

bool CCECBusDevice::IsUnsupportedFeature(cec_opcode opcode)
{
  CLockObject lock(m_mutex);
  bool bUnsupported = (m_unsupportedFeatures.find(opcode) != m_unsupportedFeatures.end());
  if (bUnsupported)
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "'%s' is marked as unsupported feature for device '%s'", ToString(opcode), GetLogicalAddressName());
  return bUnsupported;
}

bool CCECBusDevice::TransmitKeypress(const cec_logical_address initiator, cec_user_control_code key, bool bWait /* = true */)
{
  MarkBusy();
  bool bReturn = m_handler->TransmitKeypress(initiator, m_iLogicalAddress, key, bWait);
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::TransmitKeyRelease(const cec_logical_address initiator, bool bWait /* = true */)
{
  MarkBusy();
  bool bReturn = m_handler->TransmitKeyRelease(initiator, m_iLogicalAddress, bWait);
  MarkReady();
  return bReturn;
}

cec_version CCECBusDevice::GetCecVersion(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
  bool bRequestUpdate(false);
  {
    CLockObject lock(m_mutex);
    bRequestUpdate = bIsPresent &&
        (bUpdate || m_cecVersion == CEC_VERSION_UNKNOWN);
  }

  if (bRequestUpdate)
  {
    CheckVendorIdRequested(initiator);
    RequestCecVersion(initiator);
  }

  CLockObject lock(m_mutex);
  return m_cecVersion;
}

void CCECBusDevice::SetCecVersion(const cec_version newVersion)
{
  CLockObject lock(m_mutex);
  if (m_cecVersion != newVersion)
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): CEC version %s", GetLogicalAddressName(), m_iLogicalAddress, ToString(newVersion));
  m_cecVersion = newVersion;
}

bool CCECBusDevice::RequestCecVersion(const cec_logical_address initiator, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC() &&
      !IsUnsupportedFeature(CEC_OPCODE_GET_CEC_VERSION))
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting CEC version of '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->TransmitRequestCecVersion(initiator, m_iLogicalAddress, bWaitForResponse);
    MarkReady();
  }
  return bReturn;
}

bool CCECBusDevice::TransmitCECVersion(const cec_logical_address destination, bool bIsReply)
{
  cec_version version;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): cec version %s", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination, ToString(m_cecVersion));
    version = m_cecVersion;
  }

  MarkBusy();
  bool bReturn = m_handler->TransmitCECVersion(m_iLogicalAddress, destination, version, bIsReply);
  MarkReady();
  return bReturn;
}

cec_menu_language &CCECBusDevice::GetMenuLanguage(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
  bool bRequestUpdate(false);
  {
    CLockObject lock(m_mutex);
    bRequestUpdate = (bIsPresent &&
        (bUpdate || !strcmp(m_menuLanguage.language, "???")));
  }

  if (bRequestUpdate)
  {
    CheckVendorIdRequested(initiator);
    RequestMenuLanguage(initiator);
  }

  CLockObject lock(m_mutex);
  return m_menuLanguage;
}

void CCECBusDevice::SetMenuLanguage(const char *strLanguage)
{
  if (!strLanguage)
    return;

  CLockObject lock(m_mutex);
  if (strcmp(strLanguage, m_menuLanguage.language))
  {
    memcpy(m_menuLanguage.language, strLanguage, 3);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): menu language set to '%s'", GetLogicalAddressName(), m_iLogicalAddress, m_menuLanguage.language);
  }
}

void CCECBusDevice::SetMenuLanguage(const cec_menu_language &language)
{
  if (language.device == m_iLogicalAddress)
    SetMenuLanguage(language.language);
}

bool CCECBusDevice::RequestMenuLanguage(const cec_logical_address initiator, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC() &&
      !IsUnsupportedFeature(CEC_OPCODE_GET_MENU_LANGUAGE))
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting menu language of '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->TransmitRequestMenuLanguage(initiator, m_iLogicalAddress, bWaitForResponse);
    MarkReady();
  }
  return bReturn;
}

bool CCECBusDevice::TransmitSetMenuLanguage(const cec_logical_address destination, bool bIsReply)
{
  bool bReturn(false);
  cec_menu_language language;
  {
    CLockObject lock(m_mutex);
    language = m_menuLanguage;
  }

  char lang[4];
  {
    CLockObject lock(m_mutex);
    lang[0] = language.language[0];
    lang[1] = language.language[1];
    lang[2] = language.language[2];
    lang[3] = (char)0;
  }

  MarkBusy();
  if (lang[0] == '?' && lang[1] == '?' && lang[2] == '?')
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): menu language feature abort", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination);
    m_processor->TransmitAbort(m_iLogicalAddress, destination, CEC_OPCODE_GIVE_DEVICE_VENDOR_ID);
    bReturn = true;
  }
  else
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> broadcast (F): menu language '%s'", GetLogicalAddressName(), m_iLogicalAddress, lang);
    bReturn = m_handler->TransmitSetMenuLanguage(m_iLogicalAddress, lang, bIsReply);
  }
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::TransmitOSDString(const cec_logical_address destination, cec_display_control duration, const char *strMessage, bool bIsReply)
{
  bool bReturn(false);
  if (!m_processor->GetDevice(destination)->IsUnsupportedFeature(CEC_OPCODE_SET_OSD_STRING))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): display OSD message '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination, strMessage);
    MarkBusy();
    bReturn = m_handler->TransmitOSDString(m_iLogicalAddress, destination, duration, strMessage, bIsReply);
    MarkReady();
  }
  return bReturn;
}

CStdString CCECBusDevice::GetCurrentOSDName(void)
{
  CLockObject lock(m_mutex);
  return m_strDeviceName;
}

CStdString CCECBusDevice::GetOSDName(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
  bool bRequestUpdate(false);
  {
    CLockObject lock(m_mutex);
    bRequestUpdate = bIsPresent &&
        (bUpdate || m_strDeviceName.Equals(ToString(m_iLogicalAddress))) &&
        m_type != CEC_DEVICE_TYPE_TV;
  }

  if (bRequestUpdate)
  {
    CheckVendorIdRequested(initiator);
    RequestOSDName(initiator);
  }

  CLockObject lock(m_mutex);
  return m_strDeviceName;
}

void CCECBusDevice::SetOSDName(CStdString strName)
{
  CLockObject lock(m_mutex);
  if (m_strDeviceName != strName)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): osd name set to '%s'", GetLogicalAddressName(), m_iLogicalAddress, strName.c_str());
    m_strDeviceName = strName;
  }
}

bool CCECBusDevice::RequestOSDName(const cec_logical_address initiator, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC() &&
      !IsUnsupportedFeature(CEC_OPCODE_GIVE_OSD_NAME))
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting OSD name of '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->TransmitRequestOSDName(initiator, m_iLogicalAddress, bWaitForResponse);
    MarkReady();
  }
  return bReturn;
}

bool CCECBusDevice::TransmitOSDName(const cec_logical_address destination, bool bIsReply)
{
  CStdString strDeviceName;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): OSD name '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination, m_strDeviceName.c_str());
    strDeviceName = m_strDeviceName;
  }

  MarkBusy();
  bool bReturn = m_handler->TransmitOSDName(m_iLogicalAddress, destination, strDeviceName, bIsReply);
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::HasValidPhysicalAddress(void)
{
  CLockObject lock(m_mutex);
  return CLibCEC::IsValidPhysicalAddress(m_iPhysicalAddress);
}

uint16_t CCECBusDevice::GetCurrentPhysicalAddress(void)
{
  CLockObject lock(m_mutex);
  return m_iPhysicalAddress;
}

uint16_t CCECBusDevice::GetPhysicalAddress(const cec_logical_address initiator, bool bSuppressUpdate /* = false */)
{
  if (!bSuppressUpdate)
  {
    bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
    bool bRequestUpdate(false);
    {
      CLockObject lock(m_mutex);
      bRequestUpdate = bIsPresent && m_iPhysicalAddress == CEC_INVALID_PHYSICAL_ADDRESS;
    }

    if (bRequestUpdate)
    {
      CheckVendorIdRequested(initiator);
      if (!RequestPhysicalAddress(initiator))
        LIB_CEC->AddLog(CEC_LOG_ERROR, "failed to request the physical address");
    }
  }

  CLockObject lock(m_mutex);
  return m_iPhysicalAddress;
}

bool CCECBusDevice::SetPhysicalAddress(uint16_t iNewAddress)
{
  CLockObject lock(m_mutex);
  if (iNewAddress > 0 && m_iPhysicalAddress != iNewAddress)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): physical address changed from %04x to %04x", GetLogicalAddressName(), m_iLogicalAddress, m_iPhysicalAddress, iNewAddress);
    m_iPhysicalAddress = iNewAddress;
  }
  return true;
}

bool CCECBusDevice::RequestPhysicalAddress(const cec_logical_address initiator, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC())
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting physical address of '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->TransmitRequestPhysicalAddress(initiator, m_iLogicalAddress, bWaitForResponse);
    MarkReady();
  }
  return bReturn;
}

bool CCECBusDevice::TransmitPhysicalAddress(bool bIsReply)
{
  uint16_t iPhysicalAddress;
  cec_device_type type;
  {
    CLockObject lock(m_mutex);
    if (m_iPhysicalAddress == CEC_INVALID_PHYSICAL_ADDRESS)
      return false;

    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> broadcast (F): physical adddress %4x", GetLogicalAddressName(), m_iLogicalAddress, m_iPhysicalAddress);
    iPhysicalAddress = m_iPhysicalAddress;
    type = m_type;
  }

  MarkBusy();
  bool bReturn = m_handler->TransmitPhysicalAddress(m_iLogicalAddress, iPhysicalAddress, type, bIsReply);
  MarkReady();
  return bReturn;
}

cec_power_status CCECBusDevice::GetCurrentPowerStatus(void)
{
  CLockObject lock(m_mutex);
  return m_powerStatus;
}

cec_power_status CCECBusDevice::GetPowerStatus(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
  bool bRequestUpdate(false);
  {
    CLockObject lock(m_mutex);
    bRequestUpdate = (bIsPresent &&
        (bUpdate || m_powerStatus == CEC_POWER_STATUS_UNKNOWN ||
            m_powerStatus == CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON ||
            m_powerStatus == CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY ||
            GetTimeMs() - m_iLastPowerStateUpdate >= CEC_POWER_STATE_REFRESH_TIME));
  }

  if (bRequestUpdate)
  {
    CheckVendorIdRequested(initiator);
    RequestPowerStatus(initiator, bUpdate);
  }

  CLockObject lock(m_mutex);
  return m_powerStatus;
}

void CCECBusDevice::SetPowerStatus(const cec_power_status powerStatus)
{
  CLockObject lock(m_mutex);
  if (m_powerStatus != powerStatus)
  {
    m_iLastPowerStateUpdate = GetTimeMs();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): power status changed from '%s' to '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_powerStatus), ToString(powerStatus));
    m_powerStatus = powerStatus;
  }
}

void CCECBusDevice::OnImageViewOnSent(bool bSentByLibCEC)
{
  CLockObject lock(m_mutex);
  m_bImageViewOnSent = bSentByLibCEC;

  if (m_powerStatus != CEC_POWER_STATUS_ON && m_powerStatus != CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON)
  {
    m_iLastPowerStateUpdate = GetTimeMs();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): power status changed from '%s' to '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_powerStatus), ToString(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON));
    m_powerStatus = CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON;
  }
}

bool CCECBusDevice::ImageViewOnSent(void)
{
  CLockObject lock(m_mutex);
  return m_bImageViewOnSent;
}

bool CCECBusDevice::RequestPowerStatus(const cec_logical_address initiator, bool bUpdate, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC() &&
      !IsUnsupportedFeature(CEC_OPCODE_GIVE_DEVICE_POWER_STATUS))
  {
    MarkBusy();
    bReturn = m_handler->TransmitRequestPowerStatus(initiator, m_iLogicalAddress, bUpdate, bWaitForResponse);
    if (!bReturn)
      SetPowerStatus(CEC_POWER_STATUS_UNKNOWN);
    MarkReady();
  }
  return bReturn;
}

bool CCECBusDevice::TransmitPowerState(const cec_logical_address destination, bool bIsReply)
{
  cec_power_status state;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): %s", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination, ToString(m_powerStatus));
    state = m_powerStatus;
  }

  MarkBusy();
  bool bReturn = m_handler->TransmitPowerState(m_iLogicalAddress, destination, state, bIsReply);
  MarkReady();
  return bReturn;
}

cec_vendor_id CCECBusDevice::GetCurrentVendorId(void)
{
  CLockObject lock(m_mutex);
  return m_vendor;
}

cec_vendor_id CCECBusDevice::GetVendorId(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
  bool bRequestUpdate(false);
  {
    CLockObject lock(m_mutex);
    bRequestUpdate = (bIsPresent &&
        (bUpdate || m_vendor == CEC_VENDOR_UNKNOWN));
  }

  if (bRequestUpdate)
    RequestVendorId(initiator);

  CLockObject lock(m_mutex);
  return m_vendor;
}

const char *CCECBusDevice::GetVendorName(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  return ToString(GetVendorId(initiator, bUpdate));
}

bool CCECBusDevice::SetVendorId(uint64_t iVendorId)
{
  bool bVendorChanged(false);

  {
    CLockObject lock(m_mutex);
    bVendorChanged = (m_vendor != (cec_vendor_id)iVendorId);
    m_vendor = (cec_vendor_id)iVendorId;
  }

  if (bVendorChanged)
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): vendor = %s (%06x)", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_vendor), m_vendor);

  return bVendorChanged;
}

bool CCECBusDevice::RequestVendorId(const cec_logical_address initiator, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC() && initiator != CECDEVICE_UNKNOWN)
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting vendor ID of '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->TransmitRequestVendorId(initiator, m_iLogicalAddress, bWaitForResponse);
    MarkReady();

    if (bWaitForResponse)
      ReplaceHandler(true);
  }
  return bReturn;
}

bool CCECBusDevice::TransmitVendorID(const cec_logical_address destination, bool bSendAbort, bool bIsReply)
{
  bool bReturn(false);
  uint64_t iVendorId;
  {
    CLockObject lock(m_mutex);
    iVendorId = (uint64_t)m_vendor;
  }

  MarkBusy();
  if (iVendorId == CEC_VENDOR_UNKNOWN)
  {
    if (bSendAbort)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): vendor id feature abort", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination);
      m_processor->TransmitAbort(m_iLogicalAddress, destination, CEC_OPCODE_GIVE_DEVICE_VENDOR_ID);
      bReturn = true;
    }
  }
  else
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): vendor id %s (%x)", GetLogicalAddressName(), m_iLogicalAddress, ToString(destination), destination, ToString((cec_vendor_id)iVendorId), iVendorId);
    bReturn = m_handler->TransmitVendorID(m_iLogicalAddress, destination, iVendorId, bIsReply);
  }
  MarkReady();
  return bReturn;
}

cec_bus_device_status CCECBusDevice::GetStatus(bool bForcePoll /* = false */, bool bSuppressPoll /* = false */)
{
  if (m_iLogicalAddress == CECDEVICE_BROADCAST)
    return CEC_DEVICE_STATUS_NOT_PRESENT;

  cec_bus_device_status status(CEC_DEVICE_STATUS_UNKNOWN);
  bool bNeedsPoll(false);

  {
    CLockObject lock(m_mutex);
    status = m_deviceStatus;
    bNeedsPoll = !bSuppressPoll &&
        m_deviceStatus != CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC &&
            // poll forced
            (bForcePoll ||
            // don't know the status
            m_deviceStatus == CEC_DEVICE_STATUS_UNKNOWN ||
            // always poll the TV if it's marked as not present
            (m_deviceStatus == CEC_DEVICE_STATUS_NOT_PRESENT && m_iLogicalAddress == CECDEVICE_TV));
  }

  if (bNeedsPoll)
  {
    bool bPollAcked(false);
    if (bNeedsPoll)
      bPollAcked = m_processor->PollDevice(m_iLogicalAddress);

    status = bPollAcked ? CEC_DEVICE_STATUS_PRESENT : CEC_DEVICE_STATUS_NOT_PRESENT;
    SetDeviceStatus(status);
  }

  return status;
}

void CCECBusDevice::SetDeviceStatus(const cec_bus_device_status newStatus, cec_version libCECSpecVersion /* = CEC_VERSION_1_4 */)
{
  if (m_iLogicalAddress == CECDEVICE_UNREGISTERED)
    return;

  {
    CLockObject lock(m_mutex);
    switch (newStatus)
    {
    case CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC:
      if (m_deviceStatus != newStatus)
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): device status changed into 'handled by libCEC'", GetLogicalAddressName(), m_iLogicalAddress);
      SetPowerStatus   (CEC_POWER_STATUS_ON);
      SetVendorId      (CEC_VENDOR_PULSE_EIGHT);
      SetMenuState     (CEC_MENU_STATE_ACTIVATED);
      SetCecVersion    (libCECSpecVersion);
      SetStreamPath    (CEC_INVALID_PHYSICAL_ADDRESS);
      MarkAsInactiveSource();
      m_iLastActive   = 0;
      m_deviceStatus  = newStatus;
      break;
    case CEC_DEVICE_STATUS_PRESENT:
      if (m_deviceStatus != newStatus)
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): device status changed into 'present'", GetLogicalAddressName(), m_iLogicalAddress);
      m_deviceStatus = newStatus;
      m_iLastActive = GetTimeMs();
      break;
    case CEC_DEVICE_STATUS_NOT_PRESENT:
      if (m_deviceStatus != newStatus)
      {
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): device status changed into 'not present'", GetLogicalAddressName(), m_iLogicalAddress);
        ResetDeviceStatus(true);
        m_deviceStatus = newStatus;
      }
      break;
    default:
      ResetDeviceStatus();
      break;
    }
  }
}

void CCECBusDevice::ResetDeviceStatus(bool bClientUnregistered /* = false */)
{
  CLockObject lock(m_mutex);
  SetPowerStatus   (CEC_POWER_STATUS_UNKNOWN);
  SetVendorId      (CEC_VENDOR_UNKNOWN);
  SetMenuState     (CEC_MENU_STATE_ACTIVATED);
  SetCecVersion    (CEC_VERSION_UNKNOWN);
  SetStreamPath    (CEC_INVALID_PHYSICAL_ADDRESS);
  SetOSDName       (ToString(m_iLogicalAddress));
  MarkAsInactiveSource(bClientUnregistered);

  m_iLastActive = 0;
  m_bVendorIdRequested = false;
  m_unsupportedFeatures.clear();
  m_waitForResponse->Clear();

  if (m_deviceStatus != CEC_DEVICE_STATUS_UNKNOWN)
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): device status changed into 'unknown'", GetLogicalAddressName(), m_iLogicalAddress);
  m_deviceStatus = CEC_DEVICE_STATUS_UNKNOWN;
}

bool CCECBusDevice::TransmitPoll(const cec_logical_address dest, bool bUpdateDeviceStatus)
{
  bool bReturn(false);
  cec_logical_address destination(dest);
  if (destination == CECDEVICE_UNKNOWN)
    destination = m_iLogicalAddress;

  CCECBusDevice *destDevice = m_processor->GetDevice(destination);
  if (destDevice->m_deviceStatus == CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC)
    return bReturn;

  MarkBusy();
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): POLL", GetLogicalAddressName(), m_iLogicalAddress, ToString(dest), dest);
  bReturn = m_handler->TransmitPoll(m_iLogicalAddress, destination, false);
  LIB_CEC->AddLog(CEC_LOG_DEBUG, bReturn ? ">> POLL sent" : ">> POLL not sent");

  if (bUpdateDeviceStatus)
    destDevice->SetDeviceStatus(bReturn ? CEC_DEVICE_STATUS_PRESENT : CEC_DEVICE_STATUS_NOT_PRESENT);

  MarkReady();
  return bReturn;
}

void CCECBusDevice::HandlePoll(const cec_logical_address destination)
{
  if (destination >= 0 && destination < CECDEVICE_BROADCAST)
  {
    CCECBusDevice *device = m_processor->GetDevice(destination);
    if (device)
      device->HandlePollFrom(m_iLogicalAddress);
  }
}

void CCECBusDevice::HandlePollFrom(const cec_logical_address initiator)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< POLL: %s (%x) -> %s (%x)", ToString(initiator), initiator, ToString(m_iLogicalAddress), m_iLogicalAddress);
  m_bAwaitingReceiveFailed = true;
}

bool CCECBusDevice::HandleReceiveFailed(void)
{
  bool bReturn = m_bAwaitingReceiveFailed;
  m_bAwaitingReceiveFailed = false;
  return bReturn;
}

cec_menu_state CCECBusDevice::GetMenuState(const cec_logical_address UNUSED(initiator))
{
  CLockObject lock(m_mutex);
  return m_menuState;
}

void CCECBusDevice::SetMenuState(const cec_menu_state state)
{
  CLockObject lock(m_mutex);
  if (m_menuState != state)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): menu state set to '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_menuState));
    m_menuState = state;
  }
}

bool CCECBusDevice::TransmitMenuState(const cec_logical_address dest, bool bIsReply)
{
  cec_menu_state menuState;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): menu state '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(dest), dest, ToString(m_menuState));
    menuState = m_menuState;
  }

  MarkBusy();
  bool bReturn = m_handler->TransmitMenuState(m_iLogicalAddress, dest, menuState, bIsReply);
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::ActivateSource(uint64_t iDelay /* = 0 */)
{
  MarkAsActiveSource();
  MarkBusy();
  bool bReturn(true);
  if (iDelay == 0)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "sending active source message for '%s'", ToString(m_iLogicalAddress));
    bReturn = m_handler->ActivateSource();
  }
  else
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "scheduling active source message for '%s'", ToString(m_iLogicalAddress));
    m_handler->ScheduleActivateSource(iDelay);
  }
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::RequestActiveSource(bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (IsHandledByLibCEC())
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting active source");

    bReturn = m_handler->TransmitRequestActiveSource(m_iLogicalAddress, bWaitForResponse);
    MarkReady();
  }
  return bReturn;
}

void CCECBusDevice::MarkAsActiveSource(void)
{
  bool bWasActivated(false);

  // set the power status to powered on
  SetPowerStatus(CEC_POWER_STATUS_ON);

  // mark this device as active source
  {
    CLockObject lock(m_mutex);
    if (!m_bActiveSource)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "making %s (%x) the active source", GetLogicalAddressName(), m_iLogicalAddress);
      bWasActivated = true;
    }
    else
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%x) was already marked as active source", GetLogicalAddressName(), m_iLogicalAddress);

    m_bActiveSource = true;
  }

  CCECBusDevice* tv = m_processor->GetDevice(CECDEVICE_TV);
  if (tv)
    tv->OnImageViewOnSent(false);

  // mark other devices as inactive sources
  CECDEVICEVEC devices;
  m_processor->GetDevices()->Get(devices);
  for (CECDEVICEVEC::iterator it = devices.begin(); it != devices.end(); it++)
    if ((*it)->GetLogicalAddress() != m_iLogicalAddress)
      (*it)->MarkAsInactiveSource();

  if (bWasActivated && IsHandledByLibCEC())
    m_processor->SetActiveSource(true, false);

  CCECClient *client = GetClient();
  if (client)
    client->SourceActivated(m_iLogicalAddress);
}

void CCECBusDevice::MarkAsInactiveSource(bool bClientUnregistered /* = false */)
{
  bool bWasDeactivated(false);
  {
    CLockObject lock(m_mutex);
    if (m_bActiveSource)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "marking %s (%X) as inactive source", GetLogicalAddressName(), m_iLogicalAddress);
      bWasDeactivated = true;
    }
    m_bActiveSource = false;
  }

  if (bWasDeactivated)
  {
    if (IsHandledByLibCEC())
      m_processor->SetActiveSource(false, bClientUnregistered);
    CCECClient *client = GetClient();
    if (client)
      client->SourceDeactivated(m_iLogicalAddress);
  }
}

bool CCECBusDevice::TransmitActiveSource(bool bIsReply)
{
  bool bSendActiveSource(false);
  uint16_t iPhysicalAddress(CEC_INVALID_PHYSICAL_ADDRESS);

  {
    CLockObject lock(m_mutex);
    if (!HasValidPhysicalAddress())
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X) has an invalid physical address (%04x), not sending active source commands", GetLogicalAddressName(), m_iLogicalAddress, m_iPhysicalAddress);
      return false;
    }

    iPhysicalAddress = m_iPhysicalAddress;

    if (m_powerStatus != CEC_POWER_STATUS_ON && m_powerStatus != CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON)
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) is not powered on", GetLogicalAddressName(), m_iLogicalAddress);
    else if (m_bActiveSource)
    {
      LIB_CEC->AddLog(CEC_LOG_NOTICE, "<< %s (%X) -> broadcast (F): active source (%4x)", GetLogicalAddressName(), m_iLogicalAddress, m_iPhysicalAddress);
      bSendActiveSource = true;
    }
    else
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) is not the active source", GetLogicalAddressName(), m_iLogicalAddress);
  }

  bool bActiveSourceSent(false);
  if (bSendActiveSource)
  {
    MarkBusy();
    bActiveSourceSent = m_handler->TransmitActiveSource(m_iLogicalAddress, iPhysicalAddress, bIsReply);
    MarkReady();
  }

  return bActiveSourceSent;
}

bool CCECBusDevice::TransmitImageViewOn(void)
{
  {
    CLockObject lock(m_mutex);
    if (m_powerStatus != CEC_POWER_STATUS_ON && m_powerStatus != CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %s (%X) is not powered on", GetLogicalAddressName(), m_iLogicalAddress);
      return false;
    }
  }

  CCECBusDevice* tv = m_processor->GetDevice(CECDEVICE_TV);
  if (!tv)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s - couldn't get TV instance", __FUNCTION__);
    return false;
  }

  if (tv->ImageViewOnSent())
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s - 'image view on' already sent", __FUNCTION__);
    return true;
  }

  bool bImageViewOnSent(false);
  MarkBusy();
  bImageViewOnSent = m_handler->TransmitImageViewOn(m_iLogicalAddress, CECDEVICE_TV);
  MarkReady();

  if (bImageViewOnSent)
    tv->OnImageViewOnSent(true);

  return bImageViewOnSent;
}

bool CCECBusDevice::TransmitInactiveSource(void)
{
  uint16_t iPhysicalAddress;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_NOTICE, "<< %s (%X) -> broadcast (F): inactive source", GetLogicalAddressName(), m_iLogicalAddress);
    iPhysicalAddress = m_iPhysicalAddress;
  }

  MarkBusy();
  bool bReturn = m_handler->TransmitInactiveSource(m_iLogicalAddress, iPhysicalAddress);
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::TransmitPendingActiveSourceCommands(void)
{
  MarkBusy();
  bool bReturn = m_handler->ActivateSource(true);
  MarkReady();
  return bReturn;
}

void CCECBusDevice::SetActiveRoute(uint16_t iRoute)
{
  SetPowerStatus(CEC_POWER_STATUS_ON);

  CCECDeviceMap* map = m_processor->GetDevices();
  if (!map)
    return;

  CCECBusDevice* newRoute = m_processor->GetDeviceByPhysicalAddress(iRoute, true);
  if (newRoute && newRoute->IsHandledByLibCEC())
  {
    // we were made the active source, send notification
    newRoute->ActivateSource();
  }
}

void CCECBusDevice::SetStreamPath(uint16_t iNewAddress, uint16_t iOldAddress /* = CEC_INVALID_PHYSICAL_ADDRESS */)
{
  if (iNewAddress != CEC_INVALID_PHYSICAL_ADDRESS)
    SetPowerStatus(CEC_POWER_STATUS_ON);

  CLockObject lock(m_mutex);
  if (iNewAddress != m_iStreamPath)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "%s (%X): stream path changed from %04x to %04x", GetLogicalAddressName(), m_iLogicalAddress, iOldAddress == 0 ? m_iStreamPath : iOldAddress, iNewAddress);
    m_iStreamPath = iNewAddress;
  }

  if (!LIB_CEC->IsValidPhysicalAddress(iNewAddress))
    return;

  CCECBusDevice *device = m_processor->GetDeviceByPhysicalAddress(iNewAddress);
  if (device)
  {
    // if a device is found with the new physical address, mark it as active, which will automatically mark all other devices as inactive
    device->MarkAsActiveSource();

    // respond with an active source message if this device is handled by libCEC
    if (device->IsHandledByLibCEC())
      device->TransmitActiveSource(true);
  }
  else
  {
    // try to find the device with the old address, and mark it as inactive when found
    device = m_processor->GetDeviceByPhysicalAddress(iOldAddress);
    if (device)
      device->MarkAsInactiveSource();
  }
}

bool CCECBusDevice::PowerOn(const cec_logical_address initiator)
{
  bool bReturn(false);
  GetVendorId(initiator); // ensure that we got the vendor id, because the implementations vary per vendor

  MarkBusy();
  cec_power_status currentStatus;
  if (m_iLogicalAddress == CECDEVICE_TV ||
      ((currentStatus = GetPowerStatus(initiator, false)) != CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON &&
        currentStatus != CEC_POWER_STATUS_ON))
  {
    LIB_CEC->AddLog(CEC_LOG_NOTICE, "<< powering on '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->PowerOn(initiator, m_iLogicalAddress);
  }
  else
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "'%s' (%X) is already '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(currentStatus));
  }

  MarkReady();
  return bReturn;
}

bool CCECBusDevice::Standby(const cec_logical_address initiator)
{
  GetVendorId(initiator); // ensure that we got the vendor id, because the implementations vary per vendor

  LIB_CEC->AddLog(CEC_LOG_NOTICE, "<< putting '%s' (%X) in standby mode", GetLogicalAddressName(), m_iLogicalAddress);
  MarkBusy();
  bool bReturn = m_handler->TransmitStandby(initiator, m_iLogicalAddress);
  MarkReady();
  return bReturn;
}

bool CCECBusDevice::NeedsPoll(void)
{
  bool bSendPoll(false);
  cec_logical_address pollAddress(CECDEVICE_UNKNOWN);
  switch (m_iLogicalAddress)
  {
  case CECDEVICE_PLAYBACKDEVICE3:
    pollAddress = CECDEVICE_PLAYBACKDEVICE2;
    break;
  case CECDEVICE_PLAYBACKDEVICE2:
    pollAddress = CECDEVICE_PLAYBACKDEVICE1;
    break;
  case CECDEVICE_RECORDINGDEVICE3:
    pollAddress = CECDEVICE_RECORDINGDEVICE2;
    break;
  case CECDEVICE_RECORDINGDEVICE2:
    pollAddress = CECDEVICE_RECORDINGDEVICE1;
    break;
  case CECDEVICE_TUNER4:
    pollAddress = CECDEVICE_TUNER3;
    break;
  case CECDEVICE_TUNER3:
    pollAddress = CECDEVICE_TUNER2;
    break;
  case CECDEVICE_TUNER2:
    pollAddress = CECDEVICE_TUNER1;
    break;
  case CECDEVICE_AUDIOSYSTEM:
  case CECDEVICE_PLAYBACKDEVICE1:
  case CECDEVICE_RECORDINGDEVICE1:
  case CECDEVICE_TUNER1:
  case CECDEVICE_TV:
    bSendPoll = true;
    break;
  default:
    break;
  }

  if (!bSendPoll && pollAddress != CECDEVICE_UNKNOWN)
  {
    CCECBusDevice *device = m_processor->GetDevice(pollAddress);
    if (device)
    {
      cec_bus_device_status status = device->GetStatus();
      bSendPoll = (status == CEC_DEVICE_STATUS_PRESENT || status == CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC);
    }
    else
    {
      bSendPoll = true;
    }
  }

  return bSendPoll;
}

void CCECBusDevice::CheckVendorIdRequested(const cec_logical_address initiator)
{
  bool bRequestVendorId(false);
  {
    CLockObject lock(m_mutex);
    bRequestVendorId = !m_bVendorIdRequested;
    m_bVendorIdRequested = true;
  }

  if (bRequestVendorId)
  {
    ReplaceHandler(false);
    GetVendorId(initiator);
  }
}
//@}

CCECAudioSystem *CCECBusDevice::AsAudioSystem(void)
{
  return AsAudioSystem(this);
}

CCECPlaybackDevice *CCECBusDevice::AsPlaybackDevice(void)
{
  return AsPlaybackDevice(this);
}

CCECRecordingDevice *CCECBusDevice::AsRecordingDevice(void)
{
  return AsRecordingDevice(this);
}

CCECTuner *CCECBusDevice::AsTuner(void)
{
  return AsTuner(this);
}

CCECTV *CCECBusDevice::AsTV(void)
{
  return AsTV(this);
}

CCECAudioSystem *CCECBusDevice::AsAudioSystem(CCECBusDevice *device)
{
  if (device && device->GetType() == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
    return static_cast<CCECAudioSystem *>(device);
  return NULL;
}

CCECPlaybackDevice *CCECBusDevice::AsPlaybackDevice(CCECBusDevice *device)
{
  if (device &&
      (device->GetType() == CEC_DEVICE_TYPE_PLAYBACK_DEVICE ||
       device->GetType() == CEC_DEVICE_TYPE_RECORDING_DEVICE))
    return static_cast<CCECPlaybackDevice *>(device);
  return NULL;
}

CCECRecordingDevice *CCECBusDevice::AsRecordingDevice(CCECBusDevice *device)
{
  if (device && device->GetType() == CEC_DEVICE_TYPE_RECORDING_DEVICE)
    return static_cast<CCECRecordingDevice *>(device);
  return NULL;
}

CCECTuner *CCECBusDevice::AsTuner(CCECBusDevice *device)
{
  if (device && device->GetType() == CEC_DEVICE_TYPE_TUNER)
    return static_cast<CCECTuner *>(device);
  return NULL;
}

CCECTV *CCECBusDevice::AsTV(CCECBusDevice *device)
{
  if (device && device->GetType() == CEC_DEVICE_TYPE_TV)
    return static_cast<CCECTV *>(device);
  return NULL;
}

void CCECBusDevice::MarkBusy(void)
{
  CLockObject handlerLock(m_handlerMutex);
  ++m_iHandlerUseCount;
}

void CCECBusDevice::MarkReady(void)
{
  CLockObject handlerLock(m_handlerMutex);
  if (m_iHandlerUseCount > 0)
    --m_iHandlerUseCount;
}

bool CCECBusDevice::TryLogicalAddress(cec_version libCECSpecVersion /* = CEC_VERSION_1_4 */)
{
  LIB_CEC->AddLog(CEC_LOG_DEBUG, "trying logical address '%s'", GetLogicalAddressName());

  if (!TransmitPoll(m_iLogicalAddress, false))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "using logical address '%s'", GetLogicalAddressName());
    SetDeviceStatus(CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC, libCECSpecVersion);

    return true;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "logical address '%s' already taken", GetLogicalAddressName());
  SetDeviceStatus(CEC_DEVICE_STATUS_PRESENT);
  return false;
}

CCECClient *CCECBusDevice::GetClient(void)
{
  return m_processor->GetClient(m_iLogicalAddress);
}

void CCECBusDevice::SignalOpcode(cec_opcode opcode)
{
  m_waitForResponse->Received(opcode);
}

bool CCECBusDevice::WaitForOpcode(cec_opcode opcode)
{
  return m_waitForResponse->Wait(opcode);
}

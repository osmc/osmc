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
#include "SLCommandHandler.h"

#include "lib/platform/util/timeutils.h"
#include "lib/devices/CECBusDevice.h"
#include "lib/devices/CECPlaybackDevice.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"

using namespace CEC;
using namespace PLATFORM;

#define SL_COMMAND_TYPE_HDDRECORDER_DISC  0x01
#define SL_COMMAND_TYPE_VCR               0x02
#define SL_COMMAND_TYPE_DVDPLAYER         0x03
#define SL_COMMAND_TYPE_HDDRECORDER_DISC2 0x04
#define SL_COMMAND_TYPE_HDDRECORDER       0x05

#define SL_COMMAND_INIT                 0x01
#define SL_COMMAND_ACK_INIT             0x02
#define SL_COMMAND_POWER_ON             0x03
#define SL_COMMAND_CONNECT_REQUEST      0x04
#define SL_COMMAND_SET_DEVICE_MODE      0x05
#define SL_COMMAND_REQUEST_POWER_STATUS 0xa0

#define LIB_CEC     m_busDevice->GetProcessor()->GetLib()
#define ToString(p) LIB_CEC->ToString(p)

CSLCommandHandler::CSLCommandHandler(CCECBusDevice *busDevice,
                                     int32_t iTransmitTimeout /* = CEC_DEFAULT_TRANSMIT_TIMEOUT */,
                                     int32_t iTransmitWait /* = CEC_DEFAULT_TRANSMIT_WAIT */,
                                     int8_t iTransmitRetries /* = CEC_DEFAULT_TRANSMIT_RETRIES */,
                                     int64_t iActiveSourcePending /* = 0 */) :
    CCECCommandHandler(busDevice, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending),
    m_bSLEnabled(false)
{
  m_vendorId = CEC_VENDOR_LG;

  /* LG devices don't always reply to CEC version requests, so just set it to 1.3a */
  m_busDevice->SetCecVersion(CEC_VERSION_1_3A);

  /* LG devices always return "korean" as language */
  cec_menu_language lang;
  lang.device = m_busDevice->GetLogicalAddress();
  snprintf(lang.language, 4, "eng");
  m_busDevice->SetMenuLanguage(lang);
}

bool CSLCommandHandler::InitHandler(void)
{
  if (m_bHandlerInited)
    return true;
  m_bHandlerInited = true;

  if (m_busDevice->GetLogicalAddress() != CECDEVICE_TV)
    return true;

  CCECBusDevice *primary = m_processor->GetPrimaryDevice();
  if (primary && primary->GetLogicalAddress() != CECDEVICE_UNREGISTERED)
  {
    /* imitate LG devices */
    if (m_busDevice->GetLogicalAddress() != primary->GetLogicalAddress())
    {
      primary->SetVendorId(CEC_VENDOR_LG);
      primary->ReplaceHandler(false);
    }
  }

  return true;
}

int CSLCommandHandler::HandleVendorCommand(const cec_command &command)
{
  if (!m_processor->IsHandledByLibCEC(command.destination))
    return true;

  if (command.parameters.size == 1 &&
      command.parameters[0] == SL_COMMAND_INIT)
  {
    HandleVendorCommandSLInit(command);
    return COMMAND_HANDLED;
  }
  else if (command.parameters.size == 2 &&
      command.parameters[0] == SL_COMMAND_POWER_ON)
  {
    HandleVendorCommandPowerOn(command);
    return COMMAND_HANDLED;
  }
  else if (command.parameters.size == 2 &&
      command.parameters[0] == SL_COMMAND_CONNECT_REQUEST)
  {
    HandleVendorCommandSLConnect(command);
    return COMMAND_HANDLED;
  }
  else if (command.parameters.size == 1 &&
      command.parameters[0] == SL_COMMAND_REQUEST_POWER_STATUS)
  {
    HandleVendorCommandPowerOnStatus(command);
    return COMMAND_HANDLED;
  }

  return CCECCommandHandler::HandleVendorCommand(command);
}

void CSLCommandHandler::HandleVendorCommandSLInit(const cec_command &command)
{
  CCECBusDevice* dev = m_processor->GetDevice(command.destination);
  if (dev && dev->IsHandledByLibCEC())
  {
    if (!dev->IsActiveSource())
    {
      dev->SetPowerStatus(CEC_POWER_STATUS_STANDBY);
      dev->TransmitPowerState(command.initiator, true);
    }

    TransmitVendorCommandSLAckInit(command.destination, command.initiator);
  }
}

void CSLCommandHandler::TransmitVendorCommandSLAckInit(const cec_logical_address iSource, const cec_logical_address iDestination)
{
  cec_command response;
  cec_command::Format(response, iSource, iDestination, CEC_OPCODE_VENDOR_COMMAND);
  response.PushBack(SL_COMMAND_ACK_INIT);
  response.PushBack(SL_COMMAND_TYPE_HDDRECORDER);

  Transmit(response, false, true);
  SetSLInitialised();
}

void CSLCommandHandler::HandleVendorCommandPowerOn(const cec_command &command)
{
  if (command.initiator != CECDEVICE_TV)
    return;

  CCECBusDevice *device = m_processor->GetPrimaryDevice();
  if (device)
  {
    SetSLInitialised();
    device->MarkAsActiveSource();
    device->SetPowerStatus(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
    device->TransmitPowerState(command.initiator, true);

    CEvent::Sleep(2000);
    device->SetPowerStatus(CEC_POWER_STATUS_ON);
    device->TransmitPowerState(command.initiator, false);
    device->TransmitPhysicalAddress(false);

    if (device->IsActiveSource())
      ActivateSource();
  }
}
void CSLCommandHandler::HandleVendorCommandPowerOnStatus(const cec_command &command)
{
  if (command.destination != CECDEVICE_BROADCAST)
  {
    CCECBusDevice *device = m_processor->GetPrimaryDevice();
    if (device)
    {
      device->SetPowerStatus(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
      device->TransmitPowerState(command.initiator, true);
      device->SetPowerStatus(CEC_POWER_STATUS_ON);
    }
  }
}

void CSLCommandHandler::HandleVendorCommandSLConnect(const cec_command &command)
{
  SetSLInitialised();
  TransmitVendorCommandSetDeviceMode(command.destination, command.initiator, CEC_DEVICE_TYPE_RECORDING_DEVICE);


  if (m_processor->IsActiveSource(command.destination) && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice* dev = m_processor->GetDevice(command.destination);
    CCECPlaybackDevice* pb = dev->AsPlaybackDevice();
    if (pb)
      pb->TransmitDeckStatus(command.initiator, true);
    dev->TransmitPowerState(command.initiator, true);
  }
}

void CSLCommandHandler::TransmitVendorCommandSetDeviceMode(const cec_logical_address iSource, const cec_logical_address iDestination, const cec_device_type type)
{
  cec_command response;
  cec_command::Format(response, iSource, iDestination, CEC_OPCODE_VENDOR_COMMAND);
  response.PushBack(SL_COMMAND_SET_DEVICE_MODE);
  response.PushBack((uint8_t)type);
  Transmit(response, false, true);
}

int CSLCommandHandler::HandleGiveDeckStatus(const cec_command &command)
{
  if (!m_processor->CECInitialised() ||
      !m_processor->IsHandledByLibCEC(command.destination))
    return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;

  CCECPlaybackDevice *device = CCECBusDevice::AsPlaybackDevice(GetDevice(command.destination));
  if (!device || command.parameters.size == 0)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  device->SetDeckStatus(CEC_DECK_INFO_OTHER_STATUS_LG);
  if (command.parameters[0] == CEC_STATUS_REQUEST_ON)
  {
    device->TransmitDeckStatus(command.initiator, true);
    ActivateSource();
    return COMMAND_HANDLED;
  }
  else if (command.parameters[0] == CEC_STATUS_REQUEST_ONCE)
  {
    device->TransmitDeckStatus(command.initiator, true);
    return COMMAND_HANDLED;
  }

  return CCECCommandHandler::HandleGiveDeckStatus(command);
}

int CSLCommandHandler::HandleGiveDevicePowerStatus(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination) && command.initiator == CECDEVICE_TV)
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->GetCurrentPowerStatus() != CEC_POWER_STATUS_ON)
    {
      device->TransmitPowerState(command.initiator, true);
      device->SetPowerStatus(CEC_POWER_STATUS_ON);
    }
    else
    {
      if (m_resetPowerState.IsSet() && m_resetPowerState.TimeLeft() > 0)
      {
        /* TODO assume that we've bugged out. the return button no longer works after this */
        LIB_CEC->AddLog(CEC_LOG_WARNING, "FIXME: LG seems to have bugged out. resetting to 'in transition standby to on'. the return button will not work");
        device->SetPowerStatus(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
        device->TransmitPowerState(command.initiator, true);
        device->SetPowerStatus(CEC_POWER_STATUS_ON);
        m_resetPowerState.Init(5000);
      }
      else
      {
        device->TransmitPowerState(command.initiator, true);
        m_resetPowerState.Init(5000);
      }
    }

    return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CSLCommandHandler::HandleRequestActiveSource(const cec_command &command)
{
  if (m_processor->CECInitialised())
  {
    if (!SLInitialised())
      TransmitVendorCommandSLAckInit(m_processor->GetPrimaryDevice()->GetLogicalAddress(), command.initiator);
    CCECCommandHandler::HandleRequestActiveSource(command);
  }
  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CSLCommandHandler::HandleFeatureAbort(const cec_command &command)
{
  CCECBusDevice* primary = m_processor->GetPrimaryDevice();
  if (command.parameters.size == 0 && primary->GetLogicalAddress() != CECDEVICE_UNKNOWN &&
      primary->GetCurrentPowerStatus() == CEC_POWER_STATUS_ON && !SLInitialised() &&
      command.initiator == CECDEVICE_TV)
  {
    if (!SLInitialised() && m_processor->IsActiveSource(command.destination))
    {
      TransmitVendorCommandSLAckInit(command.destination, command.initiator);
      return COMMAND_HANDLED;
    }
  }

  return CCECCommandHandler::HandleFeatureAbort(command);
}

int CSLCommandHandler::HandleStandby(const cec_command &command)
{
  ResetSLState();

  return CCECCommandHandler::HandleStandby(command);
}

void CSLCommandHandler::ResetSLState(void)
{
  LIB_CEC->AddLog(CEC_LOG_NOTICE, "resetting SL initialised state");
  CLockObject lock(m_SLMutex);
  m_bSLEnabled = false;
  m_processor->GetPrimaryDevice()->SetPowerStatus(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
}

void CSLCommandHandler::SetSLInitialised(void)
{
  LIB_CEC->AddLog(CEC_LOG_NOTICE, "SL initialised");
  CLockObject lock(m_SLMutex);
  m_bSLEnabled = true;
}

bool CSLCommandHandler::SLInitialised(void)
{
  CLockObject lock(m_SLMutex);
  return m_bSLEnabled;
}

bool CSLCommandHandler::PowerOn(const cec_logical_address iInitiator, const cec_logical_address iDestination)
{
  if (iDestination != CECDEVICE_TV)
  {
    /* LG devices only allow themselves to be woken up by the TV with a vendor command */
    cec_command command;

    if (!m_bSLEnabled)
      TransmitVendorID(CECDEVICE_TV, iDestination, CEC_VENDOR_LG, false);

    cec_command::Format(command, CECDEVICE_TV, iDestination, CEC_OPCODE_VENDOR_COMMAND);
    command.PushBack(SL_COMMAND_POWER_ON);
    command.PushBack(0);
    return Transmit(command, false, false);
  }

  return CCECCommandHandler::PowerOn(iInitiator, iDestination);
}

bool CSLCommandHandler::ActivateSource(bool bTransmitDelayedCommandsOnly /* = false */)
{
  if (m_busDevice->IsActiveSource() &&
      m_busDevice->IsHandledByLibCEC())
  {
    {
      CLockObject lock(m_mutex);
      // check if we need to send a delayed source switch
      if (bTransmitDelayedCommandsOnly)
      {
        if (m_iActiveSourcePending == 0 || GetTimeMs() < m_iActiveSourcePending)
          return false;

#ifdef CEC_DEBUGGING
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "transmitting delayed activate source command");
#endif
      }
    }

    CCECPlaybackDevice *device = m_busDevice->AsPlaybackDevice();
    if (device)
      device->SetDeckStatus(!device->IsActiveSource() ? CEC_DECK_INFO_OTHER_STATUS : CEC_DECK_INFO_OTHER_STATUS_LG);

    // power on the TV
    CCECBusDevice* tv = m_processor->GetDevice(CECDEVICE_TV);
    bool bTvPresent = (tv && tv->GetStatus() == CEC_DEVICE_STATUS_PRESENT);
    bool bActiveSourceFailed(false);
    if (bTvPresent)
    {
      bActiveSourceFailed = !device->TransmitImageViewOn();
    }
    else
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "TV not present, not sending 'image view on'");
    }

    // check if we're allowed to switch sources
    bool bSourceSwitchAllowed = SourceSwitchAllowed();
    if (!bSourceSwitchAllowed)
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "source switch is currently not allowed by command handler");

    // switch sources (if allowed)
    if (!bActiveSourceFailed && bSourceSwitchAllowed)
    {
      bActiveSourceFailed = !m_busDevice->TransmitActiveSource(false);
    }

    // retry later
    if (bActiveSourceFailed || !bSourceSwitchAllowed)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "failed to make '%s' the active source. will retry later", m_busDevice->GetLogicalAddressName());
      int64_t now(GetTimeMs());
      CLockObject lock(m_mutex);
      if (m_iActiveSourcePending == 0 || m_iActiveSourcePending < now)
        m_iActiveSourcePending = now + (int64_t)CEC_ACTIVE_SOURCE_SWITCH_RETRY_TIME_MS;
      return false;
    }
    else
    {
      CLockObject lock(m_mutex);
      // clear previous pending active source command
      m_iActiveSourcePending = 0;
    }

    // mark the handler as initialised
    CLockObject lock(m_mutex);
    m_bHandlerInited = true;
  }
  return true;
}

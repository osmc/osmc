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
#include "CECCommandHandler.h"

#include "lib/devices/CECBusDevice.h"
#include "lib/devices/CECAudioSystem.h"
#include "lib/devices/CECPlaybackDevice.h"
#include "lib/CECClient.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"
#include "lib/CECTypeUtils.h"
#include "lib/platform/util/util.h"

using namespace CEC;
using namespace std;
using namespace PLATFORM;

#define LIB_CEC     m_busDevice->GetProcessor()->GetLib()
#define ToString(p) CCECTypeUtils::ToString(p)
#define REQUEST_POWER_STATUS_TIMEOUT 5000

CCECCommandHandler::CCECCommandHandler(CCECBusDevice *busDevice,
                                       int32_t iTransmitTimeout /* = CEC_DEFAULT_TRANSMIT_TIMEOUT */,
                                       int32_t iTransmitWait /* = CEC_DEFAULT_TRANSMIT_WAIT */,
                                       int8_t iTransmitRetries /* = CEC_DEFAULT_TRANSMIT_RETRIES */,
                                       int64_t iActiveSourcePending /* = 0 */) :
    m_busDevice(busDevice),
    m_processor(m_busDevice->GetProcessor()),
    m_iTransmitTimeout(iTransmitTimeout),
    m_iTransmitWait(iTransmitWait),
    m_iTransmitRetries(iTransmitRetries),
    m_bHandlerInited(false),
    m_bOPTSendDeckStatusUpdateOnActiveSource(false),
    m_vendorId(CEC_VENDOR_UNKNOWN),
    m_iActiveSourcePending(iActiveSourcePending),
    m_iPowerStatusRequested(0)
{
}

bool CCECCommandHandler::HandleCommand(const cec_command &command)
{
  if (command.opcode_set == 0)
    return HandlePoll(command);

  int iHandled(CEC_ABORT_REASON_UNRECOGNIZED_OPCODE);

  LIB_CEC->AddCommand(command);

  switch(command.opcode)
  {
  case CEC_OPCODE_REPORT_POWER_STATUS:
    iHandled = HandleReportPowerStatus(command);
    break;
  case CEC_OPCODE_CEC_VERSION:
    iHandled = HandleDeviceCecVersion(command);
    break;
  case CEC_OPCODE_SET_MENU_LANGUAGE:
    iHandled = HandleSetMenuLanguage(command);
    break;
  case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
    iHandled = HandleGivePhysicalAddress(command);
    break;
  case CEC_OPCODE_GET_MENU_LANGUAGE:
    iHandled = HandleGiveMenuLanguage(command);
    break;
  case CEC_OPCODE_GIVE_OSD_NAME:
    iHandled = HandleGiveOSDName(command);
    break;
  case CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
    iHandled = HandleGiveDeviceVendorId(command);
    break;
  case CEC_OPCODE_DEVICE_VENDOR_ID:
    iHandled = HandleDeviceVendorId(command);
    break;
  case CEC_OPCODE_VENDOR_COMMAND_WITH_ID:
    iHandled = HandleDeviceVendorCommandWithId(command);
    break;
  case CEC_OPCODE_GIVE_DECK_STATUS:
    iHandled = HandleGiveDeckStatus(command);
    break;
  case CEC_OPCODE_DECK_CONTROL:
    iHandled = HandleDeckControl(command);
    break;
  case CEC_OPCODE_MENU_REQUEST:
    iHandled = HandleMenuRequest(command);
    break;
  case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
    iHandled = HandleGiveDevicePowerStatus(command);
    break;
  case CEC_OPCODE_GET_CEC_VERSION:
    iHandled = HandleGetCecVersion(command);
    break;
  case CEC_OPCODE_USER_CONTROL_PRESSED:
    iHandled = HandleUserControlPressed(command);
    break;
  case CEC_OPCODE_USER_CONTROL_RELEASE:
    iHandled = HandleUserControlRelease(command);
    break;
  case CEC_OPCODE_GIVE_AUDIO_STATUS:
    iHandled = HandleGiveAudioStatus(command);
    break;
  case CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
    iHandled = HandleGiveSystemAudioModeStatus(command);
    break;
  case CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
    iHandled = HandleSystemAudioModeRequest(command);
    break;
  case CEC_OPCODE_REPORT_AUDIO_STATUS:
    iHandled = HandleReportAudioStatus(command);
    break;
  case CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS:
    iHandled = HandleSystemAudioModeStatus(command);
    break;
  case CEC_OPCODE_SET_SYSTEM_AUDIO_MODE:
    iHandled = HandleSetSystemAudioMode(command);
    break;
  case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
    iHandled = HandleRequestActiveSource(command);
    break;
  case CEC_OPCODE_SET_STREAM_PATH:
    iHandled = HandleSetStreamPath(command);
    break;
  case CEC_OPCODE_ROUTING_CHANGE:
    iHandled = HandleRoutingChange(command);
    break;
  case CEC_OPCODE_ROUTING_INFORMATION:
    iHandled = HandleRoutingInformation(command);
    break;
  case CEC_OPCODE_STANDBY:
    iHandled = HandleStandby(command);
    break;
  case CEC_OPCODE_ACTIVE_SOURCE:
    iHandled = HandleActiveSource(command);
    break;
  case CEC_OPCODE_REPORT_PHYSICAL_ADDRESS:
    iHandled = HandleReportPhysicalAddress(command);
    break;
  case CEC_OPCODE_SET_OSD_NAME:
    iHandled = HandleSetOSDName(command);
    break;
  case CEC_OPCODE_IMAGE_VIEW_ON:
    iHandled = HandleImageViewOn(command);
    break;
  case CEC_OPCODE_TEXT_VIEW_ON:
    iHandled = HandleTextViewOn(command);
    break;
  case CEC_OPCODE_FEATURE_ABORT:
    iHandled = HandleFeatureAbort(command);
    break;
  case CEC_OPCODE_VENDOR_COMMAND:
    iHandled = HandleVendorCommand(command);
    break;
  case CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN:
    iHandled = HandleVendorRemoteButtonDown(command);
    break;
  case CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP:
    iHandled = HandleVendorRemoteButtonUp(command);
    break;
  case CEC_OPCODE_PLAY:
    // libCEC (currently) doesn't need to do anything with this, since player applications handle it
    // but it should not respond with a feature abort
    iHandled = COMMAND_HANDLED;
    break;
  default:
    break;
  }

  if (iHandled == COMMAND_HANDLED)
    m_busDevice->SignalOpcode((command.opcode == CEC_OPCODE_FEATURE_ABORT && command.parameters.size > 0) ? (cec_opcode)command.parameters[0] : command.opcode);
  else
    UnhandledCommand(command, (cec_abort_reason)iHandled);

  return iHandled == COMMAND_HANDLED;
}

int CCECCommandHandler::HandleActiveSource(const cec_command &command)
{
  if (command.parameters.size == 2)
  {
    uint16_t iAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    CCECBusDevice *device = m_processor->GetDevice(command.initiator);
    if (device)
    {
      device->SetPhysicalAddress(iAddress);
      device->MarkAsActiveSource();
    }

    m_processor->GetDevices()->SignalAll(command.opcode);
    return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleDeckControl(const cec_command &command)
{
  CCECPlaybackDevice *device = CCECBusDevice::AsPlaybackDevice(GetDevice(command.destination));
  if (device && command.parameters.size > 0)
  {
    device->SetDeckControlMode((cec_deck_control_mode) command.parameters[0]);
    return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleDeviceCecVersion(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
      device->SetCecVersion((cec_version) command.parameters[0]);

    return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleDeviceVendorCommandWithId(const cec_command & UNUSED(command))
{
  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleDeviceVendorId(const cec_command &command)
{
  SetVendorId(command);

  if (command.initiator == CECDEVICE_TV)
  {
    CCECBusDevice* primary = m_processor->GetPrimaryDevice();
    if (primary)
      primary->TransmitVendorID(CECDEVICE_BROADCAST, false, false);
  }

  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleFeatureAbort(const cec_command &command)
{
  if (command.parameters.size == 2 &&
        (command.parameters[1] == CEC_ABORT_REASON_UNRECOGNIZED_OPCODE ||
         command.parameters[1] == CEC_ABORT_REASON_REFUSED))
    m_processor->GetDevice(command.initiator)->SetUnsupportedFeature((cec_opcode)command.parameters[0]);
  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleGetCecVersion(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->TransmitCECVersion(command.initiator, true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleGiveAudioStatus(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECAudioSystem *device = CCECBusDevice::AsAudioSystem(GetDevice(command.destination));
    if (device && device->TransmitAudioStatus(command.initiator, true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleGiveDeckStatus(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECPlaybackDevice *device = CCECBusDevice::AsPlaybackDevice(GetDevice(command.destination));
    if (device && device->TransmitDeckStatus(command.initiator, true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleGiveDevicePowerStatus(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->TransmitPowerState(command.initiator, true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleGiveDeviceVendorId(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->TransmitVendorID(command.initiator, true, true))
      return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleGiveOSDName(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->TransmitOSDName(command.initiator, true))
      return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleGivePhysicalAddress(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->TransmitPhysicalAddress(true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleGiveMenuLanguage(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->TransmitSetMenuLanguage(command.initiator, true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleGiveSystemAudioModeStatus(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECAudioSystem *device = CCECBusDevice::AsAudioSystem(GetDevice(command.destination));
    if (device && device->TransmitSystemAudioModeStatus(command.initiator, true))
      return COMMAND_HANDLED;
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleImageViewOn(const cec_command &command)
{
  CCECBusDevice *device = GetDevice(command.destination);
  if (device && device->GetCurrentStatus() == CEC_DEVICE_STATUS_PRESENT)
  {
    if (device->GetCurrentPowerStatus() == CEC_POWER_STATUS_STANDBY ||
        device->GetCurrentPowerStatus() == CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY)
      device->SetPowerStatus(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
    CCECBusDevice* tv = GetDevice(CECDEVICE_TV);
    if (tv)
      tv->OnImageViewOnSent(false);
  }
  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleMenuRequest(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device)
    {
      CCECClient *client = device->GetClient();
      if (client)
      {
        if (command.parameters[0] == CEC_MENU_REQUEST_TYPE_ACTIVATE)
        {
          if (client->MenuStateChanged(CEC_MENU_STATE_ACTIVATED) == 1)
            device->SetMenuState(CEC_MENU_STATE_ACTIVATED);
        }
        else if (command.parameters[0] == CEC_MENU_REQUEST_TYPE_DEACTIVATE)
        {
          if (client->MenuStateChanged(CEC_MENU_STATE_DEACTIVATED) == 1)
            device->SetMenuState(CEC_MENU_STATE_DEACTIVATED);
        }
      }
      if (device->TransmitMenuState(command.initiator, true))
        return COMMAND_HANDLED;
    }
    return CEC_ABORT_REASON_INVALID_OPERAND;
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

bool CCECCommandHandler::HandlePoll(const cec_command &command)
{
  m_busDevice->HandlePoll(command.destination);
  return true;
}

int CCECCommandHandler::HandleReportAudioStatus(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECAudioSystem *device = CCECBusDevice::AsAudioSystem(GetDevice(command.initiator));
    if (device)
    {
      device->SetAudioStatus(command.parameters[0]);
      return COMMAND_HANDLED;
    }
  }
  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleReportPhysicalAddress(const cec_command &command)
{
  if (command.parameters.size == 3)
  {
    uint16_t iNewAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    SetPhysicalAddress(command.initiator, iNewAddress);

    if (command.initiator == CECDEVICE_TV)
    {
      CCECBusDevice* primary = m_processor->GetPrimaryDevice();
      if (primary)
        primary->TransmitPhysicalAddress(false);
    }
    return COMMAND_HANDLED;
  }
  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleReportPowerStatus(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      device->SetPowerStatus((cec_power_status) command.parameters[0]);
      return COMMAND_HANDLED;
    }
  }
  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleRequestActiveSource(const cec_command &command)
{
  if (m_processor->CECInitialised())
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, ">> %i requests active source", (uint8_t) command.initiator);
    m_processor->GetDevice(command.initiator)->SetPowerStatus(CEC_POWER_STATUS_ON);

    vector<CCECBusDevice *> devices;
    for (size_t iDevicePtr = 0; iDevicePtr < GetMyDevices(devices); iDevicePtr++)
      devices[iDevicePtr]->TransmitActiveSource(true);
  }

  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleRoutingChange(const cec_command &command)
{
  if (command.parameters.size == 4)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      uint16_t iNewAddress = ((uint16_t)command.parameters[2] << 8) | ((uint16_t)command.parameters[3]);
      device->SetActiveRoute(iNewAddress);
      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleRoutingInformation(const cec_command &command)
{
  if (command.parameters.size == 2)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      uint16_t iNewAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
      device->SetActiveRoute(iNewAddress);
      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleSetMenuLanguage(const cec_command &command)
{
  if (command.parameters.size == 3)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      cec_menu_language language;
      language.device = command.initiator;
      for (uint8_t iPtr = 0; iPtr < 4; iPtr++)
        language.language[iPtr] = command.parameters[iPtr];
      language.language[3] = 0;
      device->SetMenuLanguage(language);
      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleSetOSDName(const cec_command &command)
{
  if (command.parameters.size > 0)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      char buf[1024];
      for (uint8_t iPtr = 0; iPtr < command.parameters.size; iPtr++)
        buf[iPtr] = (char)command.parameters[iPtr];
      buf[command.parameters.size] = 0;

      CStdString strName(buf);
      device->SetOSDName(strName);

      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleSetStreamPath(const cec_command &command)
{
  if (!m_processor->CECInitialised())
    return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;

  if (command.parameters.size >= 2)
  {
    uint16_t iStreamAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, ">> %s (%x) sets stream path to physical address %04x", ToString(command.initiator), command.initiator, iStreamAddress);

    /* one of the device handled by libCEC has been made active */
    CCECBusDevice *device = GetDeviceByPhysicalAddress(iStreamAddress);
    if (device)
    {
      if (device->IsHandledByLibCEC() && !device->IsActiveSource())
        device->ActivateSource();
      else
      {
        device->MarkAsActiveSource();
        device->TransmitActiveSource(true);
      }
      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleSystemAudioModeRequest(const cec_command &command)
{
  if (m_processor->CECInitialised() && m_processor->IsHandledByLibCEC(command.destination))
  {
    CCECAudioSystem *device = CCECBusDevice::AsAudioSystem(GetDevice(command.destination));
    if (device)
    {
      if (command.parameters.size >= 2)
      {
        device->SetPowerStatus(CEC_POWER_STATUS_ON);
        device->SetSystemAudioModeStatus(CEC_SYSTEM_AUDIO_STATUS_ON);
        uint16_t iNewAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
        CCECBusDevice *newActiveDevice = GetDeviceByPhysicalAddress(iNewAddress);
        if (newActiveDevice)
          newActiveDevice->MarkAsActiveSource();
        if (device->TransmitSetSystemAudioMode(command.initiator, true))
          return COMMAND_HANDLED;
      }
      else
      {
        device->SetSystemAudioModeStatus(CEC_SYSTEM_AUDIO_STATUS_OFF);
        if (device->TransmitSetSystemAudioMode(command.initiator, true))
          return COMMAND_HANDLED;
      }
    }
  }

  return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;
}

int CCECCommandHandler::HandleStandby(const cec_command &command)
{
  CCECBusDevice *device = GetDevice(command.initiator);
  if (device)
    device->SetPowerStatus(CEC_POWER_STATUS_STANDBY);

  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleSystemAudioModeStatus(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECAudioSystem *device = CCECBusDevice::AsAudioSystem(GetDevice(command.initiator));
    if (device)
    {
      device->SetSystemAudioModeStatus((cec_system_audio_status)command.parameters[0]);
      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleSetSystemAudioMode(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECAudioSystem *device = CCECBusDevice::AsAudioSystem(GetDevice(command.initiator));
    if (device)
    {
      device->SetSystemAudioModeStatus((cec_system_audio_status)command.parameters[0]);
      return COMMAND_HANDLED;
    }
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleTextViewOn(const cec_command &command)
{
  m_processor->GetDevice(command.initiator)->MarkAsActiveSource();
  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleUserControlPressed(const cec_command &command)
{
  if (!m_processor->CECInitialised() ||
      !m_processor->IsHandledByLibCEC(command.destination))
    return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;

  if (command.parameters.size == 0)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  CCECBusDevice *device = GetDevice(command.destination);
  if (!device)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  CCECClient *client = device->GetClient();
  if (client)
    client->SetCurrentButton((cec_user_control_code) command.parameters[0]);

  if (command.parameters[0] == CEC_USER_CONTROL_CODE_POWER ||
      command.parameters[0] == CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION||
      command.parameters[0] == CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION)
  {
    bool bPowerOn(true);

    // CEC_USER_CONTROL_CODE_POWER and CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION operate as a toggle
    // assume CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION does not
    if (command.parameters[0] == CEC_USER_CONTROL_CODE_POWER ||
        command.parameters[0] == CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION)
    {
      cec_power_status status = device->GetCurrentPowerStatus();
      bPowerOn = !(status == CEC_POWER_STATUS_ON || status == CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
    }

    if (bPowerOn)
    {
      device->ActivateSource();
    }
    else
    {
      device->MarkAsInactiveSource();
      device->TransmitInactiveSource();
      device->SetMenuState(CEC_MENU_STATE_DEACTIVATED);
    }
  }
  else if (command.parameters[0] != CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION)
  {
    // we're not marked as active source, but the tv sends keypresses to us, so assume it forgot to activate us
    if (!device->IsActiveSource() && command.initiator == CECDEVICE_TV)
      device->MarkAsActiveSource();
  }

  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleUserControlRelease(const cec_command &command)
{
  if (!m_processor->CECInitialised() ||
      !m_processor->IsHandledByLibCEC(command.destination))
    return CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND;

  CCECClient *client = m_processor->GetClient(command.destination);
  if (client)
    client->AddKey();

  return COMMAND_HANDLED;
}

int CCECCommandHandler::HandleVendorCommand(const cec_command & UNUSED(command))
{
  return CEC_ABORT_REASON_INVALID_OPERAND;
}

int CCECCommandHandler::HandleVendorRemoteButtonDown(const cec_command& command)
{
  if (command.parameters.size == 0)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  LIB_CEC->AddLog(CEC_LOG_NOTICE, "unhandled vendor remote button received with keycode %x", command.parameters[0]);
  return COMMAND_HANDLED;
}

void CCECCommandHandler::UnhandledCommand(const cec_command &command, const cec_abort_reason reason)
{
  if (m_processor->IsHandledByLibCEC(command.destination))
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "sending abort with opcode %02x and reason '%s' to %s", command.opcode, ToString(reason), ToString(command.initiator));
    m_processor->TransmitAbort(command.destination, command.initiator, command.opcode, reason);

    if (reason == CEC_ABORT_REASON_INVALID_OPERAND)
      RequestEmailFromCustomer(command);
  }
}

size_t CCECCommandHandler::GetMyDevices(vector<CCECBusDevice *> &devices) const
{
  size_t iReturn(0);

  cec_logical_addresses addresses = m_processor->GetLogicalAddresses();
  for (uint8_t iPtr = CECDEVICE_TV; iPtr < CECDEVICE_BROADCAST; iPtr++)
  {
    if (addresses[iPtr])
    {
      devices.push_back(GetDevice((cec_logical_address) iPtr));
      ++iReturn;
    }
  }

  return iReturn;
}

CCECBusDevice *CCECCommandHandler::GetDevice(cec_logical_address iLogicalAddress) const
{
  return m_processor->GetDevice(iLogicalAddress);
}

CCECBusDevice *CCECCommandHandler::GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress) const
{
  return m_processor->GetDeviceByPhysicalAddress(iPhysicalAddress);
}

bool CCECCommandHandler::SetVendorId(const cec_command &command)
{
  bool bChanged(false);
  if (command.parameters.size < 3)
  {
    LIB_CEC->AddLog(CEC_LOG_WARNING, "invalid vendor ID received");
    return bChanged;
  }

  uint64_t iVendorId = ((uint64_t)command.parameters[0] << 16) +
                       ((uint64_t)command.parameters[1] << 8) +
                        (uint64_t)command.parameters[2];

  CCECBusDevice *device = GetDevice((cec_logical_address) command.initiator);
  if (device)
    bChanged = device->SetVendorId(iVendorId);
  return bChanged;
}

void CCECCommandHandler::SetPhysicalAddress(cec_logical_address iAddress, uint16_t iNewAddress)
{
  if (!m_processor->IsHandledByLibCEC(iAddress))
  {
    CCECBusDevice *otherDevice = m_processor->GetDeviceByPhysicalAddress(iNewAddress);
    CCECClient *client = otherDevice ? otherDevice->GetClient() : NULL;

    CCECBusDevice *device = m_processor->GetDevice(iAddress);
    if (device)
      device->SetPhysicalAddress(iNewAddress);
    else
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "device with logical address %X not found", iAddress);

    /* another device reported the same physical address as ours */
    if (client)
    {
      libcec_parameter param;
      param.paramType = CEC_PARAMETER_TYPE_STRING;
      param.paramData = (void*)"Physical address in use by another device. Please verify your settings";
      client->Alert(CEC_ALERT_PHYSICAL_ADDRESS_ERROR, param);
      client->ResetPhysicalAddress();
    }
  }
  else
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "ignore physical address report for device %s (%X) because it's marked as handled by libCEC", ToString(iAddress), iAddress);
  }
}

bool CCECCommandHandler::PowerOn(const cec_logical_address iInitiator, const cec_logical_address iDestination)
{
  if (iDestination == CECDEVICE_TV)
    return TransmitImageViewOn(iInitiator, iDestination);

  return TransmitKeypress(iInitiator, iDestination, CEC_USER_CONTROL_CODE_POWER) &&
    TransmitKeyRelease(iInitiator, iDestination);
}

bool CCECCommandHandler::TransmitImageViewOn(const cec_logical_address iInitiator, const cec_logical_address iDestination)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_IMAGE_VIEW_ON);

  if (Transmit(command, false, false))
  {
    CCECBusDevice* dest = m_processor->GetDevice(iDestination);
    if (dest && dest->GetCurrentPowerStatus() != CEC_POWER_STATUS_ON)
      dest->SetPowerStatus(CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON);
    return true;
  }
  return false;
}

bool CCECCommandHandler::TransmitStandby(const cec_logical_address iInitiator, const cec_logical_address iDestination)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_STANDBY);

  return Transmit(command, false, false);
}

bool CCECCommandHandler::TransmitRequestActiveSource(const cec_logical_address iInitiator, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, CECDEVICE_BROADCAST, CEC_OPCODE_REQUEST_ACTIVE_SOURCE);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestCecVersion(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GET_CEC_VERSION);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestMenuLanguage(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GET_MENU_LANGUAGE);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestOSDName(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GIVE_OSD_NAME);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestAudioStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GIVE_AUDIO_STATUS);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestPhysicalAddress(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GIVE_PHYSICAL_ADDRESS);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestPowerStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bUpdate, bool bWaitForResponse /* = true */)
{
  if (iDestination == CECDEVICE_TV)
  {
    int64_t now(GetTimeMs());
    if (!bUpdate && now - m_iPowerStatusRequested < REQUEST_POWER_STATUS_TIMEOUT)
      return true;
    m_iPowerStatusRequested = now;
  }

  LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting power status of '%s' (%X)", m_busDevice->GetLogicalAddressName(), iDestination);

  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GIVE_DEVICE_POWER_STATUS);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitRequestVendorId(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_GIVE_DEVICE_VENDOR_ID);

  return Transmit(command, !bWaitForResponse, false);
}

bool CCECCommandHandler::TransmitActiveSource(const cec_logical_address iInitiator, uint16_t iPhysicalAddress, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, CECDEVICE_BROADCAST, CEC_OPCODE_ACTIVE_SOURCE);
  command.parameters.PushBack((uint8_t) ((iPhysicalAddress >> 8) & 0xFF));
  command.parameters.PushBack((uint8_t) (iPhysicalAddress & 0xFF));

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitCECVersion(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_version cecVersion, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_CEC_VERSION);
  command.parameters.PushBack((uint8_t)cecVersion);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitInactiveSource(const cec_logical_address iInitiator, uint16_t iPhysicalAddress)
{
  cec_command command;
  cec_command::Format(command, iInitiator, CECDEVICE_TV, CEC_OPCODE_INACTIVE_SOURCE);
  command.parameters.PushBack((iPhysicalAddress >> 8) & 0xFF);
  command.parameters.PushBack(iPhysicalAddress & 0xFF);

  return Transmit(command, false, false);
}

bool CCECCommandHandler::TransmitMenuState(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_menu_state menuState, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_MENU_STATUS);
  command.parameters.PushBack((uint8_t)menuState);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitOSDName(const cec_logical_address iInitiator, const cec_logical_address iDestination, std::string strDeviceName, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_SET_OSD_NAME);
  for (size_t iPtr = 0; iPtr < strDeviceName.length(); iPtr++)
    command.parameters.PushBack(strDeviceName.at(iPtr));

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitOSDString(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_display_control duration, const char *strMessage, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_SET_OSD_STRING);
  command.parameters.PushBack((uint8_t)duration);

  size_t iLen = strlen(strMessage);
  if (iLen > 13) iLen = 13;

  for (size_t iPtr = 0; iPtr < iLen; iPtr++)
    command.parameters.PushBack(strMessage[iPtr]);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitPhysicalAddress(const cec_logical_address iInitiator, uint16_t iPhysicalAddress, cec_device_type type, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, CECDEVICE_BROADCAST, CEC_OPCODE_REPORT_PHYSICAL_ADDRESS);
  command.parameters.PushBack((uint8_t) ((iPhysicalAddress >> 8) & 0xFF));
  command.parameters.PushBack((uint8_t) (iPhysicalAddress & 0xFF));
  command.parameters.PushBack((uint8_t) (type));

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitSetMenuLanguage(const cec_logical_address iInitiator, const char lang[4], bool bIsReply)
{
  cec_command command;
  command.Format(command, iInitiator, CECDEVICE_BROADCAST, CEC_OPCODE_SET_MENU_LANGUAGE);
  command.parameters.PushBack((uint8_t) lang[0]);
  command.parameters.PushBack((uint8_t) lang[1]);
  command.parameters.PushBack((uint8_t) lang[2]);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitPoll(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_NONE);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitPowerState(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_power_status state, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_REPORT_POWER_STATUS);
  command.parameters.PushBack((uint8_t) state);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitVendorID(const cec_logical_address iInitiator, const cec_logical_address UNUSED(iDestination), uint64_t iVendorId, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, CECDEVICE_BROADCAST, CEC_OPCODE_DEVICE_VENDOR_ID);

  command.parameters.PushBack((uint8_t) (((uint64_t)iVendorId >> 16) & 0xFF));
  command.parameters.PushBack((uint8_t) (((uint64_t)iVendorId >> 8) & 0xFF));
  command.parameters.PushBack((uint8_t) ((uint64_t)iVendorId & 0xFF));

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitAudioStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, uint8_t state, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_REPORT_AUDIO_STATUS);
  command.parameters.PushBack(state);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitSetSystemAudioMode(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_system_audio_status state, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_SET_SYSTEM_AUDIO_MODE);
  command.parameters.PushBack((uint8_t)state);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitSetStreamPath(uint16_t iStreamPath, bool bIsReply)
{
  if (m_busDevice->GetLogicalAddress() != CECDEVICE_TV)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "only the TV is allowed to send CEC_OPCODE_SET_STREAM_PATH");
    return false;
  }
  cec_command command;
  cec_command::Format(command, m_busDevice->GetLogicalAddress(), CECDEVICE_BROADCAST, CEC_OPCODE_SET_STREAM_PATH);
  command.parameters.PushBack((uint8_t) ((iStreamPath >> 8) & 0xFF));
  command.parameters.PushBack((uint8_t) (iStreamPath        & 0xFF));

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitSystemAudioModeStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_system_audio_status state, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS);
  command.parameters.PushBack((uint8_t)state);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitDeckStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_deck_info state, bool bIsReply)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_DECK_STATUS);
  command.PushBack((uint8_t)state);

  return Transmit(command, false, bIsReply);
}

bool CCECCommandHandler::TransmitKeypress(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_user_control_code key, bool bWait /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_USER_CONTROL_PRESSED);
  command.parameters.PushBack((uint8_t)key);

  return Transmit(command, !bWait, false);
}

bool CCECCommandHandler::TransmitKeyRelease(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWait /* = true */)
{
  cec_command command;
  cec_command::Format(command, iInitiator, iDestination, CEC_OPCODE_USER_CONTROL_RELEASE);

  return Transmit(command, !bWait, false);
}

bool CCECCommandHandler::Transmit(cec_command &command, bool bSuppressWait, bool bIsReply)
{
  bool bReturn(false);
  cec_opcode expectedResponse(cec_command::GetResponseOpcode(command.opcode));
  bool bExpectResponse(expectedResponse != CEC_OPCODE_NONE && !bSuppressWait);
  command.transmit_timeout = m_iTransmitTimeout;

  if (command.initiator == CECDEVICE_UNKNOWN)
  {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "not transmitting a command without a valid initiator");
    return bReturn;
  }

  // check whether the destination is not marked as not present or handled by libCEC
  if (command.destination != CECDEVICE_BROADCAST && command.opcode_set)
  {
    CCECBusDevice* destinationDevice = m_processor->GetDevice(command.destination);
    cec_bus_device_status status = destinationDevice ? destinationDevice->GetStatus() : CEC_DEVICE_STATUS_NOT_PRESENT;
    if (status == CEC_DEVICE_STATUS_NOT_PRESENT)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "not sending command '%s': destination device '%s' marked as not present", ToString(command.opcode),ToString(command.destination));
      return bReturn;
    }
    else if (status == CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC)
    {
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "not sending command '%s': destination device '%s' marked as handled by libCEC", ToString(command.opcode),ToString(command.destination));
      return bReturn;
    }
    else if (destinationDevice->IsUnsupportedFeature(command.opcode))
    {
      return true;
    }
  }

  {
    uint8_t iTries(0), iMaxTries(m_iTransmitRetries + 1);
    while (!bReturn && ++iTries <= iMaxTries)
    {
      if ((bReturn = m_processor->Transmit(command, bIsReply)) == true)
      {
#ifdef CEC_DEBUGGING
        LIB_CEC->AddLog(CEC_LOG_DEBUG, "command transmitted");
#endif
        if (bExpectResponse)
        {
          bReturn = m_busDevice->WaitForOpcode(expectedResponse);
          LIB_CEC->AddLog(CEC_LOG_DEBUG, bReturn ? "expected response received (%X: %s)" : "expected response not received (%X: %s)", (int)expectedResponse, ToString(expectedResponse));
        }
      }
    }
  }

  return bReturn;
}

bool CCECCommandHandler::ActivateSource(bool bTransmitDelayedCommandsOnly /* = false */)
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

    // update the power state and menu state
    if (!bTransmitDelayedCommandsOnly)
    {
      m_busDevice->SetPowerStatus(CEC_POWER_STATUS_ON);
      m_busDevice->SetMenuState(CEC_MENU_STATE_ACTIVATED);
    }

    // vendor specific hook
    VendorPreActivateSourceHook();

    // power on the TV
    CCECBusDevice* tv = m_processor->GetDevice(CECDEVICE_TV);
    bool bTvPresent = (tv && tv->GetStatus() == CEC_DEVICE_STATUS_PRESENT);
    bool bActiveSourceFailed(false);
    if (bTvPresent)
      bActiveSourceFailed = !tv->PowerOn(m_busDevice->GetLogicalAddress());
    else
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "TV not present, not sending 'image view on'");

    // check if we're allowed to switch sources
    bool bSourceSwitchAllowed = SourceSwitchAllowed();
    if (!bSourceSwitchAllowed)
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "source switch is currently not allowed by command handler");

    // switch sources (if allowed)
    if (!bActiveSourceFailed && bSourceSwitchAllowed)
    {
      bActiveSourceFailed = !m_busDevice->TransmitActiveSource(false);
      if (bTvPresent && !bActiveSourceFailed)
        bActiveSourceFailed = !m_busDevice->TransmitMenuState(CECDEVICE_TV, false);

      // update the deck status for playback devices
      if (bTvPresent && !bActiveSourceFailed)
      {
        CCECPlaybackDevice *playbackDevice = m_busDevice->AsPlaybackDevice();
        if (playbackDevice && SendDeckStatusUpdateOnActiveSource())
          bActiveSourceFailed = !playbackDevice->TransmitDeckStatus(CECDEVICE_TV, false);
      }

      // update system audio mode for audiosystem devices
      if (bTvPresent && !bActiveSourceFailed)
      {
        CCECAudioSystem* audioDevice = m_busDevice->AsAudioSystem();
        if (audioDevice)
          bActiveSourceFailed = !audioDevice->TransmitSetSystemAudioMode(CECDEVICE_TV, false);
      }
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

void CCECCommandHandler::ScheduleActivateSource(uint64_t iDelay)
{
  CLockObject lock(m_mutex);
  m_iActiveSourcePending = GetTimeMs() + iDelay;
}

void CCECCommandHandler::RequestEmailFromCustomer(const cec_command& command)
{
  bool bInserted(false);
  map<cec_opcode, vector<cec_command> >::iterator it = m_logsRequested.find(command.opcode);
  if (it != m_logsRequested.end())
  {
    for (vector<cec_command>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
    {
      // we already logged this one
      if ((*it2).parameters == command.parameters)
        return;
    }

    it->second.push_back(command);
    bInserted = true;
  }

  if (!bInserted)
  {
    vector<cec_command> commands;
    commands.push_back(command);
    m_logsRequested.insert(make_pair(command.opcode, commands));
  }

  LIB_CEC->AddLog(CEC_LOG_NOTICE, "Unmapped code detected. Please send an email to support@pulse-eight.com with the following details, and if you pressed a key, tell us which one you pressed, and we'll add support for this it.\nCEC command: %s\nVendor ID: %s (%06x)", ToString(command).c_str(), ToString(m_vendorId), m_vendorId);
}


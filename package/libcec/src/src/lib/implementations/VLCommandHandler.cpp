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
#include "VLCommandHandler.h"

#include "lib/devices/CECBusDevice.h"
#include "lib/devices/CECPlaybackDevice.h"
#include "lib/devices/CECTV.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"
#include "lib/CECClient.h"

#define VL_POWER_CHANGE 0x20
#define VL_POWERED_UP   0x00
#define VL_POWERED_DOWN 0x01
#define VL_UNKNOWN1     0x06

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_busDevice->GetProcessor()->GetLib()
#define ToString(p) LIB_CEC->ToString(p)

// wait this amount of ms before trying to switch sources after receiving the message from the TV that it's powered on
#define SOURCE_SWITCH_DELAY_MS 3000

CVLCommandHandler::CVLCommandHandler(CCECBusDevice *busDevice,
                                     int32_t iTransmitTimeout /* = CEC_DEFAULT_TRANSMIT_TIMEOUT */,
                                     int32_t iTransmitWait /* = CEC_DEFAULT_TRANSMIT_WAIT */,
                                     int8_t iTransmitRetries /* = CEC_DEFAULT_TRANSMIT_RETRIES */,
                                     int64_t iActiveSourcePending /* = 0 */) :
    CCECCommandHandler(busDevice, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending),
    m_iPowerUpEventReceived(0),
    m_bCapabilitiesSent(false)
{
  m_vendorId = CEC_VENDOR_PANASONIC;
}

bool CVLCommandHandler::InitHandler(void)
{
  CCECBusDevice *primary = m_processor->GetPrimaryDevice();
  if (primary && primary->GetLogicalAddress() != CECDEVICE_UNREGISTERED)
  {
    /* use the VL commandhandler for the primary device that is handled by libCEC */
    if (m_busDevice->GetLogicalAddress() == CECDEVICE_TV)
    {
      if (primary && m_busDevice->GetLogicalAddress() != primary->GetLogicalAddress())
      {
        libcec_configuration config;
        m_processor->GetPrimaryClient()->GetCurrentConfiguration(config);
        if (config.iDoubleTapTimeoutMs == 0)
        {
          config.iDoubleTapTimeoutMs = CEC_DOUBLE_TAP_TIMEOUT_MS;
          m_processor->GetPrimaryClient()->SetConfiguration(config);
        }

        primary->SetVendorId(CEC_VENDOR_PANASONIC);
        primary->ReplaceHandler(false);
      }

      if (primary->GetType() == CEC_DEVICE_TYPE_RECORDING_DEVICE)
        return m_processor->GetPrimaryClient()->ChangeDeviceType(CEC_DEVICE_TYPE_RECORDING_DEVICE, CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
    }
  }

  return CCECCommandHandler::InitHandler();
}

int CVLCommandHandler::HandleDeviceVendorCommandWithId(const cec_command &command)
{
  if (!m_processor->IsHandledByLibCEC(command.destination))
    return CEC_ABORT_REASON_INVALID_OPERAND;

  if (command.parameters[0] != 0x00 ||
      command.parameters[1] != 0x80 ||
      command.parameters[2] != 0x45)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  if (command.initiator == CECDEVICE_TV &&
      command.parameters.At(3) == VL_UNKNOWN1)
  {
    if (command.parameters.size >= 5 && command.parameters.At(4) == 0x05)
    {
      // set the power up event time
      {
        CLockObject lock(m_mutex);
        if (m_iPowerUpEventReceived == 0)
          m_iPowerUpEventReceived = GetTimeMs();
      }
      // mark the TV as powered on
      m_processor->GetTV()->SetPowerStatus(CEC_POWER_STATUS_ON);

      CCECBusDevice* dev = m_processor->GetPrimaryDevice();
      if (dev && dev->IsActiveSource())
        dev->TransmitActiveSource(false);

      return COMMAND_HANDLED;
    }
  }
  else if (command.initiator == CECDEVICE_TV &&
      command.destination == CECDEVICE_BROADCAST &&
      command.parameters.At(3) == VL_POWER_CHANGE)
  {
    if (command.parameters.At(4) == VL_POWERED_UP)
    {
      // set the power up event time
      {
        CLockObject lock(m_mutex);
        if (m_iPowerUpEventReceived == 0)
          m_iPowerUpEventReceived = GetTimeMs();
      }
      // mark the TV as powered on
      m_processor->GetTV()->SetPowerStatus(CEC_POWER_STATUS_ON);

      // send capabilties
      SendVendorCommandCapabilities(m_processor->GetLogicalAddress(), command.initiator);

      // reactivate the source, so the tv switches channels
      if (m_processor->IsActiveSource(m_processor->GetLogicalAddress()))
        m_processor->GetDevice(m_processor->GetLogicalAddress())->TransmitActiveSource(false);
    }
    else if (command.parameters.At(4) == VL_POWERED_DOWN)
    {
      // reset the power up event time
      {
        CLockObject lock(m_mutex);
        m_iPowerUpEventReceived = 0;
      }
      // mark the TV as powered off
      m_processor->GetTV()->SetPowerStatus(CEC_POWER_STATUS_STANDBY);
    }
    else
      LIB_CEC->AddLog(CEC_LOG_DEBUG, "skipping unknown vendor command");

    return COMMAND_HANDLED;
  }

  return CCECCommandHandler::HandleDeviceVendorCommandWithId(command);
}

bool CVLCommandHandler::PowerUpEventReceived(void)
{
  bool bPowerUpEventReceived(true);

  if (m_busDevice->GetLogicalAddress() != CECDEVICE_TV)
  {
    CCECBusDevice* tv = m_processor->GetTV();
    if (tv && tv->GetStatus() != CEC_DEVICE_STATUS_PRESENT)
      return true;

    // get the status from the TV
    if (tv && tv->GetCurrentVendorId() == CEC_VENDOR_PANASONIC)
    {
      CVLCommandHandler *handler = static_cast<CVLCommandHandler *>(tv->GetHandler());
      bPowerUpEventReceived = handler ? handler->PowerUpEventReceived() : false;
      tv->MarkHandlerReady();
    }
  }
  else
  {
    // get the current status
    {
      CLockObject lock(m_mutex);
      bPowerUpEventReceived = m_iPowerUpEventReceived > 0 &&
                              GetTimeMs() - m_iPowerUpEventReceived > SOURCE_SWITCH_DELAY_MS;
    }

    // if we didn't receive the event, check if the TV is already marked as powered on
    if (!bPowerUpEventReceived && m_busDevice->GetCurrentPowerStatus() == CEC_POWER_STATUS_ON)
    {
      CLockObject lock(m_mutex);
      m_iPowerUpEventReceived = GetTimeMs();
      bPowerUpEventReceived = true;
    }
  }

  return bPowerUpEventReceived;
}

int CVLCommandHandler::HandleStandby(const cec_command &command)
{
  // reset the power up event time
  {
    CLockObject lock(m_mutex);
    m_iPowerUpEventReceived = 0;
    m_bCapabilitiesSent = false;
  }

  return CCECCommandHandler::HandleStandby(command);
}

void CVLCommandHandler::VendorPreActivateSourceHook(void)
{
  bool bTransmit(false);
  {
    CLockObject lock(m_mutex);
    bTransmit = !m_bCapabilitiesSent;
  }
  if (bTransmit)
    SendVendorCommandCapabilities(m_processor->GetLogicalAddress(), CECDEVICE_TV);
}

void CVLCommandHandler::SendVendorCommandCapabilities(const cec_logical_address initiator, const cec_logical_address destination)
{
  if (PowerUpEventReceived())
  {
    cec_command response;
    cec_command::Format(response, initiator, destination, CEC_OPCODE_VENDOR_COMMAND);
    uint8_t iResponseData[] = {0x10, 0x02, 0xFF, 0xFF, 0x00, 0x05, 0x05, 0x45, 0x55, 0x5c, 0x58, 0x32};
    response.PushArray(12, iResponseData);

    if (Transmit(response, false, true))
    {
      CLockObject lock(m_mutex);
      m_bCapabilitiesSent = true;
    }
  }
}

int CVLCommandHandler::HandleVendorCommand(const cec_command &command)
{
  // some vendor command voodoo that will enable more buttons on the remote
  if (command.parameters.size == 3 &&
      command.parameters[0] == 0x10 &&
      command.parameters[1] == 0x01 &&
      m_processor->IsHandledByLibCEC(command.destination))
  {
    // XXX i've seen 0x05 and 0x03 as third param. these probably indicate different types of TVs/capabilities
    // when we feature abort this, then the TV will try the same thing with a vendor command with id
    SendVendorCommandCapabilities(m_processor->GetLogicalAddress(), command.initiator);

    CCECBusDevice* dev = m_processor->GetDevice(command.destination);
    if (dev && dev->IsActiveSource())
      dev->ActivateSource(500);
    return COMMAND_HANDLED;
  }

  return CEC_ABORT_REASON_INVALID_OPERAND;
}

bool CVLCommandHandler::SourceSwitchAllowed(void)
{
  if (!PowerUpEventReceived())
    TransmitRequestPowerStatus(m_processor->GetPrimaryDevice()->GetLogicalAddress(), CECDEVICE_TV, false, false);

  return PowerUpEventReceived();
}

int CVLCommandHandler::HandleSystemAudioModeRequest(const cec_command &command)
{
  if (command.initiator == CECDEVICE_TV)
  {
    // set the power up event time
    {
      CLockObject lock(m_mutex);
      if (m_iPowerUpEventReceived == 0)
        m_iPowerUpEventReceived = GetTimeMs();
    }
    // mark the TV as powered on
    m_processor->GetTV()->SetPowerStatus(CEC_POWER_STATUS_ON);
  }

  return CCECCommandHandler::HandleSystemAudioModeRequest(command);
}

int CVLCommandHandler::HandleReportPowerStatus(const cec_command &command)
{
  if (command.initiator == m_busDevice->GetLogicalAddress() &&
      command.parameters.size == 1 &&
      (cec_power_status)command.parameters[0] == CEC_POWER_STATUS_ON)
  {
    CLockObject lock(m_mutex);
    if (m_iPowerUpEventReceived == 0)
      m_iPowerUpEventReceived = GetTimeMs();
  }

  return CCECCommandHandler::HandleReportPowerStatus(command);
}

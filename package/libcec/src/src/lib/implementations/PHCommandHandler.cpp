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
#include "PHCommandHandler.h"

#include "lib/devices/CECBusDevice.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"
#include "lib/CECClient.h"

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_busDevice->GetProcessor()->GetLib()
#define ToString(p) LIB_CEC->ToString(p)

#define TV_ON_CHECK_TIME_MS 5000

CImageViewOnCheck::~CImageViewOnCheck(void)
{
  StopThread(-1);
  m_event.Broadcast();
  StopThread();
}

void* CImageViewOnCheck::Process(void)
{
  CCECBusDevice* tv = m_handler->m_processor->GetDevice(CECDEVICE_TV);
  cec_power_status status(CEC_POWER_STATUS_UNKNOWN);
  while (status != CEC_POWER_STATUS_ON)
  {
    m_event.Wait(TV_ON_CHECK_TIME_MS);
    if (!IsRunning())
      return NULL;

    status = tv->GetPowerStatus(m_handler->m_busDevice->GetLogicalAddress());

    if (status != CEC_POWER_STATUS_ON &&
        status != CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON)
    {
      CLockObject lock(m_handler->m_mutex);
      tv->OnImageViewOnSent(false);
      m_handler->m_iActiveSourcePending = GetTimeMs();
    }
  }
  return NULL;
}

CPHCommandHandler::CPHCommandHandler(CCECBusDevice *busDevice,
                                     int32_t iTransmitTimeout /* = CEC_DEFAULT_TRANSMIT_TIMEOUT */,
                                     int32_t iTransmitWait /* = CEC_DEFAULT_TRANSMIT_WAIT */,
                                     int8_t iTransmitRetries /* = CEC_DEFAULT_TRANSMIT_RETRIES */,
                                     int64_t iActiveSourcePending /* = 0 */) :
    CCECCommandHandler(busDevice, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending),
    m_iLastKeyCode(CEC_USER_CONTROL_CODE_UNKNOWN)
{
  m_imageViewOnCheck = new CImageViewOnCheck(this);
  m_vendorId = CEC_VENDOR_PHILIPS;
  m_bOPTSendDeckStatusUpdateOnActiveSource = false;
}

CPHCommandHandler::~CPHCommandHandler(void)
{
  delete m_imageViewOnCheck;
}

bool CPHCommandHandler::InitHandler(void)
{
  CCECBusDevice *primary = m_processor->GetPrimaryDevice();
  if (primary && primary->GetLogicalAddress() != CECDEVICE_UNREGISTERED)
  {
    //XXX hack to use this handler for the primary device
    if (m_busDevice->GetLogicalAddress() == CECDEVICE_TV &&
        primary && m_busDevice->GetLogicalAddress() != primary->GetLogicalAddress())
    {
      primary->SetVendorId(CEC_VENDOR_PHILIPS);
      primary->ReplaceHandler(false);
    }
  }

  return CCECCommandHandler::InitHandler();
}

bool CPHCommandHandler::ActivateSource(bool bTransmitDelayedCommandsOnly /* = false */)
{

  CCECBusDevice* tv = m_processor->GetDevice(CECDEVICE_TV);
  if (m_busDevice->IsActiveSource() &&
      m_busDevice->IsHandledByLibCEC() &&
      tv && tv->GetCurrentPowerStatus() != CEC_POWER_STATUS_ON &&
      !bTransmitDelayedCommandsOnly)
  {
    // tv sometimes ignores image view on. check the power status of the tv in 5 seconds, and retry when it failed to power up
    if (m_imageViewOnCheck && !m_imageViewOnCheck->IsRunning())
      return m_imageViewOnCheck->CreateThread(false);
  }

  return CCECCommandHandler::ActivateSource(bTransmitDelayedCommandsOnly);
}

int CPHCommandHandler::HandleUserControlPressed(const cec_command& command)
{
  // TV sometimes keeps sending key presses without releases
  if (m_iLastKeyCode == command.parameters[0])
    return COMMAND_HANDLED;

  m_iLastKeyCode = command.parameters[0];

  return CCECCommandHandler::HandleUserControlPressed(command);
}

int CPHCommandHandler::HandleUserControlRelease(const cec_command& command)
{
  m_iLastKeyCode = CEC_USER_CONTROL_CODE_UNKNOWN;

  return CCECCommandHandler::HandleUserControlRelease(command);
}

int CPHCommandHandler::HandleDeviceVendorId(const cec_command& command)
{
  m_busDevice->SetPowerStatus(CEC_POWER_STATUS_ON);
  return CCECCommandHandler::HandleDeviceVendorId(command);
}

bool CPHCommandHandler::TransmitVendorID(const cec_logical_address iInitiator, const cec_logical_address iDestination, uint64_t UNUSED(iVendorId), bool bIsReply)
{
  // XXX hack around the hack in CPHCommandHandler::InitHandler
  return CCECCommandHandler::TransmitVendorID(iInitiator, iDestination, CEC_VENDOR_PULSE_EIGHT, bIsReply);
}

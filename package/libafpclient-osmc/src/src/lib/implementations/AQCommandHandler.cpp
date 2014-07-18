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
#include "AQCommandHandler.h"

#include "lib/devices/CECBusDevice.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"
#include "lib/CECClient.h"

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_busDevice->GetProcessor()->GetLib()
#define ToString(p) LIB_CEC->ToString(p)

CAQCommandHandler::CAQCommandHandler(CCECBusDevice *busDevice,
                                     int32_t iTransmitTimeout /* = CEC_DEFAULT_TRANSMIT_TIMEOUT */,
                                     int32_t iTransmitWait /* = CEC_DEFAULT_TRANSMIT_WAIT */,
                                     int8_t iTransmitRetries /* = CEC_DEFAULT_TRANSMIT_RETRIES */,
                                     int64_t iActiveSourcePending /* = 0 */) :
    CCECCommandHandler(busDevice, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending),
    m_powerOnCheck(NULL)
{
  m_vendorId = CEC_VENDOR_SHARP;
}

CAQCommandHandler::~CAQCommandHandler(void)
{
  delete m_powerOnCheck;
}

bool CAQCommandHandler::PowerOn(const cec_logical_address iInitiator, const cec_logical_address iDestination)
{
  bool bCheck(false);
  bool bRetval(false);
  if (m_busDevice->GetCurrentPowerStatus() != CEC_POWER_STATUS_ON && (!m_powerOnCheck || !m_powerOnCheck->IsRunning()))
    bCheck = true;
  bRetval = CCECCommandHandler::PowerOn(iInitiator, iDestination);

  if (bRetval && bCheck)
  {
    if (!m_powerOnCheck)
      m_powerOnCheck = new CAQPowerStatusCheck(this, iInitiator, iDestination);
    if (m_powerOnCheck)
      m_powerOnCheck->CreateThread();
  }

  return bRetval;
}

void* CAQPowerStatusCheck::Process(void)
{
  // sleep for 2 seconds and query the power status
  Sleep(2000);
  if (m_handler->m_busDevice->GetProcessor()->GetDevice(m_iDestination)->GetPowerStatus(m_iInitiator, true) == CEC_POWER_STATUS_STANDBY)
    m_handler->m_busDevice->GetProcessor()->GetLib()->AddLog(CEC_LOG_WARNING, "AQUOS LINK 'auto power on' is disabled, which prevents the TV from being powered on. To correct this, press the menu button on your remote, go to 'link operation' -> 'AQUOS LINK setup' -> 'Auto power on' and set it to 'On'");
  return NULL;
}

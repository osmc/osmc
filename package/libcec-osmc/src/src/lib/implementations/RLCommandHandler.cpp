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
#include "RLCommandHandler.h"

#include "lib/platform/util/timeutils.h"
#include "lib/devices/CECBusDevice.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"
#include "lib/CECClient.h"

using namespace CEC;
using namespace PLATFORM;

#define RL_KEY_TOP_MENU           0x10
#define RL_KEY_DVD_MENU           0x11

CRLCommandHandler::CRLCommandHandler(CCECBusDevice *busDevice,
                                     int32_t iTransmitTimeout /* = CEC_DEFAULT_TRANSMIT_TIMEOUT */,
                                     int32_t iTransmitWait /* = CEC_DEFAULT_TRANSMIT_WAIT */,
                                     int8_t iTransmitRetries /* = CEC_DEFAULT_TRANSMIT_RETRIES */,
                                     int64_t iActiveSourcePending /* = 0 */) :
    CCECCommandHandler(busDevice, iTransmitTimeout, iTransmitWait, iTransmitRetries, iActiveSourcePending)
{
  m_vendorId = CEC_VENDOR_TOSHIBA;
}

bool CRLCommandHandler::InitHandler(void)
{
  if (m_bHandlerInited)
    return true;
  m_bHandlerInited = true;

  if (m_busDevice->GetLogicalAddress() != CECDEVICE_TV)
    return true;

  CCECBusDevice *primary = m_processor->GetPrimaryDevice();
  if (primary && primary->GetLogicalAddress() != CECDEVICE_UNREGISTERED)
  {
    /* imitate Toshiba devices */
    if (m_busDevice->GetLogicalAddress() != primary->GetLogicalAddress())
    {
      primary->SetVendorId(CEC_VENDOR_TOSHIBA);
      primary->ReplaceHandler(false);
    }

    if (m_busDevice->GetLogicalAddress() == CECDEVICE_TV)
    {
      /* send the vendor id */
      primary->TransmitVendorID(CECDEVICE_BROADCAST, false, false);
    }
  }

  return true;
}

int CRLCommandHandler::HandleDeviceVendorCommandWithId(const cec_command &command)
{
  if (!m_processor->IsHandledByLibCEC(command.destination))
    return CEC_ABORT_REASON_INVALID_OPERAND;

  if (command.parameters.size < 4)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  // check whether the vendor id matches
  if (command.parameters[0] != 0x00 ||
      command.parameters[1] != 0x00 ||
      command.parameters[2] != 0x39)
    return CEC_ABORT_REASON_INVALID_OPERAND;

  bool bHandled(false);
  CCECClient* client = m_processor->GetClient(command.destination);
  if (client)
  {
    switch (command.parameters[3])
    {
    // user control pressed
    case CEC_OPCODE_USER_CONTROL_PRESSED:
      if (command.parameters.size == 5)
      {
        bHandled = true;
        switch (command.parameters[4])
        {
        // top menu -> root menu
        case RL_KEY_TOP_MENU:
          client->SetCurrentButton(CEC_USER_CONTROL_CODE_ROOT_MENU);
          break;
        // dvd menu -> contents menu
        case RL_KEY_DVD_MENU:
          client->SetCurrentButton(CEC_USER_CONTROL_CODE_CONTENTS_MENU);
          break;
        default:
          bHandled = false;
          break;
        }
      }
      break;
    // user control released
    case CEC_OPCODE_USER_CONTROL_RELEASE:
      client->AddKey();
      bHandled = true;
      break;
    default:
      break;
    }
  }

  return bHandled ?
      COMMAND_HANDLED :
      CCECCommandHandler::HandleDeviceVendorCommandWithId(command);
}

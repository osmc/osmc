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
#include "CECPlaybackDevice.h"

#include "lib/implementations/CECCommandHandler.h"
#include "lib/CECProcessor.h"
#include "lib/LibCEC.h"
#include "lib/CECTypeUtils.h"

using namespace CEC;
using namespace PLATFORM;

#define ToString(p) CCECTypeUtils::ToString(p)

CCECPlaybackDevice::CCECPlaybackDevice(CCECProcessor *processor, cec_logical_address address, uint16_t iPhysicalAddress /* = CEC_INVALID_PHYSICAL_ADDRESS */) :
    CCECBusDevice(processor, address, iPhysicalAddress),
    m_deckStatus(CEC_DECK_INFO_STOP),
    m_deckControlMode(CEC_DECK_CONTROL_MODE_STOP)
{
  m_type = CEC_DEVICE_TYPE_PLAYBACK_DEVICE;
}

cec_deck_info CCECPlaybackDevice::GetDeckStatus(const cec_logical_address UNUSED(initiator))
{
  CLockObject lock(m_mutex);
  return m_deckStatus;
}

void CCECPlaybackDevice::SetDeckStatus(cec_deck_info deckStatus)
{
  CLockObject lock(m_mutex);
  if (m_deckStatus != deckStatus)
  {
    m_processor->GetLib()->AddLog(CEC_LOG_DEBUG, ">> %s (%X): deck status changed from '%s' to '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_deckStatus), ToString(deckStatus));
    m_deckStatus = deckStatus;
  }
}

cec_deck_control_mode CCECPlaybackDevice::GetDeckControlMode(const cec_logical_address UNUSED(initiator))
{
  CLockObject lock(m_mutex);
  return m_deckControlMode;
}

void CCECPlaybackDevice::SetDeckControlMode(cec_deck_control_mode mode)
{
  CLockObject lock(m_mutex);
  if (m_deckControlMode != mode)
  {
    m_processor->GetLib()->AddLog(CEC_LOG_DEBUG, ">> %s (%X): deck control mode changed from '%s' to '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_deckControlMode), ToString(mode));
    m_deckControlMode = mode;
  }
}

bool CCECPlaybackDevice::TransmitDeckStatus(cec_logical_address dest, bool bIsReply)
{
  cec_deck_info state;
  {
    CLockObject lock(m_mutex);
    m_processor->GetLib()->AddLog(CEC_LOG_DEBUG, "<< %s (%X) -> %s (%X): deck status '%s'", GetLogicalAddressName(), m_iLogicalAddress, ToString(dest), dest, ToString(m_deckStatus));
    state = m_deckStatus;
  }

  return m_handler->TransmitDeckStatus(m_iLogicalAddress, dest, state, bIsReply);
}

void CCECPlaybackDevice::ResetDeviceStatus(void)
{
  CLockObject lock(m_mutex);
  m_deckStatus      = CEC_DECK_INFO_STOP;
  m_deckControlMode = CEC_DECK_CONTROL_MODE_STOP;
  CCECBusDevice::ResetDeviceStatus();
}

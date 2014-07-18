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
#include "CECAudioSystem.h"

#include "lib/CECProcessor.h"
#include "lib/implementations/CECCommandHandler.h"
#include "lib/LibCEC.h"
#include "lib/CECTypeUtils.h"

using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC     m_processor->GetLib()
#define ToString(p) CCECTypeUtils::ToString(p)

CCECAudioSystem::CCECAudioSystem(CCECProcessor *processor, cec_logical_address address, uint16_t iPhysicalAddress /* = CEC_INVALID_PHYSICAL_ADDRESS */) :
    CCECBusDevice(processor, address, iPhysicalAddress),
    m_systemAudioStatus(CEC_SYSTEM_AUDIO_STATUS_ON),
    m_audioStatus(CEC_AUDIO_VOLUME_STATUS_UNKNOWN)
{
  m_type = CEC_DEVICE_TYPE_AUDIO_SYSTEM;
}

bool CCECAudioSystem::SetAudioStatus(uint8_t status)
{
  CLockObject lock(m_mutex);
  if (m_audioStatus != status)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, ">> %s (%X): audio status changed from %2x to %2x", GetLogicalAddressName(), m_iLogicalAddress, m_audioStatus, status);
    m_audioStatus = status;
    return true;
  }

  return false;
}

bool CCECAudioSystem::SetSystemAudioModeStatus(const cec_system_audio_status mode)
{
  CLockObject lock(m_mutex);
  if (m_systemAudioStatus != mode)
  {
    LIB_CEC->AddLog(CEC_LOG_DEBUG, ">> %s (%X): system audio mode status changed from %s to %s", GetLogicalAddressName(), m_iLogicalAddress, ToString(m_systemAudioStatus), ToString(mode));
    m_systemAudioStatus = mode;
    return true;
  }

  return false;
}

bool CCECAudioSystem::TransmitAudioStatus(cec_logical_address dest, bool bIsReply)
{
  uint8_t state;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %x -> %x: audio status '%2x'", m_iLogicalAddress, dest, m_audioStatus);
    state = m_audioStatus;
  }

  return m_handler->TransmitAudioStatus(m_iLogicalAddress, dest, state, bIsReply);
}

bool CCECAudioSystem::TransmitSetSystemAudioMode(cec_logical_address dest, bool bIsReply)
{
  cec_system_audio_status state;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %x -> %x: set system audio mode '%2x'", m_iLogicalAddress, dest, m_audioStatus);
    state = m_systemAudioStatus;
  }

  return m_handler->TransmitSetSystemAudioMode(m_iLogicalAddress, dest, state, bIsReply);
}

bool CCECAudioSystem::TransmitSystemAudioModeStatus(cec_logical_address dest, bool bIsReply)
{
  cec_system_audio_status state;
  {
    CLockObject lock(m_mutex);
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< %x -> %x: system audio mode '%s'", m_iLogicalAddress, dest, ToString(m_systemAudioStatus));
    state = m_systemAudioStatus;
  }

  return m_handler->TransmitSystemAudioModeStatus(m_iLogicalAddress, dest, state, bIsReply);
}

uint8_t CCECAudioSystem::VolumeUp(const cec_logical_address source, bool bSendRelease /* = true */)
{
  TransmitKeypress(source, CEC_USER_CONTROL_CODE_VOLUME_UP);
  if (bSendRelease)
    TransmitKeyRelease(source);

  CLockObject lock(m_mutex);
  return m_audioStatus;
}

uint8_t CCECAudioSystem::VolumeDown(const cec_logical_address source, bool bSendRelease /* = true */)
{
  TransmitKeypress(source, CEC_USER_CONTROL_CODE_VOLUME_DOWN);
  if (bSendRelease)
    TransmitKeyRelease(source);

  CLockObject lock(m_mutex);
  return m_audioStatus;
}

uint8_t CCECAudioSystem::MuteAudio(const cec_logical_address source)
{
  TransmitKeypress(source, CEC_USER_CONTROL_CODE_MUTE);
  TransmitKeyRelease(source);

  return GetAudioStatus(source, true);
}

bool CCECAudioSystem::RequestAudioStatus(const cec_logical_address initiator, bool bWaitForResponse /* = true */)
{
  bool bReturn(false);

  if (!IsHandledByLibCEC() &&
      !IsUnsupportedFeature(CEC_OPCODE_GIVE_AUDIO_STATUS))
  {
    MarkBusy();
    LIB_CEC->AddLog(CEC_LOG_DEBUG, "<< requesting audio status of '%s' (%X)", GetLogicalAddressName(), m_iLogicalAddress);
    bReturn = m_handler->TransmitRequestAudioStatus(initiator, m_iLogicalAddress, bWaitForResponse);
    MarkReady();
  }
  return bReturn;
}

uint8_t CCECAudioSystem::GetAudioStatus(const cec_logical_address initiator, bool bUpdate /* = false */)
{
  bool bIsPresent(GetStatus() == CEC_DEVICE_STATUS_PRESENT);
  bool bRequestUpdate(false);
  {
    CLockObject lock(m_mutex);
    bRequestUpdate = bIsPresent &&
        (bUpdate || m_audioStatus == CEC_AUDIO_VOLUME_STATUS_UNKNOWN);
  }

  if (bRequestUpdate)
  {
    CheckVendorIdRequested(initiator);
    RequestAudioStatus(initiator);
  }

  CLockObject lock(m_mutex);
  return m_audioStatus;
}

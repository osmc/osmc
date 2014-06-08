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
#include "USBCECAdapterMessage.h"

#include "lib/LibCEC.h"
#include "lib/platform/util/StdString.h"

using namespace CEC;
using namespace PLATFORM;

CCECAdapterMessage::CCECAdapterMessage(void)
{
  Clear();
}

CCECAdapterMessage::CCECAdapterMessage(const cec_command &command, uint8_t iLineTimeout /* = 3 */)
{
  Clear();

  //set ack polarity to high when transmitting to the broadcast address
  //set ack polarity low when transmitting to any other address
  PushBack(MSGSTART);
  PushEscaped(MSGCODE_TRANSMIT_ACK_POLARITY);
  if (command.destination == CECDEVICE_BROADCAST)
    PushEscaped(CEC_TRUE);
  else
    PushEscaped(CEC_FALSE);
  PushBack(MSGEND);

  // add source and destination
  PushBack(MSGSTART);
  PushEscaped(command.opcode_set == 0 ? (uint8_t)MSGCODE_TRANSMIT_EOM : (uint8_t)MSGCODE_TRANSMIT);
  PushBack(((uint8_t)command.initiator << 4) + (uint8_t)command.destination);
  PushBack(MSGEND);

  // add opcode
  if (command.opcode_set == 1)
  {
    PushBack(MSGSTART);
    PushEscaped(command.parameters.IsEmpty() ? (uint8_t)MSGCODE_TRANSMIT_EOM : (uint8_t)MSGCODE_TRANSMIT);
    PushBack((uint8_t) command.opcode);
    PushBack(MSGEND);

    // add parameters
    for (int8_t iPtr = 0; iPtr < command.parameters.size; iPtr++)
    {
      PushBack(MSGSTART);

      if (iPtr == command.parameters.size - 1)
        PushEscaped( MSGCODE_TRANSMIT_EOM);
      else
        PushEscaped(MSGCODE_TRANSMIT);

      PushEscaped(command.parameters[iPtr]);

      PushBack(MSGEND);
    }
  }

  // set timeout
  transmit_timeout = command.transmit_timeout;

  lineTimeout = iLineTimeout;
}

std::string CCECAdapterMessage::ToString(void) const
{
  CStdString strMsg;
  if (Size() == 0)
  {
    strMsg = "empty message";
  }
  else
  {
    strMsg = ToString(Message());

    switch (Message())
    {
    case MSGCODE_TIMEOUT_ERROR:
    case MSGCODE_HIGH_ERROR:
    case MSGCODE_LOW_ERROR:
      {
        uint32_t iLine = (Size() >= 4) ? (At(2) << 8) | At(3) : 0;
        uint32_t iTime = (Size() >= 8) ? (At(4) << 24) | (At(5) << 16) | (At(6) << 8) | At(7) : 0;
        strMsg.AppendFormat(" line:%u", iLine);
        strMsg.AppendFormat(" time:%u", iTime);
      }
      break;
    case MSGCODE_FRAME_START:
      if (Size() >= 3)
        strMsg.AppendFormat(" initiator:%1x destination:%1x ack:%s %s", Initiator(), Destination(), IsACK() ? "high" : "low", IsEOM() ? "eom" : "");
      break;
    case MSGCODE_FRAME_DATA:
      if (Size() >= 3)
        strMsg.AppendFormat(" %02x %s", At(2), IsEOM() ? "eom" : "");
      break;
    default:
      if (Size() >= 2 && (Message() == MSGCODE_COMMAND_ACCEPTED || Message() == MSGCODE_COMMAND_REJECTED))
        strMsg.AppendFormat(": %s", ToString((cec_adapter_messagecode)At(2)));
      else
      {
        for (uint8_t iPtr = 2; iPtr < Size(); iPtr++)
          if (At(iPtr) != MSGEND)
            strMsg.AppendFormat(" %02x", At(iPtr));
      }
      break;
    }
  }

  return std::string(strMsg.c_str());
}

const char *CCECAdapterMessage::ToString(cec_adapter_messagecode msgCode)
{
  switch (msgCode)
  {
  case MSGCODE_NOTHING:
    return "NOTHING";
  case MSGCODE_PING:
    return "PING";
  case MSGCODE_TIMEOUT_ERROR:
    return "TIMEOUT";
  case MSGCODE_HIGH_ERROR:
    return "HIGH_ERROR";
  case MSGCODE_LOW_ERROR:
    return "LOW_ERROR";
  case MSGCODE_FRAME_START:
    return "FRAME_START";
  case MSGCODE_FRAME_DATA:
    return "FRAME_DATA";
  case MSGCODE_RECEIVE_FAILED:
    return "RECEIVE_FAILED";
  case MSGCODE_COMMAND_ACCEPTED:
    return "COMMAND_ACCEPTED";
  case MSGCODE_COMMAND_REJECTED:
    return "COMMAND_REJECTED";
  case MSGCODE_SET_ACK_MASK:
    return "SET_ACK_MASK";
  case MSGCODE_TRANSMIT:
    return "TRANSMIT";
  case MSGCODE_TRANSMIT_EOM:
    return "TRANSMIT_EOM";
  case MSGCODE_TRANSMIT_IDLETIME:
    return "TRANSMIT_IDLETIME";
  case MSGCODE_TRANSMIT_ACK_POLARITY:
    return "CEC transmission";
  case MSGCODE_TRANSMIT_LINE_TIMEOUT:
    return "TRANSMIT_LINE_TIMEOUT";
  case MSGCODE_TRANSMIT_SUCCEEDED:
    return "TRANSMIT_SUCCEEDED";
  case MSGCODE_TRANSMIT_FAILED_LINE:
    return "TRANSMIT_FAILED_LINE";
  case MSGCODE_TRANSMIT_FAILED_ACK:
    return "TRANSMIT_FAILED_ACK";
  case MSGCODE_TRANSMIT_FAILED_TIMEOUT_DATA:
    return "TRANSMIT_FAILED_TIMEOUT_DATA";
  case MSGCODE_TRANSMIT_FAILED_TIMEOUT_LINE:
    return "TRANSMIT_FAILED_TIMEOUT_LINE";
  case MSGCODE_FIRMWARE_VERSION:
    return "FIRMWARE_VERSION";
  case MSGCODE_START_BOOTLOADER:
    return "START_BOOTLOADER";
  case MSGCODE_FRAME_EOM:
    return "FRAME_EOM";
  case MSGCODE_FRAME_ACK:
    return "FRAME_ACK";
  case MSGCODE_GET_BUILDDATE:
    return "GET_BUILDDATE";
  case MSGCODE_SET_CONTROLLED:
    return "SET_CONTROLLED";
  case MSGCODE_GET_AUTO_ENABLED:
    return "GET_AUTO_ENABLED";
  case MSGCODE_SET_AUTO_ENABLED:
    return "SET_AUTO_ENABLED";
  case MSGCODE_GET_DEFAULT_LOGICAL_ADDRESS:
    return "GET_DEFAULT_LOGICAL_ADDRESS";
  case MSGCODE_SET_DEFAULT_LOGICAL_ADDRESS:
    return "SET_DEFAULT_LOGICAL_ADDRESS";
  case MSGCODE_GET_LOGICAL_ADDRESS_MASK:
    return "GET_LOGICAL_ADDRESS_MASK";
  case MSGCODE_SET_LOGICAL_ADDRESS_MASK:
    return "SET_LOGICAL_ADDRESS_MASK";
  case MSGCODE_GET_PHYSICAL_ADDRESS:
    return "GET_PHYSICAL_ADDRESS";
  case MSGCODE_SET_PHYSICAL_ADDRESS:
    return "SET_PHYSICAL_ADDRESS";
  case MSGCODE_GET_DEVICE_TYPE:
    return "GET_DEVICE_TYPE";
  case MSGCODE_SET_DEVICE_TYPE:
    return "SET_DEVICE_TYPE";
  case MSGCODE_GET_HDMI_VERSION:
    return "GET_HDMI_VERSION";
  case MSGCODE_SET_HDMI_VERSION:
    return "SET_HDMI_VERSION";
  case MSGCODE_GET_OSD_NAME:
    return "GET_OSD_NAME";
  case MSGCODE_SET_OSD_NAME:
    return "SET_OSD_NAME";
  case MSGCODE_WRITE_EEPROM:
    return "WRITE_EEPROM";
  case MSGCODE_GET_ADAPTER_TYPE:
    return "GET_ADAPTER_TYPE";
  default:
    break;
  }

  return "unknown";
}

uint8_t CCECAdapterMessage::operator[](uint8_t pos) const
{
  return pos < packet.size ? packet[pos] : 0;
}

uint8_t CCECAdapterMessage::At(uint8_t pos) const
{
  return pos < packet.size ? packet[pos] : 0;
}

uint8_t CCECAdapterMessage::Size(void) const
{
  return packet.size;
}

bool CCECAdapterMessage::IsEmpty(void) const
{
  return packet.IsEmpty();
}

void CCECAdapterMessage::Clear(void)
{
  state               = ADAPTER_MESSAGE_STATE_UNKNOWN;
  transmit_timeout    = CEC_DEFAULT_TRANSMIT_TIMEOUT;
  response.Clear();
  packet.Clear();
  lineTimeout         = 3;
  bNextByteIsEscaped  = false;
  bFireAndForget      = false;
}

void CCECAdapterMessage::Shift(uint8_t iShiftBy)
{
  packet.Shift(iShiftBy);
}

void CCECAdapterMessage::Append(CCECAdapterMessage &data)
{
  Append(data.packet);
}

void CCECAdapterMessage::Append(cec_datapacket &data)
{
  for (uint8_t iPtr = 0; iPtr < data.size; iPtr++)
    PushBack(data[iPtr]);
}

void CCECAdapterMessage::PushBack(uint8_t byte)
{
  packet.PushBack(byte);
}

void CCECAdapterMessage::PushEscaped(uint8_t byte)
{
  if (byte >= MSGESC)
  {
    PushBack(MSGESC);
    PushBack(byte - ESCOFFSET);
  }
  else
  {
    PushBack(byte);
  }
}

bool CCECAdapterMessage::PushReceivedByte(uint8_t byte)
{
  if (byte == MSGSTART)
  {
    if (HasStartMessage())
    {
      //TODO CLibCEC::AddLog(CEC_LOG_WARNING, "received MSGSTART before MSGEND, removing previous buffer contents");
      Clear();
    }
    PushBack(byte);
  }
  else
  {
    if (bNextByteIsEscaped)
    {
      PushBack(byte + (uint8_t)ESCOFFSET);
      bNextByteIsEscaped = false;
    }
    else if (byte == MSGESC)
      bNextByteIsEscaped = true;
    else
      PushBack(byte);
  }

  return byte == MSGEND;
}

cec_adapter_messagecode CCECAdapterMessage::Message(void) const
{
  return packet.size >= 2 ?
      (cec_adapter_messagecode) (packet.At(1) & ~(MSGCODE_FRAME_EOM | MSGCODE_FRAME_ACK)) :
      MSGCODE_NOTHING;
}

cec_adapter_messagecode CCECAdapterMessage::ResponseTo(void) const
{
  return packet.size >= 3 ?
      (cec_adapter_messagecode) (packet.At(2) & ~(MSGCODE_FRAME_EOM | MSGCODE_FRAME_ACK)) :
      MSGCODE_NOTHING;
}

bool CCECAdapterMessage::IsTranmission(void) const
{
  cec_adapter_messagecode msgCode = Message();
  return msgCode == MSGCODE_FRAME_ACK ||
      msgCode == MSGCODE_FRAME_DATA ||
      msgCode == MSGCODE_FRAME_EOM ||
      msgCode == MSGCODE_FRAME_START ||
      msgCode == MSGCODE_HIGH_ERROR ||
      msgCode == MSGCODE_LOW_ERROR ||
      msgCode == MSGCODE_RECEIVE_FAILED ||
      msgCode == MSGCODE_TRANSMIT_ACK_POLARITY ||
      msgCode == MSGCODE_TRANSMIT_EOM ||
      msgCode == MSGCODE_TRANSMIT_FAILED_ACK ||
      msgCode == MSGCODE_TRANSMIT_FAILED_LINE ||
      msgCode == MSGCODE_TRANSMIT_FAILED_TIMEOUT_DATA ||
      msgCode == MSGCODE_TRANSMIT_FAILED_TIMEOUT_LINE ||
      msgCode == MSGCODE_TRANSMIT_LINE_TIMEOUT ||
      msgCode == MSGCODE_TRANSMIT_SUCCEEDED;
}

bool CCECAdapterMessage::IsEOM(void) const
{
  return packet.size >= 2 ?
      (packet.At(1) & MSGCODE_FRAME_EOM) != 0 :
      false;
}

bool CCECAdapterMessage::IsACK(void) const
{
  return packet.size >= 2 ?
      (packet.At(1) & MSGCODE_FRAME_ACK) != 0 :
      false;
}

bool CCECAdapterMessage::MessageCodeIsError(const cec_adapter_messagecode code)
{
  return (code == MSGCODE_HIGH_ERROR ||
           code == MSGCODE_LOW_ERROR ||
           code == MSGCODE_RECEIVE_FAILED ||
           code == MSGCODE_COMMAND_REJECTED ||
           code == MSGCODE_TRANSMIT_LINE_TIMEOUT ||
           code == MSGCODE_TRANSMIT_FAILED_LINE ||
           code == MSGCODE_TRANSMIT_FAILED_ACK ||
           code == MSGCODE_TRANSMIT_FAILED_TIMEOUT_DATA ||
           code == MSGCODE_TRANSMIT_FAILED_TIMEOUT_LINE);
}

bool CCECAdapterMessage::IsError(void) const
{
  return MessageCodeIsError(Message());
}

bool CCECAdapterMessage::ReplyIsError(void) const
{
  return MessageCodeIsError(Reply());
}

bool CCECAdapterMessage::NeedsRetry(void) const
{
  return Reply() == MSGCODE_NOTHING ||
         Reply() == MSGCODE_RECEIVE_FAILED ||
         Reply() == MSGCODE_TIMEOUT_ERROR ||
         Reply() == MSGCODE_TRANSMIT_FAILED_LINE ||
         Reply() == MSGCODE_TRANSMIT_FAILED_TIMEOUT_DATA ||
         Reply() == MSGCODE_TRANSMIT_FAILED_TIMEOUT_LINE ||
         Reply() == MSGCODE_TRANSMIT_LINE_TIMEOUT;
}

cec_logical_address CCECAdapterMessage::Initiator(void) const
{
  return packet.size >= 3 ?
      (cec_logical_address) (packet.At(2) >> 4) :
      CECDEVICE_UNKNOWN;
}

cec_logical_address CCECAdapterMessage::Destination(void) const
{
  return packet.size >= 3 ?
      (cec_logical_address) (packet.At(2) & 0xF) :
      CECDEVICE_UNKNOWN;
}

bool CCECAdapterMessage::HasStartMessage(void) const
{
  return packet.size >= 1 && packet.At(0) == MSGSTART;
}

bool CCECAdapterMessage::PushToCecCommand(cec_command &command) const
{
  // empty message
  if (IsEmpty())
    return false;

  cec_adapter_messagecode msgCode = Message();
  if (msgCode == MSGCODE_FRAME_START)
  {
    command.Clear();
    if (Size() >= 3)
    {
      command.initiator   = Initiator();
      command.destination = Destination();
      command.ack         = IsACK();
      command.eom         = IsEOM();
    }
    return IsEOM() && !IsError();
  }
  else if (msgCode == MSGCODE_FRAME_DATA)
  {
    if (Size() >= 3)
    {
      command.PushBack(At(2));
      command.eom = IsEOM();
    }
    return IsEOM() && !IsError();
  }

  return false;
}

cec_adapter_messagecode CCECAdapterMessage::Reply(void) const
{
  return response.size >= 2 ?
      (cec_adapter_messagecode) (response.At(1) & ~(MSGCODE_FRAME_EOM | MSGCODE_FRAME_ACK)) :
      MSGCODE_NOTHING;
}

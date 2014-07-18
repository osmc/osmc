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
#include "USBCECAdapterMessageQueue.h"

#include "USBCECAdapterCommunication.h"
#include "USBCECAdapterMessage.h"
#include "lib/platform/sockets/socket.h"
#include "lib/LibCEC.h"
#include "lib/platform/util/StdString.h"

using namespace CEC;
using namespace PLATFORM;
using namespace std;

#define MESSAGE_QUEUE_SIGNAL_WAIT_TIME 1000

CCECAdapterMessageQueueEntry::CCECAdapterMessageQueueEntry(CCECAdapterMessageQueue *queue, CCECAdapterMessage *message) :
    m_queue(queue),
    m_message(message),
    m_iPacketsLeft(message->IsTranmission() ? message->Size() / 4 : 1),
    m_bSucceeded(false),
    m_bWaiting(true),
    m_queueTimeout(message->transmit_timeout) {}

CCECAdapterMessageQueueEntry::~CCECAdapterMessageQueueEntry(void) { }

void CCECAdapterMessageQueueEntry::Broadcast(void)
{
  CLockObject lock(m_mutex);
  m_condition.Broadcast();
}

bool CCECAdapterMessageQueueEntry::MessageReceived(const CCECAdapterMessage &message)
{
  bool bHandled(false);

  if (IsResponse(message))
  {
    switch (message.Message())
    {
    case MSGCODE_COMMAND_ACCEPTED:
      bHandled = MessageReceivedCommandAccepted(message);
      break;
    case MSGCODE_TRANSMIT_SUCCEEDED:
      bHandled = MessageReceivedTransmitSucceeded(message);
      break;
    default:
      bHandled = MessageReceivedResponse(message);
      break;
    }
  }

  return bHandled;
}

void CCECAdapterMessageQueueEntry::Signal(void)
{
  CLockObject lock(m_mutex);
  m_bSucceeded = true;
  m_condition.Signal();
}

bool CCECAdapterMessageQueueEntry::Wait(uint32_t iTimeout)
{
  bool bReturn(false);
  /* wait until we receive a signal when the tranmission succeeded */
  {
    CLockObject lock(m_mutex);
    bReturn = m_bSucceeded ? true : m_condition.Wait(m_mutex, m_bSucceeded, iTimeout);
    m_bWaiting = false;
  }
  return bReturn;
}

bool CCECAdapterMessageQueueEntry::IsWaiting(void)
{
  CLockObject lock(m_mutex);
  return m_bWaiting;
}

cec_adapter_messagecode CCECAdapterMessageQueueEntry::MessageCode(void)
{
  return m_message->Message();
}

bool CCECAdapterMessageQueueEntry::IsResponseOld(const CCECAdapterMessage &msg)
{
  cec_adapter_messagecode msgCode = msg.Message();

  return msgCode == MessageCode() ||
         msgCode == MSGCODE_COMMAND_ACCEPTED ||
         msgCode == MSGCODE_COMMAND_REJECTED ||
         (m_message->IsTranmission() && (msgCode == MSGCODE_TIMEOUT_ERROR ||
             msgCode == MSGCODE_HIGH_ERROR ||
             msgCode == MSGCODE_LOW_ERROR ||
             msgCode == MSGCODE_RECEIVE_FAILED ||
             msgCode == MSGCODE_TRANSMIT_FAILED_LINE ||
             msgCode == MSGCODE_TRANSMIT_FAILED_ACK ||
             msgCode == MSGCODE_TRANSMIT_FAILED_TIMEOUT_DATA ||
             msgCode == MSGCODE_TRANSMIT_FAILED_TIMEOUT_LINE ||
             msgCode == MSGCODE_TRANSMIT_SUCCEEDED));
}

bool CCECAdapterMessageQueueEntry::IsResponse(const CCECAdapterMessage &msg)
{
  if (m_message->state == ADAPTER_MESSAGE_STATE_SENT_ACKED)
    return false;

  cec_adapter_messagecode thisMsgCode = m_message->Message();
  cec_adapter_messagecode msgCode = msg.Message();
  cec_adapter_messagecode msgResponse = msg.ResponseTo();

  // msgcode matches, always a response
  if (msgCode == MessageCode())
    return true;

  if (!ProvidesExtendedResponse())
    return IsResponseOld(msg);

  // response without a msgcode
  if (msgResponse == MSGCODE_NOTHING)
    return false;

  // commands that only repond with accepted/rejected
  if (thisMsgCode == MSGCODE_PING ||
      thisMsgCode == MSGCODE_SET_ACK_MASK ||
      thisMsgCode == MSGCODE_SET_CONTROLLED ||
      thisMsgCode == MSGCODE_SET_AUTO_ENABLED ||
      thisMsgCode == MSGCODE_SET_DEFAULT_LOGICAL_ADDRESS ||
      thisMsgCode == MSGCODE_SET_LOGICAL_ADDRESS_MASK ||
      thisMsgCode == MSGCODE_SET_PHYSICAL_ADDRESS ||
      thisMsgCode == MSGCODE_SET_DEVICE_TYPE ||
      thisMsgCode == MSGCODE_SET_HDMI_VERSION ||
      thisMsgCode == MSGCODE_SET_OSD_NAME ||
      thisMsgCode == MSGCODE_WRITE_EEPROM ||
      thisMsgCode == MSGCODE_TRANSMIT_IDLETIME ||
      thisMsgCode == MSGCODE_SET_ACTIVE_SOURCE)
    return thisMsgCode == msgResponse;

  if (!m_message->IsTranmission())
    return false;

  return ((msgCode == MSGCODE_COMMAND_ACCEPTED || msgCode == MSGCODE_COMMAND_REJECTED) &&
      (msgResponse == MSGCODE_TRANSMIT_ACK_POLARITY || msgResponse == MSGCODE_TRANSMIT || msgResponse == MSGCODE_TRANSMIT_EOM)) ||
      msgCode == MSGCODE_TIMEOUT_ERROR ||
      msgCode == MSGCODE_RECEIVE_FAILED ||
      msgCode == MSGCODE_TRANSMIT_FAILED_ACK ||
      msgCode == MSGCODE_TRANSMIT_FAILED_TIMEOUT_DATA ||
      msgCode == MSGCODE_TRANSMIT_FAILED_TIMEOUT_LINE ||
      msgCode == MSGCODE_TRANSMIT_SUCCEEDED;
}

const char *CCECAdapterMessageQueueEntry::ToString(void) const
{
  /* CEC transmissions got the 'set ack polarity' msgcode, which doesn't look nice */
  if (m_message->IsTranmission())
    return "CEC transmission";
  else
    return CCECAdapterMessage::ToString(m_message->Message());
}

bool CCECAdapterMessageQueueEntry::MessageReceivedCommandAccepted(const CCECAdapterMessage &message)
{
  bool bSendSignal(false);
  bool bHandled(false);
  {
    CLockObject lock(m_mutex);
    if (m_iPacketsLeft > 0)
    {
      /* decrease by 1 */
      m_iPacketsLeft--;

#ifdef CEC_DEBUGGING
      /* log this message */
      CStdString strLog;
      strLog.Format("%s - command accepted", ToString());
      if (m_iPacketsLeft > 0)
        strLog.AppendFormat(" - waiting for %d more", m_iPacketsLeft);
      m_queue->m_com->m_callback->GetLib()->AddLog(CEC_LOG_DEBUG, strLog);
#endif

      /* no more packets left and not a transmission, so we're done */
      if (!m_message->IsTranmission() && m_iPacketsLeft == 0)
      {
        m_message->state = ADAPTER_MESSAGE_STATE_SENT_ACKED;
        m_message->response = message.packet;
        bSendSignal = true;
      }
      bHandled = true;
    }
  }

  if (bSendSignal)
    Signal();

  return bHandled;
}

bool CCECAdapterMessageQueueEntry::MessageReceivedTransmitSucceeded(const CCECAdapterMessage &message)
{
  {
    CLockObject lock(m_mutex);
    if (m_iPacketsLeft == 0)
    {
      /* transmission succeeded, so we're done */
#ifdef CEC_DEBUGGING
      m_queue->m_com->m_callback->GetLib()->AddLog(CEC_LOG_DEBUG, "%s - transmit succeeded", m_message->ToString().c_str());
#endif
      m_message->state = ADAPTER_MESSAGE_STATE_SENT_ACKED;
      m_message->response = message.packet;
    }
    else
    {
      /* error, we expected more acks
         since the messages are processed in order, this should not happen, so this is an error situation */
      m_queue->m_com->m_callback->GetLib()->AddLog(CEC_LOG_WARNING, "%s - received 'transmit succeeded' but not enough 'command accepted' messages (%d left)", ToString(), m_iPacketsLeft);
      m_message->state = ADAPTER_MESSAGE_STATE_ERROR;
    }
  }

  Signal();

  return true;
}

bool CCECAdapterMessageQueueEntry::MessageReceivedResponse(const CCECAdapterMessage &message)
{
  {
    CLockObject lock(m_mutex);
#ifdef CEC_DEBUGGING
    m_queue->m_com->m_callback->GetLib()->AddLog(CEC_LOG_DEBUG, "%s - received response - %s", ToString(), message.ToString().c_str());
#else
    if (message.IsError())
      m_queue->m_com->m_callback->GetLib()->AddLog(CEC_LOG_DEBUG, "%s - received response - %s", ToString(), message.ToString().c_str());
#endif
    m_message->response = message.packet;
    if (m_message->IsTranmission())
      m_message->state = message.Message() == MSGCODE_TRANSMIT_SUCCEEDED ? ADAPTER_MESSAGE_STATE_SENT_ACKED : ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
    else
      m_message->state = ADAPTER_MESSAGE_STATE_SENT_ACKED;
  }

  Signal();

  return true;
}

bool CCECAdapterMessageQueueEntry::ProvidesExtendedResponse(void)
{
  return m_queue && m_queue->ProvidesExtendedResponse();
}

bool CCECAdapterMessageQueueEntry::TimedOutOrSucceeded(void) const
{
  return m_message->bFireAndForget && (m_bSucceeded || m_queueTimeout.TimeLeft() == 0);
}

CCECAdapterMessageQueue::CCECAdapterMessageQueue(CUSBCECAdapterCommunication *com) :
  PLATFORM::CThread(),
  m_com(com),
  m_iNextMessage(0)
{
  m_incomingAdapterMessage = new CCECAdapterMessage;
  m_currentCECFrame.Clear();
}

CCECAdapterMessageQueue::~CCECAdapterMessageQueue(void)
{
  StopThread(-1);
  Clear();
  StopThread();
  delete m_incomingAdapterMessage;
}

void CCECAdapterMessageQueue::Clear(void)
{
  StopThread(5);
  CLockObject lock(m_mutex);
  m_writeQueue.Clear();
  m_messages.clear();
}

void *CCECAdapterMessageQueue::Process(void)
{
  CCECAdapterMessageQueueEntry *message(NULL);
  while (!IsStopped())
  {
    /* wait for a new message */
    if (m_writeQueue.Pop(message, MESSAGE_QUEUE_SIGNAL_WAIT_TIME) && message)
    {
      /* write this message */
      {
        CLockObject lock(m_mutex);
        m_com->WriteToDevice(message->m_message);
      }
      if (message->m_message->state == ADAPTER_MESSAGE_STATE_ERROR ||
          message->m_message->Message() == MSGCODE_START_BOOTLOADER)
      {
        message->Signal();
        Clear();
        break;
      }
    }

    CheckTimedOutMessages();
  }
  return NULL;
}

void CCECAdapterMessageQueue::CheckTimedOutMessages(void)
{
  CLockObject lock(m_mutex);
  vector<uint64_t> timedOut;
  for (map<uint64_t, CCECAdapterMessageQueueEntry *>::iterator it = m_messages.begin(); it != m_messages.end(); it++)
  {
    if (it->second->TimedOutOrSucceeded())
    {
      timedOut.push_back(it->first);
      if (!it->second->m_bSucceeded)
        m_com->m_callback->GetLib()->AddLog(CEC_LOG_DEBUG, "command '%s' was not acked by the controller", CCECAdapterMessage::ToString(it->second->m_message->Message()));
      delete it->second->m_message;
      delete it->second;
    }
  }

  for (vector<uint64_t>::iterator it = timedOut.begin(); it != timedOut.end(); it++)
  {
    uint64_t iEntryId = *it;
    m_messages.erase(iEntryId);
  }
}

void CCECAdapterMessageQueue::MessageReceived(const CCECAdapterMessage &msg)
{
  bool bHandled(false);
  CLockObject lock(m_mutex);
  /* send the received message to each entry in the queue until it is handled */
  for (map<uint64_t, CCECAdapterMessageQueueEntry *>::iterator it = m_messages.begin(); !bHandled && it != m_messages.end(); it++)
    bHandled = it->second->MessageReceived(msg);

  if (!bHandled)
  {
    /* the message wasn't handled */
    bool bIsError(m_com->HandlePoll(msg));
#ifdef CEC_DEBUGGING
    m_com->m_callback->GetLib()->AddLog(bIsError ? CEC_LOG_WARNING : CEC_LOG_DEBUG, msg.ToString().c_str());
#else
    if (bIsError)
      m_com->m_callback->GetLib()->AddLog(CEC_LOG_WARNING, msg.ToString().c_str());
#endif

    /* push this message to the current frame */
    if (!bIsError && msg.PushToCecCommand(m_currentCECFrame))
    {
      /* and push the current frame back over the callback method when a full command was received */
      if (m_com->IsInitialised())
        m_com->m_callback->OnCommandReceived(m_currentCECFrame);

      /* clear the current frame */
      m_currentCECFrame.Clear();
    }
  }
}

void CCECAdapterMessageQueue::AddData(uint8_t *data, size_t iLen)
{
  for (size_t iPtr = 0; iPtr < iLen; iPtr++)
  {
    bool bFullMessage(false);
    {
      CLockObject lock(m_mutex);
      bFullMessage = m_incomingAdapterMessage->PushReceivedByte(data[iPtr]);
    }

    if (bFullMessage)
    {
      /* a full message was received */
      CCECAdapterMessage newMessage;
      newMessage.packet = m_incomingAdapterMessage->packet;
      MessageReceived(newMessage);

      /* clear the current message */
      CLockObject lock(m_mutex);
      m_incomingAdapterMessage->Clear();
    }
  }
}

bool CCECAdapterMessageQueue::Write(CCECAdapterMessage *msg)
{
  msg->state = ADAPTER_MESSAGE_STATE_WAITING_TO_BE_SENT;

  /* set the correct line timeout */
  if (msg->IsTranmission())
  {
    m_com->SetLineTimeout(msg->lineTimeout);
  }

  CCECAdapterMessageQueueEntry *entry = new CCECAdapterMessageQueueEntry(this, msg);
  if (!entry)
  {
    m_com->m_callback->GetLib()->AddLog(CEC_LOG_ERROR, "couldn't create queue entry for '%s'", CCECAdapterMessage::ToString(msg->Message()));
    msg->state = ADAPTER_MESSAGE_STATE_ERROR;
    return false;
  }

  uint64_t iEntryId(0);
  /* add to the wait for ack queue */
  if (msg->Message() != MSGCODE_START_BOOTLOADER)
  {
    CLockObject lock(m_mutex);
    iEntryId = m_iNextMessage++;
    m_messages.insert(make_pair(iEntryId, entry));
  }

  /* add the message to the write queue */
  m_writeQueue.Push(entry);

  bool bReturn(true);
  if (!msg->bFireAndForget)
  {
    if (!entry->Wait(msg->transmit_timeout <= 5 ? CEC_DEFAULT_TRANSMIT_WAIT : msg->transmit_timeout))
    {
      m_com->m_callback->GetLib()->AddLog(CEC_LOG_DEBUG, "command '%s' was not acked by the controller", CCECAdapterMessage::ToString(msg->Message()));
      msg->state = ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED;
      bReturn = false;
    }

    if (msg->Message() != MSGCODE_START_BOOTLOADER)
    {
      CLockObject lock(m_mutex);
      m_messages.erase(iEntryId);
    }

    if (msg->ReplyIsError() && msg->state != ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED)
      msg->state = ADAPTER_MESSAGE_STATE_ERROR;

    delete entry;
  }

  return bReturn;
}

bool CCECAdapterMessageQueue::ProvidesExtendedResponse(void)
{
  return m_com && m_com->ProvidesExtendedResponse();
}

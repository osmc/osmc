/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "ActorProtocol.h"

using namespace Actor;

void Message::Release()
{
  bool skip;
  origin->Lock();
  skip = isSync ? !isSyncFini : false;
  isSyncFini = true;
  origin->Unlock();

  if (skip)
    return;

  // free data buffer
  if (data != buffer)
    delete [] data;

  // delete event in case of sync message
  if (event)
    delete event;

  origin->ReturnMessage(this);
}

bool Message::Reply(int sig, void *data /* = NULL*/, int size /* = 0 */)
{
  if (!isSync)
  {
    if (isOut)
      return origin->SendInMessage(sig, data, size);
    else
      return origin->SendOutMessage(sig, data, size);
  }

  origin->Lock();

  if (!isSyncTimeout)
  {
    Message *msg = origin->GetMessage();
    msg->signal = sig;
    msg->isOut = !isOut;
    replyMessage = msg;
    if (data)
    {
      if (size > MSG_INTERNAL_BUFFER_SIZE)
        msg->data = new uint8_t[size];
      else
        msg->data = msg->buffer;
      memcpy(msg->data, data, size);
    }
  }

  origin->Unlock();

  if (event)
    event->Set();

  return true;
}

Protocol::~Protocol()
{
  Message *msg;
  Purge();
  while (!freeMessageQueue.empty())
  {
    msg = freeMessageQueue.front();
    freeMessageQueue.pop();
    delete msg;
  }
}

Message *Protocol::GetMessage()
{
  Message *msg;

  CSingleLock lock(criticalSection);

  if (!freeMessageQueue.empty())
  {
    msg = freeMessageQueue.front();
    freeMessageQueue.pop();
  }
  else
    msg = new Message();

  msg->isSync = false;
  msg->isSyncFini = false;
  msg->isSyncTimeout = false;
  msg->event = NULL;
  msg->data = NULL;
  msg->payloadSize = 0;
  msg->replyMessage = NULL;
  msg->origin = this;

  return msg;
}

void Protocol::ReturnMessage(Message *msg)
{
  CSingleLock lock(criticalSection);

  freeMessageQueue.push(msg);
}

bool Protocol::SendOutMessage(int signal, void *data /* = NULL */, int size /* = 0 */, Message *outMsg /* = NULL */)
{
  Message *msg;
  if (outMsg)
    msg = outMsg;
  else
    msg = GetMessage();

  msg->signal = signal;
  msg->isOut = true;

  if (data)
  {
    if (size > MSG_INTERNAL_BUFFER_SIZE)
      msg->data = new uint8_t[size];
    else
      msg->data = msg->buffer;
    memcpy(msg->data, data, size);
  }

  { CSingleLock lock(criticalSection);
    outMessages.push(msg);
  }
  containerOutEvent->Set();

  return true;
}

bool Protocol::SendInMessage(int signal, void *data /* = NULL */, int size /* = 0 */, Message *outMsg /* = NULL */)
{
  Message *msg;
  if (outMsg)
    msg = outMsg;
  else
    msg = GetMessage();

  msg->signal = signal;
  msg->isOut = false;

  if (data)
  {
    if (size > MSG_INTERNAL_BUFFER_SIZE)
      msg->data = new uint8_t[size];
    else
      msg->data = msg->buffer;
    memcpy(msg->data, data, size);
  }

  { CSingleLock lock(criticalSection);
    inMessages.push(msg);
  }
  containerInEvent->Set();

  return true;
}


bool Protocol::SendOutMessageSync(int signal, Message **retMsg, int timeout, void *data /* = NULL */, int size /* = 0 */)
{
  Message *msg = GetMessage();
  msg->isOut = true;
  msg->isSync = true;
  msg->event = new CEvent;
  msg->event->Reset();
  SendOutMessage(signal, data, size, msg);

  if (!msg->event->WaitMSec(timeout))
  {
    msg->origin->Lock();
    if (msg->replyMessage)
      *retMsg = msg->replyMessage;
    else
    {
      *retMsg = NULL;
      msg->isSyncTimeout = true;
    }
    msg->origin->Unlock();
  }
  else
    *retMsg = msg->replyMessage;

  msg->Release();

  if (*retMsg)
    return true;
  else
    return false;
}

bool Protocol::ReceiveOutMessage(Message **msg)
{
  CSingleLock lock(criticalSection);

  if (outMessages.empty() || outDefered)
    return false;

  *msg = outMessages.front();
  outMessages.pop();

  return true;
}

bool Protocol::ReceiveInMessage(Message **msg)
{
  CSingleLock lock(criticalSection);

  if (inMessages.empty() || inDefered)
    return false;

  *msg = inMessages.front();
  inMessages.pop();

  return true;
}


void Protocol::Purge()
{
  Message *msg;

  while (ReceiveInMessage(&msg))
    msg->Release();

  while (ReceiveOutMessage(&msg))
    msg->Release();
}

void Protocol::PurgeIn(int signal)
{
  Message *msg;
  std::queue<Message*> msgs;

  CSingleLock lock(criticalSection);

  while (!inMessages.empty())
  {
    msg = inMessages.front();
    inMessages.pop();
    if (msg->signal != signal)
      msgs.push(msg);
  }
  while (!msgs.empty())
  {
    msg = msgs.front();
    msgs.pop();
    inMessages.push(msg);
  }
}

void Protocol::PurgeOut(int signal)
{
  Message *msg;
  std::queue<Message*> msgs;

  CSingleLock lock(criticalSection);

  while (!outMessages.empty())
  {
    msg = outMessages.front();
    outMessages.pop();
    if (msg->signal != signal)
      msgs.push(msg);
  }
  while (!msgs.empty())
  {
    msg = msgs.front();
    msgs.pop();
    outMessages.push(msg);
  }
}

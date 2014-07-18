#pragma once
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

#include "platform/threads/mutex.h"
#include "platform/util/buffer.h"

namespace CEC
{
  // a buffer that priotises the input from the TV.
  // if we need more than this, we'll have to change it into a priority_queue
  class CCECInputBuffer
  {
  public:
    CCECInputBuffer(void) : m_bHasData(false) {}
    virtual ~CCECInputBuffer(void)
    {
      Broadcast();
    }

    void Broadcast(void)
    {
      PLATFORM::CLockObject lock(m_mutex);
      m_bHasData = true;
      m_condition.Broadcast();
    }

    bool Push(const cec_command &command)
    {
      bool bReturn(false);
      PLATFORM::CLockObject lock(m_mutex);
      if (command.initiator == CECDEVICE_TV)
        bReturn = m_tvInBuffer.Push(command);
      else
        bReturn = m_inBuffer.Push(command);

      m_bHasData |= bReturn;
      if (m_bHasData)
        m_condition.Signal();

      return bReturn;
    }

    bool Pop(cec_command &command, uint16_t iTimeout)
    {
      bool bReturn(false);
      PLATFORM::CLockObject lock(m_mutex);
      if (m_tvInBuffer.IsEmpty() && m_inBuffer.IsEmpty() &&
          !m_condition.Wait(m_mutex, m_bHasData, iTimeout))
        return bReturn;

      if (m_tvInBuffer.Pop(command))
        bReturn = true;
      else if (m_inBuffer.Pop(command))
        bReturn = true;

      m_bHasData = !m_tvInBuffer.IsEmpty() || !m_inBuffer.IsEmpty();
      return bReturn;
    }

  private:
    PLATFORM::CMutex                    m_mutex;
    PLATFORM::CCondition<volatile bool> m_condition;
    volatile bool                       m_bHasData;
    PLATFORM::SyncedBuffer<cec_command> m_tvInBuffer;
    PLATFORM::SyncedBuffer<cec_command> m_inBuffer;
  };
};

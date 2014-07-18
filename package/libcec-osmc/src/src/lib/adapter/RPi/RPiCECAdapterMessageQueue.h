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

#include "lib/platform/util/buffer.h"
#include <map>

namespace CEC
{
  class CRPiCECAdapterCommunication;
  class CRPiCECAdapterMessageQueue;

  class CRPiCECAdapterMessageQueueEntry
  {
  public:
    CRPiCECAdapterMessageQueueEntry(CRPiCECAdapterMessageQueue *queue, const cec_command &command);
    virtual ~CRPiCECAdapterMessageQueueEntry(void) {}

    /*!
     * @brief Signal waiting threads
     */
    void Broadcast(void);

    bool MessageReceived(cec_opcode opcode, cec_logical_address initiator, cec_logical_address destination, uint32_t response);

    /*!
     * @brief Wait for a response to this command.
     * @param iTimeout The timeout to use while waiting.
     * @return True when a response was received before the timeout passed, false otherwise.
     */
    bool Wait(uint32_t iTimeout);

    /*!
     * @return True while a thread is waiting for a signal or isn't waiting yet, false otherwise.
     */
    bool IsWaiting(void);

    /*!
     * @return The command that was sent in human readable form.
     */
    const char *ToString(void) const { return "CEC transmission"; }

    CRPiCECAdapterMessageQueue * m_queue;
    bool                         m_bWaiting;     /**< true while a thread is waiting or when it hasn't started waiting yet */
    PLATFORM::CCondition<bool>   m_condition;    /**< the condition to wait on */
    PLATFORM::CMutex             m_mutex;        /**< mutex for changes to this class */
    cec_command                  m_command;
    uint32_t                     m_retval;
    bool                         m_bSucceeded;
  };

  class CRPiCECAdapterMessageQueue
  {
    friend class CRPiCECAdapterMessageQueueEntry;

  public:
    /*!
     * @brief Create a new message queue.
     * @param com The communication handler callback to use.
     * @param iQueueSize The outgoing message queue size.
     */
    CRPiCECAdapterMessageQueue(CRPiCECAdapterCommunication *com) :
      m_com(com),
      m_iNextMessage(0)
    {
    }

    virtual ~CRPiCECAdapterMessageQueue(void)
    {
      Clear();
    }

    /*!
     * @brief Signal and delete everything in the queue
     */
    void Clear(void);

    void MessageReceived(cec_opcode opcode, cec_logical_address initiator, cec_logical_address destination, uint32_t response);

    bool Write(const cec_command &command, bool bIsReply);

  private:
    CRPiCECAdapterCommunication *                             m_com;                    /**< the communication handler */
    PLATFORM::CMutex                                          m_mutex;                  /**< mutex for changes to this class */
    std::map<uint64_t, CRPiCECAdapterMessageQueueEntry *>     m_messages;               /**< the outgoing message queue */
    uint64_t                                                  m_iNextMessage;           /**< the index of the next message */
  };
};

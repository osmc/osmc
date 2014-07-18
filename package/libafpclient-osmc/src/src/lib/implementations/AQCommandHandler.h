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

#include "CECCommandHandler.h"
#include "lib/platform/threads/threads.h"

namespace CEC
{
  class CAQPowerStatusCheck;

  class CAQCommandHandler : public CCECCommandHandler
  {
    friend class CAQPowerStatusCheck;
  public:
    CAQCommandHandler(CCECBusDevice *busDevice,
                      int32_t iTransmitTimeout = CEC_DEFAULT_TRANSMIT_TIMEOUT,
                      int32_t iTransmitWait = CEC_DEFAULT_TRANSMIT_WAIT,
                      int8_t iTransmitRetries = CEC_DEFAULT_TRANSMIT_RETRIES,
                      int64_t iActiveSourcePending = 0);
    virtual ~CAQCommandHandler(void);

  protected:
    virtual bool PowerOn(const cec_logical_address iInitiator, const cec_logical_address iDestination);

  private:
    CAQPowerStatusCheck* m_powerOnCheck;
  };

  class CAQPowerStatusCheck : public PLATFORM::CThread
  {
  public:
    CAQPowerStatusCheck(CAQCommandHandler* handler, cec_logical_address iInitiator, cec_logical_address iDestination) :
      m_handler(handler),
      m_iInitiator(iInitiator),
      m_iDestination(iDestination) {}
    virtual ~CAQPowerStatusCheck(void) {}

  private:
    void* Process(void);
    CAQCommandHandler* m_handler;
    cec_logical_address m_iInitiator;
    cec_logical_address m_iDestination;
  };
};

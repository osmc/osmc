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

#include "lib/platform/os.h"
#include "lib/platform/util/buffer.h"

#include <string>
#include <stdint.h>

#if !defined(__WINDOWS__)
#include <termios.h>
#endif

#include "socket.h"

namespace PLATFORM
{
  class CCDevSocket : public CCommonSocket<chardev_socket_t>
  {
    public:
      CCDevSocket(const std::string &strName ) :
        CCommonSocket<chardev_socket_t>(INVALID_CHARDEV_SOCKET_VALUE, strName)
        #ifdef __WINDOWS__
        ,m_iCurrentReadTimeout(MAXDWORD)
        #endif
      {}

      virtual ~CCDevSocket(void) 
      { 
	Close(); 
      }

      virtual bool Open(uint64_t iTimeoutMs = 0)
      {
        (void)iTimeoutMs;

	if (IsOpen())
          return false;

        m_socket = open(m_strName.c_str(), O_RDWR );

        if (m_socket == INVALID_CHARDEV_SOCKET_VALUE)
        {
          m_strError = strerror(errno);
          return false;
        }

        return true;
      }
      
      virtual void Close(void)
      {
        if (IsOpen())
        {
          SocketClose(m_socket);
          m_socket = INVALID_CHARDEV_SOCKET_VALUE;
        }
      }
      
      virtual void Shutdown(void)
      {
        SocketClose(m_socket);
      }
      
      virtual int Ioctl(int request, void* data)
      {
        return IsOpen() ? SocketIoctl(m_socket, &m_iError, request, data) : -1;
      }
      
      virtual ssize_t Write(void* data, size_t len)
      {
        return IsOpen() ? SocketWrite(m_socket, &m_iError, data, len) : -1;
      }
      
      virtual ssize_t Read(void* data, size_t len, uint64_t iTimeoutMs = 0)
      {
        return IsOpen() ? SocketRead(m_socket, &m_iError, data, len, iTimeoutMs) : -1;
      }

      virtual bool IsOpen(void)
      {
        return m_socket != INVALID_CHARDEV_SOCKET_VALUE;
      }
  };

};


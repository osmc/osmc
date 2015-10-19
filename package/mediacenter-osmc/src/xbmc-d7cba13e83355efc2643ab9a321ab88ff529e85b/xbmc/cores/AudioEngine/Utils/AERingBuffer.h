#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define AE_RING_BUFFER_OK 0;
#define AE_RING_BUFFER_EMPTY 1;
#define AE_RING_BUFFER_FULL 2;
#define AE_RING_BUFFER_NOTAVAILABLE 3;

//#define AE_RING_BUFFER_DEBUG

#include "utils/log.h"  //CLog
#include <string.h>     //memset, memcpy

/**
 * This buffer can be used by one read and one write thread at any one time
 * without the risk of data corruption.
 * If you intend to call the Reset() method, please use Locks.
 * All other operations are thread-safe.
 */
class AERingBuffer {

public:
  AERingBuffer() :
    m_iReadPos(0),
    m_iWritePos(0),
    m_iRead(0),
    m_iWritten(0),
    m_iSize(0),
    m_planes(0),
    m_Buffer(NULL)
  {
  }

  AERingBuffer(unsigned int size, unsigned int planes = 1) :
    m_iReadPos(0),
    m_iWritePos(0),
    m_iRead(0),
    m_iWritten(0),
    m_iSize(0),
    m_planes(0),
    m_Buffer(NULL)
  {
    Create(size, planes);
  }

  ~AERingBuffer()
  {
#ifdef AE_RING_BUFFER_DEBUG
    CLog::Log(LOGDEBUG, "AERingBuffer::~AERingBuffer: Deleting buffer.");
#endif
    for (unsigned int i = 0; i < m_planes; i++)
      _aligned_free(m_Buffer[i]);
    delete[] m_Buffer;
  }

  /**
   * Allocates space for buffer, and sets it's contents to 0.
   *
   * @return true on success, false otherwise
   */
  bool Create(int size, unsigned int planes = 1)
  {
    m_Buffer = new unsigned char*[planes];
    for (unsigned int i = 0; i < planes; i++)
    {
      m_Buffer[i] = (unsigned char*)_aligned_malloc(size,16);
      if (!m_Buffer[i])
        return false;
      memset(m_Buffer[i], 0, size);
    }
    m_iSize = size;
    m_planes = planes;
    return true;
  }

  /**
   * Fills the buffer with zeros and resets the pointers.
   * This method is not thread-safe, so before using this method
   * please acquire a Lock()
   */
  void Reset() {
#ifdef AE_RING_BUFFER_DEBUG
    CLog::Log(LOGDEBUG, "AERingBuffer::Reset: Buffer reset.");
#endif
    m_iWritten = 0;
    m_iRead = 0;
    m_iReadPos = 0;
    m_iWritePos = 0;
  }

  /**
   * Writes data to buffer.
   * Attempt to write more bytes than available results in AE_RING_BUFFER_FULL.
   *
   * @return AE_RING_BUFFER_OK on success, otherwise an error code
   */
  int Write(unsigned char *src, unsigned int size, unsigned int plane = 0)
  {
    unsigned int space = GetWriteSize();

    //do we have enough space for all the data?
    if (size > space || plane >= m_planes)
    {
#ifdef AE_RING_BUFFER_DEBUG
    CLog::Log(LOGDEBUG, "AERingBuffer: Not enough space, ignoring data. Requested: %u Available: %u",size, space);
#endif
      return AE_RING_BUFFER_FULL;
    }

    //no wrapping?
    if ( m_iSize > size + m_iWritePos )
    {
#ifdef AE_RING_BUFFER_DEBUG
      CLog::Log(LOGDEBUG, "AERingBuffer: Written to: %u size: %u space before: %u\n", m_iWritePos, size, space);
#endif
      memcpy(m_Buffer[plane] + m_iWritePos, src, size);
    }
    //need to wrap
    else
    {
      unsigned int first = m_iSize - m_iWritePos;
      unsigned int second = size - first;
#ifdef AE_RING_BUFFER_DEBUG
      CLog::Log(LOGDEBUG, "AERingBuffer: Written to (split) first: %u second: %u size: %u space before: %u\n", first, second, size, space);
#endif
      memcpy(m_Buffer[plane] + m_iWritePos, src, first);
      memcpy(m_Buffer[plane], src + first, second);
    }
    if (plane + 1 == m_planes)
      WriteFinished(size);

    return AE_RING_BUFFER_OK;
  }

  /**
   * Reads data from buffer.
   * Attempt to read more bytes than available results in RING_BUFFER_NOTAVAILABLE.
   * Reading from empty buffer returns AE_RING_BUFFER_EMPTY
   *
   * @return AE_RING_BUFFER_OK on success, otherwise an error code
   */
  int Read(unsigned char *dest, unsigned int size, unsigned int plane = 0)
  {
    unsigned int space = GetReadSize();

    //want to read more than we have written?
    if( space == 0 )
    {
#ifdef AE_RING_BUFFER_DEBUG
      CLog::Log(LOGDEBUG, "AERingBuffer: Can't read from empty buffer.");
#endif
      return AE_RING_BUFFER_EMPTY;
    }

    //want to read more than we have available
    if( size > space || plane >= m_planes)
    {
#ifdef AE_RING_BUFFER_DEBUG
      CLog::Log(LOGDEBUG, "AERingBuffer: Can't read %u bytes when we only have %u.", size, space);
#endif
      return AE_RING_BUFFER_NOTAVAILABLE;
    }

    //no wrapping?
    if ( size + m_iReadPos < m_iSize )
    {
#ifdef AE_RING_BUFFER_DEBUG
      CLog::Log(LOGDEBUG, "AERingBuffer: Reading from: %u size: %u space before: %u\n", m_iWritePos, size, space);
#endif
      if (dest)
        memcpy(dest, m_Buffer[plane] + m_iReadPos, size);
    }
    //need to wrap
    else
    {
      unsigned int first = m_iSize - m_iReadPos;
      unsigned int second = size - first;
#ifdef AE_RING_BUFFER_DEBUG
      CLog::Log(LOGDEBUG, "AERingBuffer: Reading from (split) first: %u second: %u size: %u space before: %u\n", first, second, size, space);
#endif
      if (dest)
      {
        memcpy(dest, m_Buffer[plane] + m_iReadPos, first);
        memcpy(dest + first, m_Buffer[plane], second);
      }
    }
    if (plane + 1 == m_planes)
      ReadFinished(size);

    return AE_RING_BUFFER_OK;
  }

  /**
   * Dumps the buffer.
   */
  void Dump()
  {
    unsigned char *bufferContents =  (unsigned char *)_aligned_malloc(m_iSize*m_planes + 1,16);
    unsigned char *dest = bufferContents;
    for (unsigned int j = 0; j < m_planes; j++)
    {
      for (unsigned int i=0; i<m_iSize; i++)
      {
        if (i >= m_iReadPos && i<m_iWritePos)
          *dest++ = m_Buffer[j][i];
        else
          *dest++ = '_';
      }
    }
    bufferContents[m_iSize*m_planes] = '\0';
    CLog::Log(LOGDEBUG, "AERingBuffer::Dump()\n%s",bufferContents);
    _aligned_free(bufferContents);
  }

  /**
   * Returns available space for writing to buffer.
   * Attempt to write more bytes than available results in AE_RING_BUFFER_FULL.
   */
  unsigned int GetWriteSize()
  {
    return m_iSize - ( m_iWritten - m_iRead );
  }

  /**
   * Returns available space for reading from buffer.
   * Attempt to read more bytes than available results in AE_RING_BUFFER_EMPTY.
   */
  unsigned int GetReadSize()
  {
    return m_iWritten - m_iRead;
  }

  /**
   * Returns the buffer size.
   */
  unsigned int GetMaxSize()
  {
    return m_iSize;
  }

  /**
   * Returns the number of planes
   */
  unsigned int NumPlanes() const
  {
    return m_planes;
  }
private:
  /**
   * Increments the write pointer.
   * Called at the end of writing to all planes.
   */
  void WriteFinished(unsigned int size)
  {
    if ( m_iSize > size + m_iWritePos )
      m_iWritePos += size;
    else // wrapping
      m_iWritePos = size - (m_iSize - m_iWritePos);

    //we can increase the write count now
    m_iWritten+=size;
  }

  /**
   * Increments the read pointer.
   * Called at the end of reading to all planes.
   */
  void ReadFinished(unsigned int size)
  {
    if ( size + m_iReadPos < m_iSize )
      m_iReadPos += size;
    else
      m_iReadPos = size - (m_iSize - m_iReadPos);

    //we can increase the read count now
    m_iRead+=size;
  }

  unsigned int m_iReadPos;
  unsigned int m_iWritePos;
  unsigned int m_iRead;
  unsigned int m_iWritten;
  unsigned int m_iSize;
  unsigned int m_planes;
  unsigned char **m_Buffer;
};

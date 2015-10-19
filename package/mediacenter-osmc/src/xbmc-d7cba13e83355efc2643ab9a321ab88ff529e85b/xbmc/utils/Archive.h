#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <string>
#include <vector>
#include "PlatformDefs.h" // for SYSTEMTIME

#define CARCHIVE_BUFFER_MAX 4096

namespace XFILE
{
  class CFile;
}
class CVariant;
class IArchivable;

class CArchive
{
public:
  CArchive(XFILE::CFile* pFile, int mode);
  ~CArchive();

  /* CArchive support storing and loading of all C basic integer types
   * C basic types was chosen instead of fixed size ints (int16_t - int64_t) to support all integer typedefs
   * For example size_t can be typedef of unsigned int, long or long long depending on platform 
   * while int32_t and int64_t are usually unsigned short, int or long long, but not long 
   * and even if int and long can have same binary representation they are different types for compiler 
   * According to section 5.2.4.2.1 of C99 http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf
   * minimal size of short int is 16 bits
   * minimal size of int is 16 bits (usually 32 or 64 bits, larger or equal to short int)
   * minimal size of long int is 32 bits (larger or equal to int)
   * minimal size of long long int is 64 bits (larger or equal to long int) */
  // storing
  CArchive& operator<<(float f);
  CArchive& operator<<(double d);
  CArchive& operator<<(short int s);
  CArchive& operator<<(unsigned short int us);
  CArchive& operator<<(int i);
  CArchive& operator<<(unsigned int ui);
  CArchive& operator<<(long int l);
  CArchive& operator<<(unsigned long int ul);
  CArchive& operator<<(long long int ll);
  CArchive& operator<<(unsigned long long int ull);
  CArchive& operator<<(bool b);
  CArchive& operator<<(char c);
  CArchive& operator<<(const std::string &str);
  CArchive& operator<<(const std::wstring& wstr);
  CArchive& operator<<(const SYSTEMTIME& time);
  CArchive& operator<<(IArchivable& obj);
  CArchive& operator<<(const CVariant& variant);
  CArchive& operator<<(const std::vector<std::string>& strArray);
  CArchive& operator<<(const std::vector<int>& iArray);

  // loading
  inline CArchive& operator>>(float& f)
  {
    return streamin(&f, sizeof(f));
  }

  inline CArchive& operator>>(double& d)
  {
    return streamin(&d, sizeof(d));
  }

  inline CArchive& operator>>(short int& s)
  {
    return streamin(&s, sizeof(s));
  }

  inline CArchive& operator>>(unsigned short int& us)
  {
    return streamin(&us, sizeof(us));
  }

  inline CArchive& operator>>(int& i)
  {
    return streamin(&i, sizeof(i));
  }

  inline CArchive& operator>>(unsigned int& ui)
  {
    return streamin(&ui, sizeof(ui));
  }

  inline CArchive& operator>>(long int& l)
  {
    return streamin(&l, sizeof(l));
  }

  inline CArchive& operator>>(unsigned long int& ul)
  {
    return streamin(&ul, sizeof(ul));
  }

  inline CArchive& operator>>(long long int& ll)
  {
    return streamin(&ll, sizeof(ll));
  }

  inline CArchive& operator>>(unsigned long long int& ull)
  {
    return streamin(&ull, sizeof(ull));
  }

  inline CArchive& operator>>(bool& b)
  {
    return streamin(&b, sizeof(b));
  }

  inline CArchive& operator>>(char& c)
  {
    return streamin(&c, sizeof(c));
  }

  CArchive& operator>>(std::string &str);
  CArchive& operator>>(std::wstring& wstr);
  CArchive& operator>>(SYSTEMTIME& time);
  CArchive& operator>>(IArchivable& obj);
  CArchive& operator>>(CVariant& variant);
  CArchive& operator>>(std::vector<std::string>& strArray);
  CArchive& operator>>(std::vector<int>& iArray);

  bool IsLoading() const;
  bool IsStoring() const;

  void Close();

  enum Mode {load = 0, store};

protected:
  inline CArchive &streamout(const void *dataPtr, size_t size)
  {
    const uint8_t *ptr = (const uint8_t *) dataPtr;
    /* Note, the buffer is flushed as soon as it is full (m_BufferRemain == size) rather
     * than waiting until we attempt to put more data into an already full buffer */
    if (m_BufferRemain > size)
    {
      memcpy(m_BufferPos, ptr, size);
      m_BufferPos += size;
      m_BufferRemain -= size;
      return *this;
    }
    else
    {
      return streamout_bufferwrap(ptr, size);
    }
  }

  inline CArchive &streamin(void *dataPtr, size_t size)
  {
    uint8_t *ptr = (uint8_t *) dataPtr;
    /* Note, refilling the buffer is deferred until we know we need to read more from it */
    if (m_BufferRemain >= size)
    {
      memcpy(ptr, m_BufferPos, size);
      m_BufferPos += size;
      m_BufferRemain -= size;
      return *this;
    }
    else
    {
      return streamin_bufferwrap(ptr, size);
    }
  }

  XFILE::CFile* m_pFile;
  int m_iMode;
  uint8_t *m_pBuffer;
  uint8_t *m_BufferPos;
  size_t m_BufferRemain;

private:
  void FlushBuffer();
  CArchive &streamout_bufferwrap(const uint8_t *ptr, size_t size);
  void FillBuffer();
  CArchive &streamin_bufferwrap(uint8_t *ptr, size_t size);
};

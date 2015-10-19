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

#include <map>
#include <string>
#include <stdint.h>
#include <vector>

class CAutoTexBuffer;
class CBaseTexture;

class CTextureBundleXPR
{
  struct FileHeader_t
  {
    uint32_t Offset;
    uint32_t UnpackedSize;
    uint32_t PackedSize;
  };

  FILE*  m_hFile;
  time_t m_TimeStamp;

  std::map<std::string, FileHeader_t> m_FileHeaders;
  typedef std::map<std::string, FileHeader_t>::iterator iFiles;

  bool m_themeBundle;

  bool OpenBundle();
  bool LoadFile(const std::string& Filename, CAutoTexBuffer& UnpackedBuf);

public:
  CTextureBundleXPR(void);
  ~CTextureBundleXPR(void);

  void Cleanup();

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const std::string& Filename);
  void GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures);
  static std::string Normalize(const std::string &name);

  bool LoadTexture(const std::string& Filename, CBaseTexture** ppTexture,
                       int &width, int &height);

  int LoadAnim(const std::string& Filename, CBaseTexture*** ppTextures,
                int &width, int &height, int& nLoops, int** ppDelays);
};



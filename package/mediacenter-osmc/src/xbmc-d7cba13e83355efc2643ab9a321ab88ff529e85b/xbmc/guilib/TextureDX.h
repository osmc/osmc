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

#pragma once

#ifdef HAS_DX
#include "Texture.h"
#include "D3DResource.h"

/************************************************************************/
/*    CDXTexture                                                       */
/************************************************************************/
class CDXTexture : public CBaseTexture
{
public:
  CDXTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_UNKNOWN);
  virtual ~CDXTexture();

  void CreateTextureObject();
  void DestroyTextureObject();
  virtual void LoadToGPU();
  void BindToUnit(unsigned int unit);

  ID3D11Texture2D* GetTextureObject()
  {
    return m_texture.Get();
  };

  ID3D11ShaderResourceView* GetShaderResource()
  {
    return m_texture.GetShaderResource();
  };

private:
  CD3DTexture m_texture;
  DXGI_FORMAT GetFormat();
};

#endif

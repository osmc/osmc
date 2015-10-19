#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *      Test patterns designed by Ofer LaOr - hometheater.co.il
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

#include "settings/windows/GUIWindowTestPattern.h"
#include "guilib/GUIShaderDX.h"

class CGUIWindowTestPatternDX : public CGUIWindowTestPattern
{
public:
  CGUIWindowTestPatternDX(void);
  virtual ~CGUIWindowTestPatternDX(void);

private:
  virtual void DrawVerticalLines(int top, int left, int bottom, int right);
  virtual void DrawHorizontalLines(int top, int left, int bottom, int right);
  virtual void DrawCheckers(int top, int left, int bottom, int right);
  virtual void DrawBouncingRectangle(int top, int left, int bottom, int right);
  virtual void DrawContrastBrightnessPattern(int top, int left, int bottom, int right);
  virtual void DrawCircle(int originX, int originY, int radius);
  virtual void BeginRender();
  virtual void EndRender();
  virtual void UpdateVertexBuffer(Vertex *vertecies, unsigned count);

  void DrawRectangle(float x, float y, float x2, float y2, DWORD color);
  void DrawCircleEx(float originX, float originY, float radius, DWORD color);

  ID3D11Buffer*   m_vb;
  unsigned        m_bufferWidth;
};


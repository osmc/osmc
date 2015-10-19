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

#ifndef RENDER_SYSTEM_GL_H
#define RENDER_SYSTEM_GL_H

#pragma once

#if defined(HAVE_LIBGL)

#include "system.h"
#include "system_gl.h"
#include "rendering/RenderSystem.h"

class CRenderSystemGL : public CRenderSystemBase
{
public:
  CRenderSystemGL();
  virtual ~CRenderSystemGL();
  virtual void CheckOpenGLQuirks();
  virtual bool InitRenderSystem();
  virtual bool DestroyRenderSystem();
  virtual bool ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate);

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool PresentRender(const CDirtyRegionList& dirty);
  virtual bool ClearBuffers(color_t color);
  virtual bool IsExtSupported(const char* extension);

  virtual void SetVSync(bool vsync);
  virtual void ResetVSync() { m_bVsyncInit = false; }

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);

  virtual void SetScissors(const CRect &rect);
  virtual void ResetScissors();

  virtual void CaptureStateBlock();
  virtual void ApplyStateBlock();

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f);

  virtual void ApplyHardwareTransform(const TransformMatrix &matrix);
  virtual void RestoreHardwareTransform();
  virtual void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view);
  virtual bool SupportsStereo(RENDER_STEREO_MODE mode);

  virtual bool TestRender();

  virtual void Project(float &x, float &y, float &z);

  virtual void GetGLSLVersion(int& major, int& minor);

  virtual void ResetGLErrors();

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual bool PresentRenderImpl(const CDirtyRegionList& dirty) = 0;
  void CalculateMaxTexturesize();

  int        m_iVSyncMode;
  int        m_iVSyncErrors;
  int64_t    m_iSwapStamp;
  int64_t    m_iSwapRate;
  int64_t    m_iSwapTime;
  bool       m_bVsyncInit;
  int        m_width;
  int        m_height;

  std::string m_RenderExtensions;

  int        m_glslMajor;
  int        m_glslMinor;
  
  GLint      m_viewPort[4];
};

#endif // HAVE_LIBGL

#endif // RENDER_SYSTEM_H

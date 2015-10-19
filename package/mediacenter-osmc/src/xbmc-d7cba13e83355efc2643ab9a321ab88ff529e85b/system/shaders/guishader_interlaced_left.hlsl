/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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

#define STEREO_MODE_SHADER

Texture2D texView : register(t0);

cbuffer cbViewPort : register(b1)
{
  float g_viewPortX;
  float g_viewPortY;
  float g_viewPortWidth;
  float g_viewPortHeigh;
};

#include "guishader_common.hlsl"

float4 PS(PS_INPUT input) : SV_TARGET
{
  return StereoInterlaced(input, STEREO_LEFT_EYE_INDEX);
}



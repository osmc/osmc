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

precision mediump float;
uniform sampler2D m_sampY;
uniform sampler2D m_sampU;
uniform sampler2D m_sampV;
varying vec2      m_cordY;
varying vec2      m_cordU;
varying vec2      m_cordV;
uniform float     m_alpha;
uniform mat4      m_yuvmat;

// handles both YV12 and NV12 formats
void main()
{
  vec4 yuv, rgb;
  yuv.rgba = vec4( texture2D(m_sampY, m_cordY).r
                 , texture2D(m_sampU, m_cordU).g
                 , texture2D(m_sampV, m_cordV).a
                 , 1.0);

  rgb   = m_yuvmat * yuv;
  rgb.a = m_alpha;
  gl_FragColor = rgb;
}

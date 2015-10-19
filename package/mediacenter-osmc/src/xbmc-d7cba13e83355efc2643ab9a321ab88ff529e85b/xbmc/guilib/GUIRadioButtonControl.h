/*!
\file GUIRadioButtonControl.h
\brief
*/

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

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIRadioButtonControl :
      public CGUIButtonControl
{
public:
  CGUIRadioButtonControl(int parentID, int controlID,
                         float posX, float posY, float width, float height,
                         const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
                         const CLabelInfo& labelInfo,
                         const CTextureInfo& radioOnFocus, const CTextureInfo& radioOnNoFocus,
                         const CTextureInfo& radioOffFocus, const CTextureInfo& radioOffNoFocus,
                         const CTextureInfo& radioOnDisabled, const CTextureInfo& radioOffDisabled);

  virtual ~CGUIRadioButtonControl(void);
  virtual CGUIRadioButtonControl *Clone() const { return new CGUIRadioButtonControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual std::string GetDescription() const;
  void SetRadioDimensions(float posX, float posY, float width, float height);
  void SetToggleSelect(const std::string &toggleSelect);
  bool IsSelected() const { return m_bSelected; };
protected:
  virtual bool UpdateColors();
  CGUITexture m_imgRadioOnFocus;
  CGUITexture m_imgRadioOnNoFocus;
  CGUITexture m_imgRadioOffFocus;
  CGUITexture m_imgRadioOffNoFocus;
  CGUITexture m_imgRadioOnDisabled;
  CGUITexture m_imgRadioOffDisabled;
  float m_radioPosX;
  float m_radioPosY;
  INFO::InfoPtr m_toggleSelect;
  bool m_useLabel2;
};

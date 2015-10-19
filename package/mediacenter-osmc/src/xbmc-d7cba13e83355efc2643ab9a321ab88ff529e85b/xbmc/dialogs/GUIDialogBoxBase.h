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

#include <vector>

#include "guilib/GUIDialog.h"
#include "threads/CriticalSection.h"

#define DIALOG_MAX_LINES 3
#define DIALOG_MAX_CHOICES 2

class CVariant;

class CGUIDialogBoxBase :
      public CGUIDialog
{
public:
  CGUIDialogBoxBase(int id, const std::string &xmlFile);
  virtual ~CGUIDialogBoxBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  bool IsConfirmed() const;
  void SetLine(unsigned int iLine, CVariant line);
  void SetText(CVariant text);
  void SetHeading(CVariant heading);
  void SetChoice(int iButton, const CVariant &choice);
protected:
  std::string GetDefaultLabel(int controlId) const;
  virtual int GetDefaultLabelID(int controlId) const;
  /*! \brief Get a localized string from a variant
   If the varaint is already a string we return directly, else if it's an integer we return the corresponding
   localized string.
   \param var the variant to localize.
   */
  std::string GetLocalized(const CVariant &var) const;

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  bool m_bConfirmed;
  bool m_hasTextbox;

  // actual strings
  CCriticalSection m_section;
  std::string m_strHeading;
  std::string m_text;
  std::string m_strChoices[DIALOG_MAX_CHOICES];
};

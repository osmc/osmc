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

#include "GUIViewStatePrograms.h"
#include "FileItem.h"
#include "view/ViewState.h"
#include "settings/MediaSourceSettings.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/Settings.h"
#include "view/ViewStateSettings.h"

using namespace XFILE;

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%K", "%I", "%L", ""),  // Titel, Size | Foldername, empty
    CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("programs");
  SetSortMethod(viewState->m_sortDescription);
  SetViewAsControl(viewState->m_viewMode);
  SetSortOrder(viewState->m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_PROGRAMS);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAMS, CViewStateSettings::GetInstance().Get("programs"));
}

std::string CGUIViewStateWindowPrograms::GetLockType()
{
  return "programs";
}

std::string CGUIViewStateWindowPrograms::GetExtensions()
{
  return ".xbe|.cut";
}

VECSOURCES& CGUIViewStateWindowPrograms::GetSources()
{
  AddAddonsSource("executable", g_localizeStrings.Get(1043), "DefaultAddonProgram.png");
#if defined(TARGET_ANDROID)
  AddAndroidSource("apps", g_localizeStrings.Get(20244), "DefaultProgram.png");
#endif

  VECSOURCES *programSources = CMediaSourceSettings::GetInstance().GetSources("programs");
  AddOrReplace(*programSources, CGUIViewState::GetSources());
  return *programSources;
}


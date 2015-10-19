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

#include "GUIWindowAddonBrowser.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "GUIDialogAddonInfo.h"
#include "GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "FileItem.h"
#include "filesystem/AddonsDirectory.h"
#include "addons/AddonInstaller.h"
#include "settings/Settings.h"
#include "settings/MediaSourceSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "AddonDatabase.h"
#include "storage/MediaManager.h"
#include "LangInfo.h"
#include "input/Key.h"
#include "ContextMenuManager.h"

#include <utility>

#define CONTROL_AUTOUPDATE    5
#define CONTROL_SHUTUP        6
#define CONTROL_FOREIGNFILTER 7
#define CONTROL_BROKENFILTER  8
#define CONTROL_CHECK_FOR_UPDATES  9

using namespace ADDON;
using namespace XFILE;

CGUIWindowAddonBrowser::CGUIWindowAddonBrowser(void)
: CGUIMediaWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
}

CGUIWindowAddonBrowser::~CGUIWindowAddonBrowser()
{
}

bool CGUIWindowAddonBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.AllowNonLocalSources(false);

      // is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        m_vecItems->SetPath("");

      SetProperties();
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_AUTOUPDATE)
      {
        const CGUIControl *control = GetControl(CONTROL_AUTOUPDATE);
        if (control && control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
          CSettings::GetInstance().SetInt(CSettings::SETTING_GENERAL_ADDONUPDATES, (CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES)+1) % AUTO_UPDATES_MAX);
        else
          CSettings::GetInstance().SetInt(CSettings::SETTING_GENERAL_ADDONUPDATES, (CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == 0) ? 1 : 0);
        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_SHUTUP)
      {
        CSettings::GetInstance().ToggleBool(CSettings::SETTING_GENERAL_ADDONNOTIFICATIONS);
        CSettings::GetInstance().Save();
        return true;
      }
      else if (iControl == CONTROL_FOREIGNFILTER)
      {
        CSettings::GetInstance().ToggleBool(CSettings::SETTING_GENERAL_ADDONFOREIGNFILTER);
        CSettings::GetInstance().Save();
        Refresh();
        return true;
      }
      else if (iControl == CONTROL_BROKENFILTER)
      {
        CSettings::GetInstance().ToggleBool(CSettings::SETTING_GENERAL_ADDONBROKENFILTER);
        CSettings::GetInstance().Save();
        Refresh();
        return true;
      }
      else if (iControl == CONTROL_CHECK_FOR_UPDATES)
      {
        CRepositoryUpdater::GetInstance().CheckForUpdates(true);
        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_SHOW_INFO)
        {
          if (!m_vecItems->Get(iItem)->GetProperty("Addon.ID").empty())
            return CGUIDialogAddonInfo::ShowForItem((*m_vecItems)[iItem]);
          return false;
        }
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && IsActive() && message.GetNumStringParams() == 1)
      { // update this item
        for (int i = 0; i < m_vecItems->Size(); ++i)
        {
          CFileItemPtr item = m_vecItems->Get(i);
          if (item->GetProperty("Addon.ID") == message.GetStringParam())
          {
            SetItemLabel2(item);
            return true;
          }
        }
      }
      else if (message.GetParam1() == GUI_MSG_UPDATE && IsActive())
        SetProperties();
    }
    break;
   default:
     break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowAddonBrowser::SetProperties()
{
  auto lastUpdated = CRepositoryUpdater::GetInstance().LastUpdated();
  SetProperty("Updated", lastUpdated.IsValid() ?
    lastUpdated.GetAsLocalizedDateTime() : g_localizeStrings.Get(21337));
}

void CGUIWindowAddonBrowser::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;

  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  std::string addonId = pItem->GetProperty("Addon.ID").asString();
  if (!addonId.empty())
  {
    buttons.Add(CONTEXT_BUTTON_INFO, 24003);

    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(addonId, addon, ADDON_UNKNOWN, false) && addon->HasSettings())
      buttons.Add(CONTEXT_BUTTON_SETTINGS, 24020);
    if (CAddonMgr::GetInstance().GetAddon(addonId, addon, ADDON_REPOSITORY))
      buttons.Add(CONTEXT_BUTTON_CHECK_FOR_UPDATES, 24034);
  }

  CContextMenuManager::GetInstance().AddVisibleItems(pItem, buttons);
}

bool CGUIWindowAddonBrowser::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  std::string addonId = pItem->GetProperty("Addon.ID").asString();
  if (!addonId.empty())
  {
    if (button == CONTEXT_BUTTON_INFO)
    {
      return CGUIDialogAddonInfo::ShowForItem(pItem);
    }
    else if (button == CONTEXT_BUTTON_SETTINGS)
    {
      AddonPtr addon;
      if (CAddonMgr::GetInstance().GetAddon(addonId, addon, ADDON_UNKNOWN, false))
        return CGUIDialogAddonSettings::ShowAndGetInput(addon);
    }
    else if (button == CONTEXT_BUTTON_CHECK_FOR_UPDATES)
    {
      AddonPtr addon;
      if (CAddonMgr::GetInstance().GetAddon(addonId, addon, ADDON_REPOSITORY))
        CRepositoryUpdater::GetInstance().CheckForUpdates(std::static_pointer_cast<CRepository>(addon), true);
    }
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

class UpdateAddons : public IRunnable
{
  virtual void Run()
  {
    CAddonInstaller::GetInstance().InstallUpdates(true);
  }
};


bool CGUIWindowAddonBrowser::OnClick(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item->GetPath() == "addons://install/")
  {
    // pop up filebrowser to grab an installed folder
    VECSOURCES shares = *CMediaSourceSettings::GetInstance().GetSources("files");
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
    std::string path;
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "*.zip", g_localizeStrings.Get(24041), path))
      CAddonInstaller::GetInstance().InstallFromZip(path);
    return true;
  }
  if (item->GetPath() == "addons://update_all/")
  {
    UpdateAddons updater;
    return CGUIDialogBusy::Wait(&updater);
  }
  if (!item->m_bIsFolder)
  {
    // cancel a downloading job
    if (item->HasProperty("Addon.Downloading"))
    {
      if (CGUIDialogYesNo::ShowAndGetInput(CVariant{24000}, item->GetProperty("Addon.Name"), CVariant{24066}, CVariant{""}))
      {
        if (CAddonInstaller::GetInstance().Cancel(item->GetProperty("Addon.ID").asString()))
          Refresh();
      }
      return true;
    }

    CGUIDialogAddonInfo::ShowForItem(item);
    return true;
  }
  if (item->IsPath("addons://search/"))
    return Update(item->GetPath());

  return CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowAddonBrowser::UpdateButtons()
{
  const CGUIControl *control = GetControl(CONTROL_AUTOUPDATE);
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
  { // set label
    CSettingInt *setting = (CSettingInt *)CSettings::GetInstance().GetSetting(CSettings::SETTING_GENERAL_ADDONUPDATES);
    if (setting)
    {
      const StaticIntegerSettingOptions& options = setting->GetOptions();
      for (StaticIntegerSettingOptions::const_iterator it = options.begin(); it != options.end(); ++it)
      {
        if (it->second == setting->GetValue())
        {
          SET_CONTROL_LABEL(CONTROL_AUTOUPDATE, it->first);
          break;
        }
      }
    }
  }
  else
  { // old skin with toggle button - set on if auto updates are on
    SET_CONTROL_SELECTED(GetID(),CONTROL_AUTOUPDATE, CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == AUTO_UPDATES_ON);
  }
  SET_CONTROL_SELECTED(GetID(),CONTROL_SHUTUP, CSettings::GetInstance().GetBool(CSettings::SETTING_GENERAL_ADDONNOTIFICATIONS));
  SET_CONTROL_SELECTED(GetID(),CONTROL_FOREIGNFILTER, CSettings::GetInstance().GetBool(CSettings::SETTING_GENERAL_ADDONFOREIGNFILTER));
  SET_CONTROL_SELECTED(GetID(),CONTROL_BROKENFILTER, CSettings::GetInstance().GetBool(CSettings::SETTING_GENERAL_ADDONBROKENFILTER));
  CONTROL_ENABLE(CONTROL_CHECK_FOR_UPDATES);

  bool allowFilter = CAddonsDirectory::IsRepoDirectory(CURL(m_vecItems->GetPath()));
  CONTROL_ENABLE_ON_CONDITION(CONTROL_FOREIGNFILTER, allowFilter);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BROKENFILTER, allowFilter);

  CGUIMediaWindow::UpdateButtons();
}

static bool IsForeign(const std::string& languages)
{
  if (languages.empty())
    return false;

  for (const auto& lang : StringUtils::Split(languages, " "))
  {
    if (lang == "en" ||
        lang == g_langInfo.GetLocale().GetLanguageCode() ||
        lang == g_langInfo.GetLocale().ToShortString())
      return false;

    // for backwards compatibility
    if (lang == "no" && g_langInfo.GetLocale().ToShortString() == "nb_NO")
      return false;
  }
  return true;
}

bool CGUIWindowAddonBrowser::GetDirectory(const std::string& strDirectory, CFileItemList& items)
{
  bool result;
  if (URIUtils::PathEquals(strDirectory, "addons://downloading/"))
  {
    VECADDONS addons;
    CAddonInstaller::GetInstance().GetInstallList(addons);

    CURL url(strDirectory);
    CAddonsDirectory::GenerateAddonListing(url, addons, items, g_localizeStrings.Get(24067));
    result = true;
    items.SetPath(strDirectory);

    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItemPtr pItem(new CFileItem(".."));
      pItem->SetPath(m_history.GetParentPath());
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.AddFront(pItem, 0);
    }

  }
  else
  {
    result = CGUIMediaWindow::GetDirectory(strDirectory, items);

    if (result && CAddonsDirectory::IsRepoDirectory(CURL(strDirectory)))
    {
      if (CSettings::GetInstance().GetBool(CSettings::SETTING_GENERAL_ADDONFOREIGNFILTER))
      {
        int i = 0;
        while (i < items.Size())
        {
          auto prop = items[i]->GetProperty("Addon.Language");
          if (!prop.isNull() && IsForeign(prop.asString()))
            items.Remove(i);
          else
            ++i;
        }
      }
      if (CSettings::GetInstance().GetBool(CSettings::SETTING_GENERAL_ADDONBROKENFILTER))
      {
        for (int i = items.Size() - 1; i >= 0; i--)
        {
          if (!items[i]->GetProperty("Addon.Broken").empty())
          {
            //check if it's installed
            AddonPtr addon;
            if (!CAddonMgr::GetInstance().GetAddon(items[i]->GetProperty("Addon.ID").asString(), addon))
              items.Remove(i);
          }
        }
      }
    }
  }

  if (strDirectory.empty() && CAddonInstaller::GetInstance().IsDownloading())
  {
    CFileItemPtr item(new CFileItem("addons://downloading/", true));
    item->SetLabel(g_localizeStrings.Get(24067));
    item->SetLabelPreformated(true);
    item->SetIconImage("DefaultNetwork.png");
    items.Add(item);
  }

  for (int i = 0; i < items.Size(); ++i)
    SetItemLabel2(items[i]);

  return result;
}

void CGUIWindowAddonBrowser::SetItemLabel2(CFileItemPtr item)
{
  if (!item || item->m_bIsFolder) return;
  unsigned int percent;
  if (CAddonInstaller::GetInstance().GetProgress(item->GetProperty("Addon.ID").asString(), percent))
  {
    std::string progress = StringUtils::Format(g_localizeStrings.Get(24042).c_str(), percent);
    item->SetProperty("Addon.Status", progress);
    item->SetProperty("Addon.Downloading", true);
  }
  else
    item->ClearProperty("Addon.Downloading");
  item->SetLabel2(item->GetProperty("Addon.Status").asString());
  // to avoid the view state overriding label 2
  item->SetLabelPreformated(true);
}

bool CGUIWindowAddonBrowser::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  m_thumbLoader.Load(*m_vecItems);

  return true;
}

int CGUIWindowAddonBrowser::SelectAddonID(TYPE type, std::string &addonID, bool showNone /* = false */, bool showDetails /* = true */, bool showInstalled /* = true */, bool showInstallable /*= false */, bool showMore /* = true */)
{
  std::vector<ADDON::TYPE> types;
  types.push_back(type);
  return SelectAddonID(types, addonID, showNone, showDetails, showInstalled, showInstallable, showMore);
}

int CGUIWindowAddonBrowser::SelectAddonID(ADDON::TYPE type, std::vector<std::string> &addonIDs, bool showNone /* = false */, bool showDetails /* = true */, bool multipleSelection /* = true */, bool showInstalled /* = true */, bool showInstallable /* = false */, bool showMore /* = true */)
{
  std::vector<ADDON::TYPE> types;
  types.push_back(type);
  return SelectAddonID(types, addonIDs, showNone, showDetails, multipleSelection, showInstalled, showInstallable, showMore);
}

int CGUIWindowAddonBrowser::SelectAddonID(const std::vector<ADDON::TYPE> &types, std::string &addonID, bool showNone /* = false */, bool showDetails /* = true */, bool showInstalled /* = true */, bool showInstallable /* = false */, bool showMore /* = true */)
{
  std::vector<std::string> addonIDs;
  if (!addonID.empty())
    addonIDs.push_back(addonID);
  int retval = SelectAddonID(types, addonIDs, showNone, showDetails, false, showInstalled, showInstallable, showMore);
  if (addonIDs.size() > 0)
    addonID = addonIDs.at(0);
  else
    addonID = "";
  return retval;
}

int CGUIWindowAddonBrowser::SelectAddonID(const std::vector<ADDON::TYPE> &types, std::vector<std::string> &addonIDs, bool showNone /* = false */, bool showDetails /* = true */, bool multipleSelection /* = true */, bool showInstalled /* = true */, bool showInstallable /* = false */, bool showMore /* = true */)
{
  // if we shouldn't show neither installed nor installable addons the list will be empty
  if (!showInstalled && !showInstallable)
    return 0;

  // can't show the "Get More" button if we already show installable addons
  if (showInstallable)
    showMore = false;

  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return 0;

  // get rid of any invalid addon types
  std::vector<ADDON::TYPE> validTypes(types.size());
  std::copy_if(types.begin(), types.end(), validTypes.begin(), [](ADDON::TYPE type) { return type != ADDON_UNKNOWN; });

  if (validTypes.empty())
    return 0;

  // get all addons to show
  VECADDONS addons;
  if (showInstalled)
  {
    for (std::vector<ADDON::TYPE>::const_iterator type = validTypes.begin(); type != validTypes.end(); ++type)
    {
      VECADDONS typeAddons;
      if (*type == ADDON_AUDIO)
        CAddonsDirectory::GetScriptsAndPlugins("audio", typeAddons);
      else if (*type == ADDON_EXECUTABLE)
        CAddonsDirectory::GetScriptsAndPlugins("executable", typeAddons);
      else if (*type == ADDON_IMAGE)
        CAddonsDirectory::GetScriptsAndPlugins("image", typeAddons);
      else if (*type == ADDON_VIDEO)
        CAddonsDirectory::GetScriptsAndPlugins("video", typeAddons);
      else
        CAddonMgr::GetInstance().GetAddons(*type, typeAddons);

      addons.insert(addons.end(), typeAddons.begin(), typeAddons.end());
    }
  }

  if (showInstallable || showMore)
  {
    VECADDONS installableAddons;
    CAddonDatabase database;
    if (database.Open() && database.GetAddons(installableAddons))
    {
      for (ADDON::IVECADDONS addon = installableAddons.begin(); addon != installableAddons.end();)
      {
        AddonPtr pAddon = *addon;
        
        // check if the addon matches one of the provided addon types
        bool matchesType = false;
        for (std::vector<ADDON::TYPE>::const_iterator type = validTypes.begin(); type != validTypes.end(); ++type)
        {
          if (pAddon->IsType(*type))
          {
            matchesType = true;
            break;
          }
        }

        // only show addons that match one of the provided addon types and that aren't disabled
        if (matchesType && !CAddonMgr::GetInstance().IsAddonDisabled(pAddon->ID()))
        {
          // check if the addon is installed
          bool isInstalled = CAddonMgr::GetInstance().IsAddonInstalled(pAddon->ID());

          // check if the addon is installed or can be installed
          if ((showInstallable || showMore) && !isInstalled && CAddonMgr::GetInstance().CanAddonBeInstalled(pAddon))
          {
            ++addon;
            continue;
          }
        }

        addon = installableAddons.erase(addon);
      }

      if (showInstallable)
        addons.insert(addons.end(), installableAddons.begin(), installableAddons.end());
      else if (showMore)
        showMore = !installableAddons.empty();
    }
  }

  if (addons.empty() && !showNone)
    return 0;

  // turn the addons into items
  std::map<std::string, AddonPtr> addonMap;
  CFileItemList items;
  for (ADDON::IVECADDONS addon = addons.begin(); addon != addons.end(); ++addon)
  {
    CFileItemPtr item(CAddonsDirectory::FileItemFromAddon(*addon, (*addon)->ID()));
    if (!items.Contains(item->GetPath()))
    {
      items.Add(item);
      addonMap.insert(std::make_pair(item->GetPath(), *addon));
    }
  }

  if (items.IsEmpty() && !showNone)
    return 0;

  std::string heading;
  for (std::vector<ADDON::TYPE>::const_iterator type = validTypes.begin(); type != validTypes.end(); ++type)
  {
    if (!heading.empty())
      heading += ", ";
    heading += TranslateType(*type, true);
  }

  dialog->SetHeading(CVariant{std::move(heading)});
  dialog->Reset();
  dialog->SetUseDetails(showDetails);

  if (multipleSelection)
  {
    showNone = false;
    showMore = false;
    dialog->EnableButton(true, 186);
  }
  else if (showMore)
    dialog->EnableButton(true, 21452);

  if (showNone)
  {
    CFileItemPtr item(new CFileItem("", false));
    item->SetLabel(g_localizeStrings.Get(231));
    item->SetLabel2(g_localizeStrings.Get(24040));
    item->SetIconImage("DefaultAddonNone.png");
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
  items.Sort(SortByLabel, SortOrderAscending);

  if (addonIDs.size() > 0)
  {
    for (std::vector<std::string>::const_iterator it = addonIDs.begin(); it != addonIDs.end() ; ++it)
    {
      CFileItemPtr item = items.Get(*it);
      if (item)
        item->Select(true);
    }
  }
  dialog->SetItems(items);
  dialog->SetMultiSelection(multipleSelection);
  dialog->Open();

  // if the "Get More" button has been pressed and we haven't shown the
  // installable addons so far show a list of installable addons
  if (showMore&& dialog->IsButtonPressed())
    return SelectAddonID(types, addonIDs, showNone, showDetails, multipleSelection, false, true, false);

  if (!dialog->IsConfirmed())
    return 0;

  addonIDs.clear();
  for (int i : dialog->GetSelectedItems())
  {
    const CFileItemPtr& item = items.Get(i);

    // check if one of the selected addons needs to be installed
    if (showInstallable)
    {
      std::map<std::string, AddonPtr>::const_iterator itAddon = addonMap.find(item->GetPath());
      if (itAddon != addonMap.end())
      {
        const AddonPtr& addon = itAddon->second;

        // if the addon isn't installed we need to install it
        if (!CAddonMgr::GetInstance().IsAddonInstalled(addon->ID()))
        {
          AddonPtr installedAddon;
          if (!CAddonInstaller::GetInstance().InstallModal(addon->ID(), installedAddon, false))
            continue;
        }

        // if the addon is disabled we need to enable it
        if (CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()))
          CAddonMgr::GetInstance().EnableAddon(addon->ID());
      }
    }

    addonIDs.push_back(item->GetPath());
  }
  return 1;
}

std::string CGUIWindowAddonBrowser::GetStartFolder(const std::string &dir)
{
  if (URIUtils::PathStarts(dir, "addons://"))
    return dir;
  return CGUIMediaWindow::GetStartFolder(dir);
}

#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://www.xbmc.org
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

#include <set>
#include <vector>

#include "guilib/GUIDialog.h"
#include "settings/SettingControl.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/Timer.h"

#define CONTROL_SETTINGS_LABEL          2
#define CONTROL_SETTINGS_DESCRIPTION    6

#define CONTROL_SETTINGS_OKAY_BUTTON    28
#define CONTROL_SETTINGS_CANCEL_BUTTON  29

#define CONTROL_SETTINGS_CUSTOM         100

#define CONTROL_SETTINGS_START_BUTTONS  -100
#define CONTROL_SETTINGS_START_CONTROL  -80

#define SETTINGS_RESET_SETTING_ID       "settings.reset"
#define SETTINGS_EMPTY_CATEGORY_ID      "categories.empty"

class CGUIControl;
class CGUIControlBaseSetting;
class CGUIImage;
class CGUISpinControlEx;
class CGUIEditControl;
class CGUIButtonControl;
class CGUIRadioButtonControl;
class CGUISettingsSliderControl;
class CGUILabelControl;

class CSetting;
class CSettingAction;
class CSettingCategory;
class CSettingSection;

class CVariant;

typedef std::shared_ptr<CGUIControlBaseSetting> BaseSettingControlPtr;

class CGUIDialogSettingsBase
  : public CGUIDialog,
    public CSettingControlCreator,
    protected ITimerCallback,
    protected ISettingCallback
{
public:
  CGUIDialogSettingsBase(int windowId, const std::string &xmlFile);
  virtual ~CGUIDialogSettingsBase();

  // specializations of CGUIControl
  virtual bool OnMessage(CGUIMessage &message) override;
  virtual bool OnAction(const CAction &action) override;
  virtual bool OnBack(int actionID) override;
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;

  virtual bool IsConfirmed() const { return m_confirmed; }

protected:
  // specializations of CGUIWindow
  virtual void OnInitWindow() override;

  // implementations of ITimerCallback
  virtual void OnTimeout() override;

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting) override;
  virtual void OnSettingPropertyChanged(const CSetting *setting, const char *propertyName) override;

  // new virtual methods
  virtual bool AllowResettingSettings() const { return true; }
  virtual int GetSettingLevel() const { return 0; }
  virtual CSettingSection* GetSection() = 0;
  virtual CSetting* GetSetting(const std::string &settingId) = 0;
  virtual void Save() = 0;
  virtual unsigned int GetDelayMs() const { return 1500; }
  virtual std::string GetLocalizedString(uint32_t labelId) const;
  
  virtual void OnOkay() { m_confirmed = true; }
  virtual void OnCancel() { }
  
  virtual void SetupView();
  virtual std::set<std::string> CreateSettings();
  virtual void UpdateSettings();

  /*!
    \brief Get the name for the setting entry

    Used as virtual to allow related settings dialog to give a std::string name of the setting.
    If not used on own dialog class it handle the string from int CSetting::GetLabel(),
    This must also be used if on related dialog no special entry is wanted.

    \param pSetting Base settings class which need the name
    \return Name used on settings dialog
   */
  virtual std::string GetSettingsLabel(CSetting *pSetting);

  virtual CGUIControl* AddSetting(CSetting *pSetting, float width, int &iControlID);
  virtual CGUIControl* AddSettingControl(CGUIControl *pControl, BaseSettingControlPtr pSettingControl, float width, int &iControlID);
  
  virtual void SetupControls(bool createSettings = true);
  virtual void FreeControls();
  virtual void DeleteControls();
  virtual void FreeSettingsControls();

  virtual void SetHeading(const CVariant &label);
  virtual void SetDescription(const CVariant &label);

  virtual void OnResetSettings();
  
  /*!
    \brief A setting control has been interacted with by the user

    This method is called when the user manually interacts (clicks,
    edits) with a setting control. It contains handling for both
    delayed and undelayed settings and either starts the delay timer
    or triggers the setting change which, on success, results in a
    callback to OnSettingChanged().

    \param pSettingControl Setting control that has been interacted with
   */
  virtual void OnClick(BaseSettingControlPtr pSettingControl);

  void UpdateSettingControl(const std::string &settingId);
  void UpdateSettingControl(BaseSettingControlPtr pSettingControl);
  void SetControlLabel(int controlId, const CVariant &label);

  BaseSettingControlPtr GetSettingControl(const std::string &setting);
  BaseSettingControlPtr GetSettingControl(int controlId);
  
  CGUIControl* AddSeparator(float width, int &iControlID);
  CGUIControl* AddLabel(float width, int &iControlID, int label);

  std::vector<CSettingCategory*> m_categories;
  std::vector<BaseSettingControlPtr> m_settingControls;
  
  int m_iSetting;
  int m_iCategory;
  CSettingAction *m_resetSetting;
  CSettingCategory *m_dummyCategory;
  
  CGUISpinControlEx *m_pOriginalSpin;
  CGUISettingsSliderControl *m_pOriginalSlider;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalCategoryButton;
  CGUIButtonControl *m_pOriginalButton;
  CGUIEditControl *m_pOriginalEdit;
  CGUIImage *m_pOriginalImage;
  CGUILabelControl *m_pOriginalGroupTitle;
  bool m_newOriginalEdit;
  
  BaseSettingControlPtr m_delayedSetting; ///< Current delayed setting \sa CBaseSettingControl::SetDelayed()
  CTimer m_delayedTimer;                  ///< Delayed setting timer

  bool m_confirmed;
};

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

#include "GUIDialogSlider.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"

#define CONTROL_HEADING 10
#define CONTROL_SLIDER  11
#define CONTROL_LABEL   12

CGUIDialogSlider::CGUIDialogSlider(void)
    : CGUIDialog(WINDOW_DIALOG_SLIDER, "DialogSlider.xml")
{
  m_callback = NULL;
  m_callbackData = NULL;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSlider::~CGUIDialogSlider(void)
{
}

bool CGUIDialogSlider::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSlider::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    if (message.GetSenderId() == CONTROL_SLIDER)
    {
      CGUISliderControl *slider = dynamic_cast<CGUISliderControl *>(GetControl(CONTROL_SLIDER));
      if (slider && m_callback)
      {
        m_callback->OnSliderChange(m_callbackData, slider);
        SET_CONTROL_LABEL(CONTROL_LABEL, slider->GetDescription());
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    m_callback = NULL;
    m_callbackData = NULL;
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSlider::SetSlider(const std::string &label, float value, float min, float delta, float max, ISliderCallback *callback, void *callbackData)
{
  SET_CONTROL_LABEL(CONTROL_HEADING, label);
  CGUISliderControl *slider = dynamic_cast<CGUISliderControl *>(GetControl(CONTROL_SLIDER));
  m_callback = callback;
  m_callbackData = callbackData;
  if (slider)
  {
    slider->SetType(SLIDER_CONTROL_TYPE_FLOAT);
    slider->SetFloatRange(min, max);
    slider->SetFloatInterval(delta);
    slider->SetFloatValue(value);
    if (m_callback)
    {
      m_callback->OnSliderChange(m_callbackData, slider);
      SET_CONTROL_LABEL(CONTROL_LABEL, slider->GetDescription());
    }
  }
}

void CGUIDialogSlider::OnWindowLoaded()
{
  // ensure our callbacks are NULL, incase we were loaded via some non-standard means
  m_callback = NULL;
  m_callbackData = NULL;
  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogSlider::SetModalityType(DialogModalityType type)
{
  m_modalityType = type;
}

void CGUIDialogSlider::ShowAndGetInput(const std::string &label, float value, float min, float delta, float max, ISliderCallback *callback, void *callbackData)
{
  // grab the slider dialog
  CGUIDialogSlider *slider = (CGUIDialogSlider *)g_windowManager.GetWindow(WINDOW_DIALOG_SLIDER);
  if (!slider)
    return;

  // set the label and value
  slider->Initialize();
  slider->SetSlider(label, value, min, delta, max, callback, callbackData);
  slider->SetModalityType(DialogModalityType::MODAL);
  slider->Open();
}

void CGUIDialogSlider::Display(int label, float value, float min, float delta, float max, ISliderCallback *callback)
{
  // grab the slider dialog
  CGUIDialogSlider *slider = (CGUIDialogSlider *)g_windowManager.GetWindow(WINDOW_DIALOG_SLIDER);
  if (!slider)
    return;

  // set the label and value
  slider->Initialize();
  slider->SetAutoClose(1000);
  slider->SetSlider(g_localizeStrings.Get(label), value, min, delta, max, callback, NULL);
  slider->SetModalityType(DialogModalityType::MODELESS);
  slider->Open();
}

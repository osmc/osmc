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

#include "GUIWindowWeather.h"

#include <utility>

#include "dialogs/GUIDialogOK.h"
#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/Weather.h"

using namespace ADDON;

#define CONTROL_BTNREFRESH             2
#define CONTROL_SELECTLOCATION         3
#define CONTROL_LABELUPDATED          11
#define CONTROL_IMAGELOGO            101

#define CONTROL_STATICTEMP           223
#define CONTROL_STATICFEEL           224
#define CONTROL_STATICUVID           225
#define CONTROL_STATICWIND           226
#define CONTROL_STATICDEWP           227
#define CONTROL_STATICHUMI           228

#define CONTROL_LABELD0DAY            31
#define CONTROL_LABELD0HI             32
#define CONTROL_LABELD0LOW            33
#define CONTROL_LABELD0GEN            34
#define CONTROL_IMAGED0IMG            35

#define LOCALIZED_TOKEN_FIRSTID      370
#define LOCALIZED_TOKEN_LASTID       395

/*
FIXME'S
>strings are not centered
*/

CGUIWindowWeather::CGUIWindowWeather(void)
    : CGUIWindow(WINDOW_WEATHER, "MyWeather.xml"), m_maxLocation(0)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowWeather::~CGUIWindowWeather(void)
{}

bool CGUIWindowWeather::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNREFRESH)
      {
        g_weatherManager.Refresh(); // Refresh clicked so do a complete update
      }
      else if (iControl == CONTROL_SELECTLOCATION)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SELECTLOCATION);
        OnMessage(msg);

        SetLocation(msg.GetParam1());
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
    {
      g_weatherManager.Reset();
      return true;
    }
    else if (message.GetParam1() == GUI_MSG_WEATHER_FETCHED)
    {
      UpdateLocations();
      SetProperties();
    }
    break;
  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetSenderId() == 0) //handle only message from builtin
      {
        SetLocation(message.GetParam1());
        return true;
      }
    }
    break;
  case GUI_MSG_MOVE_OFFSET:
    {
      if (message.GetSenderId() == 0 && m_maxLocation > 0) //handle only message from builtin
      {
        // Clamp location between 1 and m_maxLocation 
        int v = (g_weatherManager.GetArea() + message.GetParam1() - 1) % m_maxLocation + 1;
        if (v < 1) v += m_maxLocation;
        SetLocation(v);
        return true;
      }
    }
    break;
  default:
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowWeather::OnInitWindow()
{
  // call UpdateButtons() so that we start with our initial stuff already present
  UpdateButtons();
  UpdateLocations();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowWeather::UpdateLocations()
{
  if (!IsActive()) return;
  m_maxLocation = strtol(GetProperty("Locations").asString().c_str(),0,10);
  if (m_maxLocation < 1) return;

  std::vector< std::pair<std::string, int> > labels;

  unsigned int iCurWeather = g_weatherManager.GetArea();

  if (iCurWeather > m_maxLocation)
  {  
    g_weatherManager.SetArea(m_maxLocation);
    iCurWeather = m_maxLocation;
    ClearProperties();
    g_weatherManager.Refresh();
  }

  for (unsigned int i = 1; i <= m_maxLocation; i++)
  {
    std::string strLabel = g_weatherManager.GetLocation(i);
    if (strLabel.size() > 1) //got the location string yet?
    {
      size_t iPos = strLabel.rfind(", ");
      if (iPos != std::string::npos)
      {
        std::string strLabel2(strLabel);
        strLabel = strLabel2.substr(0,iPos);
      }
      labels.push_back(make_pair(strLabel, i));
    }
    else
    {
      strLabel = StringUtils::Format("AreaCode %i", i);
      labels.push_back(make_pair(strLabel, i));
    }
    // in case it's a button, set the label
    if (i == iCurWeather)
      SET_CONTROL_LABEL(CONTROL_SELECTLOCATION,strLabel);
  }

  SET_CONTROL_LABELS(CONTROL_SELECTLOCATION, iCurWeather, &labels);
}

void CGUIWindowWeather::UpdateButtons()
{
  CONTROL_ENABLE(CONTROL_BTNREFRESH);

  SET_CONTROL_LABEL(CONTROL_BTNREFRESH, 184);   //Refresh

  SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, g_weatherManager.GetLocation(g_weatherManager.GetArea()));
  SET_CONTROL_LABEL(CONTROL_LABELUPDATED, g_weatherManager.GetLastUpdateTime());

  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_COND, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_TEMP, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_FEEL, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_FEEL) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_UVID, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_WIND, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_DEWP, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_DEWP) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_HUMI, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  SET_CONTROL_FILENAME(WEATHER_IMAGE_CURRENT_ICON, g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));

  //static labels
  SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401);  //Temperature
  SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402);  //Feels Like
  SET_CONTROL_LABEL(CONTROL_STATICUVID, 403);  //UV Index
  SET_CONTROL_LABEL(CONTROL_STATICWIND, 404);  //Wind
  SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405);  //Dew Point
  SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406);  //Humidity

  for (int i = 0; i < NUM_DAYS; i++)
  {
    SET_CONTROL_LABEL(CONTROL_LABELD0DAY + (i*10), g_weatherManager.GetForecast(i).m_day);
    SET_CONTROL_LABEL(CONTROL_LABELD0HI + (i*10), g_weatherManager.GetForecast(i).m_high + g_langInfo.GetTemperatureUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0LOW + (i*10), g_weatherManager.GetForecast(i).m_low + g_langInfo.GetTemperatureUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0GEN + (i*10), g_weatherManager.GetForecast(i).m_overview);
    SET_CONTROL_FILENAME(CONTROL_IMAGED0IMG + (i*10), g_weatherManager.GetForecast(i).m_icon);
  }
}

void CGUIWindowWeather::FrameMove()
{
  // update our controls
  UpdateButtons();

  CGUIWindow::FrameMove();
}

/*!
 \brief Sets the location to the specified index and refreshes the weather
 \param loc the location index (in the range [1..MAXLOCATION])
 */
void CGUIWindowWeather::SetLocation(int loc)
{
  if (loc < 1 || loc > (int)m_maxLocation)
    return;
  // Avoid a settings write if old location == new location
  if (g_weatherManager.GetArea() != loc)
  {
    ClearProperties();
    g_weatherManager.SetArea(loc);
    std::string strLabel = g_weatherManager.GetLocation(loc);
    size_t iPos = strLabel.rfind(", ");
    if (iPos != std::string::npos)
      strLabel = strLabel.substr(0, iPos);
    SET_CONTROL_LABEL(CONTROL_SELECTLOCATION, strLabel);
  }
  g_weatherManager.Refresh();
}

void CGUIWindowWeather::SetProperties()
{
  // Current weather
  int iCurWeather = g_weatherManager.GetArea();
  SetProperty("Location", g_weatherManager.GetLocation(iCurWeather));
  SetProperty("LocationIndex", iCurWeather);
  SetProperty("Updated", g_weatherManager.GetLastUpdateTime());
  SetProperty("Current.ConditionIcon", g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  SetProperty("Current.Condition", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND));
  SetProperty("Current.Temperature", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP));
  SetProperty("Current.FeelsLike", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_FEEL));
  SetProperty("Current.UVIndex", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SetProperty("Current.Wind", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SetProperty("Current.DewPoint", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_DEWP));
  SetProperty("Current.Humidity", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  // we use the icons code number for fanart as it's the safest way
  std::string fanartcode = URIUtils::GetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  URIUtils::RemoveExtension(fanartcode);
  SetProperty("Current.FanartCode", fanartcode);

  // Future weather
  std::string day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day = StringUtils::Format("Day%i.", i);
    SetProperty(day + "Title", g_weatherManager.GetForecast(i).m_day);
    SetProperty(day + "HighTemp", g_weatherManager.GetForecast(i).m_high);
    SetProperty(day + "LowTemp", g_weatherManager.GetForecast(i).m_low);
    SetProperty(day + "Outlook", g_weatherManager.GetForecast(i).m_overview);
    SetProperty(day + "OutlookIcon", g_weatherManager.GetForecast(i).m_icon);
    fanartcode = URIUtils::GetFileName(g_weatherManager.GetForecast(i).m_icon);
    URIUtils::RemoveExtension(fanartcode);
    SetProperty(day + "FanartCode", fanartcode);
  }
}

void CGUIWindowWeather::ClearProperties()
{
  // Current weather
  SetProperty("Location", "");
  SetProperty("LocationIndex", "");
  SetProperty("Updated", "");
  SetProperty("Current.ConditionIcon", "");
  SetProperty("Current.Condition", "");
  SetProperty("Current.Temperature", "");
  SetProperty("Current.FeelsLike", "");
  SetProperty("Current.UVIndex", "");
  SetProperty("Current.Wind", "");
  SetProperty("Current.DewPoint", "");
  SetProperty("Current.Humidity", "");
  SetProperty("Current.FanartCode", "");
  
  // Future weather
  std::string day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day = StringUtils::Format("Day%i.", i);
    SetProperty(day + "Title", "");
    SetProperty(day + "HighTemp", "");
    SetProperty(day + "LowTemp", "");
    SetProperty(day + "Outlook", "");
    SetProperty(day + "OutlookIcon", "");
    SetProperty(day + "FanartCode", "");
  }
}

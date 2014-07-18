/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

using System.Collections.Generic;
using System.Text;
using LibCECTray.controller.applications.@internal;
using LibCECTray.settings;

namespace LibCECTray.controller.applications
{
  class Applications : CECSettingString
  {
    private Applications() :
      base("global_applications", "Applications", string.Empty, null)
    {
    }

    public static ApplicationController Get(string key)
    {
      return (_instance != null && _instance.Value.ContainsKey(key))
               ? _instance.Value[key]
               : null;
    }

    public static List<ApplicationController> GetAll()
    {
      List<ApplicationController> retVal = new List<ApplicationController>();
      if (_instance != null)
      {
        foreach (var app in _instance.Value.Values)
          retVal.Add(app);
      }
      return retVal;
    }

    public new Dictionary<string, ApplicationController> Value
    {
      get
      {
        if (_controllers == null)
        {
          _controllers = DefaultValue;
          var split = base.Value.Split('~');
          foreach (var item in split)
          {
            if (item.Length > 0)
            {
              var app = ApplicationController.FromString(_controller, _controller.Settings, item);
              if (app != null)
                _controllers.Add(app.ProcessName, app);
            }
          }
        }

        return _controllers;
      }

      set
      {
        StringBuilder sb = new StringBuilder();
        foreach (var app in value.Values)
        {
          sb.AppendFormat("{0}~", app.AsString());
        }
        base.Value = sb.ToString();
      }
    }

    public new Dictionary<string, ApplicationController> DefaultValue
    {
      get
      {
        var defaultValues = new Dictionary<string, ApplicationController>();
        WMCController wmcController = new WMCController(_controller);
        defaultValues.Add(wmcController.ProcessName, wmcController);
        XBMCController xbmcController = new XBMCController(_controller);
        defaultValues.Add(xbmcController.ProcessName, xbmcController);

        return defaultValues;
      }

      set
      {
        StringBuilder sb = new StringBuilder();
        foreach (var app in value.Values)
        {
          sb.AppendFormat("{0}~", app.AsString());
        }
        base.DefaultValue = sb.ToString();
      }
    }

    public static void Initialise(CECController controller)
    {
      _controller = controller;
      if (_instance == null)
        _instance = new Applications();
      controller.Settings["global_applications"] = _instance;
      controller.Settings.Load(_instance);

      foreach (var app in _instance.Value)
        _controller.RegisterApplication(app.Value);
    }

    private static Applications _instance;
    private static CECController _controller;
    private Dictionary<string, ApplicationController> _controllers;
  }
}

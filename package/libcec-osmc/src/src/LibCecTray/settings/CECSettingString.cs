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

using System.Windows.Forms;
using LibCECTray.ui;

namespace LibCECTray.settings
{

  /// <summary>
  /// A setting of type string that can be persisted in the registry
  /// </summary>
  class CECSettingString : CECSetting
  {
    public CECSettingString(string keyName, string friendlyName, string defaultValue, SettingChangedHandler changedHandler) :
      this(CECSettingType.String, keyName, friendlyName, defaultValue, changedHandler)
    {
    }

    public CECSettingString(CECSettingType type, string keyName, string friendlyName, string defaultValue, SettingChangedHandler changedHandler) :
      base(type, CECSettingSerialisationType.String, keyName, friendlyName, defaultValue, changedHandler)
    {
    }

    #region Serialisation methods
    protected override string GetSerialisedValue()
    {
      return Value;
    }

    protected override void SetSerialisedValue(string value)
    {
      Value = value;
    }

    protected override string GetSerialisedDefaultValue()
    {
      return DefaultValue;
    }

    protected override void SetSerialisedDefaultValue(string value)
    {
      DefaultValue = value;
    }
    #endregion

    public new string Value
    {
      get
      {
        return (string)base.Value;
      }
      set
      {
        base.Value = value;
        if (BaseValueControl != null && Form != null)
          Form.SetControlText(BaseValueControl, value);
      }
    }

    public new string DefaultValue
    {
      get
      {
        return (string)base.DefaultValue;
      }
      set
      {
        base.DefaultValue = value;
      }
    }

    public new Control ValueControl
    {
      get
      {
        if (BaseValueControl == null)
        {
          TextBox control = new TextBox
                               {
                                 Size = new System.Drawing.Size(100, 20),
                                 Enabled = InitialEnabledValue,
                                 Text = Value
                               };
          control.TextChanged += delegate { Value = control.Text; };
          BaseValueControl = control;
        }
        return BaseValueControl;
      }
    }

    public void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, Control labelControl, TextBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, labelControl, Label);
      ReplaceControl(controls, valueControl, ValueControl);
      if (ValueControl != null && Form != null)
        Form.SetControlText(ValueControl, Value);
    }

    public void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, TextBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, valueControl, ValueControl);
    }
  }
}

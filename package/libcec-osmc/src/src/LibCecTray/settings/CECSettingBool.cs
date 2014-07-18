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
  /// A setting of type bool that can be persisted in the registry
  /// </summary>
  class CECSettingBool : CECSettingNumeric
  {
    public CECSettingBool(string keyName, string friendlyName, bool defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.Bool, keyName, friendlyName, defaultValue ? 1 : 0, changedHandler)
    {
    }

    public new bool Value
    {
      get { return base.Value == 1; }
      set
      {
        base.Value = value ? 1 : 0;
        CheckBox control = BaseValueControl as CheckBox;
        if (control != null && Form != null)
          Form.SetCheckboxChecked(control, value);
      }
    }

    public new bool DefaultValue
    {
      get { return base.DefaultValue == 1; }
      set { base.DefaultValue = value ? 1 : 0; }
    }

    public new CheckBox ValueControl
    {
      get
      {
        if (BaseValueControl == null)
        {
          CheckBox control = new CheckBox
                               {
                                 AutoSize = true,
                                 Size = new System.Drawing.Size(150, 17),
                                 Text = Label.Text,
                                 UseVisualStyleBackColor = true,
                                 Enabled = InitialEnabledValue,
                                 Checked = Value
                               };
          control.CheckedChanged += delegate { Value = control.Checked; };
          BaseValueControl = control;
        }
        return BaseValueControl as CheckBox;
      }
    }

    public void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, Control labelControl, CheckBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, labelControl, Label);
      ReplaceControl(controls, valueControl, ValueControl);
    }

    public void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, CheckBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, valueControl, ValueControl);
    }
  }
}

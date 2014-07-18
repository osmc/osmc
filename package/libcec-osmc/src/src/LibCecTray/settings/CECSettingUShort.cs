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

﻿using System.Globalization;
using System.Windows.Forms;

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type ushort that can be persisted in the registry
  /// </summary>
  class CECSettingUShort : CECSettingString
  {
    public CECSettingUShort(string keyName, string friendlyName, ushort defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.UShort, keyName, friendlyName, string.Format("{0,4:X}", defaultValue), changedHandler)
    {
    }

    public new ushort Value
    {
      get
      {
        ushort iValue;
        return base.Value != null && ushort.TryParse(base.Value, NumberStyles.AllowHexSpecifier, null, out iValue) ? iValue : ushort.MinValue;
      }
      set
      {
        base.Value = string.Format("{0,4:X}", value);
        if (Form != null)
          Form.SetControlText(ValueControl, base.Value);
      }
    }

    public new ushort DefaultValue
    {
      get
      {
        ushort iValue;
        return base.DefaultValue != null && ushort.TryParse(base.DefaultValue, NumberStyles.AllowHexSpecifier, null, out iValue) ? iValue : ushort.MinValue;
      }
      set { base.DefaultValue = string.Format("{0,4:X}", value); }
    }

    public new Control ValueControl
    {
      get
      {
        if (BaseValueControl == null)
        {
          TextBox control = new TextBox
                              {
                                MaxLength = 4,
                                Size = new System.Drawing.Size(100, 20),
                                Enabled = InitialEnabledValue,
                                Text = string.Format("{0,4:X}", Value)
                              };
          control.TextChanged += delegate
                                   {
                                     ushort iValue;
                                     if (
                                       !ushort.TryParse(control.Text, NumberStyles.AllowHexSpecifier, null,
                                                        out iValue))
                                       iValue = DefaultValue;
                                     Value = iValue;
                                   };
          BaseValueControl = control;
        }
        return BaseValueControl;
      }
    }
  }
}

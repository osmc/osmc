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
using System.Windows.Forms;
using LibCECTray.ui;

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type integer that can be persisted in the registry
  /// </summary>
  class CECSettingNumeric : CECSetting
  {
    public CECSettingNumeric(CECSettingType type, string keyName, string friendlyName, int defaultValue, SettingChangedHandler changedHandler, ListControlConvertEventHandler format, List<int> items) :
      base(type, CECSettingSerialisationType.Numeric, keyName, friendlyName, defaultValue, changedHandler)
    {
      BaseItems = items;
      Format += format;
    }

    public CECSettingNumeric(CECSettingType type, string keyName, string friendlyName, int defaultValue, SettingChangedHandler changedHandler, ListControlConvertEventHandler format) :
      this(type, keyName, friendlyName, defaultValue, changedHandler, format, new List<int>())
    {
    }

    public CECSettingNumeric(string keyName, string friendlyName, int defaultValue, SettingChangedHandler changedHandler, ListControlConvertEventHandler format) :
      this(CECSettingType.Numeric, keyName, friendlyName, defaultValue, changedHandler, format)
    {
    }

    public CECSettingNumeric(string keyName, string friendlyName, int defaultValue, SettingChangedHandler changedHandler) :
      this(CECSettingType.Numeric, keyName, friendlyName, defaultValue, changedHandler, null)
    {
    }

    public CECSettingNumeric(CECSettingType type, string keyName, string friendlyName, int defaultValue, SettingChangedHandler changedHandler) :
      this(type, keyName, friendlyName, defaultValue, changedHandler, null)
    {
    }

    #region Serialisation methods
    protected override string GetSerialisedValue()
    {
      return string.Format("{0}", Value);
    }

    protected override void SetSerialisedValue(string value)
    {
      int intValue;
      Value = int.TryParse(value, out intValue) ? intValue : DefaultValue;
    }

    protected override string GetSerialisedDefaultValue()
    {
      return string.Format("{0}", DefaultValue);
    }

    protected override void SetSerialisedDefaultValue(string value)
    {
      int intValue;
      DefaultValue = int.TryParse(value, out intValue) ? intValue : int.MinValue;
    }
    #endregion

    public new int Value
    {
      get
      {
        return base.Value != null ? (int)base.Value : int.MaxValue;
      }
      set
      {
        base.Value = value;
        ComboBox control = BaseValueControl as ComboBox;
        if (Form != null && control != null && BaseItems.Contains(value))
        {
          Form.SetSelectedIndex(control, BaseItems.IndexOf(value));
        }
      }
    }

    public new int DefaultValue
    {
      get
      {
        return base.DefaultValue != null ? (int)base.DefaultValue : int.MaxValue;
      }
      set
      {
        base.DefaultValue = value;
      }
    }

    public int LowerLimit { get; set; }

    public int UpperLimit { get; set; }

    public int Step
    {
      get { return _step; }
      set { _step = value; }
    }
    private int _step = 1;

    protected virtual bool AllowedValue(int value)
    {
      return true;
    }

    protected virtual void CreateValueControl()
    {
      if (BaseValueControl != null) return;
      ComboBox control = new ComboBox
                           {
                             FormattingEnabled = true,
                             Name = KeyName,
                             Size = new System.Drawing.Size(Width, Height),
                             AutoCompleteMode = AutoCompleteMode.Append,
                             Enabled = InitialEnabledValue
                           };

      if (Format != null)
      {
        control.Format += Format;
        control.FormattingEnabled = true;
      }
      BaseValueControl = control;

      ResetItems(BaseItems.Count == 0);

      if (BaseItems.Count > 0 && control.SelectedIndex < BaseItems.Count)
      {
        control.SelectedValueChanged += delegate
                                          {
                                            Value = BaseItems[control.SelectedIndex];
                                          };
      }
    }

    public override Control ValueControl
    {
      get
      {
        CreateValueControl();
        return BaseValueControl;
      }
    }

    public void ResetItems(bool clear)
    {
      ComboBox control = BaseValueControl as ComboBox;
      if (control == null) return;
      if (clear)
        BaseItems.Clear();

      List<object> cbItems = new List<object>();
      foreach (var item in Items)
        cbItems.Add(string.Format("{0}", item));

      Form.SetComboBoxItems(control, Items.IndexOf(Value), cbItems.ToArray());
    }

    public virtual void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, Control labelControl, ComboBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, labelControl, Label);
      ReplaceControl(controls, valueControl, ValueControl);
    }

    public virtual void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, ComboBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, valueControl, ValueControl);
    }

    public override bool Enabled
    {
      set { base.Enabled = (SettingType == CECSettingType.Bool || BaseItems.Count > 1) && value; }
      get { return base.Enabled; }
    }

    public int Width = 100;
    public int Height = 21;

    public event ListControlConvertEventHandler Format;
    protected readonly List<int> BaseItems = new List<int>();
    public virtual List<int> Items
    {
      get
      {
        if (BaseItems.Count == 0)
        {
          for (var i = LowerLimit; i < UpperLimit; i += Step)
          {
            if (AllowedValue(i))
              BaseItems.Add(i);
          }
        }
        return BaseItems;
      }
      protected set
      {
        ComboBox control = BaseValueControl as ComboBox;
        if (control == null) return;
        List<object> cbItems = new List<object>();
        BaseItems.Clear();
        foreach (var item in value)
        {
          BaseItems.Add(item);
          cbItems.Add(string.Format("{0}", item));
        }

        Form.SetComboBoxItems(control, BaseItems.IndexOf(Value), cbItems.ToArray());
      }
    }
  }
}

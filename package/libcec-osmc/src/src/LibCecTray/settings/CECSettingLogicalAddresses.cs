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

using System;
using System.Windows.Forms;
using CecSharp;
using LibCECTray.Properties;
using LibCECTray.ui;

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type CecLogicalAddresses that can be persisted in the registry
  /// </summary>
  class CECSettingLogicalAddresses : CECSettingNumeric
  {
    public CECSettingLogicalAddresses(string keyName, string friendlyName, CecLogicalAddresses defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.LogicalAddresses, keyName, friendlyName, SerialiseLogicalAddresses(defaultValue), changedHandler)
    {
    }

    private static int SerialiseLogicalAddresses(CecLogicalAddresses addresses)
    {
      var retVal = 0;
      for (int addr = (int)CecLogicalAddress.Tv; addr <= (int)CecLogicalAddress.Broadcast; addr++)
      {
        if (addresses.IsSet((CecLogicalAddress)addr))
        {
          retVal += (int)Math.Pow(2, addr);
        }
      }

      return retVal;
    }

    private static CecLogicalAddresses DeserialiseLogicalAddresses(int addresses)
    {
      CecLogicalAddresses retVal = new CecLogicalAddresses();
      if (addresses == 0)
        return retVal;

      for (int addr = (int)CecLogicalAddress.Tv; addr <= (int)CecLogicalAddress.Broadcast; addr++)
      {
        int val = (int)Math.Pow(2, addr);
        if ((addresses & val) == val)
          retVal.Set((CecLogicalAddress)addr);
      }

      return retVal;
    }

    public new CecLogicalAddresses Value
    {
      get { return DeserialiseLogicalAddresses(base.Value); }
      set
      {
        base.Value = SerialiseLogicalAddresses(value);
        if (Form == null) return;
        for (int iPtr = 0; iPtr < 15; iPtr++)
          Form.SetCheckboxItemChecked(ValueControl, iPtr, value.IsSet((CecLogicalAddress) iPtr));
      }
    }

    public new CecLogicalAddresses DefaultValue
    {
      get { return DeserialiseLogicalAddresses(base.DefaultValue); }
      set { base.DefaultValue = SerialiseLogicalAddresses(value); }
    }

    public static CecLogicalAddress GetLogicalAddressFromString(string name)
    {
      switch (name.Substring(0, 1).ToLower())
      {
        case "0":
          return CecLogicalAddress.Tv;
        case "1":
          return CecLogicalAddress.RecordingDevice1;
        case "2":
          return CecLogicalAddress.RecordingDevice2;
        case "3":
          return CecLogicalAddress.Tuner1;
        case "4":
          return CecLogicalAddress.PlaybackDevice1;
        case "5":
          return CecLogicalAddress.AudioSystem;
        case "6":
          return CecLogicalAddress.Tuner2;
        case "7":
          return CecLogicalAddress.Tuner3;
        case "8":
          return CecLogicalAddress.PlaybackDevice2;
        case "9":
          return CecLogicalAddress.RecordingDevice3;
        case "a":
          return CecLogicalAddress.Tuner4;
        case "b":
          return CecLogicalAddress.PlaybackDevice3;
        case "c":
          return CecLogicalAddress.Reserved1;
        case "d":
          return CecLogicalAddress.Reserved2;
        case "e":
          return CecLogicalAddress.FreeUse;
        case "f":
          return CecLogicalAddress.Broadcast;
        default:
          return CecLogicalAddress.Unknown;
      }
    }

    public new CheckedListBox ValueControl
    {
      get
      {
        if (BaseValueControl == null)
        {
          CheckedListBox control = new CheckedListBox
          {
            FormattingEnabled = true,
            Name = KeyName,
            Size = new System.Drawing.Size(118, 94),
            Enabled = InitialEnabledValue
          };
          control.Items.AddRange(new object[] {
            "0: " + Resources.device_tv,
            "1: " + Resources.device_recorder + " 1",
            "2: " + Resources.device_recorder + " 2",
            "3: " + Resources.device_tuner + " 1",
            "4: " + Resources.device_playbackdevice + " 1",
            "5: " + Resources.device_audiosystem,
            "6: " + Resources.device_tuner + " 2",
            "7: " + Resources.device_tuner + " 3",
            "8: " + Resources.device_playbackdevice + " 2",
            "9: " + Resources.device_recorder + " 3",
            "A: " + Resources.device_tuner + " 4",
            "B: " + Resources.device_playbackdevice + " 3",
            "C: " + Resources.device_reserved + " 1",
            "D: " + Resources.device_reserved + " 2",
            "E: " + Resources.device_free_use,
            "F: " + Resources.device_broadcast});
          control.SelectedValueChanged += delegate
          {
            CecLogicalAddresses addr = new CecLogicalAddresses();
            foreach (var item in ((CheckedListBox)BaseValueControl).CheckedItems)
            {
              string c = item as string;
              addr.Set(GetLogicalAddressFromString(c));
            }
            Value = addr;
          };
          BaseValueControl = control;
        }
        return BaseValueControl as CheckedListBox;
      }
    }

    public void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, Control labelControl, CheckedListBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, labelControl, Label);
      ReplaceControl(controls, valueControl, ValueControl);
    }

    public void ReplaceControls(IAsyncControls form, Control.ControlCollection controls, CheckedListBox valueControl)
    {
      Form = form;
      ReplaceControl(controls, valueControl, ValueControl);
    }

    public override bool Enabled
    {
      set
      {
        if (Form != null)
          Form.SetControlEnabled(ValueControl, value);
      }
      get { return base.Enabled; }
    }
  }
}

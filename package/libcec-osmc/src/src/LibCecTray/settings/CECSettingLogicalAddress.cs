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

﻿using System.Windows.Forms;
using CecSharp;
using LibCECTray.Properties;

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type CecLogicalAddress that can be persisted in the registry
  /// </summary>
  class CECSettingLogicalAddress : CECSettingNumeric
  {
    public CECSettingLogicalAddress(string keyName, string friendlyName, CecLogicalAddress defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.LogicalAddress, keyName, friendlyName, (int)defaultValue, changedHandler, OnFormat)
    {
      LowerLimit = (int) CecLogicalAddress.Tv;
      UpperLimit = (int) CecLogicalAddress.Broadcast;
    }

    private static void OnFormat(object sender, ListControlConvertEventArgs listControlConvertEventArgs)
    {
      int iValue;
      if (int.TryParse((string)listControlConvertEventArgs.Value, out iValue))
        listControlConvertEventArgs.Value = FormatValue(iValue);
    }

    public new CecLogicalAddress Value
    {
      get { return base.Value >= (int)CecLogicalAddress.Tv && base.Value <= (int)CecLogicalAddress.Broadcast ? (CecLogicalAddress)base.Value : CecLogicalAddress.Unknown; }
      set { base.Value = (int)value; }
    }

    public new CecLogicalAddress DefaultValue
    {
      get { return base.DefaultValue >= (int)CecLogicalAddress.Tv && base.DefaultValue <= (int)CecLogicalAddress.Broadcast ? (CecLogicalAddress)base.DefaultValue : CecLogicalAddress.Unknown; }
      set { base.DefaultValue = (int)value; }
    }

    private static string FormatValue(int value)
    {
      switch ((CecLogicalAddress)value)
      {
        case CecLogicalAddress.AudioSystem:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_audiosystem : CECSettings.VendorNames[value] + " " + Resources.device_audiosystem;
        case CecLogicalAddress.Broadcast:
          return Resources.device_broadcast;
        case CecLogicalAddress.FreeUse:
          return Resources.device_free_use;
        case CecLogicalAddress.PlaybackDevice1:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_playbackdevice + " 1" : CECSettings.VendorNames[value] + " " + Resources.device_playbackdevice + " 1";
        case CecLogicalAddress.PlaybackDevice2:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_playbackdevice + " 2" : CECSettings.VendorNames[value] + " " + Resources.device_playbackdevice + " 2";
        case CecLogicalAddress.PlaybackDevice3:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_playbackdevice + " 3" : CECSettings.VendorNames[value] + " " + Resources.device_playbackdevice + " 3";
        case CecLogicalAddress.RecordingDevice1:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_recorder + " 1" : CECSettings.VendorNames[value] + " " + Resources.device_recorder + " 1";
        case CecLogicalAddress.RecordingDevice2:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_recorder + " 2" : CECSettings.VendorNames[value] + " " + Resources.device_recorder + " 2";
        case CecLogicalAddress.RecordingDevice3:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_recorder + " 3" : CECSettings.VendorNames[value] + " " + Resources.device_recorder + " 3";
        case CecLogicalAddress.Reserved1:
          return Resources.device_reserved + " 1";
        case CecLogicalAddress.Reserved2:
          return Resources.device_reserved + " 2";
        case CecLogicalAddress.Tuner1:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_tuner + " 1" : CECSettings.VendorNames[value] + " " + Resources.device_tuner + " 1";
        case CecLogicalAddress.Tuner2:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_tuner + " 2" : CECSettings.VendorNames[value] + " " + Resources.device_tuner + " 2";
        case CecLogicalAddress.Tuner3:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_tuner + " 3" : CECSettings.VendorNames[value] + " " + Resources.device_tuner + " 3";
        case CecLogicalAddress.Tuner4:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_tuner + " 4" : CECSettings.VendorNames[value] + " " + Resources.device_tuner + " 4";
        case CecLogicalAddress.Tv:
          return CECSettings.VendorNames[value] == null || CECSettings.VendorNames[value].Length == 0 ? Resources.device_tv : CECSettings.VendorNames[value] + " " + Resources.device_tv;
        default:
          return Resources.unknown;
      }
    }

    protected override bool AllowedValue(int value)
    {
      return AllowedAddressMask.IsSet((CecLogicalAddress)value);
    }

    private CecLogicalAddresses _allowedAddressMask;
    public CecLogicalAddresses AllowedAddressMask
    {
      get
      {
        if (_allowedAddressMask == null)
        {
          _allowedAddressMask = new CecLogicalAddresses();
          for (int iPtr = 0; iPtr < 16; iPtr++)
            _allowedAddressMask.Set((CecLogicalAddress)iPtr);
        }
        return _allowedAddressMask;
      }
      set
      {
        _allowedAddressMask = value;
        ResetItems(true);
      }
    }
  }

}

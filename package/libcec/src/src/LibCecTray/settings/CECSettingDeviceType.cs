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
using CecSharp;
using LibCECTray.Properties;

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type CecDeviceType that can be persisted in the registry
  /// </summary>
  class CECSettingDeviceType : CECSettingNumeric
  {
    public CECSettingDeviceType(string keyName, string friendlyName, CecDeviceType defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.DeviceType, keyName, friendlyName, (int)defaultValue, changedHandler, OnFormat)
    {
      LowerLimit = (int) CecDeviceType.Tv;
      UpperLimit = (int) CecDeviceType.AudioSystem;
    }

    private static void OnFormat(object sender, ListControlConvertEventArgs listControlConvertEventArgs)
    {
      int iValue;
      if (int.TryParse((string)listControlConvertEventArgs.Value, out iValue))
        listControlConvertEventArgs.Value = FormatValue(iValue);
    }

    public new CecDeviceType Value
    {
      get { return base.Value >= (int)CecDeviceType.Tv && base.Value <= (int)CecDeviceType.AudioSystem ? (CecDeviceType)base.Value : CecDeviceType.Reserved; }
      set
      {
        base.Value = (int)value;
        if (Form == null) return;
        switch (value)
        {
          case CecDeviceType.RecordingDevice:
            Form.SetControlText(ValueControl, Resources.device_recorder);
            break;
          case CecDeviceType.PlaybackDevice:
            Form.SetControlText(ValueControl, Resources.device_playbackdevice);
            break;
          case CecDeviceType.Tuner:
            Form.SetControlText(ValueControl, Resources.device_tuner);
            break;
          default:
            Form.SetControlText(ValueControl, Resources.device_recorder);
            break;
        }
      }
    }

    public new CecDeviceType DefaultValue
    {
      get { return base.DefaultValue >= (int)CecDeviceType.Tv && base.DefaultValue <= (int)CecDeviceType.AudioSystem ? (CecDeviceType)base.DefaultValue : CecDeviceType.Reserved; }
      set { base.DefaultValue = (int)value; }
    }

    private static string FormatValue(int value)
    {
      switch ((CecDeviceType)value)
      {
        case CecDeviceType.Tv:
          return Resources.device_tv;
        case CecDeviceType.PlaybackDevice:
          return Resources.device_playbackdevice;
        case CecDeviceType.AudioSystem:
          return Resources.device_audiosystem;
        case CecDeviceType.RecordingDevice:
          return Resources.device_recorder;
        case CecDeviceType.Tuner:
          return Resources.device_tuner;
        case CecDeviceType.Reserved:
          return Resources.device_reserved;
      }
      return Resources.unknown;
    }

    protected override bool AllowedValue(int value)
    {
      if ((CecDeviceType)value == CecDeviceType.Reserved)
        return false;
      for (var i = 0; i < _allowedTypeMask.Types.Length; i++)
      {
        if (_allowedTypeMask.Types[i] == (CecDeviceType) value)
          return true;
      }
      return false;
    }

    private CecDeviceTypeList _allowedTypeMask;
    public CecDeviceTypeList AllowedTypeMask
    {
      get
      {
        return _allowedTypeMask ?? (_allowedTypeMask = new CecDeviceTypeList
                                                         {
                                                           Types =
                                                             new[]
                                                               {
                                                                 CecDeviceType.Tv, CecDeviceType.PlaybackDevice,
                                                                 CecDeviceType.RecordingDevice,
                                                                 CecDeviceType.AudioSystem, CecDeviceType.Tuner
                                                               }
                                                         });
      }
      set
      {
        _allowedTypeMask = value;
        ResetItems(true);
      }
    }
  }
}

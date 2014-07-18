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
using CecSharp;
using LibCECTray.Properties;

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type CecVendorId that can be persisted in the registry
  /// </summary>
  class CECSettingVendorId : CECSettingNumeric
  {
    public CECSettingVendorId(string keyName, string friendlyName, CecVendorId defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.VendorId, keyName, friendlyName, (int)defaultValue, changedHandler, OnFormat, new List<int>
                 {
                   (int) CecVendorId.Unknown,
                   (int) CecVendorId.Akai,
                   (int) CecVendorId.Benq,
                   (int) CecVendorId.Daewoo,
                   (int) CecVendorId.Grundig,
                   (int) CecVendorId.LG,
                   (int) CecVendorId.Medion,
                   (int) CecVendorId.Onkyo,
                   (int) CecVendorId.Panasonic,
                   (int) CecVendorId.Philips,
                   (int) CecVendorId.Pioneer,
                   (int) CecVendorId.Samsung,
                   (int) CecVendorId.Sharp,
                   (int) CecVendorId.Sony,
                   (int) CecVendorId.Toshiba,
                   (int) CecVendorId.Vizio,
                   (int) CecVendorId.Yamaha
                 })
    {
    }

    private static void OnFormat(object sender, ListControlConvertEventArgs listControlConvertEventArgs)
    {
      int iValue;
      if (int.TryParse((string)listControlConvertEventArgs.Value, out iValue))
        listControlConvertEventArgs.Value = FormatValue(iValue);
    }

    private static string FormatValue(int value)
    {
      switch ((CecVendorId)value)
      {
        case CecVendorId.Akai:
          return "Akai";
        case CecVendorId.Benq:
          return "Benq";
        case CecVendorId.Daewoo:
          return "Daewoo";
        case CecVendorId.Grundig:
          return "Grundig";
        case CecVendorId.LG:
          return "LG";
        case CecVendorId.Medion:
          return "Medion";
        case CecVendorId.Onkyo:
          return "Onkyo";
        case CecVendorId.Panasonic:
          return "Panasonic";
        case CecVendorId.Philips:
          return "Philips";
        case CecVendorId.Pioneer:
          return "Pioneer";
        case CecVendorId.Samsung:
          return "Samsung";
        case CecVendorId.Sharp:
          return "Sharp";
        case CecVendorId.Sony:
          return "Sony";
        case CecVendorId.Toshiba:
          return "Toshiba";
        case CecVendorId.Vizio:
          return "Vizio";
        case CecVendorId.Yamaha:
          return "Yamaha";
        default:
          return Resources.autodetect;
      }
    }

    public new CecVendorId Value
    {
      get { return base.Value >= 0 && base.Value <= 0xFFFFFF ? (CecVendorId)base.Value : CecVendorId.Unknown; }
      set { base.Value = (int)value; }
    }

    public new CecVendorId DefaultValue
    {
      get { return base.DefaultValue >= 0 && base.DefaultValue <= 0xFFFFFF ? (CecVendorId)base.DefaultValue : CecVendorId.Unknown; }
      set { base.DefaultValue = (int)value; }
    }
  }

}

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

namespace LibCECTray.settings
{
  /// <summary>
  /// A setting of type byte that can be persisted in the registry
  /// </summary>
  class CECSettingByte : CECSettingNumeric
  {
    public CECSettingByte(string keyName, string friendlyName, byte defaultValue, SettingChangedHandler changedHandler) :
      base(CECSettingType.Byte, keyName, friendlyName, defaultValue, changedHandler)
    {
    }

    public new byte Value
    {
      get { return base.Value <= byte.MaxValue && base.Value >= byte.MinValue ? (byte)base.Value : byte.MaxValue; }
      set { base.Value = value; }
    }

    public new byte DefaultValue
    {
      get { return base.DefaultValue <= byte.MaxValue && base.DefaultValue >= byte.MinValue ? (byte)base.DefaultValue : byte.MaxValue; }
      set { base.DefaultValue = value; }
    }
  }
}

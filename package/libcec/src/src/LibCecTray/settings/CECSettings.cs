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
using System.Collections.Generic;
using LibCECTray.Properties;
using Microsoft.Win32;

namespace LibCECTray.settings
{
  class CECSettings
  {
    private const string RegistryCompanyName = "Pulse-Eight";
    private const string RegistryApplicationName = "libCECTray";

    #region Key names
    public static string KeyHDMIPort = "global_hdmi_port";
    public static string KeyConnectedToHDMIDevice = "global_connected_to_hdmi_device";
    public static string KeyActivateSource = "global_activate_source";
    public static string KeyAdvancedMode = "global_advanced_mode";
    public static string KeyPhysicalAddress = "global_physical_address";
    public static string KeyOverridePhysicalAddress = "global_override_physical_address";
    public static string KeyDeviceType = "global_device_type";
    public static string KeyTVVendor = "global_tv_vendor";
    public static string KeyOverrideTVVendor = "global_override_tv_vendor";
    public static string KeyWakeDevices = "global_wake_devices";
    public static string KeyPowerOffDevices = "global_standby_devices";
    public static string KeyStartHidden = "global_start_hidden";
    #endregion

    public CECSettings(CECSetting.SettingChangedHandler changedHandler)
    {
      _changedHandler = changedHandler;
      Load();
    }

    /// <summary>
    /// Resets all settings to their default values
    /// </summary>
    public void SetDefaultValues()
    {
      foreach (var setting in _settings.Values)
        setting.ResetDefaultValue();
    }

    /// <summary>
    /// Loads all known settings from the registry
    /// </summary>
    private void Load()
    {
      using (var subRegKey = Registry.CurrentUser.OpenSubKey(RegistryKeyName, true))
      {
        if (subRegKey == null) return;
        foreach (var setting in _settings.Values)
          if (!setting.KeyName.StartsWith("global_"))
            setting.Load(subRegKey);
        subRegKey.Close();
      }
    }

    /// <summary>
    /// Load a setting value from the registry
    /// </summary>
    /// <param name="setting">The setting to load</param>
    public void Load(CECSetting setting)
    {
      using (var subRegKey = Registry.CurrentUser.OpenSubKey(RegistryKeyName, true))
      {
        if (subRegKey == null) return;
        setting.Load(subRegKey);
        subRegKey.Close();
      }
      setting.SettingChanged += OnSettingChanged;
    }

    /// <summary>
    /// Persist all settings in the registry
    /// </summary>
    /// <returns>True when persisted, false otherwise</returns>
    public bool Persist()
    {
      if (CreateRegistryKey())
      {
        using (var subRegKey = Registry.CurrentUser.OpenSubKey(RegistryKeyName, true))
        {
          if (subRegKey != null)
          {
            foreach (var setting in _settings.Values)
              setting.Persist(subRegKey);
            subRegKey.Close();
            return true;
          }
        }
      }
      return false;
    }

    private bool EnableHDMIPortSetting(CECSetting setting, bool value)
    {
      return value && !OverridePhysicalAddress.Value;
    }

    private bool EnablePhysicalAddressSetting(CECSetting setting, bool value)
    {
      return value && OverridePhysicalAddress.Value;
    }

    private bool EnableSettingTVVendor(CECSetting setting, bool value)
    {
      return value && OverrideTVVendor.Value;
    }

    private void OnSettingChanged(CECSetting setting, object oldValue, object newValue)
    {
      if (SettingChanged != null)
        SettingChanged(setting, oldValue, newValue);
    }

    #region Global settings
    public CECSettingByte HDMIPort
    {
      get
      {
        if (!_settings.ContainsKey(KeyHDMIPort))
        {
          CECSettingByte setting = new CECSettingByte(KeyHDMIPort, "HDMI port", 1, _changedHandler) { LowerLimit = 1, UpperLimit = 15, EnableSetting = EnableHDMIPortSetting };
          setting.Format += delegate(object sender, ListControlConvertEventArgs args)
          {
            ushort tmp;
            if (ushort.TryParse((string)args.Value, out tmp))
              args.Value = "HDMI " + args.Value;
          };

          Load(setting);
          _settings[KeyHDMIPort] = setting;
        }
        return _settings[KeyHDMIPort].AsSettingByte;
      }
    }

    public CECSettingLogicalAddress ConnectedDevice
    {
      get
      {
        if (!_settings.ContainsKey(KeyConnectedToHDMIDevice))
        {
          CecLogicalAddresses allowedMask = new CecLogicalAddresses();
          allowedMask.Set(CecLogicalAddress.Tv); allowedMask.Set(CecLogicalAddress.AudioSystem);
          CECSettingLogicalAddress setting = new CECSettingLogicalAddress(KeyConnectedToHDMIDevice,
                                                                          Resources.global_connected_to_hdmi_device,
                                                                          CecLogicalAddress.Tv, _changedHandler)
                                               {
                                                 AllowedAddressMask = allowedMask,
                                                 Enabled = false,
                                                 EnableSetting = EnableHDMIPortSetting
                                               };
          Load(setting);
          _settings[KeyConnectedToHDMIDevice] = setting;
        }
        return _settings[KeyConnectedToHDMIDevice].AsSettingLogicalAddress;
      }
    }

    public CECSettingBool ActivateSource
    {
      get
      {
        if (!_settings.ContainsKey(KeyActivateSource))
        {
          CECSettingBool setting = new CECSettingBool(KeyActivateSource, Resources.global_activate_source, true,
                                                      _changedHandler) {Enabled = false};
          Load(setting);
          _settings[KeyActivateSource] = setting;
        }
        return _settings[KeyActivateSource].AsSettingBool;
      }
    }

    public CECSettingBool AdvancedMode
    {
      get
      {
        if (!_settings.ContainsKey(KeyAdvancedMode))
        {
          CECSettingBool setting = new CECSettingBool(KeyAdvancedMode, Resources.global_advanced_mode, false,
                                                      _changedHandler) {Enabled = false};
          Load(setting);
          _settings[KeyAdvancedMode] = setting;
        }
        return _settings[KeyAdvancedMode].AsSettingBool;
      }
    }

    public CECSettingUShort PhysicalAddress
    {
      get
      {
        if (!_settings.ContainsKey(KeyPhysicalAddress))
        {
          CECSettingUShort setting = new CECSettingUShort(KeyPhysicalAddress, Resources.global_physical_address, 0xFFFF, _changedHandler) { Enabled = false, EnableSetting = EnablePhysicalAddressSetting };
          Load(setting);
          _settings[KeyPhysicalAddress] = setting;
        }
        return _settings[KeyPhysicalAddress].AsSettingUShort;
      }
    }

    public CECSettingBool OverridePhysicalAddress
    {
      get
      {
        if (!_settings.ContainsKey(KeyAdvancedMode))
        {
          CECSettingBool setting = new CECSettingBool(KeyOverridePhysicalAddress,
                                                      Resources.global_override_physical_address, false, _changedHandler);
          Load(setting);
          _settings[KeyOverridePhysicalAddress] = setting;
        }
        return _settings[KeyOverridePhysicalAddress].AsSettingBool;
      }
    }

    public CECSettingDeviceType DeviceType
    {
      get
      {
        if (!_settings.ContainsKey(KeyDeviceType))
        {
          CecDeviceTypeList allowedTypes = new CecDeviceTypeList();
          allowedTypes.Types[(int)CecDeviceType.RecordingDevice] = CecDeviceType.RecordingDevice;
          allowedTypes.Types[(int)CecDeviceType.PlaybackDevice] = CecDeviceType.PlaybackDevice;

          CECSettingDeviceType setting = new CECSettingDeviceType(KeyDeviceType, Resources.global_device_type,
                                                                  CecDeviceType.RecordingDevice, _changedHandler) { Enabled = false, AllowedTypeMask = allowedTypes };
          Load(setting);
          _settings[KeyDeviceType] = setting;
        }
        return _settings[KeyDeviceType].AsSettingDeviceType;
      }
    }

    public CECSettingVendorId TVVendor
    {
      get
      {
        if (!_settings.ContainsKey(KeyTVVendor))
        {
          CECSettingVendorId setting = new CECSettingVendorId(KeyTVVendor, Resources.global_tv_vendor,
                                                              CecVendorId.Unknown, _changedHandler)
                                         {Enabled = false, EnableSetting = EnableSettingTVVendor};
          Load(setting);
          _settings[KeyTVVendor] = setting;
        }
        return _settings[KeyTVVendor].AsSettingVendorId;
      }
    }

    public CECSettingBool OverrideTVVendor
    {
      get
      {
        if (!_settings.ContainsKey(KeyOverrideTVVendor))
        {
          CECSettingBool setting = new CECSettingBool(KeyOverrideTVVendor, Resources.global_override_tv_vendor, false,
                                                      _changedHandler) {Enabled = false};
          Load(setting);
          _settings[KeyOverrideTVVendor] = setting;
        }
        return _settings[KeyOverrideTVVendor].AsSettingBool;
      }
    }

    public CECSettingLogicalAddresses WakeDevices
    {
      get
      {
        if (!_settings.ContainsKey(KeyWakeDevices))
        {
          CecLogicalAddresses defaultDeviceList = new CecLogicalAddresses();
          defaultDeviceList.Set(CecLogicalAddress.Tv);
          CECSettingLogicalAddresses setting = new CECSettingLogicalAddresses(KeyWakeDevices,
                                                                              Resources.global_wake_devices,
                                                                              defaultDeviceList, _changedHandler)
                                                 {Enabled = false};
          Load(setting);
          _settings[KeyWakeDevices] = setting;
        }
        return _settings[KeyWakeDevices].AsSettingLogicalAddresses;
      }
    }

    public CECSettingLogicalAddresses PowerOffDevices
    {
      get
      {
        if (!_settings.ContainsKey(KeyPowerOffDevices))
        {
          CecLogicalAddresses defaultDeviceList = new CecLogicalAddresses();
          defaultDeviceList.Set(CecLogicalAddress.Tv);
          CECSettingLogicalAddresses setting = new CECSettingLogicalAddresses(KeyPowerOffDevices,
                                                                              Resources.global_standby_devices,
                                                                              defaultDeviceList,
                                                                              _changedHandler) {Enabled = false};
          Load(setting);
          _settings[KeyPowerOffDevices] = setting;
        }
        return _settings[KeyPowerOffDevices].AsSettingLogicalAddresses;
      }
    }

    public CECSettingBool StartHidden
    {
      get
      {
        if (!_settings.ContainsKey(KeyStartHidden))
        {
          CECSettingBool setting = new CECSettingBool(KeyStartHidden, Resources.global_start_hidden, false, _changedHandler);
          Load(setting);
          _settings[KeyStartHidden] = setting;
        }
        return _settings[KeyStartHidden].AsSettingBool;
      }
    }
    #endregion

    public bool ContainsKey(string key)
    {
      return _settings.ContainsKey(key);
    }

    public void SetVendorName(CecLogicalAddress address, CecVendorId vendorId, string vendorName)
    {
      VendorNames[(int)address] = vendorName;

      if (address == CecLogicalAddress.Tv && vendorId == CecVendorId.Panasonic)
      {
        DeviceType.AllowedTypeMask = new CecDeviceTypeList {Types = new[] {CecDeviceType.PlaybackDevice}};
      }

      foreach (var setting in _settings)
      {
        if (setting.Value.SettingType == CECSettingType.LogicalAddress)
          setting.Value.AsSettingLogicalAddress.ResetItems(true);
      }
    }

    public bool Enabled
    {
      set
      {
        CECSetting[] settings = new CECSetting[_settings.Count + 10];
        _settings.Values.CopyTo(settings, 0);
        foreach (var setting in settings)
          if (setting != null)
            setting.Enabled = value;
      }
      get
      {
        var enabled = true;
        foreach (var setting in _settings)
          enabled &= setting.Value.Enabled;
        return enabled;
      }
    }

    /// <summary>
    /// Read or write one of the settings, given it's key
    /// </summary>
    /// <param name="key">The key of the setting</param>
    /// <returns>The setting</returns>
    public CECSetting this[string key]
    {
      get { return _settings.ContainsKey(key) ? _settings[key] : null; }
      set {_settings[key] = value; }
    }

    /// <summary>
    /// Create the registry key that holds all settings.
    /// </summary>
    /// <returns>True when created (or already existing), false otherwise</returns>
    private static bool CreateRegistryKey()
    {
      using (var regKey = Registry.CurrentUser.OpenSubKey("Software", true))
      {
        if (regKey != null)
        {
          regKey.CreateSubKey(RegistryCompanyName);
          regKey.Close();
        }
        else
        {
          return false;
        }
      }
      using (var regKey = Registry.CurrentUser.OpenSubKey("Software\\" + RegistryCompanyName, true))
      {
        if (regKey != null)
        {
          regKey.CreateSubKey(RegistryApplicationName);
          regKey.Close();
        }
        else
        {
          return false;
        }
      }
      return true;
    }

    private readonly Dictionary<string, CECSetting> _settings = new Dictionary<string, CECSetting>();
    private static string RegistryKeyName
    {
      get { return string.Format("Software\\{0}\\{1}", RegistryCompanyName, RegistryApplicationName); }
    }

    private readonly CECSetting.SettingChangedHandler _changedHandler;
    public event CECSetting.SettingChangedHandler SettingChanged;

    public static readonly string[] VendorNames = new string[15];
  }
}

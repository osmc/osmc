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
using System.Collections.Generic;
using System.Windows.Forms;
using CecSharp;
using LibCECTray.Properties;
using LibCECTray.controller.applications;
using LibCECTray.settings;
using LibCECTray.ui;

namespace LibCECTray.controller
{
  internal class CECController : CecCallbackMethods
  {
    public CECController(CECTray gui)
    {
      _gui = gui;
      CECActions = new Actions(this);

      InitialiseGui();

      Settings.SettingChanged += OnSettingChanged;
    }

    #region Settings
    /// <summary>
    /// Called when a setting changed
    /// </summary>
    /// <param name="setting">The setting that changed</param>
    /// <param name="oldValue">The old value</param>
    /// <param name="newValue">The new value</param>
    private void OnSettingChanged(CECSetting setting, object oldValue, object newValue)
    {
      if (setting.KeyName == CECSettings.KeyHDMIPort)
      {
        CECSettingByte byteSetting = setting as CECSettingByte;
        if (byteSetting != null)
        {
          if (!Settings.OverridePhysicalAddress.Value)
            Config.HDMIPort = byteSetting.Value;
          CECActions.SetConnectedDevice(Settings.ConnectedDevice.Value, byteSetting.Value);
        }
      }
      else if (setting.KeyName == CECSettings.KeyConnectedToHDMIDevice)
      {
        CECSettingLogicalAddress laSetting = setting as CECSettingLogicalAddress;
        if (laSetting != null)
        {
          if (!Settings.OverridePhysicalAddress.Value)
            Config.BaseDevice = laSetting.Value;
          CECActions.SetConnectedDevice(laSetting.Value, Settings.HDMIPort.Value);
        }
      }
      else if (setting.KeyName == CECSettings.KeyPhysicalAddress)
      {
        CECSettingUShort ushortSetting = setting as CECSettingUShort;
        if (ushortSetting != null && Settings.OverridePhysicalAddress.Value && Config.PhysicalAddress != ushortSetting.Value)
        {
          Config.PhysicalAddress = ushortSetting.Value;
          CECActions.SetPhysicalAddress(ushortSetting.Value);
        }
      }
      else if (setting.KeyName == CECSettings.KeyOverridePhysicalAddress)
      {
        CECSettingBool boolSetting = setting as CECSettingBool;
        if (boolSetting != null)
        {
          Settings.PhysicalAddress.Enabled = boolSetting.Value;
          Settings.HDMIPort.Enabled = !boolSetting.Value;
          Settings.ConnectedDevice.Enabled = !boolSetting.Value;
          if (boolSetting.Value)
          {
            Config.BaseDevice = Settings.ConnectedDevice.Value;
            Config.HDMIPort = 0;
            Config.PhysicalAddress = Settings.PhysicalAddress.Value;
          }
          else
          {
            Config.BaseDevice = Settings.ConnectedDevice.Value;
            Config.HDMIPort = Settings.HDMIPort.Value;
            Config.PhysicalAddress = 0xFFFF;
          }
        }
      }
      else if (setting.KeyName == CECSettings.KeyDeviceType)
      {
        CECSettingDeviceType dtSetting = setting as CECSettingDeviceType;
        if (dtSetting != null)
        {
          if (dtSetting.Value != Config.DeviceTypes.Types[0] && Settings.OverrideTVVendor.Value)
          {
            Config.DeviceTypes.Types[0] = dtSetting.Value;
            if (!_deviceChangeWarningDisplayed)
            {
              _deviceChangeWarningDisplayed = true;
              MessageBox.Show(Resources.device_type_changed, Resources.app_name, MessageBoxButtons.OK,
                              MessageBoxIcon.Warning);
            }
          }
        }
      }
      else if (setting.KeyName == CECSettings.KeyOverrideTVVendor)
      {
        CECSettingBool boolSetting = setting as CECSettingBool;
        if (boolSetting != null)
        {
          Settings.TVVendor.Enabled = boolSetting.Value;
          Settings.TVVendor.Value = boolSetting.Value ? Lib.GetDeviceVendorId(CecLogicalAddress.Tv) : CecVendorId.Unknown;
          Config.TvVendor = boolSetting.Value ? Settings.TVVendor.Value : CecVendorId.Unknown;
        }
      }
      else if (setting.KeyName == CECSettings.KeyActivateSource)
      {
        CECSettingBool boolSetting = setting as CECSettingBool;
        if (boolSetting != null)
          Config.ActivateSource = boolSetting.Value;
      }
      else if (setting.KeyName == CECSettings.KeyTVVendor)
      {
        CECSettingVendorId vendorSetting = setting as CECSettingVendorId;
        if (vendorSetting != null && Settings.OverrideTVVendor.Value)
          Config.TvVendor = vendorSetting.Value;
      }
      else if (setting.KeyName == CECSettings.KeyWakeDevices)
      {
        CECSettingLogicalAddresses laSetting = setting as CECSettingLogicalAddresses;
        if (laSetting != null)
          Config.WakeDevices = laSetting.Value;
      }
      else if (setting.KeyName == CECSettings.KeyPowerOffDevices)
      {
        CECSettingLogicalAddresses laSetting = setting as CECSettingLogicalAddresses;
        if (laSetting != null)
          Config.PowerOffDevices = laSetting.Value;
      }
    }

    /// <summary>
    /// Persist all known settings in the registry
    /// </summary>
    public void PersistSettings()
    {
      /* save settings in the eeprom */
      Lib.SetConfiguration(Config);
      Lib.PersistConfiguration(Config);

      /* and in the registry */
      Settings.Persist();
    }

    /// <summary>
    /// Reset all settings to their default values
    /// </summary>
    public void ResetDefaultSettings()
    {
      SetControlsEnabled(false);
      _gui.ShowHideAdvanced(false);

      CECActions.SuppressUpdates = true;
      Settings.SetDefaultValues();
      _config = null;
      Lib.SetConfiguration(Config);
      CECActions.SuppressUpdates = false;

      _gui.ShowHideAdvanced(Settings.AdvancedMode.Value);
      SetControlsEnabled(true);
    }
    #endregion

    /// <summary>
    /// Opens a connection to libCEC and register known applications
    /// </summary>
    public void Initialise()
    {
      // only load once
      if (_initialised)
        return;
      _initialised = true;

      CECActions.ConnectToDevice(Config);
      Applications.Initialise(this);
    }

    /// <summary>
    /// Closes the connection to libCEC
    /// </summary>
    public void Close()
    {
      Lib.DisableCallbacks();
      Lib.StandbyDevices(CecLogicalAddress.Broadcast);
      Lib.Close();
      _initialised = false;
    }

    /// <summary>
    /// Register a new application controller, which will add a new tab for the application
    /// </summary>
    /// <param name="app">The new application to register</param>
    /// <returns>True when registered, false otherwise</returns>
    public bool RegisterApplication(ApplicationController app)
    {
      if (_applications.Contains(app)) return false;
      _applications.Add(app);

      _gui.SuspendLayout();
      _gui.TabControls.Add(app.UiControl);
      _gui.ResumeLayout();

      app.Initialise();

      return true;
    }

    #region GUI controls
    /// <summary>
    /// Initialises the UI
    /// </summary>
    private void InitialiseGui()
    {
      _gui.SuspendLayout();
      _gui.InitialiseSettingsComponent(Settings);

      // add the controller tabs to the gui
      foreach (var ui in ApplicationUIs)
        _gui.TabControls.Add(ui);

      // enable/disable advanced mode
      _gui.ShowHideAdvanced(Settings.AdvancedMode.Value);

      _gui.ResumeLayout();
    }

    /// <summary>
    /// Display a dialog
    /// </summary>
    /// <param name="control">The dialog to display</param>
    /// <param name="modal">True when modal</param>
    public void DisplayDialog(Form control, bool modal)
    {
      _gui.DisplayDialog(control, modal);
    }

    /// <summary>
    /// Changes the status text of this window
    /// </summary>
    /// <param name="status">The new status text</param>
    public void SetStatusText(string status)
    {
      _gui.SetStatusText(status);
    }

    /// <summary>
    /// Changes the progress bar value
    /// </summary>
    /// <param name="progress">The new progress percentage</param>
    /// <param name="visible">True to make the bar visible, false otherwise</param>
    public void SetProgressBar(int progress, bool visible)
    {
      _gui.SetProgressBar(progress, visible);
    }

    /// <summary>
    /// Enable/disable all controls
    /// </summary>
    /// <param name="val">True to enable, false to disable</param>
    public void SetControlsEnabled(bool val)
    {
      Settings.Enabled = val;
      foreach (var app in _applications)
        app.UiControl.SetEnabled(val);
      _gui.SetControlsEnabled(val);
    }

    /// <summary>
    /// (re)check the logical addresses of the active devices on the bus
    /// </summary>
    public void CheckActiveDevices()
    {
      var activeDevices = Lib.GetActiveDevices();
      List<string> deviceList = new List<string>();
      foreach (var activeDevice in activeDevices.Addresses)
      {
        if (activeDevice != CecLogicalAddress.Unknown)
          deviceList.Add(string.Format("{0,1:X} : {1}", (int)activeDevice, Lib.ToString(activeDevice)));
      }
      deviceList.Add(string.Format("{0,1:X} : {1}", (int)CecLogicalAddress.Broadcast, Lib.ToString(CecLogicalAddress.Broadcast)));

      _gui.SetActiveDevices(deviceList.ToArray());
    }

    /// <summary>
    /// Show/hide the taskbar entry
    /// </summary>
    /// <param name="val">True to show, false to hide</param>
    public void SetShowInTaskbar(bool val)
    {
      _gui.SetShowInTaskbar(val);
    }

    /// <summary>
    /// Show or hide this window
    /// </summary>
    /// <param name="val">True to show, false to hide</param>
    public void Hide(bool val)
    {
      _gui.SafeHide(val);
    }
    #endregion

    #region Callbacks called by libCEC
    public override int ReceiveCommand(CecCommand command)
    {
      return 0;
    }

    public override int ReceiveKeypress(CecKeypress key)
    {
      foreach (var app in _applications)
      {
        bool keySent = app.SendKey(key, app.UiName == _gui.SelectedTabName);

        if (keySent)
        {
          string strLog = string.Format("sent key '{0}' to '{1}'", (int) key.Keycode, app.UiName) + Environment.NewLine;
          _gui.AddLogMessage(strLog);
          break;
        }
      }

      return 1;
    }

    public override int ReceiveLogMessage(CecLogMessage message)
    {
      _gui.AddLogMessage(message);
      return 1;
    }

    public override int ReceiveAlert(CecAlert alert, CecParameter data)
    {
      switch (alert)
      {
        case CecAlert.ServiceDevice:
          MessageBox.Show(Resources.alert_service_device, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning);
          break;
        case CecAlert.ConnectionLost:
          MessageBox.Show(Resources.alert_connection_lost, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning);
          break;
        case CecAlert.PermissionError:
          MessageBox.Show(Resources.alert_permission_error, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning);
          break;
        case CecAlert.PortBusy:
          MessageBox.Show(Resources.alert_port_busy, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning);
          break;
        case CecAlert.PhysicalAddressError:
          MessageBox.Show(Resources.alert_physical_address_error, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning);
          break;
        case CecAlert.TVPollFailed:
          MessageBox.Show(Resources.alert_tv_poll_failed, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning);
          break;
      }
      return 1;
    }

    public override int ConfigurationChanged(LibCECConfiguration config)
    {
      Settings.PhysicalAddress.Value = Config.PhysicalAddress;
      Settings.ConnectedDevice.Value = Config.BaseDevice == CecLogicalAddress.AudioSystem ? CecLogicalAddress.AudioSystem : CecLogicalAddress.Tv;
      Settings.HDMIPort.Value = Config.HDMIPort;
      Settings.WakeDevices.Value = Config.WakeDevices;
      Settings.PowerOffDevices.Value = Config.PowerOffDevices;
      Settings.ActivateSource.Value = Config.ActivateSource;
      Settings.DeviceType.Value = config.DeviceTypes.Types[0];

      if (config.TvVendor != CecVendorId.Unknown)
      {
        Settings.OverrideTVVendor.Value = true;
        Settings.TVVendor.Value = config.TvVendor;
      }
      else
      {
        Settings.OverrideTVVendor.Value = false;
      }

      _gui.SetControlText(_gui, Resources.app_name + " - libCEC " + Lib.ToString(Config.ServerVersion));

      CECActions.UpdatePhysicalAddress();
      return 1;
    }

    public override void SourceActivated(CecLogicalAddress logicalAddress, bool activated)
    {
      if (!activated)
        return;

      foreach (var app in _applications)
      {
        if (app.AutoStartApplication.Value)
          app.Start(false);
      }
    }
    #endregion

    #region Members
    /// <summary>
    /// List of tab pages for each application that the UI supports
    /// </summary>
    public List<ControllerTabPage> ApplicationUIs
    {
      get
      {
        List<ControllerTabPage> retVal = new List<ControllerTabPage>();
        foreach (var app in _applications)
          retVal.Add(app.UiControl);
        return retVal;
      }
    }

    /// <summary>
    /// List of application controllers that the UI supports
    /// </summary>
    private readonly List<ApplicationController> _applications = new List<ApplicationController>();

    /// <summary>
    /// Settings that are persisted in the registry (when not using the default value)
    /// </summary>
    public CECSettings Settings
    {
      get { return _settings ?? (_settings = new CECSettings(OnSettingChanged)); }
    }
    private CECSettings _settings;

    /// <summary>
    /// libCEC configuration for the application
    /// </summary>
    private LibCECConfiguration Config
    {
      get
      {
        if (_config == null)
        {
          _config = new LibCECConfiguration { DeviceName = "CEC Tray", ClientVersion = CecClientVersion.CurrentVersion };
          _config.DeviceTypes.Types[0] = CecDeviceType.RecordingDevice;
          _config.SetCallbacks(this);

          if (Settings.OverridePhysicalAddress.Value)
          {
            _config.PhysicalAddress = Settings.PhysicalAddress.Value;
            _config.HDMIPort = 0;
            _config.BaseDevice = CecLogicalAddress.Unknown;
          }
          else
          {
            _config.PhysicalAddress = 0;
            _config.HDMIPort = Settings.HDMIPort.Value;
            _config.BaseDevice = Settings.ConnectedDevice.Value;
          }
          _config.ActivateSource = Settings.ActivateSource.Value;
          _config.DeviceTypes.Types[0] = Settings.DeviceType.Value;
          if (Settings.OverrideTVVendor.Value)
            _config.TvVendor = Settings.TVVendor.Value;
          _config.WakeDevices = Settings.WakeDevices.Value;
          _config.PowerOffDevices = Settings.PowerOffDevices.Value;
        }
        return _config;
      }
    }
    private LibCECConfiguration _config;

    /// <summary>
    /// Get build info from libCEC
    /// </summary>
    public string LibInfo
    {
      get { return Lib.GetLibInfo(); }
    }

    /// <summary>
    /// libCEC API version
    /// </summary>
    public string LibServerVersion
    {
      get { return Lib.ToString(Config.ServerVersion); }
    }

    /// <summary>
    /// libCEC client API version
    /// </summary>
    public string LibClientVersion
    {
      get { return Lib.ToString(Config.ClientVersion); }
    }

    /// <summary>
    /// Get the usb vendor id
    /// </summary>
    public ushort AdapterVendorId
    {
      get { return Lib.GetAdapterVendorId(); }
    }

    /// <summary>
    /// Get the usb product id
    /// </summary>
    public ushort AdapterProductId
    {
      get { return Lib.GetAdapterProductId(); }
    }


    /// <summary>
    /// libCEC
    /// </summary>
    public LibCecSharp Lib
    {
      get { return _lib ?? (_lib = new LibCecSharp(Config)); }
    }
    private LibCecSharp _lib;

    private readonly CECTray _gui;
    public Actions CECActions;
    private bool _deviceChangeWarningDisplayed;
    private bool _initialised;

    #endregion
  }
}

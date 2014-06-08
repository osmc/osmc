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
using System.Globalization;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using CecSharp;
using LibCECTray.Properties;
using LibCECTray.settings;

namespace LibCECTray.controller.applications.@internal
{
	internal class XBMCController : ApplicationController
	{
    public XBMCController(CECController controller) :
      base(controller,
           Resources.application_xbmc,
           "XBMC",
           "XBMC.exe",
           Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles) + @"\XBMC")
    {
      IsInternal = true;
      AutoStartApplication.Value = false;
      ControlApplication.Value = false;

      LoadXMLConfiguration();

      ApplicationRunningChanged += RunningChanged;
    }

	  static void RunningChanged(bool running)
    {
      if (running)
      {
        // XBMC is running, close the application, or we'll block communication
        Application.Exit();
      }
    }

    public override ApplicationAction DefaultValue(CecKeypress key)
	  {
	    return null;
	  }

    public override ControllerTabPage UiControl
    {
      get { return UIControlInternal ?? (UIControlInternal = new XBMCControllerUI(this)); }
    }

    public bool LoadXMLConfiguration()
    {
      var xbmcDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + @"\XBMC\userdata\peripheral_data";
      return LoadXMLConfiguration(xbmcDir + string.Format(@"\usb_{0:X}_{1:X}.xml", Program.Instance.Controller.AdapterVendorId, Program.Instance.Controller.AdapterProductId)) ||
             LoadXMLConfiguration(xbmcDir + @"\usb_2548_1001.xml") ||
             LoadXMLConfiguration(xbmcDir + @"\usb_2548_1002.xml");
    }

    public bool LoadXMLConfiguration(string filename)
    {
      bool gotConfig = false;
      if (File.Exists(filename))
      {
        XmlTextReader reader = new XmlTextReader(filename);
        while (true)
        {
          try
          {
            if (!reader.Read())
              break;
          } catch (XmlException) {}
          gotConfig = true;
          switch (reader.NodeType)
          {
            case XmlNodeType.Element:
              if (reader.Name.ToLower() == "setting")
              {
                string name = string.Empty;
                string value = string.Empty;

                while (reader.MoveToNextAttribute())
                {
                  if (reader.Name.ToLower().Equals("id"))
                    name = reader.Value.ToLower();
                  if (reader.Name.ToLower().Equals("value"))
                    value = reader.Value;
                }

                switch (name)
                {
                  case "cec_hdmi_port":
                    {
                      byte iPort;
                      if (byte.TryParse(value, out iPort))
                        Settings.HDMIPort.Value = iPort;
                    }
                    break;
                  case "connected_device":
                    {
                      int iDevice;
                      if (int.TryParse(value, out iDevice))
                        Settings.ConnectedDevice.Value = iDevice == 36038 ? CecLogicalAddress.AudioSystem : CecLogicalAddress.Tv;
                    }
                    break;
                  case "cec_power_on_startup":
                    if (value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes"))
                    {
                      Settings.ActivateSource.Value = true;
                      Settings.WakeDevices.Value.Set(CecLogicalAddress.Tv);
                    }
                    break;
                  case "cec_power_off_shutdown":
                    if (value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes"))
                      Settings.PowerOffDevices.Value.Set(CecLogicalAddress.Broadcast);
                    break;
                  case "cec_standby_screensaver":
                    StandbyScreensaver.Value = value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes");
                    break;
                  case "standby_pc_on_tv_standby":
                    PowerOffOnStandby.Value = value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes");
                    break;
                  case "use_tv_menu_language":
                    UseTVLanguage.Value = value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes");
                    break;
                  // 1.5.0+ settings
                  case "physical_address":
                    {
                      ushort physicalAddress;
                      if (ushort.TryParse(value, NumberStyles.AllowHexSpecifier, null, out physicalAddress))
                        Settings.PhysicalAddress.Value = physicalAddress;
                    }
                    break;
                  case "device_type":
                    {
                      ushort iType;
                      if (ushort.TryParse(value, out iType))
                        Settings.DeviceType.Value = (CecDeviceType)iType;
                    }
                    break;
                  case "tv_vendor":
                    {
                      UInt64 iVendor;
                      if (UInt64.TryParse(value, out iVendor))
                        Settings.TVVendor.Value = (CecVendorId)iVendor;
                    }
                    break;
                  case "wake_device":
                    {
                      int iWakeDevices;
                      if (int.TryParse(value, out iWakeDevices))
                      {
                        Settings.WakeDevices.Value.Clear();
                        switch (iWakeDevices)
                        {
                          case 36037:
                            Settings.WakeDevices.Value.Set(CecLogicalAddress.Tv);
                            break;
                          case 36038:
                            Settings.WakeDevices.Value.Set(CecLogicalAddress.AudioSystem);
                            break;
                          case 36039:
                            Settings.WakeDevices.Value.Set(CecLogicalAddress.Tv);
                            Settings.WakeDevices.Value.Set(CecLogicalAddress.AudioSystem);
                            break;
                        }
                      }
                    }
                    break;
                  case "wake_devices_advanced":
                    {
                      Settings.WakeDevices.Value.Clear();
                      string[] split = value.Split(new[] { ' ' });
                      foreach (string dev in split)
                      {
                        byte iLogicalAddress;
                        if (byte.TryParse(dev, out iLogicalAddress))
                          Settings.WakeDevices.Value.Set((CecLogicalAddress)iLogicalAddress);
                      }
                    }
                    break;
                  case "standby_devices":
                    {
                      int iStandbyDevices;
                      if (int.TryParse(value, out iStandbyDevices))
                      {
                        Settings.PowerOffDevices.Value.Clear();
                        switch (iStandbyDevices)
                        {
                          case 36037:
                            Settings.PowerOffDevices.Value.Set(CecLogicalAddress.Tv);
                            break;
                          case 36038:
                            Settings.PowerOffDevices.Value.Set(CecLogicalAddress.AudioSystem);
                            break;
                          case 36039:
                            Settings.PowerOffDevices.Value.Set(CecLogicalAddress.Tv);
                            Settings.PowerOffDevices.Value.Set(CecLogicalAddress.AudioSystem);
                            break;
                        }
                      }
                    }
                    break;
                  case "standby_devices_advanced":
                    {
                      Settings.PowerOffDevices.Value.Clear();
                      string[] split = value.Split(new[] { ' ' });
                      foreach (string dev in split)
                      {
                        byte iLogicalAddress;
                        if (byte.TryParse(dev, out iLogicalAddress))
                          Settings.PowerOffDevices.Value.Set((CecLogicalAddress)iLogicalAddress);
                      }
                    }
                    break;
                  case "enabled":
                    break;
                  case "port":
                    //TODO
                    break;
                  // 1.5.1 settings
                  case "send_inactive_source":
                    SendInactiveSource.Value = value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes");
                    break;
                  // 1.9.0+ settings
                  case "pause_playback_on_deactivate":
                    PausePlaybackOnDeactivate.Value = value.Equals("1") || value.ToLower().Equals("true") || value.ToLower().Equals("yes");
                    break;
                }
              }
              break;
          }
        }
      }
      return gotConfig;
    }

    static bool HasAdvancedDeviceIdSet(CecLogicalAddresses addresses)
    {
      foreach (var val in addresses.Addresses)
        if (val != CecLogicalAddress.Tv && val != CecLogicalAddress.AudioSystem)
          return true;
      return false;
    }

    public void SaveXMLConfiguration()
    {
      Settings.Persist();

      var xbmcDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + @"\XBMC\userdata\peripheral_data";
      if (!Directory.Exists(xbmcDir))
        Directory.CreateDirectory(xbmcDir);

      if (!Directory.Exists(xbmcDir))
      {
        // couldn't create directory
        MessageBox.Show(string.Format(Resources.could_not_create_directory, xbmcDir), Resources.error,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
        return;
      }

      SaveFileDialog dialog = new SaveFileDialog
      {
        Title = Resources.store_settings_where,
        InitialDirectory = xbmcDir,
        FileName = string.Format("usb_{0:X}_{1:X}.xml", Program.Instance.Controller.AdapterVendorId, Program.Instance.Controller.AdapterProductId),
        Filter = Resources.xml_file_filter,
        FilterIndex = 1
      };
      if (dialog.ShowDialog() != DialogResult.OK) return;

      FileStream fs = null;
      string error = string.Empty;
      try
      {
        fs = (FileStream)dialog.OpenFile();
      }
      catch (Exception ex)
      {
        error = ex.Message;
      }
      if (fs == null)
      {
        MessageBox.Show(string.Format(Resources.cannot_open_file, dialog.FileName) + (error.Length > 0 ? ": " + error : string.Empty), Resources.app_name, MessageBoxButtons.OK, MessageBoxIcon.Error);
      }
      else
      {
        StreamWriter writer = new StreamWriter(fs);
        StringBuilder output = new StringBuilder();
        output.AppendLine("<settings>");
        output.AppendLine("<setting id=\"cec_hdmi_port\" value=\"" + Settings.HDMIPort.Value + "\" />");
        output.AppendLine("<setting id=\"connected_device\" value=\"" + (Settings.ConnectedDevice.Value == CecLogicalAddress.AudioSystem ? 36038 : 36037) + "\" />");
        output.AppendLine("<setting id=\"cec_power_on_startup\" value=\"" + (Settings.ActivateSource.Value ? 1 : 0) + "\" />");
        output.AppendLine("<setting id=\"cec_power_off_shutdown\" value=\"" + (Settings.PowerOffDevices.Value.IsSet(CecLogicalAddress.Broadcast) ? 1 : 0) + "\" />");
        output.AppendLine("<setting id=\"cec_standby_screensaver\" value=\"" + (StandbyScreensaver.Value ? 1 : 0) + "\" />");
        output.AppendLine("<setting id=\"standby_pc_on_tv_standby\" value=\"" + (PowerOffOnStandby.Value ? 1 : 0) + "\" />");
        output.AppendLine("<setting id=\"use_tv_menu_language\" value=\"" + (UseTVLanguage.Value ? 1 : 0) + "\" />");
        output.AppendLine("<setting id=\"enabled\" value=\"1\" />");
        output.AppendLine("<setting id=\"port\" value=\"\" />");

        // only supported by 1.5.0+ clients
        output.AppendLine("<!-- the following lines are only supported by v1.5.0+ clients -->");
        output.AppendLine("<setting id=\"activate_source\" value=\"" + (Settings.ActivateSource.Value ? 1 : 0) + "\" />");
        output.AppendLine("<setting id=\"physical_address\" value=\"" + string.Format("{0,4:X}", Settings.OverridePhysicalAddress.Value ? Settings.PhysicalAddress.Value : 0).Trim() + "\" />");
        output.AppendLine("<setting id=\"device_type\" value=\"" + (int)Settings.DeviceType.Value + "\" />");
        output.AppendLine("<setting id=\"tv_vendor\" value=\"" + string.Format("{0,6:X}", Settings.OverrideTVVendor.Value ? (int)Settings.TVVendor.Value : 0).Trim() + "\" />");

        if (HasAdvancedDeviceIdSet(Settings.WakeDevices.Value))
        {
          output.Append("<setting id=\"wake_devices_advanced\" value=\"");
          StringBuilder strWakeDevices = new StringBuilder();
          foreach (CecLogicalAddress addr in Settings.WakeDevices.Value.Addresses)
            if (addr != CecLogicalAddress.Unknown)
              strWakeDevices.Append(" " + (int)addr);
          output.Append(strWakeDevices.ToString().Trim());
          output.AppendLine("\" />");
        }

        if (Settings.WakeDevices.Value.IsSet(CecLogicalAddress.Tv) &&
            Settings.WakeDevices.Value.IsSet(CecLogicalAddress.AudioSystem))
          output.Append("<setting id=\"wake_devices\" value=\"36039\">");
        else if (Settings.WakeDevices.Value.IsSet(CecLogicalAddress.Tv))
          output.Append("<setting id=\"wake_devices\" value=\"36037\">");
        else if (Settings.WakeDevices.Value.IsSet(CecLogicalAddress.AudioSystem))
          output.Append("<setting id=\"wake_devices\" value=\"36038\">");
        else
          output.Append("<setting id=\"wake_devices\" value=\"231\">");

        if (HasAdvancedDeviceIdSet(Settings.PowerOffDevices.Value))
        {
          output.Append("<setting id=\"standby_devices_advanced\" value=\"");
          StringBuilder strSleepDevices = new StringBuilder();
          foreach (CecLogicalAddress addr in Settings.PowerOffDevices.Value.Addresses)
            if (addr != CecLogicalAddress.Unknown)
              strSleepDevices.Append(" " + (int) addr);
          output.Append(strSleepDevices.ToString().Trim());
          output.AppendLine("\" />");
        }

        if (Settings.PowerOffDevices.Value.IsSet(CecLogicalAddress.Tv) &&
            Settings.PowerOffDevices.Value.IsSet(CecLogicalAddress.AudioSystem))
          output.Append("<setting id=\"standby_devices\" value=\"36039\">");
        else if (Settings.PowerOffDevices.Value.IsSet(CecLogicalAddress.Tv))
          output.Append("<setting id=\"standby_devices\" value=\"36037\">");
        else if (Settings.PowerOffDevices.Value.IsSet(CecLogicalAddress.AudioSystem))
          output.Append("<setting id=\"standby_devices\" value=\"36038\">");
        else
          output.Append("<setting id=\"standby_devices\" value=\"231\">");

        // only supported by 1.5.1+ clients
        output.AppendLine("<!-- the following lines are only supported by v1.5.1+ clients -->");
        output.AppendLine("<setting id=\"send_inactive_source\" value=\"" + (SendInactiveSource.Value ? 1 : 0) + "\" />");

        // only supported by 1.9.0+ clients
        output.AppendLine("<setting id=\"pause_playback_on_deactivate\" value=\"" + (PausePlaybackOnDeactivate.Value ? 1 : 0) + "\" />");

        output.AppendLine("</settings>");
        writer.Write(output.ToString());
        writer.Close();
        fs.Close();
        fs.Dispose();
        MessageBox.Show(Resources.settings_stored, Resources.app_name, MessageBoxButtons.OK, MessageBoxIcon.Information);
      }
    }

    public CECSettingBool UseTVLanguage
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_use_tv_language"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_use_tv_language", Resources.app_use_tv_language, true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_use_tv_language"] = setting;
        }
        return Settings[ProcessName + "_use_tv_language"].AsSettingBool;
      }
    }

    public CECSettingBool StandbyScreensaver
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_standby_screensaver"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_standby_screensaver", Resources.app_standby_screensaver, true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_standby_screensaver"] = setting;
        }
        return Settings[ProcessName + "_standby_screensaver"].AsSettingBool;
      }
    }

    public CECSettingBool ActivateSource
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_activate_source"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_activate_source", Resources.global_activate_source, true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_activate_source"] = setting;
        }
        return Settings[ProcessName + "_activate_source"].AsSettingBool;
      }
    }

    public CECSettingBool PowerOffOnStandby
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_standby_on_tv_standby"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_standby_on_tv_standby", Resources.app_standby_on_tv_standby, true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_standby_on_tv_standby"] = setting;
        }
        return Settings[ProcessName + "_standby_on_tv_standby"].AsSettingBool;
      }
    }

    public CECSettingBool SendInactiveSource
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_send_inactive_source"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_send_inactive_source", Resources.app_send_inactive_source, true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_send_inactive_source"] = setting;
        }
        return Settings[ProcessName + "_send_inactive_source"].AsSettingBool;
      }
    }

    public CECSettingBool PausePlaybackOnDeactivate
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_pause_playback_on_deactivate"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_pause_playback_on_deactivate", Resources.app_pause_playback_on_deactivate, true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_pause_playback_on_deactivate"] = setting;
        }
        return Settings[ProcessName + "_pause_playback_on_deactivate"].AsSettingBool;
      }
    }
	}
}

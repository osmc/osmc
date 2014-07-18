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
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using CecSharp;
using LibCECTray.Properties;
using LibCECTray.settings;
using Timer = System.Timers.Timer;

namespace LibCECTray.controller.applications
{
  public delegate void OnApplicationRunningChanged(bool running);

  /// <summary>
  /// Controls an application on the PC: send key presses, open the application, close it, etc.
  /// </summary>
  class ApplicationController
  {
    public ApplicationController(CECController controller, string uiName, string processName, string filename, string workingDirectory)
    {
      Controller = controller;
      UiName = uiName;
      ProcessName = processName;
      ApplicationFilename = filename;
      ApplicationWorkingDirectory = workingDirectory;
      SuppressApplicationStart = false;
      IsInternal = false;
    }

    public static ApplicationController FromString(CECController controller, CECSettings settings, string serialisedConfig)
    {
      var splitString = serialisedConfig.Split(';');
      if (splitString.Length != 4)
        throw new InvalidDataException("incorrect number of parameters");

      return new ApplicationController(controller, splitString[0], splitString[1], splitString[2], splitString[3]);
    }

    public string AsString()
    {
      return string.Format("{0};{1};{2};{3}", UiName, ProcessName, ApplicationFilename, ApplicationWorkingDirectory);
    }

    public void BindButtonConfiguration(DataGridView gridView, BindingSource bindingSource)
    {
      CecButtonGridView = gridView;

      DataGridViewCell buttonCellTemplate = new DataGridViewTextBoxCell();
      CecButtonGridView.Columns.Add(new DataGridViewColumn(buttonCellTemplate)
                                      {
                                        DataPropertyName = "CecButtonName",
                                        Name = Resources.config_cec_button,
                                        ReadOnly = true,
                                        Width = 150
                                      });

      DataGridViewButtonCell mappedToCellTemplate = new DataGridViewButtonCell();
      CecButtonGridView.Columns.Add(new DataGridViewColumn(mappedToCellTemplate)
                                      {
                                        DataPropertyName = "MappedButtonName",
                                        Name = Resources.config_button_mapped_to,
                                        ReadOnly = true,
                                        Width = 350
                                      });

      bindingSource.DataSource = ButtonConfig;
      CecButtonGridView.DataSource = bindingSource;

      gridView.CellFormatting += delegate(object sender, DataGridViewCellFormattingEventArgs args)
                                   {
                                     DataGridView grid = sender as DataGridView;
                                     var data = grid != null ? grid.Rows[args.RowIndex].DataBoundItem as CecButtonConfigItem : null;
                                     if (data == null || !data.Enabled)
                                     {
                                       args.CellStyle.ForeColor = Color.Gray;
                                     }
                                   };

      gridView.CellClick += delegate(object sender, DataGridViewCellEventArgs args)
                              {
                                var item = args.RowIndex < ButtonConfig.Count ? ButtonConfig[args.RowIndex] : null;
                                if (item == null)
                                  return;
                                if (args.ColumnIndex >= 0)
                                {
                                  (new CecButtonConfigUI(item)).ShowDialog();
                                }
                                else
                                {
                                  var mappedButton = ButtonConfig[item.Key]; 
                                  if (mappedButton == null || mappedButton.Value.Empty())
                                    return;

                                    var controlWindow = FindInstance();
                                    if (controlWindow != IntPtr.Zero && item.Key.Duration == 0)
                                      mappedButton.Value.Transmit(controlWindow);
                                }
                              };

      foreach (var item in _buttonConfig)
      {
        item.SettingChanged += delegate
                                 {
                                   gridView.Refresh();
                                 };
      }
    }

    #region Start and stop the application
    /// <summary>
    /// Check if the application is running
    /// </summary>
    /// <returns>True when running, false otherwise</returns>
    public virtual bool IsRunning()
    {
      return FindInstance() != IntPtr.Zero;
    }

    /// <summary>
    /// Start the application if it's not running already, and suppress further starts for 5 seconds
    /// </summary>
    /// <returns>True when started or suppressed, false otherwise</returns>
    public virtual bool Start(bool bExitAfterStarting)
    {
      if (IsRunning())
      {
        SetForeground();
        return true;
      }

      if (SuppressApplicationStart)
        return false;

      SuppressApplicationStart = true;
      Timer timer = new Timer {Interval = 5000, AutoReset = false};
      timer.Elapsed += delegate { SuppressApplicationStart = false; };
      timer.Start();

      try
      {
        using (
          Process runningProcess = new Process
                                     {
                                       StartInfo =
                                         {
                                           WorkingDirectory = ApplicationWorkingDirectory,
                                           FileName = ApplicationFilename
                                         }
                                     })
        {
          // start maximised if the option is enabled
          if (StartFullScreen.Value)
            runningProcess.StartInfo.WindowStyle = ProcessWindowStyle.Maximized;

          runningProcess.Start();
        }
      }
      catch (Exception)
      {
        return false;
      }

      if (bExitAfterStarting)
        Application.Exit();

      return true;
    }

    /// <summary>
    /// Initialise the controller and autostart the application
    /// </summary>
    public virtual void Initialise()
    {
      Timer timer = new Timer { Interval = 1000, AutoReset = true };
      timer.Elapsed += delegate { CheckApplicationEnabled(); };
      timer.Start();

      if (AutoStartApplication.Value)
        Start(false);
    }

    public event OnApplicationRunningChanged ApplicationRunningChanged;

    private void CheckApplicationEnabled()
    {
      var isRunning = IsRunning();
      if (isRunning != _applicationRunning && ApplicationRunningChanged != null)
        ApplicationRunningChanged(isRunning);

      _applicationRunning = isRunning;
      UiControl.SetStartButtonEnabled(!isRunning && !SuppressApplicationStart);
    }
    #endregion

    #region Send input to the application
    /// <summary>
    /// Send a keypress to the application if it's running
    /// </summary>
    /// <param name="key">The keypress to send</param>
    /// <param name="isSelectedTab">True when this tab is currently selected in the UI</param>
    /// <returns>True when sent, false otherwise</returns>
    public virtual bool SendKey(CecKeypress key, bool isSelectedTab)
    {
      if (isSelectedTab)
        UiControl.SelectKeypressRow(UiControl, CecButtonGridView, key);

      if (isSelectedTab && SuppressKeypressWhenSelected.Value)
        return false;

      if (!ControlApplication.Value)
        return false;

      var mappedButton = ButtonConfig[key];
      if (mappedButton == null || mappedButton.Value.Empty())
        return false;

      var controlWindow = FindInstance();
      if (controlWindow != IntPtr.Zero && (key.Duration == 0 || key.Duration > 500))
        return mappedButton.Value.Transmit(controlWindow);

      return false;
    }
    #endregion

    #region Process control
    /// <summary>
    /// Make this application the foreground application if it's running
    /// </summary>
    public virtual void SetForeground()
    {
      var wmcInstance = FindInstance();
      if (wmcInstance != IntPtr.Zero)
        WindowsAPI.SetForegroundWindow(wmcInstance);
    }

    /// <summary>
    /// The main window handle of the application if it's running.
    /// </summary>
    /// <returns>The main window handle, or IntPtr.Zero if it's not found</returns>
    protected virtual IntPtr FindInstance()
    {
      var processes = Process.GetProcessesByName(ProcessName);
      return processes.Length > 0 ? processes[0].MainWindowHandle : IntPtr.Zero;
    }
    #endregion

    #region Members
    /// <summary>
    /// The name of the process in the process manager
    /// </summary>
    public string ProcessName { set; get; }

    /// <summary>
    /// The filename of the application
    /// </summary>
    public string ApplicationFilename { set; get; }

    /// <summary>
    /// The working directory of the application
    /// </summary>
    public string ApplicationWorkingDirectory { set; get; }

    /// <summary>
    /// Don't start the application while true
    /// </summary>
    public bool SuppressApplicationStart { get; private set; }

    /// <summary>
    /// The name of the application how it shows up in this application
    /// </summary>
    public string UiName { set; get; }

    /// <summary>
    /// True when this application should be autostarted when this application is activated, or made the active source
    /// </summary>
    public CECSettingBool AutoStartApplication
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_autostart"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_autostart", "Autostart application", false, null);
          Settings.Load(setting);
          Settings[ProcessName + "_autostart"] = setting;
        }
        return Settings[ProcessName + "_autostart"].AsSettingBool;
      }
    }

    /// <summary>
    /// True when keypresses should be routed to this application
    /// </summary>
    public CECSettingBool ControlApplication
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_control"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_control", "Control application", true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_control"] = setting;
        }
        return Settings[ProcessName + "_control"].AsSettingBool;
      }
    }

    /// <summary>
    /// True when this application should be autostarted when this application is activated, or made the active source
    /// </summary>
    public CECSettingBool SuppressKeypressWhenSelected
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_suppress_when_selected"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_suppress_when_selected", "Suppress keypress when this tab is selected", true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_suppress_when_selected"] = setting;
        }
        return Settings[ProcessName + "_suppress_when_selected"].AsSettingBool;
      }
    }

    /// <summary>
    /// True when the application should be started in full screen mode
    /// </summary>
    public CECSettingBool StartFullScreen
    {
      get
      {
        if (!Settings.ContainsKey(ProcessName + "_start_fullscreen"))
        {
          CECSettingBool setting = new CECSettingBool(ProcessName + "_start_fullscreen", "Start in full screen mode", true, null);
          Settings.Load(setting);
          Settings[ProcessName + "_start_fullscreen"] = setting;
        }
        return Settings[ProcessName + "_start_fullscreen"].AsSettingBool;
      }
    }

    protected ControllerTabPage UIControlInternal;
    public virtual ControllerTabPage UiControl
    {
      get { return UIControlInternal ?? (UIControlInternal = new ApplicationControllerUI(this)); }
    }

    private CecButtonConfig _buttonConfig;
    public CecButtonConfig ButtonConfig
    {
      get { return _buttonConfig ?? (_buttonConfig = new CecButtonConfig(this)); }
    }

    public CECSettings Settings
    {
      get { return Controller.Settings; }
    }
    protected DataGridView CecButtonGridView;

    public virtual ApplicationAction DefaultValue(CecKeypress key)
    {
      return null;
    }

    public virtual bool HasDefaultValue(CecKeypress key)
    {
      return DefaultValue(key) != null;
    }

    public bool IsInternal { protected set; get; }
    public bool CanConfigureProcess
    {
      get
      {
        return !IsInternal;
      }
    }

    private bool _applicationRunning;

    protected readonly CECController Controller;

    #endregion
  }
}

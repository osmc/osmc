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

using System.Threading;
using CecSharp;
using LibCECTray.controller.actions;
using LibCECTray.ui;
using System.Windows.Forms;

namespace LibCECTray.controller
{
  /// <summary>
  /// Actions that can be executed by a background thread
  /// </summary>
  internal class Actions
  {
    public Actions(CECController controller)
    {
      _controller = controller;
    }

    /// <summary>
    /// Event handler for processing updates for a background thread
    /// </summary>
    /// <param name="src">The source that sent the event</param>
    /// <param name="updateEvent">The type of event</param>
    private void ProcessEventHandler(object src, UpdateEvent updateEvent)
    {
      switch (updateEvent.Type)
      {
        case UpdateEventType.StatusText:
          _controller.SetStatusText(updateEvent.StringValue);
          break;
        case UpdateEventType.ProgressBar:
          _controller.SetProgressBar(updateEvent.IntValue, true);
          break;
        case UpdateEventType.PhysicalAddress:
          _controller.Settings.PhysicalAddress.Value = (ushort)updateEvent.IntValue;
          break;
        case UpdateEventType.TVVendorId:
          _controller.Settings.SetVendorName(CecLogicalAddress.Tv, (CecVendorId)updateEvent.IntValue, _controller.Lib.ToString((CecVendorId)updateEvent.IntValue));
          break;
        case UpdateEventType.BaseDevice:
          _controller.Settings.ConnectedDevice.Value = (CecLogicalAddress)updateEvent.IntValue;
          break;
        case UpdateEventType.HDMIPort:
          _controller.Settings.HDMIPort.Value = (byte)updateEvent.IntValue;
          break;
        case UpdateEventType.HasAVRDevice:
          CecLogicalAddresses allowedMask = new CecLogicalAddresses();
          allowedMask.Set(CecLogicalAddress.Tv);
          if (updateEvent.BoolValue)
            allowedMask.Set(CecLogicalAddress.AudioSystem);
          _controller.Settings.ConnectedDevice.AllowedAddressMask = allowedMask;
          break;
        case UpdateEventType.AVRVendorId:
          _controller.Settings.SetVendorName(CecLogicalAddress.AudioSystem, (CecVendorId)updateEvent.IntValue, _controller.Lib.ToString((CecVendorId)updateEvent.IntValue));
          break;
        case UpdateEventType.Configuration:
          SuppressUpdates = true;
          _controller.ConfigurationChanged(updateEvent.ConfigValue);
          SuppressUpdates = false;
          break;
        case UpdateEventType.PollDevices:
          _controller.CheckActiveDevices();
          break;
        case UpdateEventType.ProcessCompleted:
          if (!(_activeProcess is GetCurrentPhysicalAddress) && !SuppressUpdates)
          {
            _activeProcess = new GetCurrentPhysicalAddress(_controller.Lib);
            _activeProcess.EventHandler += ProcessEventHandler;
            (new Thread(_activeProcess.Run)).Start();
          }
          else
          {
            _activeProcess = null;
          }

          if (_updatingInfoPanel != null)
          {
            _updatingInfoPanel.SetControlEnabled(_updatingInfoPanel.bUpdate, true);
            _updatingInfoPanel = null;
          }

          _controller.SetControlsEnabled(true);
          _controller.SetProgressBar(100, false);

          if (_controller.Settings.StartHidden.Value)
          {
            _controller.SetShowInTaskbar(false);
            //SetToolStripMenuText(tsMenuShowHide, Resources.show);
            _controller.Hide(true);
          }

          break;
        case UpdateEventType.ExitApplication:
          SuppressUpdates = true;
          _activeProcess = null;
          Application.Exit();
          break;
      }
    }

    #region Actions
    /// <summary>
    /// Updates the contents of a device information window
    /// </summary>
    /// <param name="panel">The panel to update</param>
    public void UpdateInfoPanel(DeviceInformation panel)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _updatingInfoPanel = panel;
      panel.SetControlEnabled(panel.bUpdate, false);
      _activeProcess = new UpdateDeviceInfo(_controller, _controller.Lib, panel);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Change the device to which the adapter is connected and/or the HDMI port number
    /// </summary>
    /// <param name="address">The new device to which the adapter is connected</param>
    /// <param name="portnumber">The new HDMI port number</param>
    public void SetConnectedDevice(CecLogicalAddress address, int portnumber)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new UpdateConnectedDevice(_controller.Lib, address, portnumber);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Changes the physical address setting of libCEC
    /// </summary>
    /// <param name="physicalAddress">The new physical address</param>
    public void SetPhysicalAddress(ushort physicalAddress)
    {
      if (SuppressUpdates || _activeProcess != null || !_controller.Settings.OverridePhysicalAddress.Value) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new UpdatePhysicalAddress(_controller.Lib, physicalAddress);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Send an updated configuration to libCEC
    /// </summary>
    /// <param name="config">The new configuration</param>
    public void UpdateConfigurationAsync(LibCECConfiguration config)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new UpdateConfiguration(_controller.Lib, config);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Send an image view on command to the device at the given logical address
    /// </summary>
    /// <param name="address">The address to send the image view on command to</param>
    public void SendImageViewOn(CecLogicalAddress address)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new SendImageViewOn(_controller.Lib, address);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Activate the source at the given logical address. 
    /// </summary>
    /// <param name="address">The logical address of the device to activate</param>
    public void SetStreamPath(CecLogicalAddress address)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new SendActivateSource(_controller.Lib, address);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    public void ActivateSource()
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new ActivateSource(_controller.Lib);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Send a standby command to the device at the given logical address
    /// </summary>
    /// <param name="address">The logical address of the device to send to standby</param>
    public void SendStandby(CecLogicalAddress address)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new SendStandby(_controller.Lib, address);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Fetch device information and show an information dialog for the device at the given logical address
    /// </summary>
    /// <param name="address">The logical address of the device to get the info for</param>
    public void ShowDeviceInfo(CecLogicalAddress address)
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new ShowDeviceInfo(_controller, _controller.Lib, address);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Poll devices to check which ones are active
    /// </summary>
    public void RescanDevices()
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new RescanDevices(_controller.Lib);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Update the physical address of libCEC that is displayed in the UI
    /// </summary>
    public void UpdatePhysicalAddress()
    {
      if (SuppressUpdates || _activeProcess != null) return;

      _controller.SetControlsEnabled(false);
      _activeProcess = new GetCurrentPhysicalAddress(_controller.Lib);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }

    /// <summary>
    /// Connect to the adapter with the given configuration
    /// </summary>
    /// <param name="config">The client configuration</param>
    public void ConnectToDevice(LibCECConfiguration config)
    {
      if (_activeProcess != null) return;

      _activeProcess = new ConnectToDevice(_controller.Lib, config);
      _activeProcess.EventHandler += ProcessEventHandler;
      (new Thread(_activeProcess.Run)).Start();
    }
    #endregion

    #region Members
    private readonly CECController _controller;
    private DeviceInformation _updatingInfoPanel;
    public bool SuppressUpdates = true;
    private UpdateProcess _activeProcess;
    #endregion
  }
}

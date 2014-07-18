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

using CecSharp;
using System.Windows.Forms;
using LibCECTray.Properties;

namespace LibCECTray.controller.actions
{
  class ConnectToDevice : UpdateProcess
  {
    public ConnectToDevice(LibCecSharp lib, LibCECConfiguration config)
    {
      _lib = lib;
      _config = config;
    }

    public override void Process()
    {
      SendEvent(UpdateEventType.StatusText, Resources.action_opening_connection);
      SendEvent(UpdateEventType.ProgressBar, 10);

      //TODO read the com port setting from the configuration
      var adapters = _lib.FindAdapters(string.Empty);
      if (adapters.Length == 0)
      {
        var result = MessageBox.Show(Resources.could_not_connect_try_again, Resources.app_name, MessageBoxButtons.YesNo);
        if (result == DialogResult.No)
        {
          SendEvent(UpdateEventType.ExitApplication);
          return;
        }
        adapters = _lib.FindAdapters(string.Empty);
      }

      while (!_lib.Open(adapters[0].ComPort, 10000))
      {
        var result = MessageBox.Show(Resources.could_not_connect_try_again, Resources.app_name, MessageBoxButtons.YesNo);
        if (result != DialogResult.No) continue;
        SendEvent(UpdateEventType.ExitApplication);
        return;
      }

      SendEvent(UpdateEventType.ProgressBar, 20);
      SendEvent(UpdateEventType.StatusText, Resources.action_sending_power_on);
      _lib.PowerOnDevices(CecLogicalAddress.Broadcast);

      if (_lib.IsActiveDevice(CecLogicalAddress.Tv))
      {
        SendEvent(UpdateEventType.StatusText, Resources.action_detecting_tv_vendor);
        SendEvent(UpdateEventType.ProgressBar, 30);
        SendEvent(UpdateEventType.TVVendorId, (int)_lib.GetDeviceVendorId(CecLogicalAddress.Tv));
      }

      SendEvent(UpdateEventType.ProgressBar, 50);
      SendEvent(UpdateEventType.StatusText, Resources.action_detecting_avr);

      bool hasAVRDevice = _lib.IsActiveDevice(CecLogicalAddress.AudioSystem);
      SendEvent(UpdateEventType.HasAVRDevice, hasAVRDevice);

      if (hasAVRDevice)
      {
        SendEvent(UpdateEventType.ProgressBar, 60);
        SendEvent(UpdateEventType.StatusText, Resources.action_detecting_avr_vendor);
        SendEvent(UpdateEventType.AVRVendorId, (int)_lib.GetDeviceVendorId(CecLogicalAddress.AudioSystem));
      }

      if (_lib.IsActiveDevice(CecLogicalAddress.Tv)&& !_lib.GetDevicePowerStatus(CecLogicalAddress.Tv).Equals(CecPowerStatus.On))
      {
        SendEvent(UpdateEventType.ProgressBar, 70);
        SendEvent(UpdateEventType.StatusText, Resources.action_activating_source);
        _lib.SetActiveSource(CecDeviceType.Reserved);
      }

      SendEvent(UpdateEventType.ProgressBar, 80);
      SendEvent(UpdateEventType.StatusText, Resources.action_reading_device_configuration);

      _lib.GetCurrentConfiguration(_config);
      SendEvent(_config);

      SendEvent(UpdateEventType.ProgressBar, 90);
      SendEvent(UpdateEventType.StatusText, Resources.action_polling_active_devices);
      SendEvent(UpdateEventType.PollDevices);

      if (!_lib.IsActiveDevice(CecLogicalAddress.Tv))
      {
        MessageBox.Show(Resources.alert_tv_poll_failed, Resources.cec_alert, MessageBoxButtons.OK, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button1, MessageBoxOptions.DefaultDesktopOnly);
      }

      SendEvent(UpdateEventType.ProgressBar, 100);
      SendEvent(UpdateEventType.StatusText, Resources.ready);
    }

    private readonly LibCecSharp _lib;
    private readonly LibCECConfiguration _config;
  }
}

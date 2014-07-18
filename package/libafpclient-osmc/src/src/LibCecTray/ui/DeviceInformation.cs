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
using System.Windows.Forms;
using CecSharp;
using LibCECTray.Properties;
using LibCECTray.controller;

namespace LibCECTray.ui
{
  partial class DeviceInformation : AsyncForm
  {
    public DeviceInformation(CECController controller, CecLogicalAddress address, ref LibCecSharp lib,
      bool devicePresent, CecVendorId vendor, bool isActiveSource, ushort physicalAddress,
      CecVersion version, CecPowerStatus power, string osdName, string menuLanguage)
    {
      _controller = controller;
      _lib = lib;
      Address = address;
      InitializeComponent();
      lDevice.Text = lib.ToString(address);
      lLogicalAddress.Text = String.Format("{0,1:X}", (int)address);
      Update(devicePresent, vendor, isActiveSource, physicalAddress, version, power, osdName, menuLanguage);
    }

    public void Update(bool devicePresent, CecVendorId vendor, bool isActiveSource, ushort physicalAddress,
      CecVersion version, CecPowerStatus power, string osdName, string menuLanguage)
    {
      SetControlText(lPhysicalAddress, String.Format("{0,4:X}", physicalAddress));
      SetControlText(lDevicePresent, devicePresent ? Resources.yes : Resources.no);
      SetControlVisible(lActiveSource, isActiveSource);
      SetControlVisible(lInactiveSource, !isActiveSource);
      SetControlText(lVendor, vendor != CecVendorId.Unknown ? _lib.ToString(vendor) : Resources.unknown);
      SetControlText(lCecVersion, _lib.ToString(version));
      SetControlText(lPowerStatus, _lib.ToString(power));
      SetControlText(lOsdName, osdName);
      SetControlText(lMenuLanguage, menuLanguage);
      SetControlText(this, "Device: " + osdName);
    }

    private void LInactiveSourceLinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
      SetControlVisible(lInactiveSource, false);
      SetControlVisible(lActiveSource, true);
      _controller.CECActions.SetStreamPath(Address);
    }

    private void LStandbyLinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
    {
      LinkLabel label = sender as LinkLabel;
      bool sendPowerOn = label != null && label.Text != _lib.ToString(CecPowerStatus.InTransitionStandbyToOn) &&
        label.Text != _lib.ToString(CecPowerStatus.On);

      SetControlText(lPowerStatus, _lib.ToString(sendPowerOn ? CecPowerStatus.On : CecPowerStatus.Standby));
      if (sendPowerOn)
        _controller.CECActions.SendImageViewOn(Address);
      else
        _controller.CECActions.SendStandby(Address);
    }


    private void Button1Click(object sender, EventArgs e)
    {
      _controller.CECActions.UpdateInfoPanel(this);
    }

    public CecLogicalAddress Address
    {
      private set;
      get;
    }
    private readonly CECController _controller;
    private readonly LibCecSharp _lib;
  }
}

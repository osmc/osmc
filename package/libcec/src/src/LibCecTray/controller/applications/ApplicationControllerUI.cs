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

namespace LibCECTray.controller.applications
{
  partial class ApplicationControllerUI : ControllerTabPage
  {
    public ApplicationControllerUI()
    {
      InitializeComponent();
    }

    public ApplicationControllerUI(ApplicationController controller)
    {
      _controller = controller;
      InitializeComponent();
      Name = controller.UiName;
      Text = controller.UiName;

      _controller.BindButtonConfiguration(dgButtonConfig, buttonBindingSource);
      _controller.AutoStartApplication.ReplaceControls(this, Controls, cbAutoStartApplication);
      _controller.ControlApplication.ReplaceControls(this, Controls, cbControlApplication);
      _controller.SuppressKeypressWhenSelected.ReplaceControls(this, Controls, cbSuppressKeypress);
      _controller.StartFullScreen.ReplaceControls(this, Controls, cbStartFullScreen);

      bConfigure.Enabled = _controller.CanConfigureProcess;
    }

    public override sealed string Text
    {
      get { return base.Text; }
      set { base.Text = value; }
    }

    private void BStartApplicationClick(object sender, EventArgs e)
    {
      bStartApplication.Enabled = false;
      _controller.Start(false);
    }

    public override void SetEnabled(bool val)
    {
      SetControlEnabled(cbAutoStartApplication, val);
      SetControlEnabled(cbControlApplication, val);
      SetControlEnabled(bStartApplication, !_controller.IsRunning() && !_controller.SuppressApplicationStart && val);
    }

    public override void SetStartButtonEnabled(bool val)
    {
      SetControlEnabled(bStartApplication, val);
    }

    private readonly ApplicationController _controller;

    private void BConfigureClick(object sender, EventArgs e)
    {
      bConfigure.Enabled = false;
      ConfigureApplication appConfig = new ConfigureApplication(_controller);
      appConfig.Disposed += delegate { bConfigure.Enabled = true; };
      DisplayDialog(appConfig, false);
    }
  }
}

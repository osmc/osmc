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
using LibCECTray.Properties;

namespace LibCECTray.controller.applications.@internal
{
  internal partial class XBMCControllerUI : ControllerTabPage
	{
		public XBMCControllerUI()
		{
			InitializeComponent();
		}

    public XBMCControllerUI(XBMCController controller)
    {
      _controller = controller;
      InitializeComponent();
      Name = controller.UiName;
      Text = controller.UiName;

      _controller.StartFullScreen.ReplaceControls(this, Controls, cbStartFullScreen);
      _controller.UseTVLanguage.ReplaceControls(this, Controls, cbUseTvLanguage);
      _controller.StandbyScreensaver.ReplaceControls(this, Controls, cbStandbyScreensaver);
      _controller.PowerOffOnStandby.ReplaceControls(this, Controls, cbStandbyTvStandby);
      _controller.SendInactiveSource.ReplaceControls(this, Controls, cbInactiveSource);
      _controller.PausePlaybackOnDeactivate.ReplaceControls(this, Controls, cbPauseOnDeactivate);

      SetEnabled(false);
    }

    public override sealed string Text
    {
      get { return base.Text; }
      set { base.Text = value; }
    }

    public override sealed void SetEnabled(bool val)
    {
      SetControlEnabled(bStartApplication, !_controller.IsRunning() && !_controller.SuppressApplicationStart && val);
      SetControlEnabled(_controller.StartFullScreen.ValueControl, val);
      SetControlEnabled(_controller.StandbyScreensaver.ValueControl, val);
      SetControlEnabled(_controller.UseTVLanguage.ValueControl, val);
      SetControlEnabled(_controller.ActivateSource.ValueControl, val);
      SetControlEnabled(_controller.PowerOffOnStandby.ValueControl, val);
      SetControlEnabled(_controller.SendInactiveSource.ValueControl, val);
      SetControlEnabled(bSaveConfig, val);
      SetControlEnabled(bLoadConfig, val);
      SetControlEnabled(bConfigure, _controller.CanConfigureProcess && val);
      SetControlEnabled(_controller.PausePlaybackOnDeactivate.ValueControl, val);
    }

    public override void SetStartButtonEnabled(bool val)
    {
      SetControlEnabled(bStartApplication, !_controller.IsRunning() && !_controller.SuppressApplicationStart && val);
    }

    private void BConfigureClick(object sender, EventArgs e)
    {
      bConfigure.Enabled = false;
      ConfigureApplication appConfig = new ConfigureApplication(_controller);
      appConfig.Disposed += delegate { bConfigure.Enabled = true; };
      DisplayDialog(appConfig, false);
    }

    private readonly XBMCController _controller;

    private void BStartApplicationClick(object sender, EventArgs e)
    {
      if (MessageBox.Show(Resources.start_xbmc_exit_tray, Resources.title_are_you_sure, MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2) == DialogResult.Yes)
      {
        bStartApplication.Enabled = false;
        _controller.Start(true);
      }
    }

    private void BLoadConfigClick(object sender, EventArgs e)
    {
      _controller.LoadXMLConfiguration();
    }

    private void BSaveConfigClick(object sender, EventArgs e)
    {
      _controller.SaveXMLConfiguration();
    }
	}
}

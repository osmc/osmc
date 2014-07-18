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
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using LibCECTray.Properties;
using LibCECTray.settings;
using LibCECTray.ui;

namespace LibCECTray.controller.applications
{
  partial class ConfigureApplication : Form
  {
    public ConfigureApplication(ApplicationController controller) :
      this(null, null)
    {
      _controller = controller;
      tbFilename.Text = controller.ApplicationFilename;
      tbWorkingDir.Text = controller.ApplicationWorkingDirectory;
      tbUiName.Text = controller.UiName;
      tbProcessName.Text = controller.ProcessName;

      Text = string.Format(Resources.configure_application, controller.UiName);

      if (!_controller.CanConfigureProcess)
      {
        tbFilename.Enabled = false;
        tbWorkingDir.Enabled = false;
        tbUiName.Enabled = false;
        tbProcessName.Enabled = false;
        bFindFile.Enabled = false;
      }
    }

    public override sealed string Text
    {
      get { return base.Text; }
      set { base.Text = value; }
    }

    public ConfigureApplication(CECSettings settings, CECController controller)
    {
      _cecController = controller;
      _settings = settings;
      InitializeComponent();

      // take the icon of the main window
      ComponentResourceManager resources = new ComponentResourceManager(typeof(CECTray));
      Icon = resources.GetObject("$this.Icon") as Icon;

      Text = Resources.add_new_application;
    }

    private void BFindFileClick(object sender, EventArgs e)
    {
      OpenFileDialog dialog = new OpenFileDialog
                                {
                                  Title = Resources.select_exe_file,
                                  InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
                                  Filter = Resources.exe_file_filter,
                                  FilterIndex = 1
                                };

      if (dialog.ShowDialog() != DialogResult.OK) return;

      var fileName = dialog.SafeFileName;
      if (fileName != null && File.Exists(dialog.FileName))
      {
        var versionInfo = FileVersionInfo.GetVersionInfo(dialog.FileName);
        var path = dialog.FileName.Substring(0, dialog.FileName.Length - fileName.Length);
        if (path.EndsWith("\\"))
          path = path.Substring(0, path.Length - 1);

        tbFilename.Text = fileName;
        tbWorkingDir.Text = path;
        tbUiName.Text = versionInfo.FileDescription;
        tbProcessName.Text = fileName;
        Text = string.Format(Resources.configure_application, versionInfo.FileDescription);
      }
      else
      {
        tbFilename.Text = string.Empty;
        tbWorkingDir.Text = string.Empty;
        tbUiName.Text = string.Empty;
        tbProcessName.Text = string.Empty;
        Text = Resources.add_new_application;
      }
    }

    private void BOkClick(object sender, EventArgs e)
    {
      if (_controller != null)
      {
        _controller.ApplicationFilename = tbFilename.Text;
        _controller.ApplicationWorkingDirectory = tbWorkingDir.Text;
        _controller.UiName = tbUiName.Text;
        _controller.ProcessName = tbProcessName.Text;
        _controller.Settings.Persist();
      }
      else if (_cecController != null)
      {
        ApplicationController newController = new ApplicationController(_cecController, tbUiName.Text, tbProcessName.Text, tbFilename.Text, tbWorkingDir.Text);
        if (_cecController.RegisterApplication(newController))
          newController.Settings.Persist();
      }
      Dispose();
    }

    private void BCancelClick(object sender, EventArgs e)
    {
      //TODO
      Dispose();
    }

    private readonly ApplicationController _controller;
    private readonly CECController _cecController;
    private readonly CECSettings _settings;
  }
}

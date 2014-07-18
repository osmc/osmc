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
using LibCECTray.Properties;
using System.ComponentModel;
using System.Drawing;

namespace LibCECTray.ui
{
  /// <summary>
  /// Popup that is displayed when the "about" button is clicked
  /// </summary>
  partial class About : Form
  {
    public About(string serverVersion, string clientVersion, string libInfo) :
      this()
    {
      lVersionInfo.Text = string.Format(Resources.about_libcec_version, serverVersion, clientVersion);
      lBuildInfo.Text = libInfo;
    }

    public About()
    {
      InitializeComponent();

      // take the icon of the main window
      ComponentResourceManager resources = new ComponentResourceManager(typeof(CECTray));
      Icon = resources.GetObject("$this.Icon") as Icon;
    }
  }
}

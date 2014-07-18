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
using LibCECTray.controller.applications;

namespace LibCECTray.ui
{
  /// <summary>
  /// Form that implements IAsyncControls
  /// </summary>
  class AsyncForm : Form, IAsyncControls
  {
    /// <summary>
    /// Changes the ShowInTaskbar value
    /// </summary>
    /// <param name="val">True to show, false to hide</param>
    public void SetShowInTaskbar(bool val)
    {
      if (InvokeRequired)
      {
        SetShowInTaskbarCallback d = SetShowInTaskbar;
        try
        {
          Invoke(d, new object[] { val });
        }
        catch (Exception) { }
      }
      else
      {
        ShowInTaskbar = val;
      }
    }
    private delegate void SetShowInTaskbarCallback(bool val);

    /// <summary>
    /// Enable or disable a control
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">True to enable, false to disable</param>
    public void SetControlEnabled(Control control, bool val)
    {
      AsyncControls.SetControlEnabled(this, control, val);
    }

    /// <summary>
    /// Change the text label of a control
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">The new text</param>
    public void SetControlText(Control control, string val)
    {
      AsyncControls.SetControlText(this, control, val);
    }

    /// <summary>
    /// Changes the toolstrip menu text
    /// </summary>
    /// <param name="item">The toolstrip menu item to change</param>
    /// <param name="val">The new value</param>
    public void SetToolStripMenuText(ToolStripMenuItem item, string val)
    {
      AsyncControls.SetToolStripMenuText(this, item, val);
    }

    /// <summary>
    /// Change the checked status of a checkbox
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">True to change to checked, false to change to unchecked</param>
    public void SetCheckboxChecked(CheckBox control, bool val)
    {
      AsyncControls.SetCheckboxChecked(this, control, val);
    }

    /// <summary>
    /// Change the checked status of an item in a CheckedListBox
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="index">The index of the checkbox in the list to change</param>
    /// <param name="val">True to change to checked, false to change to unchecked</param>
    public void SetCheckboxItemChecked(CheckedListBox control, int index, bool val)
    {
      AsyncControls.SetCheckboxItemChecked(this, control, index, val);
    }

    /// <summary>
    /// Changes the progress value of a progress bar
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">The new percentage</param>
    public void SetProgressValue(ProgressBar control, int val)
    {
      AsyncControls.SetProgressValue(this, control, val);
    }

    /// <summary>
    /// Replaces the items of a combobox
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="selectedIndex">The new selection index</param>
    /// <param name="val">The new content</param>
    public void SetComboBoxItems(ComboBox control, int selectedIndex, object[] val)
    {
      AsyncControls.SetComboBoxItems(this, control, selectedIndex, val);
    }

    /// <summary>
    /// Make a control visible or invisible
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">True to make it visible, false to make it invisible</param>
    public void SetControlVisible(Control control, bool val)
    {
      AsyncControls.SetControlVisible(this, control, val);
    }

    /// <summary>
    /// Display a new dialog
    /// </summary>
    /// <param name="control">The control to display</param>
    /// <param name="modal">True to make it a modal dialog</param>
    public void DisplayDialog(Form control, bool modal)
    {
      AsyncControls.DisplayDialog(this, control, modal);
    }

    /// <summary>
    /// Hides a control
    /// </summary>
    /// <param name="val">True to hide, false to show</param>
    public void SafeHide(bool val)
    {
      AsyncControls.SafeHide(this, val);
    }

    /// <summary>
    /// Change the selected index
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="index">The new selected index</param>
    public void SetSelectedIndex(ComboBox control, int index)
    {
      AsyncControls.SetSelectedIndex(this, control, index);
    }

    /// <summary>
    /// Get the name of the selected tab in a TabControl
    /// </summary>
    /// <param name="container">The tab container</param>
    /// <param name="tabPages">The tab pages</param>
    /// <returns>The name of the selected tab</returns>
    public string GetSelectedTabName(TabControl container, TabControl.TabPageCollection tabPages)
    {
      return AsyncControls.GetSelectedTabName(container, tabPages);
    }

    /// <summary>
    /// Selects the row with the given CecKeypress for a datagrid
    /// </summary>
    /// <param name="container">The datagrid container</param>
    /// <param name="dgView">The datagrid</param>
    /// <param name="key">The key to selected</param>
    public void SelectKeypressRow(Control container, DataGridView dgView, CecKeypress key)
    {
      AsyncControls.SelectKeypressRow(container, dgView, key);
    }
  }

  /// <summary>
  /// TabPage that implements IAsyncControls
  /// </summary>
  class AsyncTabPage : TabPage, IAsyncControls
  {
    /// <summary>
    /// Enable or disable a control
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">True to enable, false to disable</param>
    public void SetControlEnabled(Control control, bool val)
    {
      AsyncControls.SetControlEnabled(this, control, val);
    }

    /// <summary>
    /// Change the text label of a control
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">The new text</param>
    public void SetControlText(Control control, string val)
    {
      AsyncControls.SetControlText(this, control, val);
    }

    /// <summary>
    /// Changes the toolstrip menu text
    /// </summary>
    /// <param name="item">The toolstrip menu item to change</param>
    /// <param name="val">The new value</param>
    public void SetToolStripMenuText(ToolStripMenuItem item, string val)
    {
      AsyncControls.SetToolStripMenuText(this, item, val);
    }

    /// <summary>
    /// Change the checked status of a checkbox
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">True to change to checked, false to change to unchecked</param>
    public void SetCheckboxChecked(CheckBox control, bool val)
    {
      AsyncControls.SetCheckboxChecked(this, control, val);
    }

    /// <summary>
    /// Change the checked status of an item in a CheckedListBox
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="index">The index of the checkbox in the list to change</param>
    /// <param name="val">True to change to checked, false to change to unchecked</param>
    public void SetCheckboxItemChecked(CheckedListBox control, int index, bool val)
    {
      AsyncControls.SetCheckboxItemChecked(this, control, index, val);
    }

    /// <summary>
    /// Changes the progress value of a progress bar
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">The new percentage</param>
    public void SetProgressValue(ProgressBar control, int val)
    {
      AsyncControls.SetProgressValue(this, control, val);
    }

    /// <summary>
    /// Replaces the items of a combobox
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="selectedIndex">The new selection index</param>
    /// <param name="val">The new content</param>
    public void SetComboBoxItems(ComboBox control, int selectedIndex, object[] val)
    {
      AsyncControls.SetComboBoxItems(this, control, selectedIndex, val);
    }

    /// <summary>
    /// Make a control visible or invisible
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="val">True to make it visible, false to make it invisible</param>
    public void SetControlVisible(Control control, bool val)
    {
      AsyncControls.SetControlVisible(this, control, val);
    }

    /// <summary>
    /// Display a new dialog
    /// </summary>
    /// <param name="control">The control to display</param>
    /// <param name="modal">True to make it a modal dialog</param>
    public void DisplayDialog(Form control, bool modal)
    {
      AsyncControls.DisplayDialog(this, control, modal);
    }

    /// <summary>
    /// Hides a control
    /// </summary>
    /// <param name="val">True to hide, false to show</param>
    public void SafeHide(bool val)
    {
      AsyncControls.SafeHide(this, val);
    }

    /// <summary>
    /// Change the selected index
    /// </summary>
    /// <param name="control">The control to change</param>
    /// <param name="index">The new selected index</param>
    public void SetSelectedIndex(ComboBox control, int index)
    {
      AsyncControls.SetSelectedIndex(this, control, index);
    }

    /// <summary>
    /// Get the name of the selected tab in a TabControl
    /// </summary>
    /// <param name="container">The tab container</param>
    /// <param name="tabPages">The tab pages</param>
    /// <returns>The name of the selected tab</returns>
    public string GetSelectedTabName(TabControl container, TabControl.TabPageCollection tabPages)
    {
      return AsyncControls.GetSelectedTabName(container, tabPages);
    }

    /// <summary>
    /// Selects the row with the given CecKeypress for a datagrid
    /// </summary>
    /// <param name="container">The datagrid container</param>
    /// <param name="dgView">The datagrid</param>
    /// <param name="key">The key to selected</param>
    public void SelectKeypressRow(Control container, DataGridView dgView, CecKeypress key)
    {
      AsyncControls.SelectKeypressRow(container, dgView, key);
    }
  }
}

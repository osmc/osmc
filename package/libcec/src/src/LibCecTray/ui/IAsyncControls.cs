using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using CecSharp;

namespace LibCECTray.ui
{
    interface IAsyncControls
    {
        void SetControlEnabled(Control control, bool val);
        void SetControlText(Control control, string val);
        void SetToolStripMenuText(ToolStripMenuItem item, string val);
        void SetCheckboxChecked(CheckBox control, bool val);
        void SetCheckboxItemChecked(CheckedListBox control, int index, bool val);
        void SetProgressValue(ProgressBar control, int val);
        void SetComboBoxItems(ComboBox control, int selectedIndex, object[] val);
        void SetControlVisible(Control control, bool val);
        void DisplayDialog(Form control, bool modal);
        void SafeHide(bool val);
        void SetSelectedIndex(ComboBox control, int index);
        string GetSelectedTabName(TabControl container, TabControl.TabPageCollection tabPages);
        void SelectKeypressRow(Control container, DataGridView dgView, CecKeypress key);
    }
}

namespace LibCECTray.controller.applications
{
  partial class CecButtonConfigUI
  {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.components = new System.ComponentModel.Container();
      this.lButtonName = new System.Windows.Forms.Label();
      this.tbAction = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.bDefault = new System.Windows.Forms.Button();
      this.bClose = new System.Windows.Forms.Button();
      this.label2 = new System.Windows.Forms.Label();
      this.cbAddKey = new System.Windows.Forms.ComboBox();
      this.bAddKey = new System.Windows.Forms.Button();
      this.bClear = new System.Windows.Forms.Button();
      this.label3 = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.cbAddAction = new System.Windows.Forms.ComboBox();
      this.bAddAction = new System.Windows.Forms.Button();
      this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
      this.SuspendLayout();
      // 
      // lButtonName
      // 
      this.lButtonName.AutoSize = true;
      this.lButtonName.Location = new System.Drawing.Point(91, 9);
      this.lButtonName.Name = "lButtonName";
      this.lButtonName.Size = new System.Drawing.Size(72, 13);
      this.lButtonName.TabIndex = 0;
      this.lButtonName.Text = "[button name]";
      // 
      // tbAction
      // 
      this.tbAction.Location = new System.Drawing.Point(79, 32);
      this.tbAction.Name = "tbAction";
      this.tbAction.ReadOnly = true;
      this.tbAction.Size = new System.Drawing.Size(187, 20);
      this.tbAction.TabIndex = 1;
      this.toolTip1.SetToolTip(this.tbAction, "click on an entry to remove it");
      this.tbAction.KeyDown += new System.Windows.Forms.KeyEventHandler(this.ButtonControlCheckSelection);
      this.tbAction.KeyUp += new System.Windows.Forms.KeyEventHandler(this.ButtonControlCheckSelection);
      this.tbAction.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ButtonControlCheckSelection);
      this.tbAction.MouseUp += new System.Windows.Forms.MouseEventHandler(this.ButtonControlCheckSelection);
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(12, 35);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(61, 13);
      this.label1.TabIndex = 2;
      this.label1.Text = "Mapped to:";
      // 
      // bDefault
      // 
      this.bDefault.Location = new System.Drawing.Point(15, 125);
      this.bDefault.Name = "bDefault";
      this.bDefault.Size = new System.Drawing.Size(75, 23);
      this.bDefault.TabIndex = 3;
      this.bDefault.Text = "Default";
      this.bDefault.UseVisualStyleBackColor = true;
      this.bDefault.Click += new System.EventHandler(this.BDefaultClick);
      // 
      // bClose
      // 
      this.bClose.Location = new System.Drawing.Point(178, 125);
      this.bClose.Name = "bClose";
      this.bClose.Size = new System.Drawing.Size(75, 23);
      this.bClose.TabIndex = 4;
      this.bClose.Text = "Close";
      this.bClose.UseVisualStyleBackColor = true;
      this.bClose.Click += new System.EventHandler(this.BCloseClick);
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label2.Location = new System.Drawing.Point(9, 9);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(76, 13);
      this.label2.TabIndex = 5;
      this.label2.Text = "CEC Button:";
      // 
      // cbAddKey
      // 
      this.cbAddKey.FormattingEnabled = true;
      this.cbAddKey.Location = new System.Drawing.Point(79, 58);
      this.cbAddKey.Name = "cbAddKey";
      this.cbAddKey.Size = new System.Drawing.Size(150, 21);
      this.cbAddKey.TabIndex = 6;
      // 
      // bAddKey
      // 
      this.bAddKey.Location = new System.Drawing.Point(235, 57);
      this.bAddKey.Name = "bAddKey";
      this.bAddKey.Size = new System.Drawing.Size(31, 23);
      this.bAddKey.TabIndex = 7;
      this.bAddKey.Text = "+";
      this.bAddKey.UseVisualStyleBackColor = true;
      this.bAddKey.Click += new System.EventHandler(this.BAddKeyClick);
      // 
      // bClear
      // 
      this.bClear.Location = new System.Drawing.Point(97, 125);
      this.bClear.Name = "bClear";
      this.bClear.Size = new System.Drawing.Size(75, 23);
      this.bClear.TabIndex = 8;
      this.bClear.Text = "Clear";
      this.bClear.UseVisualStyleBackColor = true;
      this.bClear.Click += new System.EventHandler(this.BClearClick);
      // 
      // label3
      // 
      this.label3.AutoSize = true;
      this.label3.Location = new System.Drawing.Point(12, 61);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(49, 13);
      this.label3.TabIndex = 9;
      this.label3.Text = "Add key:";
      // 
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(12, 88);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(61, 13);
      this.label4.TabIndex = 10;
      this.label4.Text = "Add action:";
      // 
      // cbAddAction
      // 
      this.cbAddAction.FormattingEnabled = true;
      this.cbAddAction.Location = new System.Drawing.Point(79, 85);
      this.cbAddAction.Name = "cbAddAction";
      this.cbAddAction.Size = new System.Drawing.Size(150, 21);
      this.cbAddAction.TabIndex = 11;
      // 
      // bAddAction
      // 
      this.bAddAction.Location = new System.Drawing.Point(235, 83);
      this.bAddAction.Name = "bAddAction";
      this.bAddAction.Size = new System.Drawing.Size(31, 23);
      this.bAddAction.TabIndex = 12;
      this.bAddAction.Text = "+";
      this.bAddAction.UseVisualStyleBackColor = true;
      this.bAddAction.Click += new System.EventHandler(this.BAddActionClick);
      // 
      // toolTip1
      // 
      this.toolTip1.IsBalloon = true;
      // 
      // CecButtonConfigUI
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(281, 162);
      this.Controls.Add(this.bAddAction);
      this.Controls.Add(this.cbAddAction);
      this.Controls.Add(this.label4);
      this.Controls.Add(this.label3);
      this.Controls.Add(this.bClear);
      this.Controls.Add(this.bAddKey);
      this.Controls.Add(this.cbAddKey);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.bClose);
      this.Controls.Add(this.bDefault);
      this.Controls.Add(this.label1);
      this.Controls.Add(this.tbAction);
      this.Controls.Add(this.lButtonName);
      this.Name = "CecButtonConfigUI";
      this.ShowIcon = false;
      this.ShowInTaskbar = false;
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
      this.Text = "Button configuration";
      this.TopMost = true;
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label lButtonName;
    private System.Windows.Forms.TextBox tbAction;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Button bDefault;
    private System.Windows.Forms.Button bClose;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.ComboBox cbAddKey;
    private System.Windows.Forms.Button bAddKey;
    private System.Windows.Forms.Button bClear;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.ComboBox cbAddAction;
    private System.Windows.Forms.Button bAddAction;
    private System.Windows.Forms.ToolTip toolTip1;
  }
}
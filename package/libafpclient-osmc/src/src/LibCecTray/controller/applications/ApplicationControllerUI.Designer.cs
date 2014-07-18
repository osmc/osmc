namespace LibCECTray.controller.applications
{
  partial class ApplicationControllerUI
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

    #region Component Designer generated code

    /// <summary> 
    /// Required method for Designer support - do not modify 
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.components = new System.ComponentModel.Container();
      this.cbAutoStartApplication = new System.Windows.Forms.CheckBox();
      this.bStartApplication = new System.Windows.Forms.Button();
      this.cbControlApplication = new System.Windows.Forms.CheckBox();
      this.dgButtonConfig = new System.Windows.Forms.DataGridView();
      this.buttonBindingSource = new System.Windows.Forms.BindingSource(this.components);
      this.cbSuppressKeypress = new System.Windows.Forms.CheckBox();
      this.bConfigure = new System.Windows.Forms.Button();
      this.cbStartFullScreen = new System.Windows.Forms.CheckBox();
      ((System.ComponentModel.ISupportInitialize)(this.dgButtonConfig)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.buttonBindingSource)).BeginInit();
      this.SuspendLayout();
      // 
      // cbAutoStartApplication
      // 
      this.cbAutoStartApplication.AutoSize = true;
      this.cbAutoStartApplication.Checked = true;
      this.cbAutoStartApplication.CheckState = System.Windows.Forms.CheckState.Checked;
      this.cbAutoStartApplication.Enabled = false;
      this.cbAutoStartApplication.Location = new System.Drawing.Point(131, 14);
      this.cbAutoStartApplication.Name = "cbAutoStartApplication";
      this.cbAutoStartApplication.Size = new System.Drawing.Size(91, 17);
      this.cbAutoStartApplication.TabIndex = 38;
      this.cbAutoStartApplication.Text = "app_autostart";
      this.cbAutoStartApplication.UseVisualStyleBackColor = true;
      // 
      // bStartApplication
      // 
      this.bStartApplication.Enabled = false;
      this.bStartApplication.Location = new System.Drawing.Point(12, 318);
      this.bStartApplication.Name = "bStartApplication";
      this.bStartApplication.Size = new System.Drawing.Size(119, 23);
      this.bStartApplication.TabIndex = 37;
      this.bStartApplication.Text = "Start application";
      this.bStartApplication.UseVisualStyleBackColor = true;
      this.bStartApplication.Click += new System.EventHandler(this.BStartApplicationClick);
      // 
      // cbControlApplication
      // 
      this.cbControlApplication.AutoSize = true;
      this.cbControlApplication.Checked = true;
      this.cbControlApplication.CheckState = System.Windows.Forms.CheckState.Checked;
      this.cbControlApplication.Enabled = false;
      this.cbControlApplication.Location = new System.Drawing.Point(12, 14);
      this.cbControlApplication.Name = "cbControlApplication";
      this.cbControlApplication.Size = new System.Drawing.Size(82, 17);
      this.cbControlApplication.TabIndex = 36;
      this.cbControlApplication.Text = "app_control";
      this.cbControlApplication.UseVisualStyleBackColor = true;
      // 
      // dgButtonConfig
      // 
      this.dgButtonConfig.AllowUserToAddRows = false;
      this.dgButtonConfig.AllowUserToDeleteRows = false;
      this.dgButtonConfig.AutoGenerateColumns = false;
      this.dgButtonConfig.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
      this.dgButtonConfig.DataSource = this.buttonBindingSource;
      this.dgButtonConfig.Location = new System.Drawing.Point(12, 60);
      this.dgButtonConfig.Name = "dgButtonConfig";
      this.dgButtonConfig.Size = new System.Drawing.Size(566, 252);
      this.dgButtonConfig.TabIndex = 40;
      // 
      // cbSuppressKeypress
      // 
      this.cbSuppressKeypress.AutoSize = true;
      this.cbSuppressKeypress.Checked = true;
      this.cbSuppressKeypress.CheckState = System.Windows.Forms.CheckState.Checked;
      this.cbSuppressKeypress.Enabled = false;
      this.cbSuppressKeypress.Location = new System.Drawing.Point(262, 14);
      this.cbSuppressKeypress.Name = "cbSuppressKeypress";
      this.cbSuppressKeypress.Size = new System.Drawing.Size(170, 17);
      this.cbSuppressKeypress.TabIndex = 41;
      this.cbSuppressKeypress.Text = "app_suppress_when_selected";
      this.cbSuppressKeypress.UseVisualStyleBackColor = true;
      // 
      // bConfigure
      // 
      this.bConfigure.Location = new System.Drawing.Point(503, 8);
      this.bConfigure.Name = "bConfigure";
      this.bConfigure.Size = new System.Drawing.Size(75, 23);
      this.bConfigure.TabIndex = 42;
      this.bConfigure.Text = "Configure";
      this.bConfigure.UseVisualStyleBackColor = true;
      this.bConfigure.Click += new System.EventHandler(this.BConfigureClick);
      // 
      // cbStartFullScreen
      // 
      this.cbStartFullScreen.AutoSize = true;
      this.cbStartFullScreen.Checked = true;
      this.cbStartFullScreen.CheckState = System.Windows.Forms.CheckState.Checked;
      this.cbStartFullScreen.Enabled = false;
      this.cbStartFullScreen.Location = new System.Drawing.Point(12, 37);
      this.cbStartFullScreen.Name = "cbStartFullScreen";
      this.cbStartFullScreen.Size = new System.Drawing.Size(121, 17);
      this.cbStartFullScreen.TabIndex = 43;
      this.cbStartFullScreen.Text = "app_start_fullscreen";
      this.cbStartFullScreen.UseVisualStyleBackColor = true;
      // 
      // ApplicationControllerUI
      // 
      this.ClientSize = new System.Drawing.Size(576, 306);
      this.Controls.Add(this.cbStartFullScreen);
      this.Controls.Add(this.bConfigure);
      this.Controls.Add(this.cbSuppressKeypress);
      this.Controls.Add(this.dgButtonConfig);
      this.Controls.Add(this.cbAutoStartApplication);
      this.Controls.Add(this.bStartApplication);
      this.Controls.Add(this.cbControlApplication);
      this.Name = "ApplicationControllerUI";
      ((System.ComponentModel.ISupportInitialize)(this.dgButtonConfig)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.buttonBindingSource)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.CheckBox cbAutoStartApplication;
    private System.Windows.Forms.Button bStartApplication;
    private System.Windows.Forms.CheckBox cbControlApplication;
    private System.Windows.Forms.DataGridView dgButtonConfig;
    private System.Windows.Forms.BindingSource buttonBindingSource;
    private System.Windows.Forms.CheckBox cbSuppressKeypress;
    private System.Windows.Forms.Button bConfigure;
    private System.Windows.Forms.CheckBox cbStartFullScreen;

  }
}

namespace LibCECTray.controller.applications.@internal
{
	partial class XBMCControllerUI
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
      this.cbStartFullScreen = new System.Windows.Forms.CheckBox();
      this.buttonBindingSource = new System.Windows.Forms.BindingSource(this.components);
      this.bConfigure = new System.Windows.Forms.Button();
      this.bStartApplication = new System.Windows.Forms.Button();
      this.cbStandbyScreensaver = new System.Windows.Forms.CheckBox();
      this.cbUseTvLanguage = new System.Windows.Forms.CheckBox();
      this.bLoadConfig = new System.Windows.Forms.Button();
      this.bSaveConfig = new System.Windows.Forms.Button();
      this.cbStandbyTvStandby = new System.Windows.Forms.CheckBox();
      this.cbInactiveSource = new System.Windows.Forms.CheckBox();
      this.cbPauseOnDeactivate = new System.Windows.Forms.CheckBox();
      ((System.ComponentModel.ISupportInitialize)(this.buttonBindingSource)).BeginInit();
      this.SuspendLayout();
      // 
      // cbStartFullScreen
      // 
      this.cbStartFullScreen.AutoSize = true;
      this.cbStartFullScreen.Checked = true;
      this.cbStartFullScreen.CheckState = System.Windows.Forms.CheckState.Checked;
      this.cbStartFullScreen.Enabled = false;
      this.cbStartFullScreen.Location = new System.Drawing.Point(12, 12);
      this.cbStartFullScreen.Name = "cbStartFullScreen";
      this.cbStartFullScreen.Size = new System.Drawing.Size(121, 17);
      this.cbStartFullScreen.TabIndex = 50;
      this.cbStartFullScreen.Text = "app_start_fullscreen";
      this.cbStartFullScreen.UseVisualStyleBackColor = true;
      // 
      // bConfigure
      // 
      this.bConfigure.Location = new System.Drawing.Point(503, 8);
      this.bConfigure.Name = "bConfigure";
      this.bConfigure.Size = new System.Drawing.Size(75, 23);
      this.bConfigure.TabIndex = 49;
      this.bConfigure.Text = "Configure";
      this.bConfigure.UseVisualStyleBackColor = true;
      this.bConfigure.Click += new System.EventHandler(this.BConfigureClick);
      // 
      // bStartApplication
      // 
      this.bStartApplication.Enabled = false;
      this.bStartApplication.Location = new System.Drawing.Point(11, 271);
      this.bStartApplication.Name = "bStartApplication";
      this.bStartApplication.Size = new System.Drawing.Size(119, 23);
      this.bStartApplication.TabIndex = 45;
      this.bStartApplication.Text = "Start application";
      this.bStartApplication.UseVisualStyleBackColor = true;
      this.bStartApplication.Click += new System.EventHandler(this.BStartApplicationClick);
      // 
      // cbStandbyScreensaver
      // 
      this.cbStandbyScreensaver.AutoSize = true;
      this.cbStandbyScreensaver.Enabled = false;
      this.cbStandbyScreensaver.Location = new System.Drawing.Point(11, 66);
      this.cbStandbyScreensaver.Name = "cbStandbyScreensaver";
      this.cbStandbyScreensaver.Size = new System.Drawing.Size(151, 17);
      this.cbStandbyScreensaver.TabIndex = 53;
      this.cbStandbyScreensaver.Text = "app_standby_screensaver";
      this.cbStandbyScreensaver.UseVisualStyleBackColor = true;
      // 
      // cbUseTvLanguage
      // 
      this.cbUseTvLanguage.AutoSize = true;
      this.cbUseTvLanguage.Enabled = false;
      this.cbUseTvLanguage.Location = new System.Drawing.Point(11, 43);
      this.cbUseTvLanguage.Name = "cbUseTvLanguage";
      this.cbUseTvLanguage.Size = new System.Drawing.Size(132, 17);
      this.cbUseTvLanguage.TabIndex = 51;
      this.cbUseTvLanguage.Text = "app_use_tv_language";
      this.cbUseTvLanguage.UseVisualStyleBackColor = true;
      // 
      // bLoadConfig
      // 
      this.bLoadConfig.Enabled = false;
      this.bLoadConfig.Location = new System.Drawing.Point(186, 271);
      this.bLoadConfig.Name = "bLoadConfig";
      this.bLoadConfig.Size = new System.Drawing.Size(119, 23);
      this.bLoadConfig.TabIndex = 55;
      this.bLoadConfig.Text = "Import configuration";
      this.bLoadConfig.UseVisualStyleBackColor = true;
      this.bLoadConfig.Click += new System.EventHandler(this.BLoadConfigClick);
      // 
      // bSaveConfig
      // 
      this.bSaveConfig.Enabled = false;
      this.bSaveConfig.Location = new System.Drawing.Point(311, 271);
      this.bSaveConfig.Name = "bSaveConfig";
      this.bSaveConfig.Size = new System.Drawing.Size(119, 23);
      this.bSaveConfig.TabIndex = 56;
      this.bSaveConfig.Text = "Save configuration";
      this.bSaveConfig.UseVisualStyleBackColor = true;
      this.bSaveConfig.Click += new System.EventHandler(this.BSaveConfigClick);
      // 
      // cbStandbyTvStandby
      // 
      this.cbStandbyTvStandby.AutoSize = true;
      this.cbStandbyTvStandby.Enabled = false;
      this.cbStandbyTvStandby.Location = new System.Drawing.Point(11, 89);
      this.cbStandbyTvStandby.Name = "cbStandbyTvStandby";
      this.cbStandbyTvStandby.Size = new System.Drawing.Size(163, 17);
      this.cbStandbyTvStandby.TabIndex = 57;
      this.cbStandbyTvStandby.Text = "app_standby_on_tv_standby";
      this.cbStandbyTvStandby.UseVisualStyleBackColor = true;
      // 
      // cbInactiveSource
      // 
      this.cbInactiveSource.AutoSize = true;
      this.cbInactiveSource.Enabled = false;
      this.cbInactiveSource.Location = new System.Drawing.Point(11, 112);
      this.cbInactiveSource.Name = "cbInactiveSource";
      this.cbInactiveSource.Size = new System.Drawing.Size(154, 17);
      this.cbInactiveSource.TabIndex = 58;
      this.cbInactiveSource.Text = "app_send_inactive_source";
      this.cbInactiveSource.UseVisualStyleBackColor = true;
      // 
      // cbPauseOnDeactivate
      // 
      this.cbPauseOnDeactivate.AutoSize = true;
      this.cbPauseOnDeactivate.Enabled = false;
      this.cbPauseOnDeactivate.Location = new System.Drawing.Point(11, 135);
      this.cbPauseOnDeactivate.Name = "cbPauseOnDeactivate";
      this.cbPauseOnDeactivate.Size = new System.Drawing.Size(202, 17);
      this.cbPauseOnDeactivate.TabIndex = 59;
      this.cbPauseOnDeactivate.Text = "app_pause_playback_on_deactivate";
      this.cbPauseOnDeactivate.UseVisualStyleBackColor = true;
      // 
      // XBMCControllerUI
      // 
      this.ClientSize = new System.Drawing.Size(576, 306);
      this.Controls.Add(this.cbPauseOnDeactivate);
      this.Controls.Add(this.cbInactiveSource);
      this.Controls.Add(this.cbStandbyTvStandby);
      this.Controls.Add(this.bSaveConfig);
      this.Controls.Add(this.bLoadConfig);
      this.Controls.Add(this.cbStandbyScreensaver);
      this.Controls.Add(this.cbUseTvLanguage);
      this.Controls.Add(this.cbStartFullScreen);
      this.Controls.Add(this.bConfigure);
      this.Controls.Add(this.bStartApplication);
      this.Name = "XBMCControllerUI";
      this.Text = "XBMCControllerUI";
      ((System.ComponentModel.ISupportInitialize)(this.buttonBindingSource)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

		}

		#endregion

    private System.Windows.Forms.CheckBox cbStartFullScreen;
    private System.Windows.Forms.BindingSource buttonBindingSource;
    private System.Windows.Forms.Button bConfigure;
    private System.Windows.Forms.Button bStartApplication;
    private System.Windows.Forms.CheckBox cbStandbyScreensaver;
    private System.Windows.Forms.CheckBox cbUseTvLanguage;
    private System.Windows.Forms.Button bLoadConfig;
    private System.Windows.Forms.Button bSaveConfig;
    private System.Windows.Forms.CheckBox cbStandbyTvStandby;
    private System.Windows.Forms.CheckBox cbInactiveSource;
    private System.Windows.Forms.CheckBox cbPauseOnDeactivate;
	}
}
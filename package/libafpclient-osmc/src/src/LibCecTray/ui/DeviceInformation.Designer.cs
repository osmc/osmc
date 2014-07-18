namespace LibCECTray.ui
{
  partial class DeviceInformation
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DeviceInformation));
      this.label1 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.label3 = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.label5 = new System.Windows.Forms.Label();
      this.label6 = new System.Windows.Forms.Label();
      this.label7 = new System.Windows.Forms.Label();
      this.label8 = new System.Windows.Forms.Label();
      this.label9 = new System.Windows.Forms.Label();
      this.label10 = new System.Windows.Forms.Label();
      this.lMenuLanguage = new System.Windows.Forms.Label();
      this.lOsdName = new System.Windows.Forms.Label();
      this.lCecVersion = new System.Windows.Forms.Label();
      this.lVendor = new System.Windows.Forms.Label();
      this.lActiveSource = new System.Windows.Forms.Label();
      this.lDevicePresent = new System.Windows.Forms.Label();
      this.lPhysicalAddress = new System.Windows.Forms.Label();
      this.lLogicalAddress = new System.Windows.Forms.Label();
      this.lDevice = new System.Windows.Forms.Label();
      this.lInactiveSource = new System.Windows.Forms.LinkLabel();
      this.lPowerStatus = new System.Windows.Forms.LinkLabel();
      this.bUpdate = new System.Windows.Forms.Button();
      this.SuspendLayout();
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label1.Location = new System.Drawing.Point(12, 9);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(47, 13);
      this.label1.TabIndex = 0;
      this.label1.Text = "Device";
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label2.Location = new System.Drawing.Point(12, 29);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(96, 13);
      this.label2.TabIndex = 1;
      this.label2.Text = "Logical address";
      // 
      // label3
      // 
      this.label3.AutoSize = true;
      this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label3.Location = new System.Drawing.Point(12, 49);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(102, 13);
      this.label3.TabIndex = 2;
      this.label3.Text = "Physical address";
      // 
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label4.Location = new System.Drawing.Point(12, 69);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(93, 13);
      this.label4.TabIndex = 3;
      this.label4.Text = "Device present";
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label5.Location = new System.Drawing.Point(12, 89);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(85, 13);
      this.label5.TabIndex = 4;
      this.label5.Text = "Active source";
      // 
      // label6
      // 
      this.label6.AutoSize = true;
      this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label6.Location = new System.Drawing.Point(12, 109);
      this.label6.Name = "label6";
      this.label6.Size = new System.Drawing.Size(47, 13);
      this.label6.TabIndex = 5;
      this.label6.Text = "Vendor";
      // 
      // label7
      // 
      this.label7.AutoSize = true;
      this.label7.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label7.Location = new System.Drawing.Point(12, 129);
      this.label7.Name = "label7";
      this.label7.Size = new System.Drawing.Size(76, 13);
      this.label7.TabIndex = 6;
      this.label7.Text = "CEC version";
      // 
      // label8
      // 
      this.label8.AutoSize = true;
      this.label8.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label8.Location = new System.Drawing.Point(12, 149);
      this.label8.Name = "label8";
      this.label8.Size = new System.Drawing.Size(80, 13);
      this.label8.TabIndex = 7;
      this.label8.Text = "Power status";
      // 
      // label9
      // 
      this.label9.AutoSize = true;
      this.label9.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label9.Location = new System.Drawing.Point(12, 169);
      this.label9.Name = "label9";
      this.label9.Size = new System.Drawing.Size(67, 13);
      this.label9.TabIndex = 8;
      this.label9.Text = "OSD name";
      // 
      // label10
      // 
      this.label10.AutoSize = true;
      this.label10.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label10.Location = new System.Drawing.Point(12, 189);
      this.label10.Name = "label10";
      this.label10.Size = new System.Drawing.Size(94, 13);
      this.label10.TabIndex = 9;
      this.label10.Text = "Menu language";
      // 
      // lMenuLanguage
      // 
      this.lMenuLanguage.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lMenuLanguage.Location = new System.Drawing.Point(122, 189);
      this.lMenuLanguage.Name = "lMenuLanguage";
      this.lMenuLanguage.Size = new System.Drawing.Size(150, 13);
      this.lMenuLanguage.TabIndex = 19;
      this.lMenuLanguage.Text = "unknown";
      this.lMenuLanguage.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lOsdName
      // 
      this.lOsdName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lOsdName.Location = new System.Drawing.Point(122, 169);
      this.lOsdName.Name = "lOsdName";
      this.lOsdName.Size = new System.Drawing.Size(150, 13);
      this.lOsdName.TabIndex = 20;
      this.lOsdName.Text = "unknown";
      this.lOsdName.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lCecVersion
      // 
      this.lCecVersion.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lCecVersion.Location = new System.Drawing.Point(122, 129);
      this.lCecVersion.Name = "lCecVersion";
      this.lCecVersion.Size = new System.Drawing.Size(150, 13);
      this.lCecVersion.TabIndex = 22;
      this.lCecVersion.Text = "unknown";
      this.lCecVersion.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lVendor
      // 
      this.lVendor.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lVendor.Location = new System.Drawing.Point(122, 109);
      this.lVendor.Name = "lVendor";
      this.lVendor.Size = new System.Drawing.Size(150, 13);
      this.lVendor.TabIndex = 23;
      this.lVendor.Text = "unknown";
      this.lVendor.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lActiveSource
      // 
      this.lActiveSource.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lActiveSource.Location = new System.Drawing.Point(122, 89);
      this.lActiveSource.Name = "lActiveSource";
      this.lActiveSource.Size = new System.Drawing.Size(150, 13);
      this.lActiveSource.TabIndex = 24;
      this.lActiveSource.Text = "yes";
      this.lActiveSource.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lDevicePresent
      // 
      this.lDevicePresent.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lDevicePresent.Location = new System.Drawing.Point(122, 69);
      this.lDevicePresent.Name = "lDevicePresent";
      this.lDevicePresent.Size = new System.Drawing.Size(150, 13);
      this.lDevicePresent.TabIndex = 25;
      this.lDevicePresent.Text = "unknown";
      this.lDevicePresent.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lPhysicalAddress
      // 
      this.lPhysicalAddress.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lPhysicalAddress.Location = new System.Drawing.Point(122, 49);
      this.lPhysicalAddress.Name = "lPhysicalAddress";
      this.lPhysicalAddress.Size = new System.Drawing.Size(150, 13);
      this.lPhysicalAddress.TabIndex = 26;
      this.lPhysicalAddress.Text = "unknown";
      this.lPhysicalAddress.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lLogicalAddress
      // 
      this.lLogicalAddress.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lLogicalAddress.Location = new System.Drawing.Point(122, 29);
      this.lLogicalAddress.Name = "lLogicalAddress";
      this.lLogicalAddress.Size = new System.Drawing.Size(150, 13);
      this.lLogicalAddress.TabIndex = 27;
      this.lLogicalAddress.Text = "unknown";
      this.lLogicalAddress.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lDevice
      // 
      this.lDevice.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.lDevice.Location = new System.Drawing.Point(122, 9);
      this.lDevice.Name = "lDevice";
      this.lDevice.Size = new System.Drawing.Size(150, 13);
      this.lDevice.TabIndex = 28;
      this.lDevice.Text = "unknown";
      this.lDevice.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // lInactiveSource
      // 
      this.lInactiveSource.Location = new System.Drawing.Point(122, 89);
      this.lInactiveSource.Name = "lInactiveSource";
      this.lInactiveSource.Size = new System.Drawing.Size(150, 13);
      this.lInactiveSource.TabIndex = 29;
      this.lInactiveSource.TabStop = true;
      this.lInactiveSource.Text = "no";
      this.lInactiveSource.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      this.lInactiveSource.Visible = false;
      this.lInactiveSource.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.LInactiveSourceLinkClicked);
      // 
      // lPowerStatus
      // 
      this.lPowerStatus.Location = new System.Drawing.Point(122, 149);
      this.lPowerStatus.Name = "lPowerStatus";
      this.lPowerStatus.Size = new System.Drawing.Size(150, 13);
      this.lPowerStatus.TabIndex = 30;
      this.lPowerStatus.TabStop = true;
      this.lPowerStatus.Text = "unknown";
      this.lPowerStatus.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      this.lPowerStatus.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.LStandbyLinkClicked);
      // 
      // bUpdate
      // 
      this.bUpdate.Location = new System.Drawing.Point(102, 207);
      this.bUpdate.Name = "bUpdate";
      this.bUpdate.Size = new System.Drawing.Size(75, 23);
      this.bUpdate.TabIndex = 31;
      this.bUpdate.Text = "Refresh";
      this.bUpdate.UseVisualStyleBackColor = true;
      this.bUpdate.Click += new System.EventHandler(this.Button1Click);
      // 
      // DeviceInformation
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(284, 239);
      this.Controls.Add(this.bUpdate);
      this.Controls.Add(this.lPowerStatus);
      this.Controls.Add(this.lInactiveSource);
      this.Controls.Add(this.lDevice);
      this.Controls.Add(this.lLogicalAddress);
      this.Controls.Add(this.lPhysicalAddress);
      this.Controls.Add(this.lDevicePresent);
      this.Controls.Add(this.lActiveSource);
      this.Controls.Add(this.lVendor);
      this.Controls.Add(this.lCecVersion);
      this.Controls.Add(this.lOsdName);
      this.Controls.Add(this.lMenuLanguage);
      this.Controls.Add(this.label10);
      this.Controls.Add(this.label9);
      this.Controls.Add(this.label8);
      this.Controls.Add(this.label7);
      this.Controls.Add(this.label6);
      this.Controls.Add(this.label5);
      this.Controls.Add(this.label4);
      this.Controls.Add(this.label3);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.label1);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "DeviceInformation";
      this.ShowInTaskbar = false;
      this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "Device: [unknown]";
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.Label label6;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.Label label8;
    private System.Windows.Forms.Label label9;
    private System.Windows.Forms.Label label10;
    private System.Windows.Forms.Label lMenuLanguage;
    private System.Windows.Forms.Label lOsdName;
    private System.Windows.Forms.Label lCecVersion;
    private System.Windows.Forms.Label lVendor;
    private System.Windows.Forms.Label lActiveSource;
    private System.Windows.Forms.Label lDevicePresent;
    private System.Windows.Forms.Label lPhysicalAddress;
    private System.Windows.Forms.Label lLogicalAddress;
    private System.Windows.Forms.Label lDevice;
    private System.Windows.Forms.LinkLabel lInactiveSource;
    private System.Windows.Forms.LinkLabel lPowerStatus;
    public System.Windows.Forms.Button bUpdate;
  }
}
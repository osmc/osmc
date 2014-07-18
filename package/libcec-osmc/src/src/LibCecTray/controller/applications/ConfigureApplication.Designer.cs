namespace LibCECTray.controller.applications
{
  partial class ConfigureApplication
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
      this.lFilename = new System.Windows.Forms.Label();
      this.tbFilename = new System.Windows.Forms.TextBox();
      this.bFindFile = new System.Windows.Forms.Button();
      this.lProcessName = new System.Windows.Forms.Label();
      this.tbProcessName = new System.Windows.Forms.TextBox();
      this.tbWorkingDir = new System.Windows.Forms.TextBox();
      this.lWorkingDir = new System.Windows.Forms.Label();
      this.bOK = new System.Windows.Forms.Button();
      this.bCancel = new System.Windows.Forms.Button();
      this.tbUiName = new System.Windows.Forms.TextBox();
      this.lUiName = new System.Windows.Forms.Label();
      this.SuspendLayout();
      // 
      // lFilename
      // 
      this.lFilename.AutoSize = true;
      this.lFilename.Location = new System.Drawing.Point(12, 17);
      this.lFilename.Name = "lFilename";
      this.lFilename.Size = new System.Drawing.Size(46, 13);
      this.lFilename.TabIndex = 0;
      this.lFilename.Text = "filename";
      // 
      // tbFilename
      // 
      this.tbFilename.Location = new System.Drawing.Point(93, 14);
      this.tbFilename.Name = "tbFilename";
      this.tbFilename.Size = new System.Drawing.Size(229, 20);
      this.tbFilename.TabIndex = 1;
      this.tbFilename.Text = "filename";
      // 
      // bFindFile
      // 
      this.bFindFile.Location = new System.Drawing.Point(328, 12);
      this.bFindFile.Name = "bFindFile";
      this.bFindFile.Size = new System.Drawing.Size(44, 23);
      this.bFindFile.TabIndex = 2;
      this.bFindFile.Text = "...";
      this.bFindFile.UseVisualStyleBackColor = true;
      this.bFindFile.Click += new System.EventHandler(this.BFindFileClick);
      // 
      // lProcessName
      // 
      this.lProcessName.AutoSize = true;
      this.lProcessName.Location = new System.Drawing.Point(12, 70);
      this.lProcessName.Name = "lProcessName";
      this.lProcessName.Size = new System.Drawing.Size(76, 13);
      this.lProcessName.TabIndex = 3;
      this.lProcessName.Text = "process_name";
      // 
      // tbProcessName
      // 
      this.tbProcessName.Location = new System.Drawing.Point(93, 67);
      this.tbProcessName.Name = "tbProcessName";
      this.tbProcessName.Size = new System.Drawing.Size(279, 20);
      this.tbProcessName.TabIndex = 4;
      this.tbProcessName.Text = "process_name";
      // 
      // tbWorkingDir
      // 
      this.tbWorkingDir.Location = new System.Drawing.Point(93, 41);
      this.tbWorkingDir.Name = "tbWorkingDir";
      this.tbWorkingDir.Size = new System.Drawing.Size(279, 20);
      this.tbWorkingDir.TabIndex = 6;
      this.tbWorkingDir.Text = "working_dir";
      // 
      // lWorkingDir
      // 
      this.lWorkingDir.AutoSize = true;
      this.lWorkingDir.Location = new System.Drawing.Point(12, 44);
      this.lWorkingDir.Name = "lWorkingDir";
      this.lWorkingDir.Size = new System.Drawing.Size(61, 13);
      this.lWorkingDir.TabIndex = 5;
      this.lWorkingDir.Text = "working_dir";
      // 
      // bOK
      // 
      this.bOK.Location = new System.Drawing.Point(204, 129);
      this.bOK.Name = "bOK";
      this.bOK.Size = new System.Drawing.Size(75, 23);
      this.bOK.TabIndex = 7;
      this.bOK.Text = "OK";
      this.bOK.UseVisualStyleBackColor = true;
      this.bOK.Click += new System.EventHandler(this.BOkClick);
      // 
      // bCancel
      // 
      this.bCancel.Location = new System.Drawing.Point(91, 129);
      this.bCancel.Name = "bCancel";
      this.bCancel.Size = new System.Drawing.Size(75, 23);
      this.bCancel.TabIndex = 8;
      this.bCancel.Text = "Cancel";
      this.bCancel.UseVisualStyleBackColor = true;
      this.bCancel.Click += new System.EventHandler(this.BCancelClick);
      // 
      // tbUiName
      // 
      this.tbUiName.Location = new System.Drawing.Point(93, 93);
      this.tbUiName.Name = "tbUiName";
      this.tbUiName.Size = new System.Drawing.Size(279, 20);
      this.tbUiName.TabIndex = 10;
      this.tbUiName.Text = "ui_name";
      // 
      // lUiName
      // 
      this.lUiName.AutoSize = true;
      this.lUiName.Location = new System.Drawing.Point(12, 96);
      this.lUiName.Name = "lUiName";
      this.lUiName.Size = new System.Drawing.Size(47, 13);
      this.lUiName.TabIndex = 9;
      this.lUiName.Text = "ui_name";
      // 
      // ConfigureApplication
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(384, 165);
      this.Controls.Add(this.tbUiName);
      this.Controls.Add(this.lUiName);
      this.Controls.Add(this.bCancel);
      this.Controls.Add(this.bOK);
      this.Controls.Add(this.tbWorkingDir);
      this.Controls.Add(this.lWorkingDir);
      this.Controls.Add(this.tbProcessName);
      this.Controls.Add(this.lProcessName);
      this.Controls.Add(this.bFindFile);
      this.Controls.Add(this.tbFilename);
      this.Controls.Add(this.lFilename);
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "ConfigureApplication";
      this.ShowIcon = false;
      this.ShowInTaskbar = false;
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "ConfigureApplication";
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label lFilename;
    private System.Windows.Forms.TextBox tbFilename;
    private System.Windows.Forms.Button bFindFile;
    private System.Windows.Forms.Label lProcessName;
    private System.Windows.Forms.TextBox tbProcessName;
    private System.Windows.Forms.TextBox tbWorkingDir;
    private System.Windows.Forms.Label lWorkingDir;
    private System.Windows.Forms.Button bOK;
    private System.Windows.Forms.Button bCancel;
    private System.Windows.Forms.TextBox tbUiName;
    private System.Windows.Forms.Label lUiName;
  }
}
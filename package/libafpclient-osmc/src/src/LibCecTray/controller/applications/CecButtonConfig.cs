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

using System.Collections.Generic;
using CecSharp;
using LibCECTray.settings;
using System;

namespace LibCECTray.controller.applications
{
  class CecButtonConfigItem : CECSettingString
  {
    public CecButtonConfigItem(ApplicationController controller, CecKeypress key) :
      base(CECSettingType.Button, null, ButtonName(key.Keycode), ApplicationInput.DefaultValue(controller, key).AsString(), null)
    {
      Key = key;
    }

    public new ApplicationInput Value
    {
      get
      {
        return base.Value != null
                 ? ApplicationInput.FromString(Controller, base.Value)
                 : new ApplicationInput(Controller);
      }
      set
      {
        base.Value = value == null ? null : value.AsString();
      }
    }

    public new ApplicationInput DefaultValue
    {
      get { return base.DefaultValue != null ? ApplicationInput.FromString(Controller, base.DefaultValue) : null; }
      set { base.DefaultValue = value.AsString(); }
    }

    public string MappedButtonName
    {
      get { return Value == null ? string.Empty : Value.AsFriendlyString(); }
      set { base.Value = value; }
    }

    public new bool Enabled
    {
      get
      {
        return _enabled || !string.IsNullOrEmpty(base.Value);
      }
      set
      {
        _enabled = value;
      }
    }
    private bool _enabled;

    public string CecButtonName
    {
      get { return ButtonName(Key.Keycode); }
    }

    public void SetController(ApplicationController controller)
    {
      Controller = controller;
      KeyName = string.Format("{0}_key_{1}", controller.ProcessName, (int)Key.Keycode);
    }

    public static string ButtonName(CecUserControlCode key)
    {
      switch (key)
      {
        case CecUserControlCode.Select:
          return "Select";
        case CecUserControlCode.Up:
          return "Up";
        case CecUserControlCode.Down:
          return "Down";
        case CecUserControlCode.Left:
          return "Left";
        case CecUserControlCode.Right:
          return "Right";
        case CecUserControlCode.RightUp:
          return "Right+Up";
        case CecUserControlCode.RightDown:
          return "Right+Down";
        case CecUserControlCode.LeftUp:
          return "Left+Up";
        case CecUserControlCode.LeftDown:
          return "Left+Down";
        case CecUserControlCode.RootMenu:
          return "Root menu";
        case CecUserControlCode.SetupMenu:
          return "Setup menu";
        case CecUserControlCode.ContentsMenu:
          return "Contents menu";
        case CecUserControlCode.FavoriteMenu:
          return "Favourite menu";
        case CecUserControlCode.Exit:
          return "Exit";
        case CecUserControlCode.Number0:
          return "0";
        case CecUserControlCode.Number1:
          return "1";
        case CecUserControlCode.Number2:
          return "2";
        case CecUserControlCode.Number3:
          return "3";
        case CecUserControlCode.Number4:
          return "4";
        case CecUserControlCode.Number5:
          return "5";
        case CecUserControlCode.Number6:
          return "6";
        case CecUserControlCode.Number7:
          return "7";
        case CecUserControlCode.Number8:
          return "8";
        case CecUserControlCode.Number9:
          return "9";
        case CecUserControlCode.Dot:
          return ".";
        case CecUserControlCode.Enter:
          return "Enter";
        case CecUserControlCode.Clear:
          return "Clear";
        case CecUserControlCode.NextFavorite:
          return "Next favourite";
        case CecUserControlCode.ChannelUp:
          return "Channel up";
        case CecUserControlCode.ChannelDown:
          return "Channel down";
        case CecUserControlCode.PreviousChannel:
          return "Previous channel";
        case CecUserControlCode.SoundSelect:
          return "Sound select";
        case CecUserControlCode.InputSelect:
          return "Input select";
        case CecUserControlCode.DisplayInformation:
          return "Display information";
        case CecUserControlCode.Help:
          return "Help";
        case CecUserControlCode.PageUp:
          return "Page up";
        case CecUserControlCode.PageDown:
          return "Page down";
        case CecUserControlCode.Power:
          return "Power";
        case CecUserControlCode.VolumeUp:
          return "Volume up";
        case CecUserControlCode.VolumeDown:
          return "Volume down";
        case CecUserControlCode.Mute:
          return "Mute";
        case CecUserControlCode.Play:
          return "Play";
        case CecUserControlCode.Stop:
          return "Stop";
        case CecUserControlCode.Pause:
          return "Pause";
        case CecUserControlCode.Record:
          return "Record";
        case CecUserControlCode.Rewind:
          return "Rewind";
        case CecUserControlCode.FastForward:
          return "Fast forward";
        case CecUserControlCode.Eject:
          return "Eject";
        case CecUserControlCode.Forward:
          return "Forward";
        case CecUserControlCode.Backward:
          return "Backward";
        case CecUserControlCode.StopRecord:
          return "Stop record";
        case CecUserControlCode.PauseRecord:
          return "Pause record";
        case CecUserControlCode.Angle:
          return "Angle";
        case CecUserControlCode.SubPicture:
          return "Sub picture";
        case CecUserControlCode.VideoOnDemand:
          return "Video on demand";
        case CecUserControlCode.ElectronicProgramGuide:
          return "Electronic program guide";
        case CecUserControlCode.TimerProgramming:
          return "Timer programming";
        case CecUserControlCode.InitialConfiguration:
          return "Initial configuration";
        case CecUserControlCode.PlayFunction:
          return "Play (function)";
        case CecUserControlCode.PausePlayFunction:
          return "Pause play (function)";
        case CecUserControlCode.RecordFunction:
          return "Record (function)";
        case CecUserControlCode.PauseRecordFunction:
          return "Pause record (function)";
        case CecUserControlCode.StopFunction:
          return "Stop (function)";
        case CecUserControlCode.MuteFunction:
          return "Mute (function)";
        case CecUserControlCode.RestoreVolumeFunction:
          return "Restore volume";
        case CecUserControlCode.TuneFunction:
          return "Tune";
        case CecUserControlCode.SelectMediaFunction:
          return "Select media";
        case CecUserControlCode.SelectAVInputFunction:
          return "Select AV input";
        case CecUserControlCode.SelectAudioInputFunction:
          return "Select audio input";
        case CecUserControlCode.PowerToggleFunction:
          return "Power toggle";
        case CecUserControlCode.PowerOffFunction:
          return "Power off";
        case CecUserControlCode.PowerOnFunction:
          return "Power on";
        case CecUserControlCode.F1Blue:
          return "F1 (blue)";
        case CecUserControlCode.F2Red:
          return "F2 (red)";
        case CecUserControlCode.F3Green:
          return "F3 (green)";
        case CecUserControlCode.F4Yellow:
          return "F4 (yellow)";
        case CecUserControlCode.F5:
          return "F5";
        case CecUserControlCode.Data:
          return "Data";
        case CecUserControlCode.SamsungReturn:
          return "Return (Samsung)";
      }
      return "Unknown";
    }

    public CecKeypress Key { get; private set; }
    public ApplicationController Controller { get; private set; }
  }

  internal class CecButtonConfig : List<CecButtonConfigItem>
  {
    public CecButtonConfig(ApplicationController controller)
    {
      _controller = controller;

      foreach (CecUserControlCode key in Enum.GetValues(typeof(CecUserControlCode)))
        AddConfigItem(new CecButtonConfigItem(controller, (new CecKeypress { Keycode = key })));

      Load();
    }

    private void AddConfigItem(CecButtonConfigItem item)
    {
      if (!HasItem(item) && item.Key.Keycode != CecUserControlCode.Unknown)
        Add(item);
    }

    public bool HasItem(CecButtonConfigItem item)
    {
      foreach (var entry in  this)
      {
        if (item.Key.Keycode == entry.Key.Keycode)
          return true;
      }
      return false;
    }

    public void Load()
    {
      foreach (var item in this)
      {
        item.SetController(_controller);
        _controller.Settings[item.KeyName] = item;
        _controller.Settings.Load(item);
      }
    }

    public CecButtonConfigItem this[CecKeypress key]
    {
      get
      {
        foreach (var item in this)
        {
          if (item.Key.Keycode == key.Keycode)
            return item;
        }
        return null;
      }
    }

    private readonly ApplicationController _controller;
  }
}

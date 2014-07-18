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
using CecSharp;

namespace LibCECTray.controller.actions
{
  internal enum UpdateEventType
  {
    ProcessCompleted,
    StatusText,
    ProgressBar,
    TVVendorId,
    BaseDevicePhysicalAddress,
    BaseDevice,
    HDMIPort,
    PhysicalAddress,
    HasAVRDevice,
    AVRVendorId,
    Configuration,
    PollDevices,
    ExitApplication
  }

  class UpdateEvent : EventArgs
  {
    public UpdateEvent(UpdateEventType type)
    {
      Type = type;
    }

    public UpdateEvent(UpdateEventType type, bool value)
    {
      Type = type;
      BoolValue = value;
    }

    public UpdateEvent(UpdateEventType type, int value)
    {
      Type = type;
      IntValue = value;
    }

    public UpdateEvent(UpdateEventType type, string value)
    {
      Type = type;
      StringValue = value;
    }

    public UpdateEvent(LibCECConfiguration config)
    {
      Type = UpdateEventType.Configuration;
      ConfigValue = config;
    }

    public UpdateEventType Type;
    public bool BoolValue;
    public int IntValue = -1;
    public string StringValue = String.Empty;
    public LibCECConfiguration ConfigValue;
  }

  internal abstract class UpdateProcess
  {
    public void SendEvent(UpdateEventType type)
    {
      if (EventHandler != null)
        EventHandler(this, new UpdateEvent(type));
    }

    public void SendEvent(UpdateEventType type, bool value)
    {
      if (EventHandler != null)
        EventHandler(this, new UpdateEvent(type, value));
    }

    public void SendEvent(UpdateEventType type, int value)
    {
      if (EventHandler != null)
        EventHandler(this, new UpdateEvent(type, value));
    }

    public void SendEvent(UpdateEventType type, string value)
    {
      if (EventHandler != null)
        EventHandler(this, new UpdateEvent(type, value));
    }

    public void SendEvent(LibCECConfiguration config)
    {
      if (EventHandler != null)
        EventHandler(this, new UpdateEvent(config));
    }

    public void Run()
    {
      Process();
      SendEvent(UpdateEventType.ProcessCompleted, true);
    }

    public abstract void Process();
    public event EventHandler<UpdateEvent> EventHandler;
  }
}

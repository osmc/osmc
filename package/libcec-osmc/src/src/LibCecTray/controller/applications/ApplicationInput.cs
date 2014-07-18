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
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using CecSharp;
using LibCECTray.Properties;

namespace LibCECTray.controller.applications
{
  /// <summary>
  /// The type of the action that is executed
  /// </summary>
  enum ActionType
  {
    Generic = 0,
    CloseControllerApplication = 1,
    StartApplication = 2,
    SendKey = 3
  }

  /// <summary>
  /// Class that contains one or more actions that can be executed or sent to the application
  /// </summary>
  abstract class ApplicationAction
  {
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="controller">The controller instance of the application that this action is targeting</param>
    /// <param name="type">The type of the action that is executed</param>
    protected ApplicationAction(ApplicationController controller, ActionType type)
    {
      Controller = controller;
      ActionType = type;
    }

    /// <summary>
    /// The type of the action that is executed
    /// </summary>
    public ActionType ActionType { private set; get; }

    /// <summary>
    /// Execute the action
    /// </summary>
    /// <param name="windowHandle">The window of the application that is targeted</param>
    /// <returns>True when executed, false otherwise</returns>
    public abstract bool Transmit(IntPtr windowHandle);

    /// <summary>
    /// Serialisable string representation of this action
    /// </summary>
    /// <returns>The requested string</returns>
    public abstract string AsString();

    /// <summary>
    /// String representation of this action in human readable form
    /// </summary>
    /// <returns>The requested string</returns>
    public abstract string AsFriendlyString();

    /// <summary>
    /// Check whether this action is empty (not doing anything)
    /// </summary>
    /// <returns>True when empty, false otherwise</returns>
    public abstract bool Empty();

    /// <summary>
    /// Checks whether the given action can be appended to this one.
    /// </summary>
    /// <param name="value">The action to check</param>
    /// <returns>True when it can be appended, false otherwise</returns>
    public abstract bool CanAppend(ApplicationAction value);

    /// <summary>
    /// Append the given action to this action, and return the combined result
    /// </summary>
    /// <param name="value">The action to append to this one</param>
    /// <returns>The combined result</returns>
    public abstract ApplicationAction Append(ApplicationAction value);

    /// <summary>
    /// Remove item at the given index from this action
    /// </summary>
    /// <param name="index">The index of the item to remove</param>
    /// <returns>True when removed, false otherwise</returns>
    public abstract ApplicationAction RemoveKey(int index);

    /// <summary>
    /// The prefix to use for serialisation for this type
    /// </summary>
    protected string TypePrefix
    {
      get { return Enum.GetName(typeof(ActionType), ActionType); }
    }

    /// <summary>
    /// Get the parameter value from a string representation of an action of this type
    /// </summary>
    /// <param name="value">The value to get the parameter from</param>
    /// <returns>The parameter</returns>
    protected string GetParameterFromString(string value)
    {
      var trimmedItem = value.Trim();
      if (!trimmedItem.StartsWith(TypePrefix + "(") || !trimmedItem.EndsWith(")"))
        return string.Empty;

      return trimmedItem.Substring(TypePrefix.Length + 1, trimmedItem.Length - TypePrefix.Length - 2);
    }

    protected readonly ApplicationController Controller;
  }

  /// <summary>
  /// Closes LibCecTray
  /// </summary>
  internal class ApplicationActionCloseController : ApplicationAction
  {
    public ApplicationActionCloseController(ApplicationController controller)
      : base(controller, ActionType.CloseControllerApplication)
    {
    }

    public override bool Transmit(IntPtr windowHandle)
    {
      Application.Exit();
      return true;
    }

    public override string AsString()
    {
      return TypePrefix;
    }

    public override string AsFriendlyString()
    {
      return string.Format("[{0}]", Resources.action_type_close_controller_application);
    }

    public static bool HasDefaultValue(CecKeypress key)
    {
      return DefaultValue(key) != null;
    }

    public static ApplicationAction DefaultValue(CecKeypress key)
    {
      switch (key.Keycode)
      {
        case CecUserControlCode.Power:
        case CecUserControlCode.PowerOnFunction:
        case CecUserControlCode.PowerOffFunction:
        case CecUserControlCode.PowerToggleFunction:
          return new ApplicationActionCloseController(null);
      }
      return null;
    }

    public override bool Empty()
    {
      return false;
    }

    public override bool CanAppend(ApplicationAction value)
    {
      return false;
    }

    public override ApplicationAction Append(ApplicationAction value)
    {
      return this;
    }

    public override ApplicationAction RemoveKey(int index)
    {
      return null;
    }

    public static ApplicationAction FromString(ApplicationController controller, string value)
    {
      ApplicationActionCloseController retVal = new ApplicationActionCloseController(controller);
      return value.Trim().Equals(retVal.AsString()) ? retVal : null;
    }
  }

  /// <summary>
  /// Starts an application
  /// </summary>
  internal class ApplicationActionStart : ApplicationAction
  {
    public ApplicationActionStart(ApplicationController controller) :
      base(controller, ActionType.StartApplication)
    {
    }

    public override bool Transmit(IntPtr windowHandle)
    {
      return Controller.Start(false);
    }

    public override string AsString()
    {
      return TypePrefix;
    }

    public override string AsFriendlyString()
    {
      return string.Format("[{0}]", Resources.action_type_start_application);
    }

    public override bool Empty()
    {
      return false;
    }

    public override bool CanAppend(ApplicationAction value)
    {
      return false;
    }

    public override ApplicationAction Append(ApplicationAction value)
    {
      return this;
    }

    public override ApplicationAction RemoveKey(int index)
    {
      return null;
    }

    public static ApplicationAction FromString(ApplicationController controller, string value)
    {
      ApplicationActionStart retVal = new ApplicationActionStart(controller);
      return value.Trim().Equals(retVal.AsString()) ? retVal : null;
    }

    public static bool HasDefaultValue(CecKeypress key)
    {
      return DefaultValue(key) != null;
    }

    public static ApplicationAction DefaultValue(CecKeypress key)
    {
      return null;
    }
  }

  /// <summary>
  /// Sends one or more actions to an application
  /// </summary>
  internal class ApplicationInput : ApplicationAction
  {
    public ApplicationInput(ApplicationController controller) :
      base(controller, ActionType.Generic)
    {
    }

    public static string FriendlyActionName(ActionType type)
    {
      switch (type)
      {
        case ActionType.Generic:
          return Resources.action_type_generic;
        case ActionType.CloseControllerApplication:
          return Resources.action_type_close_controller_application;
        case ActionType.StartApplication:
          return Resources.action_type_start_application;
        case ActionType.SendKey:
          return Resources.action_type_sendkey;
        default:
          return type.ToString();
      }
    }

    public static ApplicationAction DefaultValue(ApplicationController controller, CecKeypress key)
    {
      return controller.HasDefaultValue(key) ? controller.DefaultValue(key) : new ApplicationInput(null);
    }

    public override bool Empty()
    {
      foreach (var item in _input)
        if (!item.Empty())
          return false;
      return true;
    }

    public override bool CanAppend(ApplicationAction value)
    {
      return true;
    }

    public override ApplicationAction Append(ApplicationAction value)
    {
      if (value.Empty())
        return this;

      var added = false;
      if (_input.Count > 0)
      {
        if (_input[_input.Count - 1].CanAppend(value))
        {
          _input[_input.Count - 1].Append(value);
          added = true;
        }
      }
      if (!added)
        _input.Add(value);

      return this;
    }

    public override bool Transmit(IntPtr windowHandle)
    {
      var retval = true;
      foreach (var input in _input)
      {
        retval &= input.Transmit(windowHandle);
      }
      return retval;
    }

    public override string AsString()
    {
      StringBuilder sb = new StringBuilder();
      foreach (var input in _input)
      {
        sb.AppendFormat("{0} ", input.AsString());
      }
      return sb.ToString().TrimEnd();
    }

    public override string AsFriendlyString()
    {
      StringBuilder sb = new StringBuilder();
      foreach (var input in _input)
        sb.AppendFormat("{0} ", input.AsFriendlyString());
      return sb.ToString().Trim();
    }

    public override ApplicationAction RemoveKey(int index)
    {
      var ptr = 0;
      for (var itemPtr = 0; itemPtr < _input.Count; itemPtr++)
      {
        var item = _input[itemPtr];
        var currentPtr = item.AsFriendlyString().Length;
        if (index <= ptr + currentPtr)
        {
          var newItem = item.RemoveKey(index - ptr);
          if (newItem == null || newItem.Empty())
            _input.Remove(item);
          else
            _input[itemPtr] = newItem;
          break;
        }
        ptr += currentPtr;
      }
      return this;
    }

    public ApplicationInput RemoveItem(int index)
    {
      return RemoveKey(index) as ApplicationInput;
    }

    public static ApplicationInput FromString(ApplicationController controller, string value)
    {
      ApplicationInput retVal = new ApplicationInput(controller);
      var split = value.Trim().Split(' ');
      foreach (var item in split)
      {
        var addAction = KeyInput.FromString(controller, item);

        if (addAction == null || addAction.Empty())
          addAction = ApplicationActionCloseController.FromString(controller, item);

        if (addAction == null || addAction.Empty())
          addAction = ApplicationActionStart.FromString(controller, item);

        if (addAction != null && !addAction.Empty())
          retVal.Append(addAction);
      }
      return retVal;
    }

    public ApplicationInput AddKey(WindowsAPI.VirtualKeyCode keyCode)
    {
      var key = new KeyInput(Controller, keyCode);
      if (!key.Empty())
        Append(key);
      return this;
    }

    public ApplicationInput AddAction(ActionType action)
    {
      switch (action)
      {
        case ActionType.CloseControllerApplication:
          Append(new ApplicationActionCloseController(Controller));
          break;
        case ActionType.StartApplication:
          Append(new ApplicationActionStart(Controller));
          break;
      }
      return this;
    }

    private readonly List<ApplicationAction> _input = new List<ApplicationAction>();
  }
}

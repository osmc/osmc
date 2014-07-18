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
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Reflection;
using LibCECTray.Properties;

namespace LibCECTray.controller.applications
{
  /// <summary>
  /// Windows API methods and types
  /// </summary>
  internal class WindowsAPI
  {
    #region Types
    public enum VirtualKeyCode : ushort
    {
      VK_LBUTTON = 0x01,
      VK_RBUTTON = 0x02,
      VK_CANCEL = 0x03,
      VK_MBUTTON = 0x04,
      VK_XBUTTON1 = 0x05,
      VK_XBUTTON2 = 0x06,
      VK_BACK = 0x08,
      VK_TAB = 0x09,
      VK_CLEAR = 0x0C,
      VK_RETURN = 0x0D,
      VK_SHIFT = 0x10,
      VK_CONTROL = 0x11,
      VK_MENU = 0x12,
      VK_PAUSE = 0x13,
      VK_CAPITAL = 0x14,
      VK_KANA = 0x15,
      VK_HANGEUL = 0x15,
      VK_HANGUL = 0x15,
      VK_JUNJA = 0x17,
      VK_FINAL = 0x18,
      VK_HANJA = 0x19,
      VK_KANJI = 0x19,
      VK_ESCAPE = 0x1B,
      VK_CONVERT = 0x1C,
      VK_NONCONVERT = 0x1D,
      VK_ACCEPT = 0x1E,
      VK_MODECHANGE = 0x1F,
      VK_SPACE = 0x20,
      VK_PRIOR = 0x21,
      VK_NEXT = 0x22,
      VK_END = 0x23,
      VK_HOME = 0x24,
      VK_LEFT = 0x25,
      VK_UP = 0x26,
      VK_RIGHT = 0x27,
      VK_DOWN = 0x28,
      VK_SELECT = 0x29,
      VK_PRINT = 0x2A,
      VK_EXECUTE = 0x2B,
      VK_SNAPSHOT = 0x2C,
      VK_INSERT = 0x2D,
      VK_DELETE = 0x2E,
      VK_HELP = 0x2F,
      VK_0 = 0x30,
      VK_1 = 0x31,
      VK_2 = 0x32,
      VK_3 = 0x33,
      VK_4 = 0x34,
      VK_5 = 0x35,
      VK_6 = 0x36,
      VK_7 = 0x37,
      VK_8 = 0x38,
      VK_9 = 0x39,
      VK_B = 0x42,
      VK_C = 0x43,
      VK_D = 0x44,
      VK_E = 0x45,
      VK_F = 0x46,
      VK_G = 0x47,
      VK_H = 0x48,
      VK_I = 0x49,
      VK_J = 0x4A,
      VK_K = 0x4B,
      VK_L = 0x4C,
      VK_M = 0x4D,
      VK_N = 0x4E,
      VK_O = 0x4F,
      VK_P = 0x50,
      VK_Q = 0x51,
      VK_R = 0x52,
      VK_S = 0x53,
      VK_T = 0x54,
      VK_U = 0x55,
      VK_V = 0x56,
      VK_W = 0x57,
      VK_X = 0x58,
      VK_Y = 0x59,
      VK_Z = 0x5A,
      VK_LWIN = 0x5B,
      VK_RWIN = 0x5C,
      VK_APPS = 0x5D,
      VK_SLEEP = 0x5F,
      VK_NUMPAD0 = 0x60,
      VK_NUMPAD1 = 0x61,
      VK_NUMPAD2 = 0x62,
      VK_NUMPAD3 = 0x63,
      VK_NUMPAD4 = 0x64,
      VK_NUMPAD5 = 0x65,
      VK_NUMPAD6 = 0x66,
      VK_NUMPAD7 = 0x67,
      VK_NUMPAD8 = 0x68,
      VK_NUMPAD9 = 0x69,
      VK_MULTIPLY = 0x6A,
      VK_ADD = 0x6B,
      VK_SEPARATOR = 0x6C,
      VK_SUBTRACT = 0x6D,
      VK_DECIMAL = 0x6E,
      VK_DIVIDE = 0x6F,
      VK_F1 = 0x70,
      VK_F2 = 0x71,
      VK_F3 = 0x72,
      VK_F4 = 0x73,
      VK_F5 = 0x74,
      VK_F6 = 0x75,
      VK_F7 = 0x76,
      VK_F8 = 0x77,
      VK_F9 = 0x78,
      VK_F10 = 0x79,
      VK_F11 = 0x7A,
      VK_F12 = 0x7B,
      VK_F13 = 0x7C,
      VK_F14 = 0x7D,
      VK_F15 = 0x7E,
      VK_F16 = 0x7F,
      VK_F17 = 0x80,
      VK_F18 = 0x81,
      VK_F19 = 0x82,
      VK_F20 = 0x83,
      VK_F21 = 0x84,
      VK_F22 = 0x85,
      VK_F23 = 0x86,
      VK_F24 = 0x87,
      VK_NUMLOCK = 0x90,
      VK_SCROLL = 0x91,
      VK_LSHIFT = 0xA0,
      VK_RSHIFT = 0xA1,
      VK_LCONTROL = 0xA2,
      VK_RCONTROL = 0xA3,
      VK_LMENU = 0xA4,
      VK_RMENU = 0xA5,
      VK_BROWSER_BACK = 0xA6,
      VK_BROWSER_FORWARD = 0xA7,
      VK_BROWSER_REFRESH = 0xA8,
      VK_BROWSER_STOP = 0xA9,
      VK_BROWSER_SEARCH = 0xAA,
      VK_BROWSER_FAVORITES = 0xAB,
      VK_BROWSER_HOME = 0xAC,
      VK_VOLUME_MUTE = 0xAD,
      VK_VOLUME_DOWN = 0xAE,
      VK_VOLUME_UP = 0xAF,
      VK_MEDIA_NEXT_TRACK = 0xB0,
      VK_MEDIA_PREV_TRACK = 0xB1,
      VK_MEDIA_STOP = 0xB2,
      VK_MEDIA_PLAY_PAUSE = 0xB3,
      VK_LAUNCH_MAIL = 0xB4,
      VK_LAUNCH_MEDIA_SELECT = 0xB5,
      VK_LAUNCH_APP1 = 0xB6,
      VK_LAUNCH_APP2 = 0xB7,
      VK_OEM_1 = 0xBA,
      VK_OEM_PLUS = 0xBB,
      VK_OEM_COMMA = 0xBC,
      VK_OEM_MINUS = 0xBD,
      VK_OEM_PERIOD = 0xBE,
      VK_OEM_2 = 0xBF,
      VK_OEM_3 = 0xC0,
      VK_OEM_4 = 0xDB,
      VK_OEM_5 = 0xDC,
      VK_OEM_6 = 0xDD,
      VK_OEM_7 = 0xDE,
      VK_OEM_8 = 0xDF,
      VK_OEM_102 = 0xE2,
      VK_PROCESSKEY = 0xE5,
      VK_PACKET = 0xE7,
      VK_ATTN = 0xF6,
      VK_CRSEL = 0xF7,
      VK_EXSEL = 0xF8,
      VK_EREOF = 0xF9,
      VK_PLAY = 0xFA,
      VK_ZOOM = 0xFB,
      VK_NONAME = 0xFC,
      VK_PA1 = 0xFD,
      VK_OEM_CLEAR = 0xFE,
    }

    public static string GetVirtualKeyName(VirtualKeyCode key)
    {
      var keyName = Enum.GetName(typeof(VirtualKeyCode), key).ToUpper();
      var friendlyName = Resources.ResourceManager.GetString(keyName, Resources.Culture);
      return friendlyName ?? keyName.ToLower().Substring(3).Replace('_', ' ');
    }

    public enum InputType : uint
    {
      Mouse = 0,
      Keyboard = 1,
      Hardware = 2
    }

    public enum KeyEvent : uint
    {
      ExtendedKey = 0x0001,
      KeyUp = 0x0002,
      Unicode = 0x0003,
      ScanCode = 0x0008
    }

    public enum XButton : uint
    {
      One = 0x0001,
      Two = 0x0002
    }

    public enum MouseEvent : uint
    {
      Move = 0x0001,
      LeftDown = 0x0002,
      LeftUp = 0x0004,
      RightDown = 0x0008,
      RightUp = 0x0010,
      MiddleDown = 0x0020,
      Middleup = 0x0040,
      XDown = 0x0080,
      XUp = 0x0100,
      Wheel = 0x0800,
      VirtualDesk = 0x4000,
      Absolute = 0x8000
    }

    public enum ShowType
    {
      ShowNormal = 1
    }

    #pragma warning disable 649
    public struct MouseInput
    {
      public Int32 X;
      public Int32 Y;
      public UInt32 MouseData;
      public UInt32 Flags;
      public UInt32 Time;
      public IntPtr ExtraInfo;
    }
    public struct KeyboardInput
    {
      public UInt16 KeyCode;
      public UInt16 Scan;
      public UInt32 Flags;
      public UInt32 Time;
      public IntPtr ExtraInfo;
    }
    public struct HardwareInput
    {
      public UInt32 Msg;
      public UInt16 ParamL;
      public UInt16 ParamH;
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct CombinedInput
    {
      [FieldOffset(0)]
      public MouseInput Mouse;
      [FieldOffset(0)]
      public KeyboardInput Keyboard;
      [FieldOffset(0)]
      public HardwareInput Hardware;
    }

    public struct Input
    {
      public InputType Type;
      public CombinedInput Data;
    }
    #pragma warning restore 649
    #endregion

    #region DllImports
    [DllImport("kernel32.dll")]
    public static extern uint GetCurrentThreadId();

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, [Out] byte[] lpBuffer,
                                                int dwSize, out int lpNumberOfBytesRead);

    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern bool AttachThreadInput(uint idAttach, uint idAttachTo, bool fAttach);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowEx(IntPtr hwndParent, IntPtr hwndChildAfter, string lpszClass,
                                             string lpszWindow);

    [DllImport("user32.Dll", EntryPoint = "PostMessageA")]
    public static extern bool PostMessage(IntPtr hWnd, uint msg, int wParam, int lParam);

    [DllImport("user32.dll")]
    public static extern byte VkKeyScan(char ch);

    [DllImport("user32.dll")]
    public static extern uint MapVirtualKey(uint uCode, uint uMapType);

    [DllImport("user32.dll")]
    public static extern IntPtr SetFocus(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool AllowSetForegroundWindow(int dwProcessId);

    [DllImport("user32.dll")]
    public static extern uint SendInput(uint numberOfInputs,
                                        [MarshalAs(UnmanagedType.LPArray, SizeConst = 1)] Input[] input, int structSize);

    [DllImport("user32.dll")]
    public static extern IntPtr GetMessageExtraInfo();

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
    public static extern IntPtr SendMessage(IntPtr hWnd, int msg, int wParam, int lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern bool ShowWindowAsync(IntPtr hWnd, int cmdShow);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    #endregion

    /// <summary>
    /// Forces a window to the foreground
    /// </summary>
    /// <param name="windowHandle">Window that becomes the foreground window</param>
    /// <returns>True when succeeded, false otherwise</returns>
    public static bool ForceForeground(IntPtr windowHandle)
    {
      // get current foreground
      var currentForeground = GetForegroundWindow();

      // window already foreground window
      if (currentForeground == windowHandle)
        return true;

      // get thread id
      uint temp;
      var windowThreadId = GetWindowThreadProcessId(windowHandle, out temp);

      // attach thread input
      if (currentForeground != IntPtr.Zero && !AttachThreadInput(GetCurrentThreadId(), windowThreadId, true))
        return false;

      // switch foreground
      SetForegroundWindow(windowHandle);
      while (GetForegroundWindow() != windowHandle){}

      // (re)attach input
      if (currentForeground != IntPtr.Zero)
        AttachThreadInput(GetCurrentThreadId(), windowThreadId, false);

      return (GetForegroundWindow() == windowHandle);
    }

    /// <summary>
    /// Makes an application the foreground window and send input to it
    /// </summary>
    /// <param name="windowHandle">The window to send the input to</param>
    /// <param name="numberOfInputs">Number of inputs in the input parameter</param>
    /// <param name="input">The input to send</param>
    /// <param name="structSize">The size of an input struct</param>
    /// <returns>True when sent false otherwise</returns>
    public static bool SendInputTo(IntPtr windowHandle, uint numberOfInputs, Input[] input, int structSize)
    {
      return ShowWindowAsync(windowHandle, (int)ShowType.ShowNormal) &&
             ForceForeground(windowHandle) &&
             SendInput(numberOfInputs, input, structSize) == 0;
    }

    /// <summary>
    /// Find a window handle given it's name
    /// </summary>
    /// <param name="name">The name of the window</param>
    /// <returns>The requested handle, or IntPtr.Zero when not found</returns>
    public static IntPtr FindWindow(string name)
    {
      foreach (var proc in Process.GetProcesses())
      {
        if (proc.MainWindowTitle == name)
          return proc.MainWindowHandle;
      }

      return IntPtr.Zero;
    }

    /// <summary>
    /// Check whether there's another instance of this program running, and return the process when found
    /// </summary>
    /// <returns>The running process, or null when not found</returns>
    public static Process RunningInstance()
    {
      var current = Process.GetCurrentProcess();
      foreach (var process in Process.GetProcessesByName(current.ProcessName))
      {
        if (process.Id != current.Id && Assembly.GetExecutingAssembly().Location.Replace("/", "\\") == current.MainModule.FileName)
          return process;
      }

      return null;
    }
  }
}

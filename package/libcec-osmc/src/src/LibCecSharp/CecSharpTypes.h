#pragma once
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

#include "../lib/platform/threads/mutex.h"
#include <vcclr.h>
#include <msclr/marshal.h>
#include "../../include/cec.h"
#include <vector>

#using <System.dll>

/// <summary>
/// LibCecSharp namespace
/// </summary>
/// <see cref="LibCecSharp" />
namespace CecSharp
{
  /// <summary>
  /// The device type. For client applications, libCEC only supports RecordingDevice, PlaybackDevice or Tuner.
  /// libCEC uses RecordingDevice by default.
  /// </summary>
  public enum class CecDeviceType
  {
    /// <summary>
    /// Television
    /// </summary>
    Tv              = 0,
    /// <summary>
    /// Recording device
    /// </summary>
    RecordingDevice = 1,
    /// <summary>
    /// Reserved / do not use
    /// </summary>
    Reserved        = 2,
    /// <summary>
    /// Tuner
    /// </summary>
    Tuner           = 3,
    /// <summary>
    /// Playback device
    /// </summary>
    PlaybackDevice  = 4,
    /// <summary>
    /// Audio system / AVR
    /// </summary>
    AudioSystem     = 5
  };

  /// <summary>
  /// Log level that can be used by the logging callback method to filter messages from libCEC.
  /// </summary>
  public enum class CecLogLevel
  {
    /// <summary>
    /// No logging
    /// </summary>
    None    = 0,
    /// <summary>
    /// libCEC encountered a serious problem, and couldn't complete an action.
    /// </summary>
    Error   = 1,
    /// <summary>
    /// libCEC warns that it encountered a problem, but recovered.
    /// </summary>
    Warning = 2,
    /// <summary>
    /// libCEC informs the client about a CEC state change.
    /// </summary>
    Notice  = 4,
    /// <summary>
    /// Raw CEC data traffic
    /// </summary>
    Traffic = 8,
    /// <summary>
    /// Debugging messages
    /// </summary>
    Debug   = 16,
    /// <summary>
    /// Display all messages
    /// </summary>
    All     = 31
  };

  /// <summary>
  /// A logical address on the CEC bus
  /// </summary>
  public enum class CecLogicalAddress
  {
    /// <summary>
    /// Not a valid logical address
    /// </summary>
    Unknown          = -1,
    /// <summary>
    /// Television
    /// </summary>
    Tv               = 0,
    /// <summary>
    /// Recording device 1
    /// </summary>
    RecordingDevice1 = 1,
    /// <summary>
    /// Recording device 2
    /// </summary>
    RecordingDevice2 = 2,
    /// <summary>
    /// Tuner 1
    /// </summary>
    Tuner1           = 3,
    /// <summary>
    /// Playback device 1
    /// </summary>
    PlaybackDevice1  = 4,
    /// <summary>
    /// Audio system / AVR
    /// </summary>
    AudioSystem      = 5,
    /// <summary>
    /// Tuner 2
    /// </summary>
    Tuner2           = 6,
    /// <summary>
    /// Tuner 3
    /// </summary>
    Tuner3           = 7,
    /// <summary>
    /// Playback device 2
    /// </summary>
    PlaybackDevice2  = 8,
    /// <summary>
    /// Recording device 3
    /// </summary>
    RecordingDevice3 = 9,
    /// <summary>
    /// Tuner 4
    /// </summary>
    Tuner4           = 10,
    /// <summary>
    /// Playback device 3
    /// </summary>
    PlaybackDevice3  = 11,
    /// <summary>
    /// Reserved address 1
    /// </summary>
    Reserved1        = 12,
    /// <summary>
    /// Reserved address 2
    /// </summary>
    Reserved2        = 13,
    /// <summary>
    /// Free to use
    /// </summary>
    FreeUse          = 14,
    /// <summary>
    /// Unregistered / new device
    /// </summary>
    Unregistered     = 15,
    /// <summary>
    /// Broadcast address
    /// </summary>
    Broadcast        = 15
  };

  /// <summary>
  /// The type of alert when libCEC calls the CecAlert callback
  /// </summary>
  public enum class CecAlert
  {
    /// <summary>
    /// The device needs servicing. This is set when the firmware can be upgraded, or when a problem with the firmware is detected.
    /// The latest firmware flash tool can be downloaded from http://packages.pulse-eight.net/
    /// </summary>
    ServiceDevice = 0,
    /// <summary>
    /// The connection to the adapter was lost, probably because the device got unplugged.
    /// </summary>
    ConnectionLost,
    /// <summary>
    /// No permission from the OS to access the adapter.
    /// </summary>
    PermissionError,
    /// <summary>
    /// The device is being used by another program.
    /// </summary>
    PortBusy,
    /// <summary>
    /// The physical address that is assigned to the adapter is already being used.
    /// </summary>
    PhysicalAddressError,
    /// <summary>
    /// The TV does not respond to polls.
    /// </summary>
    TVPollFailed
  };

  /// <summary>
  /// The type of parameter that is sent with the CecAlert callback
  /// </summary>
  public enum class CecParameterType
  {
    /// <summary>
    /// The parameter is a string
    /// </summary>
    ParameterTypeString = 1
  };

  /// <summary>
  /// A parameter for the CecAlert callback
  /// </summary>
  public ref class CecParameter
  {
  public:
    /// <summary>
    /// Create a new parameter
    /// </summary>
    /// <param name="type">The type of this parameter.</param>
    /// <param name="data">The value of this parameter.</param>
    CecParameter(CecParameterType type, System::String ^ data)
    {
      Type = type;
      Data = data;
    }

    /// <summary>
    /// The type of this parameter
    /// </summary>
    property CecParameterType Type;
    /// <summary>
    /// The value of this parameter
    /// </summary>
    property System::String ^ Data;
  };

  /// <summary>
  /// The power status of a CEC device
  /// </summary>
  public enum class CecPowerStatus
  {
    /// <summary>
    /// Powered on
    /// </summary>
    On                      = 0x00,
    /// <summary>
    /// In standby mode
    /// </summary>
    Standby                 = 0x01,
    /// <summary>
    /// In transition from standby to on
    /// </summary>
    InTransitionStandbyToOn = 0x02,
    /// <summary>
    /// In transition from on to standby
    /// </summary>
    InTransitionOnToStandby = 0x03,
    /// <summary>
    /// Unknown status
    /// </summary>
    Unknown                 = 0x99
  };

  /// <summary>
  /// The CEC version of a CEC device
  /// </summary>
  public enum class CecVersion
  {
    /// <summary>
    /// Unknown version
    /// </summary>
    Unknown = 0x00,
    /// <summary>
    /// Version 1.2
    /// </summary>
    V1_2    = 0x01,
    /// <summary>
    /// Version 1.2a
    /// </summary>
    V1_2A   = 0x02,
    /// <summary>
    /// Version 1.3
    /// </summary>
    V1_3    = 0x03,
    /// <summary>
    /// Version 1.3a
    /// </summary>
    V1_3A   = 0x04,
    /// <summary>
    /// Version 1.4
    /// </summary>
    V1_4    = 0x05
  };

  /// <summary>
  /// Parameter for OSD string display, that controls how to display the string
  /// </summary>
  public enum class CecDisplayControl
  {
    /// <summary>
    /// Display for the default time
    /// </summary>
    DisplayForDefaultTime = 0x00,
    /// <summary>
    /// Display until it is cleared by ClearPreviousMessage
    /// </summary>
    DisplayUntilCleared   = 0x40,
    /// <summary>
    /// Clear message displayed by DisplayUntilCleared
    /// </summary>
    ClearPreviousMessage  = 0x80,
    /// <summary>
    /// Reserved / do not use
    /// </summary>
    ReservedForFutureUse  = 0xC0
  };

  /// <summary>
  /// The menu state of a CEC device
  /// </summary>
  public enum class CecMenuState
  {
    /// <summary>
    /// Menu active
    /// </summary>
    Activated   = 0,
    /// <summary>
    /// Menu not active
    /// </summary>
    Deactivated = 1
  };

  /// <summary>
  /// Deck control mode for playback and recording devices
  /// </summary>
  public enum class CecDeckControlMode
  {
    /// <summary>
    /// Skip forward / wind
    /// </summary>
    SkipForwardWind   = 1,
    /// <summary>
    /// Skip reverse / rewind
    /// </summary>
    SkipReverseRewind = 2,
    /// <summary>
    /// Stop
    /// </summary>
    Stop              = 3,
    /// <summary>
    /// Eject
    /// </summary>
    Eject             = 4
  };

  /// <summary>
  /// Deck status for playback and recording devices
  /// </summary>
  public enum class CecDeckInfo
  {
    /// <summary>
    /// Playing
    /// </summary>
    Play               = 0x11,
    /// <summary>
    /// Recording
    /// </summary>
    Record             = 0x12,
    /// <summary>
    /// Reverse
    /// </summary>
    Reverse            = 0x13,
    /// <summary>
    /// Showing still frame
    /// </summary>
    Still              = 0x14,
    /// <summary>
    /// Playing slow
    /// </summary>
    Slow               = 0x15,
    /// <summary>
    /// Playing slow reverse
    /// </summary>
    SlowReverse        = 0x16,
    /// <summary>
    /// Fast forward
    /// </summary>
    FastForward        = 0x17,
    /// <summary>
    /// Fast reverse
    /// </summary>
    FastReverse        = 0x18,
    /// <summary>
    /// No media detected
    /// </summary>
    NoMedia            = 0x19,
    /// <summary>
    /// Stop / not playing
    /// </summary>
    Stop               = 0x1A,
    /// <summary>
    /// Skip forward / wind
    /// </summary>
    SkipForwardWind    = 0x1B,
    /// <summary>
    /// Skip reverse / rewind
    /// </summary>
    SkipReverseRewind  = 0x1C,
    /// <summary>
    /// Index search forward
    /// </summary>
    IndexSearchForward = 0x1D,
    /// <summary>
    /// Index search reverse
    /// </summary>
    IndexSearchReverse = 0x1E,
    /// <summary>
    /// Other / unknown status
    /// </summary>
    OtherStatus        = 0x1F
  };

  /// <summary>
  /// User control code, the key code when the user presses or releases a button on the remote.
  /// Used by SendKeypress() and the CecKey callback.
  /// </summary>
  public enum class CecUserControlCode
  {
    /// <summary>
    /// Select / OK
    /// </summary>
    Select                      = 0x00,
    /// <summary>
    /// Direction up
    /// </summary>
    Up                          = 0x01,
    /// <summary>
    /// Direction down
    /// </summary>
    Down                        = 0x02,
    /// <summary>
    /// Direction left
    /// </summary>
    Left                        = 0x03,
    /// <summary>
    /// Direction right
    /// </summary>
    Right                       = 0x04,
    /// <summary>
    /// Direction right + up
    /// </summary>
    RightUp                     = 0x05,
    /// <summary>
    /// Direction right + down
    /// </summary>
    RightDown                   = 0x06,
    /// <summary>
    /// Direction left + up
    /// </summary>
    LeftUp                      = 0x07,
    /// <summary>
    /// Direction left + down
    /// </summary>
    LeftDown                    = 0x08,
    /// <summary>
    /// Root menu
    /// </summary>
    RootMenu                    = 0x09,
    /// <summary>
    /// Setup menu
    /// </summary>
    SetupMenu                   = 0x0A,
    /// <summary>
    /// Contents menu
    /// </summary>
    ContentsMenu                = 0x0B,
    /// <summary>
    /// Favourite menu
    /// </summary>
    FavoriteMenu                = 0x0C,
    /// <summary>
    /// Exit / back
    /// </summary>
    Exit                        = 0x0D,
    /// <summary>
    /// Number 0
    /// </summary>
    Number0                     = 0x20,
    /// <summary>
    /// Number 1
    /// </summary>
    Number1                     = 0x21,
    /// <summary>
    /// Number 2
    /// </summary>
    Number2                     = 0x22,
    /// <summary>
    /// Number 3
    /// </summary>
    Number3                     = 0x23,
    /// <summary>
    /// Number 4
    /// </summary>
    Number4                     = 0x24,
    /// <summary>
    /// Number 5
    /// </summary>
    Number5                     = 0x25,
    /// <summary>
    /// Number 6
    /// </summary>
    Number6                     = 0x26,
    /// <summary>
    /// Number 7
    /// </summary>
    Number7                     = 0x27,
    /// <summary>
    /// Number 8
    /// </summary>
    Number8                     = 0x28,
    /// <summary>
    /// Number 9
    /// </summary>
    Number9                     = 0x29,
    /// <summary>
    /// .
    /// </summary>
    Dot                         = 0x2A,
    /// <summary>
    /// Enter input
    /// </summary>
    Enter                       = 0x2B,
    /// <summary>
    /// Clear input
    /// </summary>
    Clear                       = 0x2C,
    /// <summary>
    /// Next favourite
    /// </summary>
    NextFavorite                = 0x2F,
    /// <summary>
    /// Channel up
    /// </summary>
    ChannelUp                   = 0x30,
    /// <summary>
    /// Channel down
    /// </summary>
    ChannelDown                 = 0x31,
    /// <summary>
    /// Previous channel
    /// </summary>
    PreviousChannel             = 0x32,
    /// <summary>
    /// Select sound track
    /// </summary>
    SoundSelect                 = 0x33,
    /// <summary>
    /// Select input
    /// </summary>
    InputSelect                 = 0x34,
    /// <summary>
    /// Display information
    /// </summary>
    DisplayInformation          = 0x35,
    /// <summary>
    /// Show help
    /// </summary>
    Help                        = 0x36,
    /// <summary>
    /// Page up
    /// </summary>
    PageUp                      = 0x37,
    /// <summary>
    /// Page down
    /// </summary>
    PageDown                    = 0x38,
    /// <summary>
    /// Toggle powered on / standby
    /// </summary>
    Power                       = 0x40,
    /// <summary>
    /// Volume up
    /// </summary>
    VolumeUp                    = 0x41,
    /// <summary>
    /// Volume down
    /// </summary>
    VolumeDown                  = 0x42,
    /// <summary>
    /// Mute audio
    /// </summary>
    Mute                        = 0x43,
    /// <summary>
    /// Start playback
    /// </summary>
    Play                        = 0x44,
    /// <summary>
    /// Stop playback
    /// </summary>
    Stop                        = 0x45,
    /// <summary>
    /// Pause playback
    /// </summary>
    Pause                       = 0x46,
    /// <summary>
    /// Toggle recording
    /// </summary>
    Record                      = 0x47,
    /// <summary>
    /// Rewind
    /// </summary>
    Rewind                      = 0x48,
    /// <summary>
    /// Fast forward
    /// </summary>
    FastForward                 = 0x49,
    /// <summary>
    /// Eject media
    /// </summary>
    Eject                       = 0x4A,
    /// <summary>
    /// Forward
    /// </summary>
    Forward                     = 0x4B,
    /// <summary>
    /// Backward
    /// </summary>
    Backward                    = 0x4C,
    /// <summary>
    /// Stop recording
    /// </summary>
    StopRecord                  = 0x4D,
    /// <summary>
    /// Pause recording
    /// </summary>
    PauseRecord                 = 0x4E,
    /// <summary>
    /// Change angle
    /// </summary>
    Angle                       = 0x50,
    /// <summary>
    /// Toggle sub picture
    /// </summary>
    SubPicture                  = 0x51,
    /// <summary>
    /// Toggle video on demand
    /// </summary>
    VideoOnDemand               = 0x52,
    /// <summary>
    /// Toggle electronic program guide (EPG)
    /// </summary>
    ElectronicProgramGuide      = 0x53,
    /// <summary>
    /// Toggle timer programming
    /// </summary>
    TimerProgramming            = 0x54,
    /// <summary>
    /// Set initial configuration
    /// </summary>
    InitialConfiguration        = 0x55,
    /// <summary>
    /// Start playback function
    /// </summary>
    PlayFunction                = 0x60,
    /// <summary>
    /// Pause playback function
    /// </summary>
    PausePlayFunction           = 0x61,
    /// <summary>
    /// Toggle recording function
    /// </summary>
    RecordFunction              = 0x62,
    /// <summary>
    /// Pause recording function
    /// </summary>
    PauseRecordFunction         = 0x63,
    /// <summary>
    /// Stop playback function
    /// </summary>
    StopFunction                = 0x64,
    /// <summary>
    /// Mute audio function
    /// </summary>
    MuteFunction                = 0x65,
    /// <summary>
    /// Restore volume function
    /// </summary>
    RestoreVolumeFunction       = 0x66,
    /// <summary>
    /// Tune function
    /// </summary>
    TuneFunction                = 0x67,
    /// <summary>
    /// Select media function
    /// </summary>
    SelectMediaFunction         = 0x68,
    /// <summary>
    /// Select AV input function
    /// </summary>
    SelectAVInputFunction       = 0x69,
    /// <summary>
    /// Select audio input function
    /// </summary>
    SelectAudioInputFunction    = 0x6A,
    /// <summary>
    /// Toggle powered on / standby function
    /// </summary>
    PowerToggleFunction         = 0x6B,
    /// <summary>
    /// Power off function
    /// </summary>
    PowerOffFunction            = 0x6C,
    /// <summary>
    /// Power on function
    /// </summary>
    PowerOnFunction             = 0x6D,
    /// <summary>
    /// F1 / blue button
    /// </summary>
    F1Blue                      = 0x71,
    /// <summary>
    /// F2 / red button
    /// </summary>
    F2Red                       = 0X72,
    /// <summary>
    /// F3 / green button
    /// </summary>
    F3Green                     = 0x73,
    /// <summary>
    /// F4 / yellow button
    /// </summary>
    F4Yellow                    = 0x74,
    /// <summary>
    /// F5
    /// </summary>
    F5                          = 0x75,
    /// <summary>
    /// Data / teletext
    /// </summary>
    Data                        = 0x76,
    /// <summary>
    /// Max. valid key code for standard buttons
    /// </summary>
    Max                         = 0x76,
    /// <summary>
    /// Extra return button on Samsung remotes
    /// </summary>
    SamsungReturn               = 0x91,
    /// <summary>
    /// Unknown / invalid key code
    /// </summary>
    Unknown
  };

  /// <summary>
  /// Vendor IDs for CEC devices
  /// </summary>
  public enum class CecVendorId
  {
    Toshiba      = 0x000039,
    Samsung      = 0x0000F0,
    Denon        = 0x0005CD,
    Marantz      = 0x000678,
    Loewe        = 0x000982,
    Onkyo        = 0x0009B0,
    Medion       = 0x000CB8,
    Toshiba2     = 0x000CE7,
    PulseEight   = 0x001582,
    Akai         = 0x0020C7,
    AOC          = 0x002467,
    Panasonic    = 0x008045,
    Philips      = 0x00903E,
    Daewoo       = 0x009053,
    Yamaha       = 0x00A0DE,
    Grundig      = 0x00D0D5,
    Pioneer      = 0x00E036,
    LG           = 0x00E091,
    Sharp        = 0x08001F,
    Sony         = 0x080046,
    Broadcom     = 0x18C086,
    Vizio        = 0x6B746D,
    Benq         = 0x8065E9,
    HarmanKardon = 0x9C645E,
    Unknown      = 0
  };

  /// <summary>
  /// Audio status of audio system / AVR devices
  /// </summary>
  public enum class CecAudioStatus
  {
    /// <summary>
    /// Muted
    /// </summary>
    MuteStatusMask      = 0x80,
    /// <summary>
    /// Not muted, volume status mask
    /// </summary>
    VolumeStatusMask    = 0x7F,
    /// <summary>
    /// Minumum volume
    /// </summary>
    VolumeMin           = 0x00,
    /// <summary>
    /// Maximum volume
    /// </summary>
    VolumeMax           = 0x64,
    /// <summary>
    /// Unknown status
    /// </summary>
    VolumeStatusUnknown = 0x7F
  };

  /// <summary>
  /// CEC opcodes, as described in the HDMI CEC specification
  /// </summary>
  public enum class CecOpcode
  {
    /// <summary>
    /// Active source
    /// </summary>
    ActiveSource                  = 0x82,
    /// <summary>
    /// Image view on: power on display for image display
    /// </summary>
    ImageViewOn                   = 0x04,
    /// <summary>
    /// Text view on: power on display for text display
    /// </summary>
    TextViewOn                    = 0x0D,
    /// <summary>
    /// Device no longer is the active source
    /// </summary>
    InactiveSource                = 0x9D,
    /// <summary>
    /// Request which device has the active source status
    /// </summary>
    RequestActiveSource           = 0x85,
    /// <summary>
    /// Routing change for HDMI switches
    /// </summary>
    RoutingChange                 = 0x80,
    /// <summary>
    /// Routing information for HDMI switches
    /// </summary>
    RoutingInformation            = 0x81,
    /// <summary>
    /// Change the stream path to the given physical address
    /// </summary>
    SetStreamPath                 = 0x86,
    /// <summary>
    /// Inform that a device went into standby mode
    /// </summary>
    Standby                       = 0x36,
    /// <summary>
    /// Stop recording
    /// </summary>
    RecordOff                     = 0x0B,
    /// <summary>
    /// Start recording
    /// </summary>
    RecordOn                      = 0x09,
    /// <summary>
    /// Recording status information
    /// </summary>
    RecordStatus                  = 0x0A,
    /// <summary>
    /// Record current display
    /// </summary>
    RecordTvScreen                = 0x0F,
    /// <summary>
    /// Clear analogue timer
    /// </summary>
    ClearAnalogueTimer            = 0x33,
    /// <summary>
    /// Clear digital timer
    /// </summary>
    ClearDigitalTimer             = 0x99,
    /// <summary>
    /// Clear external timer
    /// </summary>
    ClearExternalTimer            = 0xA1,
    /// <summary>
    /// Set analogue timer
    /// </summary>
    SetAnalogueTimer              = 0x34,
    /// <summary>
    /// Set digital timer
    /// </summary>
    SetDigitalTimer               = 0x97,
    /// <summary>
    /// Set external timer
    /// </summary>
    SetExternalTimer              = 0xA2,
    /// <summary>
    /// Set program title of a timer
    /// </summary>
    SetTimerProgramTitle          = 0x67,
    /// <summary>
    /// Timer status cleared
    /// </summary>
    TimerClearedStatus            = 0x43,
    /// <summary>
    /// Timer status information
    /// </summary>
    TimerStatus                   = 0x35,
    /// <summary>
    /// CEC version used by a device
    /// </summary>
    CecVersion                    = 0x9E,
    /// <summary>
    /// Request CEC version of a device
    /// </summary>
    GetCecVersion                 = 0x9F,
    /// <summary>
    /// Request physical address of a device
    /// </summary>
    GivePhysicalAddress           = 0x83,
    /// <summary>
    /// Request language code of the menu language of a device
    /// 3 character ISO 639-2 country code. see http://http://www.loc.gov/standards/iso639-2/
    /// </summary>
    GetMenuLanguage               = 0x91,
    /// <summary>
    /// Report the physical address
    /// </summary>
    ReportPhysicalAddress         = 0x84,
    /// <summary>
    /// Report the language code of the menu language
    /// 3 character ISO 639-2 country code. see http://http://www.loc.gov/standards/iso639-2/
    /// </summary>
    SetMenuLanguage               = 0x32,
    /// <summary>
    /// Deck control for playback and recording devices
    /// </summary>
    DeckControl                   = 0x42,
    /// <summary>
    /// Deck status for playback and recording devices
    /// </summary>
    DeckStatus                    = 0x1B,
    /// <summary>
    /// Request deck status from playback and recording devices
    /// </summary>
    GiveDeckStatus                = 0x1A,
    /// <summary>
    /// Start playback on playback and recording devices
    /// </summary>
    Play                          = 0x41,
    /// <summary>
    /// Request tuner status
    /// </summary>
    GiveTunerDeviceStatus         = 0x08,
    /// <summary>
    /// Select analogue service on a tuner
    /// </summary>
    SelectAnalogueService         = 0x92,
    /// <summary>
    /// Select digital service on a tuner
    /// </summary>
    SelectDigtalService           = 0x93,
    /// <summary>
    /// Report tuner device status
    /// </summary>
    TunerDeviceStatus             = 0x07,
    /// <summary>
    /// Tuner step decrement
    /// </summary>
    TunerStepDecrement            = 0x06,
    /// <summary>
    /// Tuner step increment
    /// </summary>
    TunerStepIncrement            = 0x05,
    /// <summary>
    /// Report device vendor ID
    /// </summary>
    DeviceVendorId                = 0x87,
    /// <summary>
    /// Request device vendor ID
    /// </summary>
    GiveDeviceVendorId            = 0x8C,
    /// <summary>
    /// Vendor specific command
    /// </summary>
    VendorCommand                 = 0x89,
    /// <summary>
    /// Vendor specific command with vendor ID
    /// </summary>
    VendorCommandWithId           = 0xA0,
    /// <summary>
    /// Vendor specific remote button pressed
    /// </summary>
    VendorRemoteButtonDown        = 0x8A,
    /// <summary>
    /// Vendor specific remote button released
    /// </summary>
    VendorRemoteButtonUp          = 0x8B,
    /// <summary>
    /// Display / clear OSD string
    /// </summary>
    SetOsdString                  = 0x64,
    /// <summary>
    /// Request device OSD name
    /// </summary>
    GiveOsdName                   = 0x46,
    /// <summary>
    /// Report device OSD name
    /// </summary>
    SetOsdName                    = 0x47,
    /// <summary>
    /// Request device menu status
    /// </summary>
    MenuRequest                   = 0x8D,
    /// <summary>
    /// Report device menu status
    /// </summary>
    MenuStatus                    = 0x8E,
    /// <summary>
    /// Remote button pressed
    /// </summary>
    UserControlPressed            = 0x44,
    /// <summary>
    /// Remote button released
    /// </summary>
    UserControlRelease            = 0x45,
    /// <summary>
    /// Request device power status
    /// </summary>
    GiveDevicePowerStatus         = 0x8F,
    /// <summary>
    /// Report device power status
    /// </summary>
    ReportPowerStatus             = 0x90,
    /// <summary>
    /// Feature abort / unsupported command
    /// </summary>
    FeatureAbort                  = 0x00,
    /// <summary>
    /// Abort command
    /// </summary>
    Abort                         = 0xFF,
    /// <summary>
    /// Give audio status
    /// </summary>
    GiveAudioStatus               = 0x71,
    /// <summary>
    /// Give audiosystem mode
    /// </summary>
    GiveSystemAudioMode           = 0x7D,
    /// <summary>
    /// Report device audio status
    /// </summary>
    ReportAudioStatus             = 0x7A,
    /// <summary>
    /// Set audiosystem mode
    /// </summary>
    SetSystemAudioMode            = 0x72,
    /// <summary>
    /// Request audiosystem mode
    /// </summary>
    SystemAudioModeRequest        = 0x70,
    /// <summary>
    /// Report audiosystem mode
    /// </summary>
    SystemAudioModeStatus         = 0x7E,
    /// <summary>
    /// Set audio bitrate
    /// </summary>
    SetAudioRate                  = 0x9A,
    /// <summary>
    /// When this opcode is set, no opcode will be sent to the device / poll message
    /// This is one of the reserved numbers
    /// </summary>
    None                          = 0xFD
  };

  /// <summary>
  /// Audiosystem status
  /// </summary>
  public enum class CecSystemAudioStatus
  {
    /// <summary>
    /// Turned off
    /// </summary>
    Off = 0,
    /// <summary>
    /// Turned on
    /// </summary>
    On  = 1
  };

  /// <summary>
  /// libCEC client application version
  /// </summary>
  public enum class CecClientVersion
  {
    /// <summary>
    /// before v1.5.0
    /// </summary>
    VersionPre1_5 = 0,
    /// <summary>
    /// v1.5.0
    /// </summary>
    Version1_5_0  = 0x1500,
    /// <summary>
    /// v1.5.1
    /// </summary>
    Version1_5_1  = 0x1501,
    /// <summary>
    /// v1.5.2
    /// </summary>
    Version1_5_2  = 0x1502,
    /// <summary>
    /// v1.5.3
    /// </summary>
    Version1_5_3  = 0x1503,
    /// <summary>
    /// v1.6.0
    /// </summary>
    Version1_6_0  = 0x1600,
    /// <summary>
    /// v1.6.1
    /// </summary>
    Version1_6_1  = 0x1601,
    /// <summary>
    /// v1.6.2
    /// </summary>
    Version1_6_2  = 0x1602,
    /// <summary>
    /// v1.6.3
    /// </summary>
    Version1_6_3  = 0x1603,
    /// <summary>
    /// v1.7.0
    /// </summary>
    Version1_7_0  = 0x1700,
    /// <summary>
    /// v1.7.1
    /// </summary>
    Version1_7_1  = 0x1701,
    /// <summary>
    /// v1.7.2
    /// </summary>
    Version1_7_2  = 0x1702,
    /// <summary>
    /// v1.8.0
    /// </summary>
    Version1_8_0  = 0x1800,
    /// <summary>
    /// v1.8.1
    /// </summary>
    Version1_8_1  = 0x1801,
    /// <summary>
    /// v1.8.2
    /// </summary>
    Version1_8_2  = 0x1802,
    /// <summary>
    /// v1.9.0
    /// </summary>
    Version1_9_0  = 0x1900,
    /// <summary>
    /// v2.0.0-pre
    /// </summary>
    Version1_99_0  = 0x1990,
    /// <summary>
    /// v2.0.0
    /// </summary>
    Version2_0_0   = 0x2000,
    /// <summary>
    /// v2.0.1
    /// </summary>
    Version2_0_1   = 0x2001,
    /// <summary>
    /// v2.0.2
    /// </summary>
    Version2_0_2   = 0x2002,
    /// <summary>
    /// v2.0.3
    /// </summary>
    Version2_0_3   = 0x2003,
    /// <summary>
    /// v2.0.4
    /// </summary>
    Version2_0_4   = 0x2004,
    /// <summary>
    /// v2.0.5
    /// </summary>
    Version2_0_5   = 0x2005,
    /// <summary>
    /// v2.1.0
    /// </summary>
    Version2_1_0   = 0x2100,
    /// <summary>
    /// v2.1.1
    /// </summary>
    Version2_1_1   = 0x2101,
    /// <summary>
    /// v2.1.2
    /// </summary>
    Version2_1_2   = 0x2102,
    /// <summary>
    /// v2.1.3
    /// </summary>
    Version2_1_3   = 0x2103,
    /// <summary>
    /// v2.1.4
    /// </summary>
    Version2_1_4   = 0x2104,
	/// <summary>
    /// The current version
    /// </summary>
    CurrentVersion = 0x2104
  };

  /// <summary>
  /// libCEC version
  /// </summary>
  public enum class CecServerVersion
  {
    /// <summary>
    /// before v1.5.0
    /// </summary>
    VersionPre1_5 = 0,
    /// <summary>
    /// v1.5.0
    /// </summary>
    Version1_5_0  = 0x1500,
    /// <summary>
    /// v1.5.1
    /// </summary>
    Version1_5_1  = 0x1501,
    /// <summary>
    /// v1.5.2
    /// </summary>
    Version1_5_2  = 0x1502,
    /// <summary>
    /// v1.5.3
    /// </summary>
    Version1_5_3  = 0x1503,
    /// <summary>
    /// v1.6.0
    /// </summary>
    Version1_6_0  = 0x1600,
    /// <summary>
    /// v1.6.1
    /// </summary>
    Version1_6_1  = 0x1601,
    /// <summary>
    /// v1.6.2
    /// </summary>
    Version1_6_2  = 0x1602,
    /// <summary>
    /// v1.6.3
    /// </summary>
    Version1_6_3  = 0x1603,
    /// <summary>
    /// v1.7.0
    /// </summary>
    Version1_7_0  = 0x1700,
    /// <summary>
    /// v1.7.1
    /// </summary>
    Version1_7_1  = 0x1701,
    /// <summary>
    /// v1.7.2
    /// </summary>
    Version1_7_2  = 0x1702,
    /// <summary>
    /// v1.8.0
    /// </summary>
    Version1_8_0  = 0x1800,
    /// <summary>
    /// v1.8.1
    /// </summary>
    Version1_8_1  = 0x1801,
    /// <summary>
    /// v1.8.2
    /// </summary>
    Version1_8_2  = 0x1802,
    /// <summary>
    /// v1.9.0
    /// </summary>
    Version1_9_0  = 0x1900,
    /// <summary>
    /// v2.0.0-pre
    /// </summary>
    Version1_99_0  = 0x1990,
    /// <summary>
    /// v2.0.0
    /// </summary>
    Version2_0_0   = 0x2000,
    /// <summary>
    /// v2.0.1
    /// </summary>
    Version2_0_1   = 0x2001,
    /// <summary>
    /// v2.0.2
    /// </summary>
    Version2_0_2   = 0x2002,
    /// <summary>
    /// v2.0.3
    /// </summary>
    Version2_0_3   = 0x2003,
    /// <summary>
    /// v2.0.4
    /// </summary>
    Version2_0_4   = 0x2004,
    /// <summary>
    /// v2.0.5
    /// </summary>
    Version2_0_5   = 0x2005,
    /// <summary>
    /// v2.1.0
    /// </summary>
    Version2_1_0   = 0x2100,
    /// <summary>
    /// v2.1.1
    /// </summary>
    Version2_1_1   = 0x2101,
    /// <summary>
    /// v2.1.2
    /// </summary>
    Version2_1_2   = 0x2102,
    /// <summary>
    /// v2.1.3
    /// </summary>
    Version2_1_3   = 0x2103,
	/// <summary>
    /// v2.1.4
    /// </summary>
    Version2_1_4   = 0x2104,
    /// <summary>
    /// The current version
    /// </summary>
    CurrentVersion = 0x2104
  };

  /// <summary>
  /// Type of adapter to which libCEC is connected
  /// </summary>
  public enum class CecAdapterType
  {
    /// <summary>
    /// Unknown adapter type
    /// </summary>
    Unknown                 = 0,
    /// <summary>
    /// Pulse-Eight USB-CEC adapter
    /// </summary>
    PulseEightExternal      = 0x1,
    /// <summary>
    /// Pulse-Eight CEC daughterboard
    /// </summary>
    PulseEightDaughterboard = 0x2,
    /// <summary>
    /// Raspberry Pi
    /// </summary>
    RaspberryPi             = 0x100,
    /// <summary>
    /// TDA995x
    /// </summary>
    TDA995x                 = 0x200
  };

  /// <summary>
  /// Descriptor of a CEC adapter, returned when scanning for adapters that are connected to the system
  /// </summary>
  public ref class CecAdapter
  {
  public:
    /// <summary>
    /// Create a new CEC adapter descriptor
    /// </summary>
    /// <param name="path"> The path descriptor for this CEC adapter</param>
    /// <param name="comPort">The COM port of this CEC adapter</param>
    CecAdapter(System::String ^ path, System::String ^ comPort)
    {
      Path = path;
      ComPort = comPort;
    }

    /// <summary>
    /// The path descriptor for this CEC adapter
    /// </summary>
    property System::String ^ Path;

    /// <summary>
    /// The COM port of this CEC adapter
    /// </summary>
    property System::String ^ ComPort;
  };

  /// <summary>
  /// A list of CEC device types
  /// </summary>
  public ref class CecDeviceTypeList
  {
  public:
    /// <summary>
    /// Create a new empty list of CEC device types
    /// </summary>
    CecDeviceTypeList(void)
    {
      Types = gcnew array<CecDeviceType>(5);
      for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
        Types[iPtr] = CecDeviceType::Reserved;
    }

    /// <summary>
    /// The array with CecDeviceType instances in this list.
    /// </summary>
    property array<CecDeviceType> ^ Types;
  };

  /// <summary>
  /// A list of logical addresses
  /// </summary>
  public ref class CecLogicalAddresses
  {
  public:
    /// <summary>
    /// Create a new empty list of logical addresses
    /// </summary>
    CecLogicalAddresses(void)
    {
      Addresses = gcnew array<CecLogicalAddress>(16);
      Clear();
    }

    /// <summary>
    /// Clears this list
    /// </summary>
    void Clear(void)
    {
      Primary = CecLogicalAddress::Unknown;
      for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
        Addresses[iPtr] = CecLogicalAddress::Unknown;
    }

    /// <summary>
    /// Checks whether a logical address is set in this list.
    /// </summary>
    /// <param name="address">The address to check.</param>
    /// <returns>True when set, false otherwise</returns>
    bool IsSet(CecLogicalAddress address)
    {
      return Addresses[(unsigned int)address] != CecLogicalAddress::Unknown;
    }

    /// <summary>
    /// Add a logical address to this list (if it's not set already)
    /// </summary>
    /// <param name="address">The address to add.</param>
    void Set(CecLogicalAddress address)
    {
      Addresses[(unsigned int)address] = address;
      if (Primary == CecLogicalAddress::Unknown)
        Primary = address;
    }

    /// <summary>
    /// The primary (first) address in this list
    /// </summary>
    property CecLogicalAddress          Primary;

    /// <summary>
    /// The list of addresses
    /// </summary>
    property array<CecLogicalAddress> ^ Addresses;
  };


  /// <summary>
  /// Byte array used for CEC command parameters
  /// </summary>
  public ref class CecDatapacket
  {
  public:
    /// <summary>
    /// Create a new byte array with maximum size 100
    /// </summary>
    CecDatapacket(void)
    {
      Data = gcnew array<uint8_t>(100);
      Size = 0;
    }

    /// <summary>
    /// Adds a byte to this byte array
    /// </summary>
    /// <param name="data">The byte to add.</param>
    void PushBack(uint8_t data)
    {
      if (Size < 100)
      {
        Data[Size] = data;
        Size++;
      }
    }

    /// <summary>
    /// Array data
    /// </summary>
    property array<uint8_t> ^ Data;

    /// <summary>
    /// Current data size
    /// </summary>
    property uint8_t          Size;
  };

  /// <summary>
  /// A CEC command that is received or transmitted over the CEC bus
  /// </summary>
  public ref class CecCommand
  {
  public:
    /// <summary>
    /// Create a new CEC command instance
    /// </summary>
    /// <param name="initiator">The initiator of the command</param>
    /// <param name="destination">The receiver of the command</param>
    /// <param name="ack">True when the ack bit is set, false otherwise</param>
    /// <param name="eom">True when the eom bit is set, false otherwise</param>
    /// <param name="opcode">The CEC opcode of this command</param>
    /// <param name="transmitTimeout">The timeout to use when transmitting a command</param>
    CecCommand(CecLogicalAddress initiator, CecLogicalAddress destination, bool ack, bool eom, CecOpcode opcode, int32_t transmitTimeout)
    {
      Initiator       = initiator;
      Destination     = destination;
      Ack             = ack;
      Eom             = eom;
      Opcode          = opcode;
      OpcodeSet       = true;
      TransmitTimeout = transmitTimeout;
      Parameters      = gcnew CecDatapacket;
      Empty           = false;
    }

    /// <summary>
    /// Create a new empty CEC command instance
    /// </summary>
    CecCommand(void)
    {
      Initiator       = CecLogicalAddress::Unknown;
      Destination     = CecLogicalAddress::Unknown;
      Ack             = false;
      Eom             = false;
      Opcode          = CecOpcode::None;
      OpcodeSet       = false;
      TransmitTimeout = 0;
      Parameters      = gcnew CecDatapacket;
      Empty           = true;
    }

    /// <summary>
    /// Pushes a byte of data to this CEC command
    /// </summary>
    /// <param name="data">The byte to add</param>
    void PushBack(uint8_t data)
    {
      if (Initiator == CecLogicalAddress::Unknown && Destination == CecLogicalAddress::Unknown)
      {
        Initiator   = (CecLogicalAddress) (data >> 4);
        Destination = (CecLogicalAddress) (data & 0xF);
      }
      else if (!OpcodeSet)
      {
        OpcodeSet = true;
        Opcode    = (CecOpcode)data;
      }
      else
      {
        Parameters->PushBack(data);
      }
    }

    /// <summary>
    /// True when this command is empty, false otherwise.
    /// </summary>
    property bool               Empty;
    /// <summary>
    /// The initiator of the command
    /// </summary>
    property CecLogicalAddress  Initiator;
    /// <summary>
    /// The destination of the command
    /// </summary>
    property CecLogicalAddress  Destination;
    /// <summary>
    /// True when the ack bit is set, false otherwise
    /// </summary>
    property bool               Ack;
    /// <summary>
    /// True when the eom bit is set, false otherwise
    /// </summary>
    property bool               Eom;
    /// <summary>
    /// The CEC opcode of the command
    /// </summary>
    property CecOpcode          Opcode;
    /// <summary>
    /// The parameters of this command
    /// </summary>
    property CecDatapacket ^    Parameters;
    /// <summary>
    /// True when an opcode is set, false otherwise (poll message)
    /// </summary>
    property bool               OpcodeSet;
    /// <summary>
    /// The timeout to use when transmitting a command
    /// </summary>
    property int32_t            TransmitTimeout;
  };

  /// <summary>
  /// A key press that was received
  /// </summary>
  public ref class CecKeypress
  {
  public:
    /// <summary>
    /// Create a new key press instance
    /// </summary>
    /// <param name="keycode">The key code of this key press</param>
    /// <param name="duration">The duration of this key press in milliseconds</param>
    CecKeypress(CecUserControlCode keycode, unsigned int duration)
    {
      Keycode  = keycode;
      Duration = duration;
      Empty    = false;
    }

    /// <summary>
    /// Create a new empty key press instance
    /// </summary>
    CecKeypress(void)
    {
      Keycode  = CecUserControlCode::Unknown;
      Duration = 0;
      Empty    = true;
    }

    /// <summary>
    /// True when empty, false otherwise
    /// </summary>
    property bool               Empty;
    /// <summary>
    /// The key code of this key press
    /// </summary>
    property CecUserControlCode Keycode;
    /// <summary>
    /// The duration of this key press in milliseconds
    /// </summary>
    property unsigned int       Duration;
  };

  /// <summary>
  /// A log message that libCEC generated
  /// </summary>
  public ref class CecLogMessage
  {
  public:
    /// <summary>
    /// Create a new log message
    /// </summary>
    /// <param name="message">The actual message</param>
    /// <param name="level">The log level, so the application can choose what type information to display</param>
    /// <param name="time">The timestamp of this message, in milliseconds after connecting</param>
    CecLogMessage(System::String ^ message, CecLogLevel level, int64_t time)
    {
      Message = message;
      Level   = level;
      Time    = time;
      Empty   = false;
    }

    /// <summary>
    /// Create a new empty log message
    /// </summary>
    CecLogMessage(void)
    {
      Message = "";
      Level   = CecLogLevel::None;
      Time    = 0;
      Empty   = true;
    }

    /// <summary>
    /// True when empty, false otherwise.
    /// </summary>
    property bool            Empty;
    /// <summary>
    /// The actual message
    /// </summary>
    property System::String ^Message;
    /// <summary>
    /// The log level, so the application can choose what type information to display
    /// </summary>
    property CecLogLevel     Level;
    /// <summary>
    /// The timestamp of this message, in milliseconds after connecting
    /// </summary>
    property int64_t         Time;
  };

  ref class CecCallbackMethods; //forward declaration

  /// <summary>
  /// The configuration that libCEC uses.
  /// </summary>
  public ref class LibCECConfiguration
  {
  public:
    /// <summary>
    /// Create a new configuration instance with default settings.
    /// </summary>
    LibCECConfiguration(void)
    {
      DeviceName          = "";
      DeviceTypes         = gcnew CecDeviceTypeList();
      AutodetectAddress  = true;
      PhysicalAddress     = CEC_DEFAULT_PHYSICAL_ADDRESS;
      BaseDevice          = (CecLogicalAddress)CEC_DEFAULT_BASE_DEVICE;
      HDMIPort            = CEC_DEFAULT_HDMI_PORT;
      ClientVersion       = CecClientVersion::CurrentVersion;
      ServerVersion       = CecServerVersion::CurrentVersion;
      TvVendor            = CecVendorId::Unknown;

      GetSettingsFromROM  = false;
      UseTVMenuLanguage   = CEC_DEFAULT_SETTING_USE_TV_MENU_LANGUAGE == 1;
      ActivateSource      = CEC_DEFAULT_SETTING_ACTIVATE_SOURCE == 1;

      WakeDevices         = gcnew CecLogicalAddresses();
      if (CEC_DEFAULT_SETTING_ACTIVATE_SOURCE == 1)
        WakeDevices->Set(CecLogicalAddress::Tv);

      PowerOffDevices     = gcnew CecLogicalAddresses();
      if (CEC_DEFAULT_SETTING_POWER_OFF_SHUTDOWN == 1)
        PowerOffDevices->Set(CecLogicalAddress::Broadcast);

      PowerOffScreensaver = CEC_DEFAULT_SETTING_POWER_OFF_SCREENSAVER == 1;
      PowerOffOnStandby   = CEC_DEFAULT_SETTING_POWER_OFF_ON_STANDBY == 1;

      SendInactiveSource  = CEC_DEFAULT_SETTING_SEND_INACTIVE_SOURCE == 1;
      LogicalAddresses    = gcnew CecLogicalAddresses();
      FirmwareVersion     = 1;
      PowerOffDevicesOnStandby = CEC_DEFAULT_SETTING_POWER_OFF_DEVICES_STANDBY == 1;
      ShutdownOnStandby   = CEC_DEFAULT_SETTING_SHUTDOWN_ON_STANDBY == 1;
      DeviceLanguage      = "";
      FirmwareBuildDate   = gcnew System::DateTime(1970,1,1,0,0,0,0);
      CECVersion          = (CecVersion)CEC_DEFAULT_SETTING_CEC_VERSION;
      AdapterType         = CecAdapterType::Unknown;
    }

    /// <summary>
    /// Change the callback method pointers in this configuration instance.
    /// </summary>
    /// <param name="callbacks">The new callbacks</param>
    void SetCallbacks(CecCallbackMethods ^callbacks)
    {
      Callbacks = callbacks;
    }

    /// <summary>
    /// Update this configuration with data received from libCEC
    /// </summary>
    /// <param name="config">The configuration that was received from libCEC</param>
    void Update(const CEC::libcec_configuration &config)
    {
      DeviceName = gcnew System::String(config.strDeviceName);

      for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
        DeviceTypes->Types[iPtr] = (CecDeviceType)config.deviceTypes.types[iPtr];

      AutodetectAddress  = config.bAutodetectAddress == 1;
      PhysicalAddress    = config.iPhysicalAddress;
      BaseDevice         = (CecLogicalAddress)config.baseDevice;
      HDMIPort           = config.iHDMIPort;
      ClientVersion      = (CecClientVersion)config.clientVersion;
      ServerVersion      = (CecServerVersion)config.serverVersion;
      TvVendor           = (CecVendorId)config.tvVendor;

      // player specific settings
      GetSettingsFromROM = config.bGetSettingsFromROM == 1;
      UseTVMenuLanguage = config.bUseTVMenuLanguage == 1;
      ActivateSource = config.bActivateSource == 1;

      WakeDevices->Clear();
      for (uint8_t iPtr = 0; iPtr <= 16; iPtr++)
        if (config.wakeDevices[iPtr])
          WakeDevices->Set((CecLogicalAddress)iPtr);

      PowerOffDevices->Clear();
      for (uint8_t iPtr = 0; iPtr <= 16; iPtr++)
        if (config.powerOffDevices[iPtr])
          PowerOffDevices->Set((CecLogicalAddress)iPtr);

      PowerOffScreensaver = config.bPowerOffScreensaver == 1;
      PowerOffOnStandby = config.bPowerOffOnStandby == 1;

      if (ServerVersion >= CecServerVersion::Version1_5_1)
        SendInactiveSource = config.bSendInactiveSource == 1;

      if (ServerVersion >= CecServerVersion::Version1_5_3)
      {
        LogicalAddresses->Clear();
        for (uint8_t iPtr = 0; iPtr <= 16; iPtr++)
          if (config.logicalAddresses[iPtr])
            LogicalAddresses->Set((CecLogicalAddress)iPtr);
      }

      if (ServerVersion >= CecServerVersion::Version1_6_0)
      {
        FirmwareVersion          = config.iFirmwareVersion;
        PowerOffDevicesOnStandby = config.bPowerOffDevicesOnStandby == 1;
        ShutdownOnStandby        = config.bShutdownOnStandby == 1;
      }

      if (ServerVersion >= CecServerVersion::Version1_6_2)
      {
        DeviceLanguage = gcnew System::String(config.strDeviceLanguage);
        FirmwareBuildDate = gcnew System::DateTime(1970,1,1,0,0,0,0);
        FirmwareBuildDate = FirmwareBuildDate->AddSeconds(config.iFirmwareBuildDate);
      }

      if (ServerVersion >= CecServerVersion::Version1_6_3)
        MonitorOnlyClient = config.bMonitorOnly == 1;

      if (ServerVersion >= CecServerVersion::Version1_8_0)
        CECVersion = (CecVersion)config.cecVersion;

      if (ServerVersion >= CecServerVersion::Version1_8_2)
        AdapterType = (CecAdapterType)config.adapterType;

      if (ServerVersion >= CecServerVersion::Version2_1_0)
        PowerOnScreensaver = config.bPowerOnScreensaver == 1;
    }

    /// <summary>
    /// The device name to use on the CEC bus
    /// </summary>
    property System::String ^     DeviceName;

    /// <summary>
    /// The device type(s) to use on the CEC bus for libCEC
    /// </summary>
    property CecDeviceTypeList ^  DeviceTypes;

    /// <summary>
    /// (read only) set to true by libCEC when the physical address was autodetected
    /// </summary>
    property bool                 AutodetectAddress;

    /// <summary>
    /// The physical address of the CEC adapter
    /// </summary>
    property uint16_t             PhysicalAddress;

    /// <summary>
    /// The logical address of the device to which the adapter is connected. Only used when PhysicalAddress = 0 or when the adapter doesn't support autodetection
    /// </summary>
    property CecLogicalAddress    BaseDevice;

    /// <summary>
    /// The HDMI port to which the adapter is connected. Only used when iPhysicalAddress = 0 or when the adapter doesn't support autodetection
    /// </summary>
    property uint8_t              HDMIPort;

    /// <summary>
    /// The client API version to use
    /// </summary>
    property CecClientVersion     ClientVersion;

    /// <summary>
    /// The version of libCEC
    /// </summary>
    property CecServerVersion     ServerVersion;

    /// <summary>
    /// Override the vendor ID of the TV. Leave this untouched to autodetect
    /// </summary>
    property CecVendorId          TvVendor;

    /// <summary>
    /// True to read the settings from the EEPROM, which possibly override the settings passed here
    /// </summary>
    property bool                 GetSettingsFromROM;

    /// <summary>
    /// Use the language setting of the TV in the client application. Must be implemented by the client application.
    /// 3 character ISO 639-2 country code. see http://http://www.loc.gov/standards/iso639-2/
    /// </summary>
    property bool                 UseTVMenuLanguage;

    /// <summary>
    /// Make libCEC the active source when starting the client application
    /// </summary>
    property bool                 ActivateSource;

    /// <summary>
    /// List of devices to wake when initialising libCEC or when calling PowerOnDevices() without any parameter.
    /// </summary>
    property CecLogicalAddresses ^WakeDevices;

    /// <summary>
    /// List of devices to power off when calling StandbyDevices() without any parameter.
    /// </summary>
    property CecLogicalAddresses ^PowerOffDevices;

    /// <summary>
    /// Send standby commands when the client application activates the screensaver. Must be implemented by the client application.
    /// </summary>
    property bool                 PowerOffScreensaver;

    /// <summary>
    /// Power off the PC when the TV powers off. Must be implemented by the client application.
    /// </summary>
    property bool                 PowerOffOnStandby;

    /// <summary>
    /// Send an inactive source message when exiting the client application.
    /// </summary>
    property bool                 SendInactiveSource;

    /// <summary>
    /// The list of logical addresses that libCEC is using
    /// </summary>
    property CecLogicalAddresses ^LogicalAddresses;

    /// <summary>
    /// The firmware version of the adapter to which libCEC is connected
    /// </summary>
    property uint16_t             FirmwareVersion;

    /// <summary>
    /// Send standby commands when the client application activates standby mode (S3). Must be implemented by the client application.
    /// </summary>
    property bool                 PowerOffDevicesOnStandby;

    /// <summary>
    /// Shutdown this PC when the TV is switched off. only used when PowerOffOnStandby = false
    /// </summary>
    property bool                 ShutdownOnStandby;

    /// <summary>
    /// True to start a monitor-only client, false to start a standard client.
    /// </summary>
    property bool                 MonitorOnlyClient;

    /// <summary>
    /// The language code of the menu language that libCEC reports to other devices.
    /// 3 character ISO 639-2 country code. see http://http://www.loc.gov/standards/iso639-2/
    /// </summary>
    property System::String ^     DeviceLanguage;

    /// <summary>
    /// The callback methods to use.
    /// </summary>
    property CecCallbackMethods ^ Callbacks;

    /// <summary>
    /// The build date of the firmware.
    /// </summary>
    property System::DateTime ^   FirmwareBuildDate;

    /// <summary>
    /// The CEC version that libCEC uses.
    /// </summary>
    property CecVersion           CECVersion;

    /// <summary>
    /// The type of adapter that libCEC is connected to.
    /// </summary>
    property CecAdapterType       AdapterType;

	/// <summary>
    /// True to power on when quitting the screensaver.
    /// </summary>
	property bool                 PowerOnScreensaver;
  };

  // the callback methods are called by unmanaged code, so we need some delegates for this
#pragma unmanaged
  // unmanaged callback methods
  typedef int (__stdcall *LOGCB)    (const CEC::cec_log_message &message);
  typedef int (__stdcall *KEYCB)    (const CEC::cec_keypress &key);
  typedef int (__stdcall *COMMANDCB)(const CEC::cec_command &command);
  typedef int (__stdcall *CONFIGCB) (const CEC::libcec_configuration &config);
  typedef int (__stdcall *ALERTCB)  (const CEC::libcec_alert, const CEC::libcec_parameter &data);
  typedef int (__stdcall *MENUCB)   (const CEC::cec_menu_state newVal);
  typedef void (__stdcall *ACTICB)  (const CEC::cec_logical_address logicalAddress, const uint8_t bActivated);

  /// <summary>
  /// libCEC callback methods. Unmanaged code.
  /// </summary>
  typedef struct
  {
    /// <summary>
    /// Log message callback
    /// </summary>
    LOGCB     logCB;
    /// <summary>
    /// Key press/release callback
    /// </summary>
    KEYCB     keyCB;
    /// <summary>
    /// Raw CEC data callback
    /// </summary>
    COMMANDCB commandCB;
    /// <summary>
    /// Updated configuration callback
    /// </summary>
    CONFIGCB  configCB;
    /// <summary>
    /// Alert message callback
    /// </summary>
    ALERTCB   alertCB;
    /// <summary>
    /// Menu status change callback
    /// </summary>
    MENUCB    menuCB;
    /// <summary>
    /// Source (de)activated callback
    /// </summary>
    ACTICB    sourceActivatedCB;
  } UnmanagedCecCallbacks;

  static PLATFORM::CMutex                   g_callbackMutex;
  static std::vector<UnmanagedCecCallbacks> g_unmanagedCallbacks;
  static CEC::ICECCallbacks                 g_cecCallbacks;

  /// <summary>
  /// Called by libCEC to send back a log message to the application
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="message">The log message</param>
  /// <return>1 when handled, 0 otherwise</return>
  int CecLogMessageCB(void *cbParam, const CEC::cec_log_message message)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        return g_unmanagedCallbacks[iPtr].logCB(message);
    }
    return 0;
  }

  /// <summary>
  /// Called by libCEC to send back a key press or release to the application
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="key">The key press command that libCEC received</param>
  /// <return>1 when handled, 0 otherwise</return>
  int CecKeyPressCB(void *cbParam, const CEC::cec_keypress key)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        return g_unmanagedCallbacks[iPtr].keyCB(key);
    }
    return 0;
  }

  /// <summary>
  /// Called by libCEC to send back raw CEC data to the application
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="command">The raw CEC data</param>
  /// <return>1 when handled, 0 otherwise</return>
  int CecCommandCB(void *cbParam, const CEC::cec_command command)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        return g_unmanagedCallbacks[iPtr].commandCB(command);
    }
    return 0;
  }

  /// <summary>
  /// Called by libCEC to send back an updated configuration to the application
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="config">The new configuration</param>
  /// <return>1 when handled, 0 otherwise</return>
  int CecConfigCB(void *cbParam, const CEC::libcec_configuration config)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        return g_unmanagedCallbacks[iPtr].configCB(config);
    }
    return 0;
  }

  /// <summary>
  /// Called by libCEC to send back an alert message to the application
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="data">The alert message</param>
  /// <return>1 when handled, 0 otherwise</return>
  int CecAlertCB(void *cbParam, const CEC::libcec_alert alert, const CEC::libcec_parameter data)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        return g_unmanagedCallbacks[iPtr].alertCB(alert, data);
    }
    return 0;
  }

  /// <summary>
  /// Called by libCEC to send back a menu state change to the application
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="newVal">The new menu state</param>
  /// <return>1 when handled, 0 otherwise</return>
  int CecMenuCB(void *cbParam, const CEC::cec_menu_state newVal)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        return g_unmanagedCallbacks[iPtr].menuCB(newVal);
    }
    return 0;
  }

  /// <summary>
  /// Called by libCEC to notify the application that the source that is handled by libCEC was (de)activated
  /// </summary>
  /// <param name="cbParam">Pointer to the callback struct</param>
  /// <param name="logicalAddress">The logical address that was (de)activated</param>
  /// <param name="activated">True when activated, false when deactivated</param>
  void CecSourceActivatedCB(void *cbParam, const CEC::cec_logical_address logicalAddress, const uint8_t activated)
  {
    if (cbParam)
    {
      size_t iPtr = (size_t)cbParam;
      PLATFORM::CLockObject lock(g_callbackMutex);
      if (iPtr >= 0 && iPtr < g_unmanagedCallbacks.size())
        g_unmanagedCallbacks[iPtr].sourceActivatedCB(logicalAddress, activated);
    }
  }

#pragma managed
  /// <summary>
  /// Delegate method for the CecLogMessageCB callback in CecCallbackMethods
  /// </summary>
  public delegate int  CecLogMessageManagedDelegate(const CEC::cec_log_message &);
  /// <summary>
  /// Delegate method for the CecKeyPressCB callback in CecCallbackMethods
  /// </summary>
  public delegate int  CecKeyPressManagedDelegate(const CEC::cec_keypress &);
  /// <summary>
  /// Delegate method for the CecCommandCB callback in CecCallbackMethods
  /// </summary>
  public delegate int  CecCommandManagedDelegate(const CEC::cec_command &);
  /// <summary>
  /// Delegate method for the CecConfigCB callback in CecCallbackMethods
  /// </summary>
  public delegate int  CecConfigManagedDelegate(const CEC::libcec_configuration &);
  /// <summary>
  /// Delegate method for the CecAlertCB callback in CecCallbackMethods
  /// </summary>
  public delegate int  CecAlertManagedDelegate(const CEC::libcec_alert, const CEC::libcec_parameter &);
  /// <summary>
  /// Delegate method for the CecMenuCB callback in CecCallbackMethods
  /// </summary>
  public delegate int  CecMenuManagedDelegate(const CEC::cec_menu_state);
  /// <summary>
  /// Delegate method for the CecSourceActivatedCB callback in CecCallbackMethods
  /// </summary>
  public delegate void CecSourceActivatedManagedDelegate(const CEC::cec_logical_address, const uint8_t);

  /// <summary>
  /// Assign the callback methods in the g_cecCallbacks struct
  /// </summary>
  void AssignCallbacks()
  {
    g_cecCallbacks.CBCecLogMessage           = CecLogMessageCB;
    g_cecCallbacks.CBCecKeyPress             = CecKeyPressCB;
    g_cecCallbacks.CBCecCommand              = CecCommandCB;
    g_cecCallbacks.CBCecConfigurationChanged = CecConfigCB;
    g_cecCallbacks.CBCecAlert                = CecAlertCB;
    g_cecCallbacks.CBCecMenuStateChanged     = CecMenuCB;
    g_cecCallbacks.CBCecSourceActivated      = CecSourceActivatedCB;
  }

  /// <summary>
  /// The callback methods that libCEC uses
  /// </summary>
  public ref class CecCallbackMethods
  {
  public:
    CecCallbackMethods(void)
    {
      m_iCallbackPtr = -1;
      AssignCallbacks();
      m_bHasCallbacks = false;
      m_bDelegatesCreated = false;
    }

    ~CecCallbackMethods(void)
    {
      DestroyDelegates();
    }

    /// <summary>
    /// Pointer to the callbacks struct entry
    /// </summary>
    size_t GetCallbackPtr(void)
    {
      PLATFORM::CLockObject lock(g_callbackMutex);
      return m_iCallbackPtr;
    }

  protected:
    !CecCallbackMethods(void)
    {
      DestroyDelegates();
    }

  public:
    /// <summary>
    /// Disable callback methods
    /// </summary>
    virtual void DisableCallbacks(void)
    {
      DestroyDelegates();
    }

    /// <summary>
    /// Enable callback methods
    /// </summary>
    /// <param name="callbacks">Callback methods to activate</param>
    /// <return>true when handled, false otherwise</return>
    virtual bool EnableCallbacks(CecCallbackMethods ^ callbacks)
    {
      CreateDelegates();
      if (!m_bHasCallbacks)
      {
        m_bHasCallbacks = true;
        m_callbacks = callbacks;
        return true;
      }

      return false;
    }

    /// <summary>
    /// Called by libCEC to send back a log message to the application.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="message">The log message</param>
    /// <return>1 when handled, 0 otherwise</return>
    virtual int ReceiveLogMessage(CecLogMessage ^ message)
    {
      return 0;
    }

    /// <summary>
    /// Called by libCEC to send back a key press or release to the application.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="key">The key press command that libCEC received</param>
    /// <return>1 when handled, 0 otherwise</return>
    virtual int ReceiveKeypress(CecKeypress ^ key)
    {
      return 0;
    }

    /// <summary>
    /// Called by libCEC to send back raw CEC data to the application.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="command">The raw CEC data</param>
    /// <return>1 when handled, 0 otherwise</return>
    virtual int ReceiveCommand(CecCommand ^ command)
    {
      return 0;
    }

    /// <summary>
    /// Called by libCEC to send back an updated configuration to the application.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="config">The new configuration</param>
    /// <return>1 when handled, 0 otherwise</return>
    virtual int ConfigurationChanged(LibCECConfiguration ^ config)
    {
      return 0;
    }

    /// <summary>
    /// Called by libCEC to send back an alert message to the application.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="data">The alert message</param>
    /// <return>1 when handled, 0 otherwise</return>
    virtual int ReceiveAlert(CecAlert alert, CecParameter ^ data)
    {
      return 0;
    }

    /// <summary>
    /// Called by libCEC to send back a menu state change to the application.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="newVal">The new menu state</param>
    /// <return>1 when handled, 0 otherwise</return>
    virtual int ReceiveMenuStateChange(CecMenuState newVal)
    {
      return 0;
    }

    /// <summary>
    /// Called by libCEC to notify the application that the source that is handled by libCEC was (de)activated.
    /// Override in the application to handle this callback.
    /// </summary>
    /// <param name="logicalAddress">The logical address that was (de)activated</param>
    /// <param name="activated">True when activated, false when deactivated</param>
    virtual void SourceActivated(CecLogicalAddress logicalAddress, bool activated)
    {
    }

  protected:
    // managed callback methods
    int CecLogMessageManaged(const CEC::cec_log_message &message)
    {
      int iReturn(0);
      if (m_bHasCallbacks)
        iReturn = m_callbacks->ReceiveLogMessage(gcnew CecLogMessage(gcnew System::String(message.message), (CecLogLevel)message.level, message.time));
      return iReturn;
    }

    int CecKeyPressManaged(const CEC::cec_keypress &key)
    {
      int iReturn(0);
      if (m_bHasCallbacks)
        iReturn = m_callbacks->ReceiveKeypress(gcnew CecKeypress((CecUserControlCode)key.keycode, key.duration));
      return iReturn;
    }

    int CecCommandManaged(const CEC::cec_command &command)
    {
      int iReturn(0);
      if (m_bHasCallbacks)
      {
        CecCommand ^ newCommand = gcnew CecCommand((CecLogicalAddress)command.initiator, (CecLogicalAddress)command.destination, command.ack == 1 ? true : false, command.eom == 1 ? true : false, (CecOpcode)command.opcode, command.transmit_timeout);
        for (uint8_t iPtr = 0; iPtr < command.parameters.size; iPtr++)
          newCommand->Parameters->PushBack(command.parameters[iPtr]);
        iReturn = m_callbacks->ReceiveCommand(newCommand);
      }
      return iReturn;
    }

    int CecConfigManaged(const CEC::libcec_configuration &config)
    {
      int iReturn(0);
      if (m_bHasCallbacks)
      {
        LibCECConfiguration ^netConfig = gcnew LibCECConfiguration();
        netConfig->Update(config);
        iReturn = m_callbacks->ConfigurationChanged(netConfig);
      }
      return iReturn;
    }

    int CecAlertManaged(const CEC::libcec_alert alert, const CEC::libcec_parameter &data)
    {
      int iReturn(0);
      if (m_bHasCallbacks)
      {
        CecParameterType newType = (CecParameterType)data.paramType;
        if (newType == CecParameterType::ParameterTypeString)
        {
          System::String ^ newData = gcnew System::String(data.paramData ? (const char *)data.paramData : "", 0, 128);
          CecParameter ^ newParam = gcnew CecParameter(newType, newData);
          iReturn = m_callbacks->ReceiveAlert((CecAlert)alert, newParam);
        }
      }
      return iReturn;
    }

    int CecMenuManaged(const CEC::cec_menu_state newVal)
    {
      int iReturn(0);
      if (m_bHasCallbacks)
      {
        iReturn = m_callbacks->ReceiveMenuStateChange((CecMenuState)newVal);
      }
      return iReturn;
    }

    void CecSourceActivatedManaged(const CEC::cec_logical_address logicalAddress, const uint8_t bActivated)
    {
      if (m_bHasCallbacks)
        m_callbacks->SourceActivated((CecLogicalAddress)logicalAddress, bActivated == 1);
    }

    void DestroyDelegates()
    {
      m_bHasCallbacks = false;
      if (m_bDelegatesCreated)
      {
        m_bDelegatesCreated = false;
        m_logMessageGCHandle.Free();
        m_keypressGCHandle.Free();
        m_commandGCHandle.Free();
        m_alertGCHandle.Free();
        m_menuGCHandle.Free();
        m_sourceActivatedGCHandle.Free();
      }
    }

    void CreateDelegates()
    {
      DestroyDelegates();

      if (!m_bDelegatesCreated)
      {
        msclr::interop::marshal_context ^ context = gcnew msclr::interop::marshal_context();

        // create the delegate method for the log message callback
        m_logMessageDelegate      = gcnew CecLogMessageManagedDelegate(this, &CecCallbackMethods::CecLogMessageManaged);
        m_logMessageGCHandle      = System::Runtime::InteropServices::GCHandle::Alloc(m_logMessageDelegate);
        m_logMessageCallback      = static_cast<LOGCB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_logMessageDelegate).ToPointer());

        // create the delegate method for the keypress callback
        m_keypressDelegate        = gcnew CecKeyPressManagedDelegate(this, &CecCallbackMethods::CecKeyPressManaged);
        m_keypressGCHandle        = System::Runtime::InteropServices::GCHandle::Alloc(m_keypressDelegate);
        m_keypressCallback        = static_cast<KEYCB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_keypressDelegate).ToPointer());

        // create the delegate method for the command callback
        m_commandDelegate         = gcnew CecCommandManagedDelegate(this, &CecCallbackMethods::CecCommandManaged);
        m_commandGCHandle         = System::Runtime::InteropServices::GCHandle::Alloc(m_commandDelegate);
        m_commandCallback         = static_cast<COMMANDCB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_commandDelegate).ToPointer());

        // create the delegate method for the configuration change callback
        m_configDelegate          = gcnew CecConfigManagedDelegate(this, &CecCallbackMethods::CecConfigManaged);
        m_configGCHandle          = System::Runtime::InteropServices::GCHandle::Alloc(m_configDelegate);
        m_configCallback          = static_cast<CONFIGCB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_configDelegate).ToPointer());

        // create the delegate method for the alert callback
        m_alertDelegate           = gcnew CecAlertManagedDelegate(this, &CecCallbackMethods::CecAlertManaged);
        m_alertGCHandle           = System::Runtime::InteropServices::GCHandle::Alloc(m_alertDelegate);
        m_alertCallback           = static_cast<ALERTCB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_alertDelegate).ToPointer());

        // create the delegate method for the menu callback
        m_menuDelegate            = gcnew CecMenuManagedDelegate(this, &CecCallbackMethods::CecMenuManaged);
        m_menuGCHandle            = System::Runtime::InteropServices::GCHandle::Alloc(m_menuDelegate);
        m_menuCallback            = static_cast<MENUCB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_menuDelegate).ToPointer());

        // create the delegate method for the source activated callback
        m_sourceActivatedDelegate = gcnew CecSourceActivatedManagedDelegate(this, &CecCallbackMethods::CecSourceActivatedManaged);
        m_sourceActivatedGCHandle = System::Runtime::InteropServices::GCHandle::Alloc(m_sourceActivatedDelegate);
        m_sourceActivatedCallback = static_cast<ACTICB>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_sourceActivatedDelegate).ToPointer());

        delete context;

        UnmanagedCecCallbacks unmanagedCallbacks;
        unmanagedCallbacks.logCB             = m_logMessageCallback;
        unmanagedCallbacks.keyCB             = m_keypressCallback;
        unmanagedCallbacks.commandCB         = m_commandCallback;
        unmanagedCallbacks.configCB          = m_configCallback;
        unmanagedCallbacks.alertCB           = m_alertCallback;
        unmanagedCallbacks.menuCB            = m_menuCallback;
        unmanagedCallbacks.sourceActivatedCB = m_sourceActivatedCallback;

        PLATFORM::CLockObject lock(g_callbackMutex);
        g_unmanagedCallbacks.push_back(unmanagedCallbacks);
        m_iCallbackPtr = g_unmanagedCallbacks.size() - 1;
        m_bDelegatesCreated = true;
      }
    }

    CecLogMessageManagedDelegate ^                    m_logMessageDelegate;
    static System::Runtime::InteropServices::GCHandle m_logMessageGCHandle;
    LOGCB                                             m_logMessageCallback;

    CecKeyPressManagedDelegate ^                      m_keypressDelegate;
    static System::Runtime::InteropServices::GCHandle m_keypressGCHandle;
    KEYCB                                             m_keypressCallback;

    CecCommandManagedDelegate ^                       m_commandDelegate;
    static System::Runtime::InteropServices::GCHandle m_commandGCHandle;
    COMMANDCB                                         m_commandCallback;

    CecConfigManagedDelegate ^                        m_configDelegate;
    static System::Runtime::InteropServices::GCHandle m_configGCHandle;
    CONFIGCB                                          m_configCallback;

    CecAlertManagedDelegate ^                         m_alertDelegate;
    static System::Runtime::InteropServices::GCHandle m_alertGCHandle;
    ALERTCB                                           m_alertCallback;

    CecMenuManagedDelegate ^                          m_menuDelegate;
    static System::Runtime::InteropServices::GCHandle m_menuGCHandle;
    MENUCB                                            m_menuCallback;

    CecSourceActivatedManagedDelegate ^               m_sourceActivatedDelegate;
    static System::Runtime::InteropServices::GCHandle m_sourceActivatedGCHandle;
    ACTICB                                            m_sourceActivatedCallback;

    CecCallbackMethods ^ m_callbacks;
    bool                 m_bHasCallbacks;
    bool                 m_bDelegatesCreated;
    size_t               m_iCallbackPtr;
  };
}

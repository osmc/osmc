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

#include "../env.h"
#include "../include/cec.h"

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <signal.h>
#include "../lib/platform/os.h"
#include "../lib/implementations/CECCommandHandler.h"
#include "../lib/platform/util/StdString.h"
#include "../lib/platform/threads/threads.h"

using namespace CEC;
using namespace std;
using namespace PLATFORM;

#define CEC_CONFIG_VERSION CEC_CLIENT_VERSION_CURRENT;

#include "../../include/cecloader.h"

static void PrintToStdOut(const char *strFormat, ...);

ICECCallbacks        g_callbacks;
libcec_configuration g_config;
int                  g_cecLogLevel(-1);
int                  g_cecDefaultLogLevel(CEC_LOG_ALL);
ofstream             g_logOutput;
bool                 g_bShortLog(false);
CStdString           g_strPort;
bool                 g_bSingleCommand(false);
bool                 g_bExit(false);
bool                 g_bHardExit(false);
CMutex               g_outputMutex;
ICECAdapter*         g_parser;

class CReconnect : public PLATFORM::CThread
{
public:
  static CReconnect& Get(void)
  {
    static CReconnect _instance;
    return _instance;
  }

  virtual ~CReconnect(void) {}

  void* Process(void)
  {
    if (g_parser)
    {
      g_parser->Close();
      if (!g_parser->Open(g_strPort.c_str()))
      {
        PrintToStdOut("Failed to reconnect\n");
        g_bExit = true;
      }
    }
    return NULL;
  }

private:
  CReconnect(void) {}
};

static void PrintToStdOut(const char *strFormat, ...)
{
  CStdString strLog;

  va_list argList;
  va_start(argList, strFormat);
  strLog.FormatV(strFormat, argList);
  va_end(argList);

  CLockObject lock(g_outputMutex);
  cout << strLog << endl;
}

inline bool HexStrToInt(const std::string& data, uint8_t& value)
{
  int iTmp(0);
  if (sscanf(data.c_str(), "%x", &iTmp) == 1)
  {
    if (iTmp > 256)
      value = 255;
	  else if (iTmp < 0)
      value = 0;
    else
      value = (uint8_t) iTmp;

    return true;
  }

  return false;
}

//get the first word (separated by whitespace) from string data and place that in word
//then remove that word from string data
bool GetWord(string& data, string& word)
{
  stringstream datastream(data);
  string end;

  datastream >> word;
  if (datastream.fail())
  {
    data.clear();
    return false;
  }

  size_t pos = data.find(word) + word.length();

  if (pos >= data.length())
  {
    data.clear();
    return true;
  }

  data = data.substr(pos);

  datastream.clear();
  datastream.str(data);

  datastream >> end;
  if (datastream.fail())
    data.clear();

  return true;
}

int CecLogMessage(void *UNUSED(cbParam), const cec_log_message message)
{
  if ((message.level & g_cecLogLevel) == message.level)
  {
    CStdString strLevel;
    switch (message.level)
    {
    case CEC_LOG_ERROR:
      strLevel = "ERROR:   ";
      break;
    case CEC_LOG_WARNING:
      strLevel = "WARNING: ";
      break;
    case CEC_LOG_NOTICE:
      strLevel = "NOTICE:  ";
      break;
    case CEC_LOG_TRAFFIC:
      strLevel = "TRAFFIC: ";
      break;
    case CEC_LOG_DEBUG:
      strLevel = "DEBUG:   ";
      break;
    default:
      break;
    }

    CStdString strFullLog;
    strFullLog.Format("%s[%16lld]\t%s", strLevel.c_str(), message.time, message.message);
    PrintToStdOut(strFullLog.c_str());

    if (g_logOutput.is_open())
    {
      if (g_bShortLog)
        g_logOutput << message.message << endl;
      else
        g_logOutput << strFullLog.c_str() << endl;
    }
  }

  return 0;
}

int CecKeyPress(void *UNUSED(cbParam), const cec_keypress UNUSED(key))
{
  return 0;
}

int CecCommand(void *UNUSED(cbParam), const cec_command UNUSED(command))
{
  return 0;
}

int CecAlert(void *UNUSED(cbParam), const libcec_alert type, const libcec_parameter UNUSED(param))
{
  switch (type)
  {
  case CEC_ALERT_CONNECTION_LOST:
    if (!CReconnect::Get().IsRunning())
    {
      PrintToStdOut("Connection lost - trying to reconnect\n");
      CReconnect::Get().CreateThread(false);
    }
    break;
  default:
    break;
  }
  return 0;
}

void ListDevices(ICECAdapter *parser)
{
  cec_adapter_descriptor devices[10];
  int8_t iDevicesFound = parser->DetectAdapters(devices, 10, NULL);
  if (iDevicesFound <= 0)
  {
    PrintToStdOut("Found devices: NONE");
  }
  else
  {
    PrintToStdOut("Found devices: %d\n", iDevicesFound);

    for (int8_t iDevicePtr = 0; iDevicePtr < iDevicesFound; iDevicePtr++)
    {
      PrintToStdOut("device:              %d", iDevicePtr + 1);
      PrintToStdOut("com port:            %s", devices[iDevicePtr].strComName);
      PrintToStdOut("vendor id:           %04x", devices[iDevicePtr].iVendorId);
      PrintToStdOut("product id:          %04x", devices[iDevicePtr].iProductId);
      PrintToStdOut("firmware version:    %d", devices[iDevicePtr].iFirmwareVersion);

      if (devices[iDevicePtr].iFirmwareBuildDate != CEC_FW_BUILD_UNKNOWN)
      {
        time_t buildTime = (time_t)devices[iDevicePtr].iFirmwareBuildDate;
        CStdString strDeviceInfo;
        strDeviceInfo.AppendFormat("firmware build date: %s", asctime(gmtime(&buildTime)));
        strDeviceInfo = strDeviceInfo.Left(strDeviceInfo.length() > 1 ? (unsigned)(strDeviceInfo.length() - 1) : 0); // strip \n added by asctime
        strDeviceInfo.append(" +0000");
        PrintToStdOut(strDeviceInfo.c_str());
      }

      if (devices[iDevicePtr].adapterType != ADAPTERTYPE_UNKNOWN)
      {
        PrintToStdOut("type:                %s", parser->ToString(devices[iDevicePtr].adapterType));
      }

      PrintToStdOut("");
    }
  }
}

void ShowHelpCommandLine(const char* strExec)
{
  CLockObject lock(g_outputMutex);
  cout << endl <<
      strExec << " {-h|--help|-l|--list-devices|[COM PORT]}" << endl <<
      endl <<
      "parameters:" << endl <<
      "  -h --help                   Shows this help text" << endl <<
      "  -l --list-devices           List all devices on this system" << endl <<
      "  -t --type {p|r|t|a}         The device type to use. More than one is possible." << endl <<
      "  -p --port {int}             The HDMI port to use as active source." << endl <<
      "  -b --base {int}             The logical address of the device to with this " << endl <<
      "                              adapter is connected." << endl <<
      "  -f --log-file {file}        Writes all libCEC log message to a file" << endl <<
      "  -r --rom                    Read persisted settings from the EEPROM" << endl <<
      "  -sf --short-log-file {file} Writes all libCEC log message without timestamps" << endl <<
      "                              and log levels to a file." << endl <<
      "  -d --log-level {level}      Sets the log level. See cectypes.h for values." << endl <<
      "  -s --single-command         Execute a single command and exit. Does not power" << endl <<
      "                              on devices on startup and power them off on exit." << endl <<
      "  -o --osd-name {osd name}    Use a custom osd name." << endl <<
      "  -m --monitor                Start a monitor-only client." << endl <<
      "  -i --info                   Shows information about how libCEC was compiled." << endl <<
      "  [COM PORT]                  The com port to connect to. If no COM" << endl <<
      "                              port is given, the client tries to connect to the" << endl <<
      "                              first device that is detected." << endl <<
      endl <<
      "Type 'h' or 'help' and press enter after starting the client to display all " << endl <<
      "available commands" << endl;
}

void ShowHelpConsole(void)
{
  CLockObject lock(g_outputMutex);
  cout << endl <<
  "================================================================================" << endl <<
  "Available commands:" << endl <<
  endl <<
  "[tx] {bytes}              transfer bytes over the CEC line." << endl <<
  "[txn] {bytes}             transfer bytes but don't wait for transmission ACK." << endl <<
  "[on] {address}            power on the device with the given logical address." << endl <<
  "[standby] {address}       put the device with the given address in standby mode." << endl <<
  "[la] {logical address}    change the logical address of the CEC adapter." << endl <<
  "[p] {device} {port}       change the HDMI port number of the CEC adapter." << endl <<
  "[pa] {physical address}   change the physical address of the CEC adapter." << endl <<
  "[as]                      make the CEC adapter the active source." << endl <<
  "[is]                      mark the CEC adapter as inactive source." << endl <<
  "[osd] {addr} {string}     set OSD message on the specified device." << endl <<
  "[ver] {addr}              get the CEC version of the specified device." << endl <<
  "[ven] {addr}              get the vendor ID of the specified device." << endl <<
  "[lang] {addr}             get the menu language of the specified device." << endl <<
  "[pow] {addr}              get the power status of the specified device." << endl <<
  "[name] {addr}             get the OSD name of the specified device." << endl <<
  "[poll] {addr}             poll the specified device." << endl <<
  "[lad]                     lists active devices on the bus" << endl <<
  "[ad] {addr}               checks whether the specified device is active." << endl <<
  "[at] {type}               checks whether the specified device type is active." << endl <<
  "[sp] {addr}               makes the specified physical address active." << endl <<
  "[spl] {addr}              makes the specified logical address active." << endl <<
  "[volup]                   send a volume up command to the amp if present" << endl <<
  "[voldown]                 send a volume down command to the amp if present" << endl <<
  "[mute]                    send a mute/unmute command to the amp if present" << endl <<
  "[self]                    show the list of addresses controlled by libCEC" << endl <<
  "[scan]                    scan the CEC bus and display device info" << endl <<
  "[mon] {1|0}               enable or disable CEC bus monitoring." << endl <<
  "[log] {1 - 31}            change the log level. see cectypes.h for values." << endl <<
  "[ping]                    send a ping command to the CEC adapter." << endl <<
  "[bl]                      to let the adapter enter the bootloader, to upgrade" << endl <<
  "                          the flash rom." << endl <<
  "[r]                       reconnect to the CEC adapter." << endl <<
  "[h] or [help]             show this help." << endl <<
  "[q] or [quit]             to quit the CEC test client and switch off all" << endl <<
  "                          connected CEC devices." << endl <<
  "================================================================================" << endl;
}

bool ProcessCommandSELF(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "self")
  {
    cec_logical_addresses addr = parser->GetLogicalAddresses();
    CStdString strOut = "Addresses controlled by libCEC: ";
    bool bFirst(true);
    for (uint8_t iPtr = 0; iPtr <= 15; iPtr++)
    {
      if (addr[iPtr])
      {
        strOut.AppendFormat((bFirst ? "%d%s" : ", %d%s"), iPtr, parser->IsActiveSource((cec_logical_address)iPtr) ? "*" : "");
        bFirst = false;
      }
    }
    PrintToStdOut(strOut.c_str());
    return true;
  }

  return false;
}

bool ProcessCommandSP(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "sp")
  {
    string strAddress;
    int iAddress;
    if (GetWord(arguments, strAddress))
    {
      sscanf(strAddress.c_str(), "%x", &iAddress);
      if (iAddress >= 0 && iAddress <= CEC_INVALID_PHYSICAL_ADDRESS)
        parser->SetStreamPath((uint16_t)iAddress);
      return true;
    }
  }

  return false;
}

bool ProcessCommandSPL(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "spl")
  {
    string strAddress;
    cec_logical_address iAddress;
    if (GetWord(arguments, strAddress))
    {
      iAddress = (cec_logical_address)atoi(strAddress.c_str());
      if (iAddress >= CECDEVICE_TV && iAddress < CECDEVICE_BROADCAST)
        parser->SetStreamPath(iAddress);
      return true;
    }
  }

  return false;
}

bool ProcessCommandTX(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "tx" || command == "txn")
  {
    string strvalue;
    uint8_t ivalue;
    cec_command bytes;
    bytes.Clear();

    CStdString strArguments(arguments);
    strArguments.Replace(':', ' ');
    arguments = strArguments;

    while (GetWord(arguments, strvalue) && HexStrToInt(strvalue, ivalue))
      bytes.PushBack(ivalue);

    if (command == "txn")
      bytes.transmit_timeout = 0;

    parser->Transmit(bytes);

    return true;
  }

  return false;
}

bool ProcessCommandON(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "on")
  {
    string strValue;
    uint8_t iValue = 0;
    if (GetWord(arguments, strValue) && HexStrToInt(strValue, iValue) && iValue <= 0xF)
    {
      parser->PowerOnDevices((cec_logical_address) iValue);
      return true;
    }
    else
    {
      PrintToStdOut("invalid destination");
    }
  }

  return false;
}

bool ProcessCommandSTANDBY(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "standby")
  {
    string strValue;
    uint8_t iValue = 0;
    if (GetWord(arguments, strValue) && HexStrToInt(strValue, iValue) && iValue <= 0xF)
    {
      parser->StandbyDevices((cec_logical_address) iValue);
      return true;
    }
    else
    {
      PrintToStdOut("invalid destination");
    }
  }

  return false;
}

bool ProcessCommandPOLL(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "poll")
  {
    string strValue;
    uint8_t iValue = 0;
    if (GetWord(arguments, strValue) && HexStrToInt(strValue, iValue) && iValue <= 0xF)
    {
      if (parser->PollDevice((cec_logical_address) iValue))
        PrintToStdOut("POLL message sent");
      else
        PrintToStdOut("POLL message not sent");
      return true;
    }
    else
    {
      PrintToStdOut("invalid destination");
    }
  }

  return false;
}

bool ProcessCommandLA(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "la")
  {
    string strvalue;
    if (GetWord(arguments, strvalue))
    {
      parser->SetLogicalAddress((cec_logical_address) atoi(strvalue.c_str()));
      return true;
    }
  }

  return false;
}

bool ProcessCommandP(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "p")
  {
    string strPort, strDevice;
    if (GetWord(arguments, strDevice) && GetWord(arguments, strPort))
    {
      parser->SetHDMIPort((cec_logical_address)atoi(strDevice.c_str()), (uint8_t)atoi(strPort.c_str()));
      return true;
    }
  }

  return false;
}

bool ProcessCommandPA(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "pa")
  {
    string strB1, strB2;
    uint8_t iB1, iB2;
    if (GetWord(arguments, strB1) && HexStrToInt(strB1, iB1) &&
        GetWord(arguments, strB2) && HexStrToInt(strB2, iB2))
    {
      uint16_t iPhysicalAddress = ((uint16_t)iB1 << 8) + iB2;
      parser->SetPhysicalAddress(iPhysicalAddress);
      return true;
    }
  }

  return false;
}

bool ProcessCommandOSD(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "osd")
  {
    bool bFirstWord(false);
    string strAddr, strMessage, strWord;
    uint8_t iAddr;
    if (GetWord(arguments, strAddr) && HexStrToInt(strAddr, iAddr) && iAddr < 0xF)
    {
      while (GetWord(arguments, strWord))
      {
        if (bFirstWord)
        {
          bFirstWord = false;
          strMessage.append(" ");
        }
        strMessage.append(strWord);
      }
      parser->SetOSDString((cec_logical_address) iAddr, CEC_DISPLAY_CONTROL_DISPLAY_FOR_DEFAULT_TIME, strMessage.c_str());
      return true;
    }
  }

  return false;
}

bool ProcessCommandAS(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "as")
  {
    parser->SetActiveSource();
    // wait for the source switch to finish for 15 seconds tops
    if (g_bSingleCommand)
    {
      CTimeout timeout(15000);
      bool bActiveSource(false);
      while (timeout.TimeLeft() > 0 && !bActiveSource)
      {
        bActiveSource = parser->IsLibCECActiveSource();
        if (!bActiveSource)
          CEvent::Sleep(100);
      }
    }
    return true;
  }

  return false;
}

bool ProcessCommandIS(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "is")
    return parser->SetInactiveView();

  return false;
}

bool ProcessCommandPING(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "ping")
  {
    parser->PingAdapter();
    return true;
  }

  return false;
}

bool ProcessCommandVOLUP(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "volup")
  {
    PrintToStdOut("volume up: %2X", parser->VolumeUp());
    return true;
  }

  return false;
}

bool ProcessCommandVOLDOWN(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "voldown")
  {
    PrintToStdOut("volume down: %2X", parser->VolumeDown());
    return true;
  }

  return false;
}

bool ProcessCommandMUTE(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "mute")
  {
    PrintToStdOut("mute: %2X", parser->MuteAudio());
    return true;
  }

  return false;
}

bool ProcessCommandMON(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "mon")
  {
    CStdString strEnable;
    if (GetWord(arguments, strEnable) && (strEnable.Equals("0") || strEnable.Equals("1")))
    {
      parser->SwitchMonitoring(strEnable.Equals("1"));
      return true;
    }
  }

  return false;
}

bool ProcessCommandBL(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "bl")
  {
    if (parser->StartBootloader())
    {
      PrintToStdOut("entered bootloader mode. exiting cec-client");
      g_bExit = true;
      g_bHardExit = true;
    }
    return true;
  }

  return false;
}

bool ProcessCommandLANG(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "lang")
  {
    CStdString strDev;
    if (GetWord(arguments, strDev))
    {
      int iDev = atoi(strDev);
      if (iDev >= 0 && iDev < 15)
      {
        CStdString strLog;
        cec_menu_language language;
        if (parser->GetDeviceMenuLanguage((cec_logical_address) iDev, &language))
          strLog.Format("menu language '%s'", language.language);
        else
          strLog = "failed!";
        PrintToStdOut(strLog);
        return true;
      }
    }
  }

  return false;
}

bool ProcessCommandVEN(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "ven")
  {
    CStdString strDev;
    if (GetWord(arguments, strDev))
    {
      int iDev = atoi(strDev);
      if (iDev >= 0 && iDev < 15)
      {
        uint64_t iVendor = parser->GetDeviceVendorId((cec_logical_address) iDev);
        PrintToStdOut("vendor id: %06llx", iVendor);
        return true;
      }
    }
  }

  return false;
}

bool ProcessCommandVER(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "ver")
  {
    CStdString strDev;
    if (GetWord(arguments, strDev))
    {
      int iDev = atoi(strDev);
      if (iDev >= 0 && iDev < 15)
      {
        cec_version iVersion = parser->GetDeviceCecVersion((cec_logical_address) iDev);
        PrintToStdOut("CEC version %s", parser->ToString(iVersion));
        return true;
      }
    }
  }

  return false;
}

bool ProcessCommandPOW(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "pow")
  {
    CStdString strDev;
    if (GetWord(arguments, strDev))
    {
      int iDev = atoi(strDev);
      if (iDev >= 0 && iDev < 15)
      {
        cec_power_status iPower = parser->GetDevicePowerStatus((cec_logical_address) iDev);
        PrintToStdOut("power status: %s", parser->ToString(iPower));
        return true;
      }
    }
  }

  return false;
}

bool ProcessCommandNAME(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "name")
  {
    CStdString strDev;
    if (GetWord(arguments, strDev))
    {
      int iDev = atoi(strDev);
      if (iDev >= 0 && iDev < 15)
      {
        cec_osd_name name = parser->GetDeviceOSDName((cec_logical_address)iDev);
        PrintToStdOut("OSD name of device %d is '%s'", iDev, name.name);
      }
      return true;
    }
  }

  return false;
}

bool ProcessCommandLAD(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "lad")
  {
    PrintToStdOut("listing active devices:");
    cec_logical_addresses addresses = parser->GetActiveDevices();
    for (uint8_t iPtr = 0; iPtr <= 11; iPtr++)
      if (addresses[iPtr])
      {
        PrintToStdOut("logical address %X", (int)iPtr);
      }
    return true;
  }

  return false;
}

bool ProcessCommandAD(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "ad")
  {
    CStdString strDev;
    if (GetWord(arguments, strDev))
    {
      int iDev = atoi(strDev);
      if (iDev >= 0 && iDev < 15)
        PrintToStdOut("logical address %X is %s", iDev, (parser->IsActiveDevice((cec_logical_address)iDev) ? "active" : "not active"));
    }
  }

  return false;
}

bool ProcessCommandAT(ICECAdapter *parser, const string &command, string &arguments)
{
  if (command == "at")
  {
    CStdString strType;
    if (GetWord(arguments, strType))
    {
      cec_device_type type = CEC_DEVICE_TYPE_TV;
      if (strType.Equals("a"))
        type = CEC_DEVICE_TYPE_AUDIO_SYSTEM;
      else if (strType.Equals("p"))
        type = CEC_DEVICE_TYPE_PLAYBACK_DEVICE;
      else if (strType.Equals("r"))
        type = CEC_DEVICE_TYPE_RECORDING_DEVICE;
      else if (strType.Equals("t"))
        type = CEC_DEVICE_TYPE_TUNER;

      PrintToStdOut("device %d is %s", type, (parser->IsActiveDeviceType(type) ? "active" : "not active"));
      return true;
    }
  }

  return false;
}

bool ProcessCommandR(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "r")
  {
    bool bReactivate = parser->IsLibCECActiveSource();

    PrintToStdOut("closing the connection");
    parser->Close();

    PrintToStdOut("opening a new connection");
    parser->Open(g_strPort.c_str());

    if (bReactivate)
    {
      PrintToStdOut("setting active source");
      parser->SetActiveSource();
    }
    return true;
  }

  return false;
}

bool ProcessCommandH(ICECAdapter * UNUSED(parser), const string &command, string & UNUSED(arguments))
{
  if (command == "h" || command == "help")
  {
    ShowHelpConsole();
    return true;
  }

  return false;
}

bool ProcessCommandLOG(ICECAdapter * UNUSED(parser), const string &command, string &arguments)
{
  if (command == "log")
  {
    CStdString strLevel;
    if (GetWord(arguments, strLevel))
    {
      int iNewLevel = atoi(strLevel);
      if (iNewLevel >= CEC_LOG_ERROR && iNewLevel <= CEC_LOG_ALL)
      {
        g_cecLogLevel = iNewLevel;

        PrintToStdOut("log level changed to %s", strLevel.c_str());
        return true;
      }
    }
  }

  return false;
}

bool ProcessCommandSCAN(ICECAdapter *parser, const string &command, string & UNUSED(arguments))
{
  if (command == "scan")
  {
    CStdString strLog;
    PrintToStdOut("requesting CEC bus information ...");

    strLog.append("CEC bus information\n===================\n");
    cec_logical_addresses addresses = parser->GetActiveDevices();
    cec_logical_address activeSource = parser->GetActiveSource();
    for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
    {
      if (addresses[iPtr])
      {
        uint64_t iVendorId        = parser->GetDeviceVendorId((cec_logical_address)iPtr);
        uint16_t iPhysicalAddress = parser->GetDevicePhysicalAddress((cec_logical_address)iPtr);
        bool     bActive          = parser->IsActiveSource((cec_logical_address)iPtr);
        cec_version iCecVersion   = parser->GetDeviceCecVersion((cec_logical_address)iPtr);
        cec_power_status power    = parser->GetDevicePowerStatus((cec_logical_address)iPtr);
        cec_osd_name osdName      = parser->GetDeviceOSDName((cec_logical_address)iPtr);
        CStdString strAddr;
        strAddr.Format("%x.%x.%x.%x", (iPhysicalAddress >> 12) & 0xF, (iPhysicalAddress >> 8) & 0xF, (iPhysicalAddress >> 4) & 0xF, iPhysicalAddress & 0xF);
        cec_menu_language lang;
        lang.device = CECDEVICE_UNKNOWN;
        parser->GetDeviceMenuLanguage((cec_logical_address)iPtr, &lang);

        strLog.AppendFormat("device #%X: %s\n", (int)iPtr, parser->ToString((cec_logical_address)iPtr));
        strLog.AppendFormat("address:       %s\n", strAddr.c_str());
        strLog.AppendFormat("active source: %s\n", (bActive ? "yes" : "no"));
        strLog.AppendFormat("vendor:        %s\n", parser->ToString((cec_vendor_id)iVendorId));
        strLog.AppendFormat("osd string:    %s\n", osdName.name);
        strLog.AppendFormat("CEC version:   %s\n", parser->ToString(iCecVersion));
        strLog.AppendFormat("power status:  %s\n", parser->ToString(power));
        if ((uint8_t)lang.device == iPtr)
          strLog.AppendFormat("language:      %s\n", lang.language);
        strLog.append("\n\n");
      }
    }

    activeSource = parser->GetActiveSource();
    strLog.AppendFormat("currently active source: %s (%d)", parser->ToString(activeSource), (int)activeSource);

    PrintToStdOut(strLog);
    return true;
  }

  return false;
}

bool ProcessConsoleCommand(ICECAdapter *parser, string &input)
{
  if (!input.empty())
  {
    string command;
    if (GetWord(input, command))
    {
      if (command == "q" || command == "quit")
        return false;

      ProcessCommandTX(parser, command, input) ||
      ProcessCommandON(parser, command, input) ||
      ProcessCommandSTANDBY(parser, command, input) ||
      ProcessCommandPOLL(parser, command, input) ||
      ProcessCommandLA(parser, command, input) ||
      ProcessCommandP(parser, command, input) ||
      ProcessCommandPA(parser, command, input) ||
      ProcessCommandAS(parser, command, input) ||
      ProcessCommandIS(parser, command, input) ||
      ProcessCommandOSD(parser, command, input) ||
      ProcessCommandPING(parser, command, input) ||
      ProcessCommandVOLUP(parser, command, input) ||
      ProcessCommandVOLDOWN(parser, command, input) ||
      ProcessCommandMUTE(parser, command, input) ||
      ProcessCommandMON(parser, command, input) ||
      ProcessCommandBL(parser, command, input) ||
      ProcessCommandLANG(parser, command, input) ||
      ProcessCommandVEN(parser, command, input) ||
      ProcessCommandVER(parser, command, input) ||
      ProcessCommandPOW(parser, command, input) ||
      ProcessCommandNAME(parser, command, input) ||
      ProcessCommandLAD(parser, command, input) ||
      ProcessCommandAD(parser, command, input) ||
      ProcessCommandAT(parser, command, input) ||
      ProcessCommandR(parser, command, input) ||
      ProcessCommandH(parser, command, input) ||
      ProcessCommandLOG(parser, command, input) ||
      ProcessCommandSCAN(parser, command, input) ||
      ProcessCommandSP(parser, command, input) ||
      ProcessCommandSPL(parser, command, input) ||
      ProcessCommandSELF(parser, command, input);
    }
  }
  return true;
}

bool ProcessCommandLineArguments(int argc, char *argv[])
{
  bool bReturn(true);
  int iArgPtr = 1;
  while (iArgPtr < argc && bReturn)
  {
    if (argc >= iArgPtr + 1)
    {
      if (!strcmp(argv[iArgPtr], "-f") ||
          !strcmp(argv[iArgPtr], "--log-file") ||
          !strcmp(argv[iArgPtr], "-sf") ||
          !strcmp(argv[iArgPtr], "--short-log-file"))
      {
        if (argc >= iArgPtr + 2)
        {
          g_logOutput.open(argv[iArgPtr + 1]);
          g_bShortLog = (!strcmp(argv[iArgPtr], "-sf") || !strcmp(argv[iArgPtr], "--short-log-file"));
          iArgPtr += 2;
        }
        else
        {
          cout << "== skipped log-file parameter: no file given ==" << endl;
          ++iArgPtr;
        }
      }
      else if (!strcmp(argv[iArgPtr], "-d") ||
          !strcmp(argv[iArgPtr], "--log-level"))
      {
        if (argc >= iArgPtr + 2)
        {
          int iNewLevel = atoi(argv[iArgPtr + 1]);
          if (iNewLevel >= CEC_LOG_ERROR && iNewLevel <= CEC_LOG_ALL)
          {
            g_cecLogLevel = iNewLevel;
            if (!g_bSingleCommand)
              cout << "log level set to " << argv[iArgPtr + 1] << endl;
          }
          else
          {
            cout << "== skipped log-level parameter: invalid level '" << argv[iArgPtr + 1] << "' ==" << endl;
          }
          iArgPtr += 2;
        }
        else
        {
          cout << "== skipped log-level parameter: no level given ==" << endl;
          ++iArgPtr;
        }
      }
      else if (!strcmp(argv[iArgPtr], "-t") ||
               !strcmp(argv[iArgPtr], "--type"))
      {
        if (argc >= iArgPtr + 2)
        {
          if (!strcmp(argv[iArgPtr + 1], "p"))
          {
            if (!g_bSingleCommand)
              cout << "== using device type 'playback device'" << endl;
            g_config.deviceTypes.Add(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
          }
          else if (!strcmp(argv[iArgPtr + 1], "r"))
          {
            if (!g_bSingleCommand)
              cout << "== using device type 'recording device'" << endl;
            g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
          }
          else if (!strcmp(argv[iArgPtr + 1], "t"))
          {
            if (!g_bSingleCommand)
              cout << "== using device type 'tuner'" << endl;
            g_config.deviceTypes.Add(CEC_DEVICE_TYPE_TUNER);
          }
          else if (!strcmp(argv[iArgPtr + 1], "a"))
          {
            if (!g_bSingleCommand)
              cout << "== using device type 'audio system'" << endl;
            g_config.deviceTypes.Add(CEC_DEVICE_TYPE_AUDIO_SYSTEM);
          }
          else
          {
            cout << "== skipped invalid device type '" << argv[iArgPtr + 1] << "'" << endl;
          }
          ++iArgPtr;
        }
        ++iArgPtr;
      }
      else if (!strcmp(argv[iArgPtr], "--info") ||
               !strcmp(argv[iArgPtr], "-i"))
      {
        if (g_cecLogLevel == -1)
          g_cecLogLevel = CEC_LOG_WARNING + CEC_LOG_ERROR;
        ICECAdapter *parser = LibCecInitialise(&g_config);
        if (parser)
        {
          CStdString strMessage;
          strMessage.Format("libCEC version: %s", parser->ToString((cec_server_version)g_config.serverVersion));
          if (g_config.serverVersion >= CEC_SERVER_VERSION_1_7_2)
            strMessage.AppendFormat(", %s", parser->GetLibInfo());
          PrintToStdOut(strMessage.c_str());
          UnloadLibCec(parser);
          parser = NULL;
        }
        bReturn = false;
      }
      else if (!strcmp(argv[iArgPtr], "--list-devices") ||
               !strcmp(argv[iArgPtr], "-l"))
      {
        if (g_cecLogLevel == -1)
          g_cecLogLevel = CEC_LOG_WARNING + CEC_LOG_ERROR;
        ICECAdapter *parser = LibCecInitialise(&g_config);
        if (parser)
        {
          ListDevices(parser);
          UnloadLibCec(parser);
          parser = NULL;
        }
        bReturn = false;
      }
      else if (!strcmp(argv[iArgPtr], "--bootloader"))
      {
        LibCecBootloader();
        bReturn = false;
      }
      else if (!strcmp(argv[iArgPtr], "--single-command") ||
          !strcmp(argv[iArgPtr], "-s"))
      {
        g_bSingleCommand = true;
        ++iArgPtr;
      }
      else if (!strcmp(argv[iArgPtr], "--help") ||
               !strcmp(argv[iArgPtr], "-h"))
      {
        if (g_cecLogLevel == -1)
          g_cecLogLevel = CEC_LOG_WARNING + CEC_LOG_ERROR;

        ShowHelpCommandLine(argv[0]);
        return 0;
      }
      else if (!strcmp(argv[iArgPtr], "-b") ||
               !strcmp(argv[iArgPtr], "--base"))
      {
        if (argc >= iArgPtr + 2)
        {
          g_config.baseDevice = (cec_logical_address)atoi(argv[iArgPtr + 1]);
          cout << "using base device '" << (int)g_config.baseDevice << "'" << endl;
          ++iArgPtr;
        }
        ++iArgPtr;
      }
      else if (!strcmp(argv[iArgPtr], "-p") ||
               !strcmp(argv[iArgPtr], "--port"))
      {
        if (argc >= iArgPtr + 2)
        {
          uint8_t hdmiport = (int8_t)atoi(argv[iArgPtr + 1]);
          if (hdmiport < 1)
              hdmiport = 1;
          if (hdmiport > 15)
              hdmiport = 15;
          g_config.iHDMIPort = hdmiport;
          cout << "using HDMI port '" << (int)g_config.iHDMIPort << "'" << endl;
          ++iArgPtr;
        }
        ++iArgPtr;
      }
      else if (!strcmp(argv[iArgPtr], "-r") ||
               !strcmp(argv[iArgPtr], "--rom"))
      {
        cout << "using settings from EEPROM" << endl;
        g_config.bGetSettingsFromROM = 1;
        ++iArgPtr;
      }
      else if (!strcmp(argv[iArgPtr], "-o") ||
               !strcmp(argv[iArgPtr], "--osd-name"))
      {
        if (argc >= iArgPtr + 2)
        {
          snprintf(g_config.strDeviceName, 13, "%s", argv[iArgPtr + 1]);
          cout << "using osd name " << g_config.strDeviceName << endl;
          ++iArgPtr;
        }
        ++iArgPtr;
      }
      else if (!strcmp(argv[iArgPtr], "-m") ||
               !strcmp(argv[iArgPtr], "--monitor"))
      {
        cout << "starting a monitor-only client. use 'mon 0' to switch to normal mode" << endl;
        g_config.bMonitorOnly = 1;
        ++iArgPtr;
      }
      else
      {
        g_strPort = argv[iArgPtr++];
      }
    }
  }

  return bReturn;
}

void sighandler(int iSignal)
{
  PrintToStdOut("signal caught: %d - exiting", iSignal);
  g_bExit = true;
}

int main (int argc, char *argv[])
{
  if (signal(SIGINT, sighandler) == SIG_ERR)
  {
    PrintToStdOut("can't register sighandler");
    return -1;
  }

  g_config.Clear();
  g_callbacks.Clear();
  snprintf(g_config.strDeviceName, 13, "CECTester");
  g_config.clientVersion       = CEC_CONFIG_VERSION;
  g_config.bActivateSource     = 0;
  g_callbacks.CBCecLogMessage  = &CecLogMessage;
  g_callbacks.CBCecKeyPress    = &CecKeyPress;
  g_callbacks.CBCecCommand     = &CecCommand;
  g_callbacks.CBCecAlert       = &CecAlert;
  g_config.callbacks           = &g_callbacks;

  if (!ProcessCommandLineArguments(argc, argv))
    return 0;

  if (g_cecLogLevel == -1)
    g_cecLogLevel = g_cecDefaultLogLevel;

  if (g_config.deviceTypes.IsEmpty())
  {
    if (!g_bSingleCommand)
      cout << "No device type given. Using 'recording device'" << endl;
    g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
  }

  g_parser = LibCecInitialise(&g_config);
  if (!g_parser)
  {
#ifdef __WINDOWS__
    cout << "Cannot load libcec.dll" << endl;
#else
    cout << "Cannot load libcec.so" << endl;
#endif

    if (g_parser)
      UnloadLibCec(g_parser);

    return 1;
  }

  // init video on targets that need this
  g_parser->InitVideoStandalone();

  if (!g_bSingleCommand)
  {
    CStdString strLog;
    strLog.Format("CEC Parser created - libCEC version %s", g_parser->ToString((cec_server_version)g_config.serverVersion));
    cout << strLog.c_str() << endl;

    //make stdin non-blocking
  #ifndef __WINDOWS__
    int flags = fcntl(0, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(0, F_SETFL, flags);
  #endif
  }

  if (g_strPort.IsEmpty())
  {
    if (!g_bSingleCommand)
      cout << "no serial port given. trying autodetect: ";
    cec_adapter devices[10];
    uint8_t iDevicesFound = g_parser->FindAdapters(devices, 10, NULL);
    if (iDevicesFound <= 0)
    {
      if (g_bSingleCommand)
        cout << "autodetect ";
      cout << "FAILED" << endl;
      UnloadLibCec(g_parser);
      return 1;
    }
    else
    {
      if (!g_bSingleCommand)
      {
        cout << endl << " path:     " << devices[0].path << endl <<
            " com port: " << devices[0].comm << endl << endl;
      }
      g_strPort = devices[0].comm;
    }
  }

  PrintToStdOut("opening a connection to the CEC adapter...");

  if (!g_parser->Open(g_strPort.c_str()))
  {
    PrintToStdOut("unable to open the device on port %s", g_strPort.c_str());
    UnloadLibCec(g_parser);
    return 1;
  }

  if (!g_bSingleCommand)
    PrintToStdOut("waiting for input");

  while (!g_bExit && !g_bHardExit)
  {
    string input;
    getline(cin, input);
    cin.clear();

    if (ProcessConsoleCommand(g_parser, input) && !g_bSingleCommand && !g_bExit && !g_bHardExit)
    {
      if (!input.empty())
        PrintToStdOut("waiting for input");
    }
    else
      g_bExit = true;

    if (!g_bExit && !g_bHardExit)
      CEvent::Sleep(50);
  }

  g_parser->Close();
  UnloadLibCec(g_parser);

  if (g_logOutput.is_open())
    g_logOutput.close();

  return 0;
}

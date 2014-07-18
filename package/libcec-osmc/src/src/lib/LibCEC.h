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

#include <string>
#include "cec.h"
#include "platform/util/buffer.h"
#include "CECTypeUtils.h"

namespace CEC
{
  class CAdapterCommunication;
  class CCECProcessor;
  class CCECClient;

  class CLibCEC : public ICECAdapter
  {
    public:
      CLibCEC(void);
      virtual ~CLibCEC(void);

      bool Open(const char *strPort, uint32_t iTimeout = CEC_DEFAULT_CONNECT_TIMEOUT);
      void Close(void);
      bool EnableCallbacks(void *cbParam, ICECCallbacks *callbacks);
      int8_t FindAdapters(cec_adapter *deviceList, uint8_t iBufSize, const char *strDevicePath = NULL);
      int8_t DetectAdapters(cec_adapter_descriptor *deviceList, uint8_t iBufSize, const char *strDevicePath = NULL, bool bQuickScan = false);
      bool PingAdapter(void);
      bool StartBootloader(void);

      bool Transmit(const cec_command &data);
      bool SetLogicalAddress(cec_logical_address iLogicalAddress = CECDEVICE_PLAYBACKDEVICE1);
      bool SetPhysicalAddress(uint16_t iPhysicalAddress = CEC_DEFAULT_PHYSICAL_ADDRESS);

      bool PowerOnDevices(cec_logical_address address = CECDEVICE_TV);
      bool StandbyDevices(cec_logical_address address = CECDEVICE_BROADCAST);
      bool SetActiveSource(cec_device_type type = CEC_DEVICE_TYPE_RESERVED);
      bool SetDeckControlMode(cec_deck_control_mode mode, bool bSendUpdate = true);
      bool SetDeckInfo(cec_deck_info info, bool bSendUpdate = true);
      bool SetInactiveView(void);
      bool SetMenuState(cec_menu_state state, bool bSendUpdate = true);
      bool SetOSDString(cec_logical_address iLogicalAddress, cec_display_control duration, const char *strMessage);
      bool SwitchMonitoring(bool bEnable);
      cec_version GetDeviceCecVersion(cec_logical_address iAddress);
      bool GetDeviceMenuLanguage(cec_logical_address iAddress, cec_menu_language *language);
      uint64_t GetDeviceVendorId(cec_logical_address iAddress);
      uint16_t GetDevicePhysicalAddress(cec_logical_address iAddress);
      cec_power_status GetDevicePowerStatus(cec_logical_address iAddress);
      bool PollDevice(cec_logical_address iAddress);
      cec_logical_addresses GetActiveDevices(void);
      bool IsActiveDevice(cec_logical_address iAddress);
      bool IsActiveDeviceType(cec_device_type type);
      bool SetHDMIPort(cec_logical_address iBaseDevice, uint8_t iPort = CEC_DEFAULT_HDMI_PORT);
      uint8_t VolumeUp(bool bSendRelease = true);
      uint8_t VolumeDown(bool bSendRelease = true);
      uint8_t MuteAudio(bool bSendRelease = true);
      bool SendKeypress(cec_logical_address iDestination, cec_user_control_code key, bool bWait = true);
      bool SendKeyRelease(cec_logical_address iDestination, bool bWait = true);
      cec_osd_name GetDeviceOSDName(cec_logical_address iAddress);
      cec_logical_address GetActiveSource(void);
      bool IsActiveSource(cec_logical_address iAddress);
      bool SetStreamPath(cec_logical_address iAddress);
      bool SetStreamPath(uint16_t iPhysicalAddress);
      cec_logical_addresses GetLogicalAddresses(void);
      bool GetCurrentConfiguration(libcec_configuration *configuration);
      bool SetConfiguration(const libcec_configuration *configuration);
      bool CanPersistConfiguration(void);
      bool PersistConfiguration(libcec_configuration *configuration);
      void RescanActiveDevices(void);
      bool IsLibCECActiveSource(void);

      const char *ToString(const cec_menu_state state)         { return CCECTypeUtils::ToString(state); }
      const char *ToString(const cec_version version)          { return CCECTypeUtils::ToString(version); }
      const char *ToString(const cec_power_status status)      { return CCECTypeUtils::ToString(status); }
      const char *ToString(const cec_logical_address address)  { return CCECTypeUtils::ToString(address); }
      const char *ToString(const cec_deck_control_mode mode)   { return CCECTypeUtils::ToString(mode); }
      const char *ToString(const cec_deck_info status)         { return CCECTypeUtils::ToString(status); }
      const char *ToString(const cec_opcode opcode)            { return CCECTypeUtils::ToString(opcode); }
      const char *ToString(const cec_system_audio_status mode) { return CCECTypeUtils::ToString(mode); }
      const char *ToString(const cec_audio_status status)      { return CCECTypeUtils::ToString(status); }
      const char *ToString(const cec_vendor_id vendor)         { return CCECTypeUtils::ToString(vendor); }
      const char *ToString(const cec_client_version version)   { return CCECTypeUtils::ToString(version); }
      const char *ToString(const cec_server_version version)   { return CCECTypeUtils::ToString(version); }
      const char *ToString(const cec_device_type type)         { return CCECTypeUtils::ToString(type); }
      const char *ToString(const cec_user_control_code key)    { return CCECTypeUtils::ToString(key); }
      const char *ToString(const cec_adapter_type type)        { return CCECTypeUtils::ToString(type); }

      static cec_device_type GetType(cec_logical_address address);
      static uint16_t GetMaskForType(cec_logical_address address);
      static uint16_t GetMaskForType(cec_device_type type);

      bool GetDeviceInformation(const char *strPort, libcec_configuration *config, uint32_t iTimeoutMs = CEC_DEFAULT_CONNECT_TIMEOUT);

      void AddLog(const cec_log_level level, const char *strFormat, ...);
      void AddCommand(const cec_command &command);
      void CheckKeypressTimeout(void);
      void Alert(const libcec_alert type, const libcec_parameter &param);

      static bool IsValidPhysicalAddress(uint16_t iPhysicalAddress);
      CCECClient *RegisterClient(libcec_configuration &configuration);
      void UnregisterClients(void);
      std::vector<CCECClient *> GetClients(void) { return m_clients; };
      const char *GetLibInfo(void);
      void InitVideoStandalone(void);
      uint16_t GetAdapterVendorId(void) const;
      uint16_t GetAdapterProductId(void) const;

      uint8_t AudioToggleMute(void);
      uint8_t AudioMute(void);
      uint8_t AudioUnmute(void);
      uint8_t AudioStatus(void);

      CCECProcessor *           m_cec;

    protected:
      int64_t                   m_iStartTime;
      PLATFORM::CMutex          m_mutex;
      CCECClient *              m_client;
      std::vector<CCECClient *> m_clients;
  };
};

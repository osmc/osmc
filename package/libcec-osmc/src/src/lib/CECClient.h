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
#include "platform/threads/mutex.h"
#include "platform/util/buffer.h"

namespace CEC
{
  class CCECProcessor;
  class CCECBusDevice;
  class CCECPlaybackDevice;

  class CCECClient
  {
    friend class CCECProcessor;

  public:
    CCECClient(CCECProcessor *processor, const libcec_configuration &configuration);
    virtual ~CCECClient(void);

    /*!
     * @return True when initialised and registered, false otherwise.
     */
    virtual bool IsInitialised(void);

    /*!
     * @return True when registered in the processor, false otherwise.
     */
    virtual bool IsRegistered(void);

    /*!
     * @return The primary logical address that this client is controlling.
     */
    virtual cec_logical_address GetPrimaryLogicalAdddress(void);

    /*!
     * @return The primary device that this client is controlling, or NULL if none.
     */
    virtual CCECBusDevice *GetPrimaryDevice(void);

    /*!
     * @return Get the playback device or recording device that this client is controlling, or NULL if none.
     */
    virtual CCECPlaybackDevice *GetPlaybackDevice(void);

    /*!
     * @brief Change one of the device types that this client is controlling into another.
     * @param from The type to change.
     * @param to The new value.
     * @return True when changed, false otherwise.
     */
    virtual bool ChangeDeviceType(const cec_device_type from, const cec_device_type to);

    /*!
     * @brief Get a device that this client is controlling, given it's type.
     * @param type The type of the device to get.
     * @return The requested device, or NULL if not found.
     */
    virtual CCECBusDevice *GetDeviceByType(const cec_device_type type) const;

    /*!
     * @brief Reset the physical address from the configuration.
     */
    virtual void ResetPhysicalAddress(void);

    /*!
     * @return A string that describes this client.
     */
    virtual std::string GetConnectionInfo(void);

    /*!
     * @return The current value of the TV vendor override setting.
     */
    virtual cec_vendor_id GetTVVendorOverride(void);

    /*!
     * @return The current value of the OSD name setting.
     */
    virtual std::string GetOSDName(void);

    /*!
     * @return Get the current value of the wake device setting.
     */
    virtual cec_logical_addresses GetWakeDevices(void);

    /*!
     * @return The version of this client.
     */
    virtual cec_client_version GetClientVersion(void);

    /*!
     * @return The device types that this client is controlling.
     */
    virtual cec_device_type_list GetDeviceTypes(void);

    // client-specific part of ICECAdapter
    virtual bool                  EnableCallbacks(void *cbParam, ICECCallbacks *callbacks);
    virtual bool                  PingAdapter(void);
    virtual bool                  Transmit(const cec_command &data, bool bIsReply);
    virtual bool                  SetLogicalAddress(const cec_logical_address iLogicalAddress);
    virtual bool                  SetPhysicalAddress(const uint16_t iPhysicalAddress);
    virtual bool                  SetHDMIPort(const cec_logical_address iBaseDevice, const uint8_t iPort, bool bForce = false);
    virtual bool                  SendPowerOnDevices(const cec_logical_address address = CECDEVICE_TV);
    virtual bool                  SendStandbyDevices(const cec_logical_address address = CECDEVICE_BROADCAST);
    virtual bool                  SendSetActiveSource(const cec_device_type type = CEC_DEVICE_TYPE_RESERVED);
    virtual bool                  SendSetDeckControlMode(const cec_deck_control_mode mode, bool bSendUpdate = true);
    virtual bool                  SendSetDeckInfo(const cec_deck_info info, bool bSendUpdate = true);
    virtual bool                  SendSetInactiveView(void);
    virtual bool                  SendSetMenuState(const cec_menu_state state, bool bSendUpdate = true);
    virtual bool                  SendSetOSDString(const cec_logical_address iLogicalAddress, const cec_display_control duration, const char *strMessage);
    virtual bool                  SwitchMonitoring(bool bEnable);
    virtual cec_version           GetDeviceCecVersion(const cec_logical_address iAddress);
    virtual bool                  GetDeviceMenuLanguage(const cec_logical_address iAddress, cec_menu_language &language);
    virtual uint64_t              GetDeviceVendorId(const cec_logical_address iAddress);
    virtual cec_power_status      GetDevicePowerStatus(const cec_logical_address iAddress);
    virtual uint16_t              GetDevicePhysicalAddress(const cec_logical_address iAddress);
    virtual bool                  PollDevice(const cec_logical_address iAddress);
    virtual cec_logical_addresses GetActiveDevices(void);
    virtual bool                  IsActiveDevice(const cec_logical_address iAddress);
    virtual bool                  IsActiveDeviceType(const cec_device_type type);
    virtual uint8_t               SendVolumeUp(bool bSendRelease = true);
    virtual uint8_t               SendVolumeDown(bool bSendRelease = true);
    virtual uint8_t               SendMuteAudio(void);
    virtual uint8_t               AudioToggleMute(void);
    virtual uint8_t               AudioMute(void);
    virtual uint8_t               AudioUnmute(void);
    virtual uint8_t               AudioStatus(void);
    virtual bool                  SendKeypress(const cec_logical_address iDestination, const cec_user_control_code key, bool bWait = true);
    virtual bool                  SendKeyRelease(const cec_logical_address iDestination, bool bWait = true);
    virtual cec_osd_name          GetDeviceOSDName(const cec_logical_address iAddress);
    virtual cec_logical_address   GetActiveSource(void);
    virtual bool                  IsActiveSource(const cec_logical_address iAddress);
    virtual bool                  SetStreamPath(const cec_logical_address iAddress);
    virtual bool                  SetStreamPath(const uint16_t iPhysicalAddress);
    virtual cec_logical_addresses GetLogicalAddresses(void);
    virtual void                  RescanActiveDevices(void);
    virtual bool                  IsLibCECActiveSource(void);

    // configuration
    virtual bool                  GetCurrentConfiguration(libcec_configuration &configuration);
    virtual bool                  SetConfiguration(const libcec_configuration &configuration);
    virtual bool                  CanPersistConfiguration(void);
    virtual bool                  PersistConfiguration(const libcec_configuration &configuration);
    virtual void                  SetPhysicalAddress(const libcec_configuration &configuration);

    // callbacks
    virtual void                  AddCommand(const cec_command &command);
    virtual int                   MenuStateChanged(const cec_menu_state newState);
    virtual void                  Alert(const libcec_alert type, const libcec_parameter &param) { CallbackAlert(type, param); }
    virtual void                  AddLog(const cec_log_message &message) { CallbackAddLog(message); }
    virtual void                  AddKey(bool bSendComboKey = false);
    virtual void                  AddKey(const cec_keypress &key);
    virtual void                  SetCurrentButton(const cec_user_control_code iButtonCode);
    virtual void                  CheckKeypressTimeout(void);
    virtual void                  SourceActivated(const cec_logical_address logicalAddress);
    virtual void                  SourceDeactivated(const cec_logical_address logicalAddress);

  protected:
    /*!
     * @brief Register this client in the processor
     * @return True when registered, false otherwise.
     */
    virtual bool OnRegister(void);

    /*!
     * @brief Called by the processor when this client is unregistered
     */
    virtual void OnUnregister(void) { SetRegistered(false); SetInitialised(false); }

    /*!
     * @brief Set the registered state of this client.
     * @param bSetTo The new value.
     */
    virtual void SetRegistered(bool bSetTo);

    /*!
     * @brief Set the initialised state of this client.
     * @param bSetTo The new value
     */
    virtual void SetInitialised(bool bSetTo);

    /*!
     * @brief Change the TV vendor id override setting.
     * @param id The new value.
     */
    virtual void SetTVVendorOverride(const cec_vendor_id id);

    /*!
     * @brief Change the OSD name of the primary device that this client is controlling.
     * @param strDeviceName The new value.
     */
    virtual void SetOSDName(const std::string &strDeviceName);

    /*!
     * @brief Change the value of the devices to wake.
     * @param addresses The new value.
     */
    virtual void SetWakeDevices(const cec_logical_addresses &addresses);

    /*!
     * @brief Change the value of the client version setting.
     * @param version The new version setting.
     */
    virtual void SetClientVersion(const cec_client_version version);

    /*!
     * @brief Change the device types that this client is controlling.
     * @param deviceTypes The new types.
     * @return True when the client needs to be re-registered to pick up the new setting, false otherwise.
     */
    virtual bool SetDeviceTypes(const cec_device_type_list &deviceTypes);

    /*!
     * @return A pointer to the current configuration of this client.
     */
    virtual libcec_configuration *GetConfiguration(void) { return &m_configuration; }

    /*!
     * @brief Called by the processor when registering this client to allocate the logical addresses.
     * @return True when the addresses for all types were allocated, false otherwise.
     */
    virtual bool AllocateLogicalAddresses(void);

    /*!
     * @brief Try to allocate a logical address for a recording device controlled by this client.
     * @return The logical address that was allocated, or CECDEVICE_UNKNOWN if none could be allocated.
     */
    virtual cec_logical_address AllocateLogicalAddressRecordingDevice(void);

    /*!
     * @brief Try to allocate a logical address for a tuner controlled by this client.
     * @return The logical address that was allocated, or CECDEVICE_UNKNOWN if none could be allocated.
     */
    virtual cec_logical_address AllocateLogicalAddressTuner(void);

    /*!
     * @brief Try to allocate a logical address for a playback device controlled by this client.
     * @return The logical address that was allocated, or CECDEVICE_UNKNOWN if none could be allocated.
     */
    virtual cec_logical_address AllocateLogicalAddressPlaybackDevice(void);

    /*!
     * @brief Try to allocate a logical address for an audiosystem controlled by this client.
     * @return The logical address that was allocated, or CECDEVICE_UNKNOWN if none could be allocated.
     */
    virtual cec_logical_address AllocateLogicalAddressAudioSystem(void);

    /*!
     * @brief Change the physical address of the devices controlled by this client.
     * @param iPhysicalAddress The new physical address.
     * @return True when changed, false otherwise.
     */
    virtual bool SetDevicePhysicalAddress(const uint16_t iPhysicalAddress);

    /*!
     * @brief Try to autodetect the physical address.
     * @return True when autodetected (and set in m_configuration), false otherwise.
     */
    virtual bool AutodetectPhysicalAddress(void);

    /*!
     * @brief Replaces all device types in m_configuration by types that are supported by the command handler of the TV
     */
    virtual void SetSupportedDeviceTypes(void);

    virtual void CallbackAddCommand(const cec_command &command);
    virtual void CallbackAddKey(const cec_keypress &key);
    virtual void CallbackAddLog(const cec_log_message &message);
    virtual void CallbackAlert(const libcec_alert type, const libcec_parameter &param);
    virtual void CallbackConfigurationChanged(const libcec_configuration &config);
    virtual int  CallbackMenuStateChanged(const cec_menu_state newState);
    virtual void CallbackSourceActivated(bool bActivated, const cec_logical_address logicalAddress);

    CCECProcessor *       m_processor;                         /**< a pointer to the processor */
    libcec_configuration  m_configuration;                     /**< the configuration of this client */
    bool                  m_bInitialised;                      /**< true when initialised, false otherwise */
    bool                  m_bRegistered;                       /**< true when registered in the processor, false otherwise */
    PLATFORM::CMutex      m_mutex;                             /**< mutex for changes to this instance */
    PLATFORM::CMutex      m_cbMutex;                           /**< mutex that is held when doing anything with callbacks */
    cec_user_control_code m_iCurrentButton;                    /**< the control code of the button that's currently held down (if any) */
    int64_t               m_buttontime;                        /**< the timestamp when the button was pressed (in seconds since epoch), or 0 if none was pressed. */
    int64_t               m_iPreventForwardingPowerOffCommand; /**< prevent forwarding standby commands until this time */
    int64_t               m_iLastKeypressTime;                 /**< last time a key press was sent to the client */
    cec_keypress          m_lastKeypress;                      /**< the last key press that was sent to the client */
  };
}

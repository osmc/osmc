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

#include "lib/platform/threads/mutex.h"
#include "USBCECAdapterMessage.h"

namespace CEC
{
  class CUSBCECAdapterCommunication;

  class CUSBCECAdapterCommands
  {
  public:
    CUSBCECAdapterCommands(CUSBCECAdapterCommunication *comm);

    /*!
     * @brief Request the firmware version from the adapter.
     * @return The firmware version, or 1 (default) if it couldn't be retrieved.
     */
    uint16_t RequestFirmwareVersion(void);

    /*!
     * @return The firmware version of the adapter, retrieved when the connection is opened.
     */
    uint16_t GetFirmwareVersion(void) const { return m_persistedConfiguration.iFirmwareVersion; };

    /*!
     * @brief Update the current configuration in the adapter. Does not do an eeprom update.
     * @attention Not all settings are persisted at this time.
     * @param configuration The configuration to persist.
     * @return True when something changed, false otherwise.
     */
    bool PersistConfiguration(const libcec_configuration &configuration);

    /*!
     * @brief Get the persisted configuration from the EEPROM.
     * @param configuration The persisted configuration.
     * @return True when retrieved, false otherwise.
     */
    bool GetConfiguration(libcec_configuration &configuration);

    /*!
     * @brief Send a ping command to the adapter.
     * @return True when acked by the adapter, false otherwise.
     */
    bool PingAdapter(void);

    /*!
     * @brief Change the ackmask of the adapter.
     * @param iMask The new mask.
     * @return True when the change was acked by the adapter, false otherwise.
     */
    bool SetAckMask(uint16_t iMask);

    /*!
     * @brief Put the adapter in bootloader mode.
     * @attention The connection needs to be closed after this call, since the adapter will no longer be available.
     * @return True when the command was sent, false otherwise.
     */
    bool StartBootloader(void);

    /*!
     * @brief Change the current CEC line timeout.
     * @param iTimeout The new timeout.
     * @return True when the change was acked by the adapter, false otherwise.
     */
    bool SetLineTimeout(uint8_t iTimeout);

    /*!
     * @brief Put the adapter in controlled or autonomous mode.
     * @param controlled True to switch to controlled mode, false to switch to auto mode.
     * @return True when acked by the controller, false otherwise.
     */
    bool SetControlledMode(bool controlled);

    /*!
     * @brief Request the firmware build date from the device.
     * @return The build date in seconds since epoch, or 0 when no (valid) reply was received.
     */
    uint32_t RequestBuildDate(void);

    /*!
     * @return The persisted build date.
     */
    uint32_t GetPersistedBuildDate(void) const { return m_iBuildDate; };

    /*!
     * @brief Request the adapter type.
     * @return The type
     */
    p8_cec_adapter_type RequestAdapterType(void);

    /*!
     * @return The persisted build date.
     */
    p8_cec_adapter_type GetPersistedAdapterType(void) const { return m_adapterType; };

    /*!
     * @brief Persist the current settings in the EEPROM
     * @return True when persisted, false otherwise.
     */
    bool WriteEEPROM(void);

    void SetActiveSource(bool bSetTo, bool bClientUnregistered);

  private:
    /*!
     * @brief Reads all settings from the eeprom.
     * @return True when read, false otherwise.
     */
    bool RequestSettings(void);

    /*!
     * @brief Request a setting value from the adapter.
     * @param msgCode The setting to retrieve.
     * @return The response from the adapter.
     */
    cec_datapacket RequestSetting(cec_adapter_messagecode msgCode);

    /*!
     * @brief Change the value of the "auto enabled" setting.
     * @param enabled The new value.
     * @return True when changed and set, false otherwise.
     */
    bool SetSettingAutoEnabled(bool enabled);

    /*!
     * @brief Request the value of the "auto enabled" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingAutoEnabled(void);

    /*!
     * @brief Change the value of the "device type" setting, used when the device is in autonomous mode.
     * @param type The new value.
     * @return True when changed and set, false otherwise.
     */
    bool SetSettingDeviceType(cec_device_type type);

    /*!
     * @brief Request the value of the "device type" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingDeviceType(void);

    /*!
     * @brief Change the value of the "default logical address" setting, used when the device is in autonomous mode.
     * @param address The new value.
     * @return True when changed and set, false otherwise.
     */
    bool SetSettingDefaultLogicalAddress(cec_logical_address address);

    /*!
     * @brief Request the value of the "default logical address" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingDefaultLogicalAddress(void);

    /*!
     * @brief Change the value of the "logical address mask" setting, used when the device is in autonomous mode.
     * @param iMask The new value.
     * @return True when changed and set, false otherwise.
     */
    bool SetSettingLogicalAddressMask(uint16_t iMask);

    /*!
     * @brief Request the value of the "logical address mask" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingLogicalAddressMask(void);

    /*!
     * @brief Change the value of the "physical address" setting, used when the device is in autonomous mode.
     * @param iPhysicalAddress The new value.
     * @return True when changed and set, false otherwise.
     */
    bool SetSettingPhysicalAddress(uint16_t iPhysicalAddress);

    /*!
     * @brief Request the value of the "physical address" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingPhysicalAddress(void);

    /*!
     * @brief Change the value of the "CEC version" setting, used when the device is in autonomous mode.
     * @param version The new value.
     * @return True when changed and set, false otherwise.
     */
    bool SetSettingCECVersion(cec_version version);

    /*!
     * @brief Request the value of the "CEC version" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingCECVersion(void);

    /*!
     * @brief Change the value of the "OSD name" setting, used when the device is in autonomous mode.
     * @param strOSDName The new value.
     * @return True when set, false otherwise.
     */
    bool SetSettingOSDName(const char *strOSDName);

    /*!
     * @brief Request the value of the "OSD name" setting from the adapter.
     * @return True when retrieved, false otherwise.
     */
    bool RequestSettingOSDName(void);

    CUSBCECAdapterCommunication *m_comm;                   /**< the communication handler */
    bool                         m_bSettingsRetrieved;     /**< true when the settings were read from the eeprom, false otherwise */
    bool                         m_bSettingAutoEnabled;    /**< the value of the auto-enabled setting */
    cec_version                  m_settingCecVersion;      /**< the value of the cec version setting */
    uint16_t                     m_iSettingLAMask;         /**< the value of the LA mask setting */
    bool                         m_bNeedsWrite;            /**< true when we sent changed settings to the adapter that have not been persisted */
    libcec_configuration         m_persistedConfiguration; /**< the configuration that is persisted in the eeprom */
    uint32_t                     m_iBuildDate;             /**< the build date of the firmware */
    bool                         m_bControlledMode;        /**< current value of the controlled mode feature */
    p8_cec_adapter_type          m_adapterType;            /**< the type of the adapter that we're connected to */
    PLATFORM::CMutex             m_mutex;
  };
}

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

#ifndef CECEXPORTS_H_
#define CECEXPORTS_H_

#include "cectypes.h"

#define LIBCEC_VERSION_CURRENT CEC_SERVER_VERSION_CURRENT

namespace CEC
{
  /*!
   * To create a new libCEC instance, call CECInitialise() and pass the
   * configuration as argument. Then call Open() to open a connection to the
   * adapter. Close() closes the connection and CECDestroy() cleans up the
   * libCEC instance.
   *
   * libCEC can send commands to other devices on the CEC bus via the methods
   * on this interface, and all commands that libCEC received are sent back
   * to the application via callback methods. The callback methods can be
   * found in cectypes.h, ICECCallbacks.
   */
  class ICECAdapter
  {
  public:
    virtual ~ICECAdapter() {};
    /*! @name Adapter methods */
    //@{

    /*!
     * @brief Open a connection to the CEC adapter.
     * @param strPort The path to the port.
     * @param iTimeoutMs Connection timeout in ms.
     * @return True when connected, false otherwise.
     */
    virtual bool Open(const char *strPort, uint32_t iTimeoutMs = 10000) = 0;

    /*!
     * @brief Close the connection to the CEC adapter.
     */
    virtual void Close(void) = 0;

    /*!
     * @deprecated Use DetectAdapters() instead
     * @brief Try to find all connected CEC adapters.
     * @param deviceList The vector to store device descriptors in.
     * @param iBufSize The size of the deviceList buffer.
     * @param strDevicePath Optional device path. Only adds device descriptors that match the given device path.
     * @return The number of devices that were found, or -1 when an error occured.
     */
    virtual int8_t FindAdapters(cec_adapter *deviceList, uint8_t iBufSize, const char *strDevicePath = NULL) = 0;

    /*!
     * @brief Sends a ping command to the adapter, to check if it's responding.
     * @return True when the ping was succesful, false otherwise.
     */
    virtual bool PingAdapter(void) = 0;

    /*!
     * @brief Start the bootloader of the CEC adapter. Closes the connection when successful.
     * @return True when the command was sent successfully, false otherwise.
     */
    virtual bool StartBootloader(void) = 0;
    //@}

    /*!
     * @brief Transmit a raw CEC command over the CEC line.
     * @param data The command to send.
     * @return True when the data was sent and acked, false otherwise.
     */
    virtual bool Transmit(const cec_command &data) = 0;

    /*!
     * @brief Change the logical address on the CEC bus of the CEC adapter. libCEC automatically assigns a logical address, and this method is only available for debugging purposes.
     * @param iLogicalAddress The CEC adapter's new logical address.
     * @return True when the logical address was set successfully, false otherwise.
     */
    virtual bool SetLogicalAddress(cec_logical_address iLogicalAddress = CECDEVICE_PLAYBACKDEVICE1) = 0;

    /*!
     * @brief Change the physical address (HDMI port) of the CEC adapter. libCEC will try to autodetect the physical address when connecting. If it did, it's set in libcec_configuration.
     * @param iPhysicalAddress The CEC adapter's new physical address.
     * @brief True when the physical address was set successfully, false otherwise.
     */
    virtual bool SetPhysicalAddress(uint16_t iPhysicalAddress = CEC_DEFAULT_PHYSICAL_ADDRESS) = 0;

    /*!
     * @brief Power on the given CEC capable devices. If CECDEVICE_BROADCAST is used, then wakeDevice in libcec_configuration will be used.
     * @param address The logical address to power on.
     * @return True when the command was sent succesfully, false otherwise.
     */
    virtual bool PowerOnDevices(cec_logical_address address = CECDEVICE_TV) = 0;

    /*!
     * @brief Put the given CEC capable devices in standby mode. If CECDEVICE_BROADCAST is used, then standbyDevices in libcec_configuration will be used.
     * @brief address The logical address of the device to put in standby.
     * @return True when the command was sent succesfully, false otherwise.
     */
    virtual bool StandbyDevices(cec_logical_address address = CECDEVICE_BROADCAST) = 0;

    /*!
     * @brief Change the active source to a device type handled by libCEC. Use CEC_DEVICE_TYPE_RESERVED to make the default type used by libCEC active.
     * @param type The new active source. Leave empty to use the primary type
     * @return True when the command was sent succesfully, false otherwise.
     */
    virtual bool SetActiveSource(cec_device_type type = CEC_DEVICE_TYPE_RESERVED) = 0;

    /*!
     * @brief Change the deck control mode, if this adapter is registered as playback or recording device.
     * @param mode The new control mode.
     * @param bSendUpdate True to send the new status over the CEC line.
     * @return True if set, false otherwise.
     */
    virtual bool SetDeckControlMode(cec_deck_control_mode mode, bool bSendUpdate = true) = 0;

    /*!
     * @brief Change the deck info, if this adapter is a playback or recording device.
     * @param info The new deck info.
     * @param bSendUpdate True to send the new status over the CEC line.
     * @return True if set, false otherwise.
     */
    virtual bool SetDeckInfo(cec_deck_info info, bool bSendUpdate = true) = 0;

    /*!
     * @brief Broadcast a message that notifies connected CEC capable devices that this device is no longer the active source.
     * @return True when the command was sent succesfully, false otherwise.
     */
    virtual bool SetInactiveView(void) = 0;

    /*!
     * @brief Change the menu state. This value is already changed by libCEC automatically if a device is (de)activated.
     * @param state The new state.
     * @param bSendUpdate True to send the new status over the CEC line.
     * @return True if set, false otherwise.
     */
    virtual bool SetMenuState(cec_menu_state state, bool bSendUpdate = true) = 0;

    /*!
     * @brief Display a message on the device with the given logical address. Not supported by most TVs.
     * @param iLogicalAddress The logical address of the device to display the message on.
     * @param duration The duration of the message
     * @param strMessage The message to display.
     * @return True when the command was sent, false otherwise.
     */
    virtual bool SetOSDString(cec_logical_address iLogicalAddress, cec_display_control duration, const char *strMessage) = 0;

    /*!
     * @brief Enable or disable monitoring mode, for debugging purposes. If monitoring mode is enabled, libCEC won't respond to any command, but only log incoming data.
     * @param bEnable True to enable, false to disable.
     * @return True when switched successfully, false otherwise.
     */
    virtual bool SwitchMonitoring(bool bEnable) = 0;

    /*!
     * @brief Get the CEC version of the device with the given logical address
     * @param iLogicalAddress The logical address of the device to get the CEC version for.
     * @return The version or CEC_VERSION_UNKNOWN when the version couldn't be fetched.
     */
    virtual cec_version GetDeviceCecVersion(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Get the menu language of the device with the given logical address
     * @param iLogicalAddress The logical address of the device to get the menu language for.
     * @param language The requested menu language.
     * @return True when fetched succesfully, false otherwise.
     */
    virtual bool GetDeviceMenuLanguage(cec_logical_address iLogicalAddress, cec_menu_language *language) = 0;

    /*!
     * @brief Get the vendor ID of the device with the given logical address.
     * @param iLogicalAddress The logical address of the device to get the vendor ID for.
     * @return The vendor ID or 0 if it wasn't found.
     */
    virtual uint64_t GetDeviceVendorId(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Get the power status of the device with the given logical address.
     * @param iLogicalAddress The logical address of the device to get the power status for.
     * @return The power status or CEC_POWER_STATUS_UNKNOWN if it wasn't found.
     */
    virtual cec_power_status GetDevicePowerStatus(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Sends a POLL message to a device, to check if it's present and responding.
     * @param iLogicalAddress The device to send the message to.
     * @return True if the POLL was acked, false otherwise.
     */
    virtual bool PollDevice(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @return The logical addresses of the devices that are active on the bus, including those handled by libCEC.
     */
    virtual cec_logical_addresses GetActiveDevices(void) = 0;

    /*!
     * @brief Check whether a device is active on the bus.
     * @param iLogicalAddress The address to check.
     * @return True when active, false otherwise.
     */
    virtual bool IsActiveDevice(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Check whether a device of the given type is active on the bus.
     * @param type The type to check.
     * @return True when active, false otherwise.
     */
    virtual bool IsActiveDeviceType(cec_device_type type) = 0;

    /*!
     * @brief Sends a volume up keypress to an audiosystem if it's present.
     * @param bSendRelease Send a key release after the keypress.
     * @return The new audio status.
     */
    virtual uint8_t VolumeUp(bool bSendRelease = true) = 0;

    /*!
     * @brief Sends a volume down keypress to an audiosystem if it's present.
     * @param bSendRelease Send a key release after the keypress.
     * @return The new audio status.
     */
    virtual uint8_t VolumeDown(bool bSendRelease = true) = 0;

    /*!
     * @deprecated Use AudioToggleMute() instead
     * @brief Sends a mute keypress to an audiosystem if it's present.
     * @param bSendRelease Send a key release after the keypress.
     * @return The new audio status.
     */
    virtual uint8_t MuteAudio(bool bSendRelease = true) = 0;

    /*!
     * @brief Send a keypress to a device on the CEC bus.
     * @param iDestination The logical address of the device to send the message to.
     * @param key The key to send.
     * @param bWait True to wait for a response, false otherwise.
     * @return True when the keypress was acked, false otherwise.
     */
    virtual bool SendKeypress(cec_logical_address iDestination, cec_user_control_code key, bool bWait = false) = 0;

    /*!
     * @brief Send a key release to a device on the CEC bus.
     * @param iDestination The logical address of the device to send the message to.
     * @param bWait True to wait for a response, false otherwise.
     * @return True when the key release was acked, false otherwise.
     */
    virtual bool SendKeyRelease(cec_logical_address iDestination, bool bWait = false) = 0;

    /*!
     * @brief Get the OSD name of a device on the CEC bus.
     * @param iLogicalAddress The device to get the OSD name for.
     * @return The OSD name.
     */
    virtual cec_osd_name GetDeviceOSDName(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Get the logical address of the device that is currently the active source on the CEC bus.
     * @return The active source or CECDEVICE_UNKNOWN when unknown.
     */
    virtual cec_logical_address GetActiveSource(void) = 0;

    /*!
     * @brief Check whether a device is currently the active source on the CEC bus.
     * @param iLogicalAddress The logical address of the device to check.
     * @return True when it is the active source, false otherwise.
     */
    virtual bool IsActiveSource(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Sets the stream path to the device on the given logical address.
     * @param iLogicalAddress The address to activate.
     * @return True when the command was sent, false otherwise.
     */
    virtual bool SetStreamPath(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @brief Sets the stream path to the device on the given physical address.
     * @param iPhysicalAddress The address to activate.
     * @return True when the command was sent, false otherwise.
     */
    virtual bool SetStreamPath(uint16_t iPhysicalAddress) = 0;

    /*!
     * @return The list of logical addresses that libCEC is controlling
     */
    virtual cec_logical_addresses GetLogicalAddresses(void) = 0;

    /*!
     * @brief Get libCEC's current configuration.
     * @param configuration The configuration.
     * @return True when the configuration was updated, false otherwise.
     */
    virtual bool GetCurrentConfiguration(libcec_configuration *configuration) = 0;

    /*!
     * @brief Change libCEC's configuration.
     * @param configuration The new configuration.
     * @return True when the configuration was changed successfully, false otherwise.
     */
    virtual bool SetConfiguration(const libcec_configuration *configuration) = 0;

    /*!
     * @return True when this CEC adapter can persist the user configuration, false otherwise.
     */
    virtual bool CanPersistConfiguration(void) = 0;

    /*!
     * @brief Persist the given configuration in adapter (if supported)
     * @brief configuration The configuration to store.
     * @return True when the configuration was persisted, false otherwise.
     */
    virtual bool PersistConfiguration(libcec_configuration *configuration) = 0;

    /*!
     * @brief Tell libCEC to poll for active devices on the bus.
     */
    virtual void RescanActiveDevices(void) = 0;

    /*!
     * @return true when libCEC is the active source on the bus, false otherwise.
     */
    virtual bool IsLibCECActiveSource(void) = 0;

    /*!
     * @brief Get information about the given CEC adapter.
     * @param strPort The port to which the device is connected
     * @param config The device configuration
     * @param iTimeoutMs The timeout in milliseconds
     * @return True when the device was found, false otherwise
     */
    virtual bool GetDeviceInformation(const char *strPort, libcec_configuration *config, uint32_t iTimeoutMs = 10000) = 0;

    /*!
     * @brief Set and enable the callback methods. If this method is not called, the GetNext...() methods will have to be used.
     * @param cbParam Parameter to pass to callback methods.
     * @param callbacks The callbacks to set.
     * @return True when enabled, false otherwise.
     */
    virtual bool EnableCallbacks(void *cbParam, ICECCallbacks *callbacks) = 0;

    /*!
     * @brief Changes the active HDMI port.
     * @param iBaseDevice The device to which this libCEC is connected.
     * @param iPort The new port number.
     * @return True when changed, false otherwise.
     */
    virtual bool SetHDMIPort(cec_logical_address iBaseDevice, uint8_t iPort) = 0;

    /*!
     * @brief Get the physical address of the device with the given logical address.
     * @param iLogicalAddress The logical address of the device to get the physical address for.
     * @return The physical address or 0 if it wasn't found.
     */
    virtual uint16_t GetDevicePhysicalAddress(cec_logical_address iLogicalAddress) = 0;

    /*!
     * @return A string with information about how libCEC was compiled.
     */
    virtual const char *GetLibInfo(void) = 0;

    /*!
     * @brief Calling this method will initialise the host on which libCEC is running.
     * Calling this method will initialise the host on which libCEC is running. On the RPi, it calls
     * bcm_host_init(), which may only be called once per process, and is called by any process using
     * the video api on that system. So only call this method if libCEC is used in an application that
     * does not already initialise the video api.
     *
     * Should be called as first call to libCEC, directly after CECInitialise() and before using Open()
     */
    virtual void InitVideoStandalone(void) = 0;

    /*!
     * @return The (virtual) USB vendor id
     */
    virtual uint16_t GetAdapterVendorId(void) const = 0;

    /*!
     * @return The (virtual) USB product id
     */
    virtual uint16_t GetAdapterProductId(void) const = 0;

    virtual const char *ToString(const cec_menu_state state) = 0;
    virtual const char *ToString(const cec_version version) = 0;
    virtual const char *ToString(const cec_power_status status) = 0;
    virtual const char *ToString(const cec_logical_address address) = 0;
    virtual const char *ToString(const cec_deck_control_mode mode) = 0;
    virtual const char *ToString(const cec_deck_info status) = 0;
    virtual const char *ToString(const cec_opcode opcode) = 0;
    virtual const char *ToString(const cec_system_audio_status mode) = 0;
    virtual const char *ToString(const cec_audio_status status) = 0;
    virtual const char *ToString(const cec_vendor_id vendor) = 0;
    virtual const char *ToString(const cec_client_version version) = 0;
    virtual const char *ToString(const cec_server_version version) = 0;
    virtual const char *ToString(const cec_user_control_code key) = 0;
    virtual const char *ToString(const cec_adapter_type type) = 0;

    /*!
     * @brief Toggle the mute status of the AVR (if present)
     * @return The new audio status.
     */
    virtual uint8_t AudioToggleMute(void) = 0;

    /*!
     * @brief Mute the AVR (if present)
     * @return The new audio status.
     */
    virtual uint8_t AudioMute(void) = 0;

    /*!
     * @brief Mute the AVR (if connected)
     * @return The new audio status.
     */
    virtual uint8_t AudioUnmute(void) = 0;

    /*!
     * @brief Get the current audio status (if an AVR is connected)
     * @return The current audio status, or cec_audio_status if unknown.
     */
    virtual uint8_t AudioStatus(void) = 0;

    /*!
     * @brief Try to find all connected CEC adapters.
     * @param deviceList The vector to store device descriptors in.
     * @param iBufSize The size of the deviceList buffer.
     * @param strDevicePath Optional device path. Only adds device descriptors that match the given device path.
     * @param bQuickScan True to do a "quick scan", which will not open a connection to the adapter. Firmware version information and the exact device type will be missing
     * @return The number of devices that were found, or -1 when an error occured.
     */
    virtual int8_t DetectAdapters(cec_adapter_descriptor *deviceList, uint8_t iBufSize, const char *strDevicePath = NULL, bool bQuickScan = false) = 0;

  };
};

/*!
 * @brief Unload the CEC adapter library.
 */
extern "C" DECLSPEC void CECDestroy(CEC::ICECAdapter *instance);

/*!
 * @brief Load the CEC adapter library.
 * @param configuration The configuration to pass to libCEC
 * @return An instance of ICECAdapter or NULL on error.
 */
extern "C" DECLSPEC void * CECInitialise(CEC::libcec_configuration *configuration);

/*!
 * @brief Try to connect to the adapter and send the "start bootloader" command, without initialising libCEC and going through all checks
 * @return True when the command was send, false otherwise.
 */
extern "C" DECLSPEC bool CECStartBootloader(void);

#endif /* CECEXPORTS_H_ */

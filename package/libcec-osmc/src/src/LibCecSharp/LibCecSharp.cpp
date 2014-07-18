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

#include "CecSharpTypes.h"
#using <System.dll>

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace CEC;
using namespace msclr::interop;

namespace CecSharp
{
  /// <summary>
  /// Create a LibCecSharp instance and pass the configuration as argument.
  /// Then call Open() to open a connection to the adapter. Close() closes the
  /// connection.
  ///
  /// libCEC can send commands to other devices on the CEC bus via the methods
  /// on this interface, and all commands that libCEC received are sent back
  /// to the application via callback methods. The callback methods can be
  /// found in CecSharpTypes.h, CecCallbackMethods.
  /// </summary>
  public ref class LibCecSharp : public CecCallbackMethods
  {
  public:
    /// <summary>
    /// Create a new LibCecSharp instance.
    /// </summary>
    /// <param name="config">The configuration to pass to libCEC.</param>
    LibCecSharp(LibCECConfiguration ^config)
    {
      m_callbacks = config->Callbacks;
      CecCallbackMethods::EnableCallbacks(m_callbacks);
      if (!InitialiseLibCec(config))
        throw gcnew Exception("Could not initialise LibCecSharp");
    }

    ~LibCecSharp(void)
    {
      Close();
      m_libCec = NULL;
    }

    /// <summary>
    /// Try to find all connected CEC adapters.
    /// </summary>
    /// <param name="path">The path filter for adapters. Leave empty to return all adapters.</param>
    /// <returns>The adapters that were found.</returns>
    array<CecAdapter ^> ^ FindAdapters(String ^ path)
    {
      cec_adapter *devices = new cec_adapter[10];

      marshal_context ^ context = gcnew marshal_context();
      const char* strPathC = path->Length > 0 ? context->marshal_as<const char*>(path) : NULL;

      uint8_t iDevicesFound = m_libCec->FindAdapters(devices, 10, NULL);

      array<CecAdapter ^> ^ adapters = gcnew array<CecAdapter ^>(iDevicesFound);
      for (unsigned int iPtr = 0; iPtr < iDevicesFound; iPtr++)
        adapters[iPtr] = gcnew CecAdapter(gcnew String(devices[iPtr].path), gcnew String(devices[iPtr].comm));

      delete devices;
      delete context;
      return adapters;
    }

    /// <summary>
    /// Open a connection to the CEC adapter.
    /// </summary>
    /// <param name="strPort">The COM port of the adapter</param>
    /// <param name="iTimeoutMs">Connection timeout in milliseconds</param>
    /// <returns>True when a connection was opened, false otherwise.</returns>
    bool Open(String ^ strPort, int iTimeoutMs)
    {
      CecCallbackMethods::EnableCallbacks(m_callbacks);
      EnableCallbacks(m_callbacks);
      marshal_context ^ context = gcnew marshal_context();
      const char* strPortC = context->marshal_as<const char*>(strPort);
      bool bReturn = m_libCec->Open(strPortC, iTimeoutMs);
      delete context;
      return bReturn;
    }

    /// <summary>
    /// Close the connection to the CEC adapter
    /// </summary>
    void Close(void)
    {
      DisableCallbacks();
      m_libCec->Close();
    }

    /// <summary>
    /// Disable all calls to callback methods.
    /// </summary>
    virtual void DisableCallbacks(void) override
    {
      // delete the callbacks, since these might already have been destroyed in .NET
      CecCallbackMethods::DisableCallbacks();
      if (m_libCec)
        m_libCec->EnableCallbacks(NULL, NULL);
    }

    /// <summary>
    /// Enable or change the callback methods that libCEC uses to send changes to the client application.
    /// </summary>
    /// <param name="callbacks">The new callback methods to use.</param>
    /// <returns>True when the callbacks were changed, false otherwise</returns>
    virtual bool EnableCallbacks(CecCallbackMethods ^ callbacks) override
    {
      if (m_libCec && CecCallbackMethods::EnableCallbacks(callbacks))
        return m_libCec->EnableCallbacks((void*)GetCallbackPtr(), &g_cecCallbacks);

      return false;
    }

    /// <summary>
    /// Sends a ping command to the adapter, to check if it's responding.
    /// </summary>
    /// <returns>True when the ping was succesful, false otherwise</returns>
    bool PingAdapter(void)
    {
      return m_libCec->PingAdapter();
    }

    /// <summary>
    /// Start the bootloader of the CEC adapter. Closes the connection when successful.
    /// </summary>
    /// <returns>True when the command was sent successfully, false otherwise.</returns>
    bool StartBootloader(void)
    {
      return m_libCec->StartBootloader();
    }

    /// <summary>
    /// Transmit a raw CEC command over the CEC line.
    /// </summary>
    /// <param name="command">The command to transmit</param>
    /// <returns>True when the data was sent and acked, false otherwise.</returns>
    bool Transmit(CecCommand ^ command)
    {
      cec_command ccommand;
      cec_command::Format(ccommand, (cec_logical_address)command->Initiator, (cec_logical_address)command->Destination, (cec_opcode)command->Opcode);
      ccommand.transmit_timeout = command->TransmitTimeout;
      ccommand.eom              = command->Eom;
      ccommand.ack              = command->Ack;
      for (unsigned int iPtr = 0; iPtr < command->Parameters->Size; iPtr++)
        ccommand.parameters.PushBack(command->Parameters->Data[iPtr]);

      return m_libCec->Transmit(ccommand);
    }

    /// <summary>
    /// Change the logical address on the CEC bus of the CEC adapter. libCEC automatically assigns a logical address, and this method is only available for debugging purposes.
    /// </summary>
    /// <param name="logicalAddress">The CEC adapter's new logical address.</param>
    /// <returns>True when the logical address was set successfully, false otherwise.</returns>
    bool SetLogicalAddress(CecLogicalAddress logicalAddress)
    {
      return m_libCec->SetLogicalAddress((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Change the physical address (HDMI port) of the CEC adapter. libCEC will try to autodetect the physical address when connecting. If it did, it's set in libcec_configuration.
    /// </summary>
    /// <param name="physicalAddress">The CEC adapter's new physical address.</param>
    /// <returns>True when the physical address was set successfully, false otherwise.</returns>
    bool SetPhysicalAddress(uint16_t physicalAddress)
    {
      return m_libCec->SetPhysicalAddress(physicalAddress);
    }

    /// <summary>
    /// Power on the given CEC capable devices. If CECDEVICE_BROADCAST is used, then wakeDevice in libcec_configuration will be used.
    /// </summary>
    /// <param name="logicalAddress">The logical address to power on.</param>
    /// <returns>True when the command was sent succesfully, false otherwise.</returns>
    bool PowerOnDevices(CecLogicalAddress logicalAddress)
    {
      return m_libCec->PowerOnDevices((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Put the given CEC capable devices in standby mode. If CECDEVICE_BROADCAST is used, then standbyDevices in libcec_configuration will be used.
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to put in standby.</param>
    /// <returns>True when the command was sent succesfully, false otherwise.</returns>
    bool StandbyDevices(CecLogicalAddress logicalAddress)
    {
      return m_libCec->StandbyDevices((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Sends a POLL message to a device, to check if it's present and responding.
    /// </summary>
    /// <param name="logicalAddress">The device to send the message to.</param>
    /// <returns>True if the POLL was acked, false otherwise.</returns>
    bool PollDevice(CecLogicalAddress logicalAddress)
    {
      return m_libCec->PollDevice((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Change the active source to a device type handled by libCEC. Use CEC_DEVICE_TYPE_RESERVED to make the default type used by libCEC active.
    /// </summary>
    /// <param name="type">The new active source. Use CEC_DEVICE_TYPE_RESERVED to use the primary type</param>
    /// <returns>True when the command was sent succesfully, false otherwise.</returns>
    bool SetActiveSource(CecDeviceType type)
    {
      return m_libCec->SetActiveSource((cec_device_type) type);
    }

    /// <summary>
    /// Change the deck control mode, if this adapter is registered as playback or recording device.
    /// </summary>
    /// <param name="mode">The new control mode.</param>
    /// <param name="sendUpdate">True to send the new status over the CEC line.</param>
    /// <returns>True if set, false otherwise.</returns>
    bool SetDeckControlMode(CecDeckControlMode mode, bool sendUpdate)
    {
      return m_libCec->SetDeckControlMode((cec_deck_control_mode) mode, sendUpdate);
    }

    /// <summary>
    /// Change the deck info, if this adapter is a playback or recording device.
    /// </summary>
    /// <param name="info">The new deck info.</param>
    /// <param name="sendUpdate">True to send the new status over the CEC line.</param>
    /// <returns>True if set, false otherwise.</returns>
    bool SetDeckInfo(CecDeckInfo info, bool sendUpdate)
    {
      return m_libCec->SetDeckInfo((cec_deck_info) info, sendUpdate);
    }

    /// <summary>
    /// Broadcast a message that notifies connected CEC capable devices that this device is no longer the active source.
    /// </summary>
    /// <returns>True when the command was sent succesfully, false otherwise.</returns>
    bool SetInactiveView(void)
    {
      return m_libCec->SetInactiveView();
    }

    /// <summary>
    /// Change the menu state. This value is already changed by libCEC automatically if a device is (de)activated.
    /// </summary>
    /// <param name="state">The new state.</param>
    /// <param name="sendUpdate">True to send the new status over the CEC line.</param>
    /// <returns>True if set, false otherwise.</returns>
    bool SetMenuState(CecMenuState state, bool sendUpdate)
    {
      return m_libCec->SetMenuState((cec_menu_state) state, sendUpdate);
    }

    /// <summary>
    /// Display a message on the device with the given logical address. Not supported by most TVs.
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to display the message on.</param>
    /// <param name="duration">The duration of the message</param>
    /// <param name="message">The message to display.</param>
    /// <returns>True when the command was sent, false otherwise.</returns>
    bool SetOSDString(CecLogicalAddress logicalAddress, CecDisplayControl duration, String ^ message)
    {
      marshal_context ^ context = gcnew marshal_context();
      const char* strMessageC = context->marshal_as<const char*>(message);

      bool bReturn = m_libCec->SetOSDString((cec_logical_address) logicalAddress, (cec_display_control) duration, strMessageC);

      delete context;
      return bReturn;
    }

    /// <summary>
    /// Enable or disable monitoring mode, for debugging purposes. If monitoring mode is enabled, libCEC won't respond to any command, but only log incoming data.
    /// </summary>
    /// <param name="enable">True to enable, false to disable.</param>
    /// <returns>True when switched successfully, false otherwise.</returns>
    bool SwitchMonitoring(bool enable)
    {
      return m_libCec->SwitchMonitoring(enable);
    }

    /// <summary>
    /// Get the CEC version of the device with the given logical address
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to get the CEC version for.</param>
    /// <returns>The version or CEC_VERSION_UNKNOWN when the version couldn't be fetched.</returns>
    CecVersion GetDeviceCecVersion(CecLogicalAddress logicalAddress)
    {
      return (CecVersion) m_libCec->GetDeviceCecVersion((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Get the menu language of the device with the given logical address
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to get the menu language for.</param>
    /// <returns>The requested menu language.</returns>
    String ^ GetDeviceMenuLanguage(CecLogicalAddress logicalAddress)
    {
      cec_menu_language lang;
      if (m_libCec->GetDeviceMenuLanguage((cec_logical_address) logicalAddress, &lang))
      {
        return gcnew String(lang.language);
      }

      return gcnew String("");
    }

    /// <summary>
    /// Get the vendor ID of the device with the given logical address.
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to get the vendor ID for.</param>
    /// <returns>The vendor ID or 0 if it wasn't found.</returns>
    CecVendorId GetDeviceVendorId(CecLogicalAddress logicalAddress)
    {
      return (CecVendorId)m_libCec->GetDeviceVendorId((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Get the power status of the device with the given logical address.
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to get the power status for.</param>
    /// <returns>The power status or CEC_POWER_STATUS_UNKNOWN if it wasn't found.</returns>
    CecPowerStatus GetDevicePowerStatus(CecLogicalAddress logicalAddress)
    {
      return (CecPowerStatus) m_libCec->GetDevicePowerStatus((cec_logical_address) logicalAddress);
    }

    /// <summary>
    /// Tell libCEC to poll for active devices on the bus.
    /// </summary>
    void RescanActiveDevices(void)
    {
      m_libCec->RescanActiveDevices();
    }

    /// <summary>
    /// Get the logical addresses of the devices that are active on the bus, including those handled by libCEC.
    /// </summary>
    /// <returns>The logical addresses of the active devices</returns>
    CecLogicalAddresses ^ GetActiveDevices(void)
    {
      CecLogicalAddresses ^ retVal = gcnew CecLogicalAddresses();
      unsigned int iDevices = 0;

      cec_logical_addresses activeDevices = m_libCec->GetActiveDevices();

      for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
        if (activeDevices[iPtr])
          retVal->Set((CecLogicalAddress)iPtr);

      return retVal;
    }

    /// <summary>
    /// Check whether a device is active on the bus.
    /// </summary>
    /// <param name="logicalAddress">The address to check.</param>
    /// <returns>True when active, false otherwise.</returns>
    bool IsActiveDevice(CecLogicalAddress logicalAddress)
    {
      return m_libCec->IsActiveDevice((cec_logical_address)logicalAddress);
    }

    /// <summary>
    /// Check whether a device of the given type is active on the bus.
    /// </summary>
    /// <param name="type">The type to check.</param>
    /// <returns>True when active, false otherwise.</returns>
    bool IsActiveDeviceType(CecDeviceType type)
    {
      return m_libCec->IsActiveDeviceType((cec_device_type)type);
    }

    /// <summary>
    /// Changes the active HDMI port.
    /// </summary>
    /// <param name="address">The device to which this libCEC is connected.</param>
    /// <param name="port">The new port number.</param>
    /// <returns>True when changed, false otherwise.</returns>
    bool SetHDMIPort(CecLogicalAddress address, uint8_t port)
    {
      return m_libCec->SetHDMIPort((cec_logical_address)address, port);
    }

    /// <summary>
    /// Sends a volume up keypress to an audiosystem if it's present.
    /// </summary>
    /// <param name="sendRelease">Send a key release after the keypress.</param>
    /// <returns>The new audio status.</returns>
    uint8_t VolumeUp(bool sendRelease)
    {
      return m_libCec->VolumeUp(sendRelease);
    }

    /// <summary>
    /// Sends a volume down keypress to an audiosystem if it's present.
    /// </summary>
    /// <param name="sendRelease">Send a key release after the keypress.</param>
    /// <returns>The new audio status.</returns>
    uint8_t VolumeDown(bool sendRelease)
    {
      return m_libCec->VolumeDown(sendRelease);
    }

    /// <summary>
    /// Sends a mute keypress to an audiosystem if it's present.
    /// </summary>
    /// <param name="sendRelease">Send a key release after the keypress.</param>
    /// <returns>The new audio status.</returns>
    uint8_t MuteAudio(bool sendRelease)
    {
      return m_libCec->MuteAudio(sendRelease);
    }

    /// <summary>
    /// Send a keypress to a device on the CEC bus.
    /// </summary>
    /// <param name="destination">The logical address of the device to send the message to.</param>
    /// <param name="key">The key to send.</param>
    /// <param name="wait">True to wait for a response, false otherwise.</param>
    /// <returns>True when the keypress was acked, false otherwise.</returns>
    bool SendKeypress(CecLogicalAddress destination, CecUserControlCode key, bool wait)
    {
      return m_libCec->SendKeypress((cec_logical_address)destination, (cec_user_control_code)key, wait);
    }

    /// <summary>
    /// Send a key release to a device on the CEC bus.
    /// </summary>
    /// <param name="destination">The logical address of the device to send the message to.</param>
    /// <param name="wait">True to wait for a response, false otherwise.</param>
    /// <returns>True when the key release was acked, false otherwise.</returns>
    bool SendKeyRelease(CecLogicalAddress destination, bool wait)
    {
      return m_libCec->SendKeyRelease((cec_logical_address)destination, wait);
    }

    /// <summary>
    /// Get the OSD name of a device on the CEC bus.
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to get the OSD name for.</param>
    /// <returns>The OSD name.</returns>
    String ^ GetDeviceOSDName(CecLogicalAddress logicalAddress)
    {
      cec_osd_name osd = m_libCec->GetDeviceOSDName((cec_logical_address) logicalAddress);
      // we need to terminate with \0, and we only got 14 chars in osd.name
      char strOsdName[15];
      memset(strOsdName, 0, sizeof(strOsdName));
      memcpy(strOsdName, osd.name, sizeof(osd.name));
      return gcnew String(strOsdName);
    }

    /// <summary>
    /// Get the logical address of the device that is currently the active source on the CEC bus.
    /// </summary>
    /// <returns>The active source or CECDEVICE_UNKNOWN when unknown.</returns>
    CecLogicalAddress GetActiveSource()
    {
      return (CecLogicalAddress)m_libCec->GetActiveSource();
    }

    /// <summary>
    /// Check whether a device is currently the active source on the CEC bus.
    /// </summary>
    /// <param name="logicalAddress">The logical address of the device to check.</param>
    /// <returns>True when it is the active source, false otherwise.</returns>
    bool IsActiveSource(CecLogicalAddress logicalAddress)
    {
      return m_libCec->IsActiveSource((cec_logical_address)logicalAddress);
    }

    /// <summary>
    /// Get the physical address of the device with the given logical address.
    /// </summary>
    /// <param name="address">The logical address of the device to get the physical address for.</param>
    /// <returns>The physical address or 0 if it wasn't found.</returns>
    uint16_t GetDevicePhysicalAddress(CecLogicalAddress address)
    {
      return m_libCec->GetDevicePhysicalAddress((cec_logical_address)address);
    }

    /// <summary>
    /// Sets the stream path to the device on the given logical address.
    /// </summary>
    /// <param name="address">The address to activate.</param>
    /// <returns>True when the command was sent, false otherwise.</returns>
    bool SetStreamPath(CecLogicalAddress address)
    {
      return m_libCec->SetStreamPath((cec_logical_address)address);
    }

    /// <summary>
    /// Sets the stream path to the device on the given physical address.
    /// </summary>
    /// <param name="physicalAddress">The address to activate.</param>
    /// <returns>True when the command was sent, false otherwise.</returns>
    bool SetStreamPath(uint16_t physicalAddress)
    {
      return m_libCec->SetStreamPath(physicalAddress);
    }

    /// <summary>
    /// Get the list of logical addresses that libCEC is controlling
    /// </summary>
    /// <returns>The list of logical addresses that libCEC is controlling</returns>
    CecLogicalAddresses ^GetLogicalAddresses(void)
    {
      CecLogicalAddresses ^addr = gcnew CecLogicalAddresses();
      cec_logical_addresses libAddr = m_libCec->GetLogicalAddresses();
      for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
        addr->Addresses[iPtr] = (CecLogicalAddress)libAddr.addresses[iPtr];
      addr->Primary = (CecLogicalAddress)libAddr.primary;
      return addr;
    }

    /// <summary>
    /// Get libCEC's current configuration.
    /// </summary>
    /// <param name="configuration">The configuration.</param>
    /// <returns>True when the configuration was updated, false otherwise.</returns>
    bool GetCurrentConfiguration(LibCECConfiguration ^configuration)
    {
      libcec_configuration config;
      config.Clear();

      if (m_libCec->GetCurrentConfiguration(&config))
      {
        configuration->Update(config);
        return true;
      }
      return false;
    }

    /// <summary>
    /// Check whether the CEC adapter can persist a configuration.
    /// </summary>
    /// <returns>True when this CEC adapter can persist the user configuration, false otherwise.</returns>
    bool CanPersistConfiguration(void)
    {
      return m_libCec->CanPersistConfiguration();
    }

    /// <summary>
    /// Persist the given configuration in adapter (if supported)
    /// </summary>
    /// <param name="configuration">The configuration to store.</param>
    /// <returns>True when the configuration was persisted, false otherwise.</returns>
    bool PersistConfiguration(LibCECConfiguration ^configuration)
    {
      marshal_context ^ context = gcnew marshal_context();
      libcec_configuration config;
      ConvertConfiguration(context, configuration, config);

      bool bReturn = m_libCec->PersistConfiguration(&config);

      delete context;
      return bReturn;
    }

    /// <summary>
    /// Change libCEC's configuration.
    /// </summary>
    /// <param name="configuration">The new configuration.</param>
    /// <returns>True when the configuration was changed successfully, false otherwise.</returns>
    bool SetConfiguration(LibCECConfiguration ^configuration)
    {
      marshal_context ^ context = gcnew marshal_context();
      libcec_configuration config;
      ConvertConfiguration(context, configuration, config);

      bool bReturn = m_libCec->SetConfiguration(&config);

      delete context;
      return bReturn;
    }

    /// <summary>
    /// Check whether libCEC is the active source on the bus.
    /// </summary>
    /// <returns>True when libCEC is the active source on the bus, false otherwise.</returns>
    bool IsLibCECActiveSource()
    {
      return m_libCec->IsLibCECActiveSource();
    }

    /// <summary>
    /// Get information about the given CEC adapter.
    /// </summary>
    /// <param name="port">The COM port to which the device is connected</param>
    /// <param name="configuration">The device configuration</param>
    /// <param name="timeoutMs">The timeout in milliseconds</param>
    /// <returns>True when the device was found, false otherwise</returns>
    bool GetDeviceInformation(String ^ port, LibCECConfiguration ^configuration, uint32_t timeoutMs)
    {
      bool bReturn(false);
      marshal_context ^ context = gcnew marshal_context();

      libcec_configuration config;
      config.Clear();

      const char* strPortC = port->Length > 0 ? context->marshal_as<const char*>(port) : NULL;

      if (m_libCec->GetDeviceInformation(strPortC, &config, timeoutMs))
      {
        configuration->Update(config);
        bReturn = true;
      }

      delete context;
      return bReturn;
    }

    String ^ ToString(CecLogicalAddress iAddress)
    {
      const char *retVal = m_libCec->ToString((cec_logical_address)iAddress);
      return gcnew String(retVal);
    }

    String ^ ToString(CecVendorId iVendorId)
    {
      const char *retVal = m_libCec->ToString((cec_vendor_id)iVendorId);
      return gcnew String(retVal);
    }

    String ^ ToString(CecVersion iVersion)
    {
      const char *retVal = m_libCec->ToString((cec_version)iVersion);
      return gcnew String(retVal);
    }

    String ^ ToString(CecPowerStatus iState)
    {
      const char *retVal = m_libCec->ToString((cec_power_status)iState);
      return gcnew String(retVal);
    }

    String ^ ToString(CecMenuState iState)
    {
      const char *retVal = m_libCec->ToString((cec_menu_state)iState);
      return gcnew String(retVal);
    }

    String ^ ToString(CecDeckControlMode iMode)
    {
      const char *retVal = m_libCec->ToString((cec_deck_control_mode)iMode);
      return gcnew String(retVal);
    }

    String ^ ToString(CecDeckInfo status)
    {
      const char *retVal = m_libCec->ToString((cec_deck_info)status);
      return gcnew String(retVal);
    }

    String ^ ToString(CecOpcode opcode)
    {
      const char *retVal = m_libCec->ToString((cec_opcode)opcode);
      return gcnew String(retVal);
    }

    String ^ ToString(CecSystemAudioStatus mode)
    {
      const char *retVal = m_libCec->ToString((cec_system_audio_status)mode);
      return gcnew String(retVal);
    }

    String ^ ToString(CecAudioStatus status)
    {
      const char *retVal = m_libCec->ToString((cec_audio_status)status);
      return gcnew String(retVal);
    }

    String ^ ToString(CecClientVersion version)
    {
      const char *retVal = m_libCec->ToString((cec_client_version)version);
      return gcnew String(retVal);
    }

    String ^ ToString(CecServerVersion version)
    {
      const char *retVal = m_libCec->ToString((cec_server_version)version);
      return gcnew String(retVal);
    }

    /// <summary>
    /// Get a string with information about how libCEC was compiled.
    /// </summary>
    /// <returns>A string with information about how libCEC was compiled.</returns>
    String ^ GetLibInfo()
    {
      const char *retVal = m_libCec->GetLibInfo();
      return gcnew String(retVal);
    }

    /// <summary>
    /// Calling this method will initialise the host on which libCEC is running.
    /// On the RPi, it calls bcm_host_init(), which may only be called once per process, and is called by any process using
    /// the video api on that system. So only call this method if libCEC is used in an application that
    /// does not already initialise the video api.
    /// </summary>
    /// <remarks>Should be called as first call to libCEC, directly after CECInitialise() and before using Open()</remarks>
    void InitVideoStandalone()
    {
      m_libCec->InitVideoStandalone();
    }

    /// <summary>
    /// Get the (virtual) USB vendor id
    /// </summary>
    /// <returns>The (virtual) USB vendor id</returns>
    uint16_t GetAdapterVendorId()
    {
      return m_libCec->GetAdapterVendorId();
    }

    /// <summary>
    /// Get the (virtual) USB product id
    /// </summary>
    /// <returns>The (virtual) USB product id</returns>
    uint16_t GetAdapterProductId()
    {
      return m_libCec->GetAdapterProductId();
    }

  private:
    !LibCecSharp(void)
    {
      Close();
      m_libCec = NULL;
    }

    bool InitialiseLibCec(LibCECConfiguration ^config)
    {
      marshal_context ^ context = gcnew marshal_context();
      libcec_configuration libCecConfig;
      ConvertConfiguration(context, config, libCecConfig);

      m_libCec = (ICECAdapter *) CECInitialise(&libCecConfig);
      config->Update(libCecConfig);

      delete context;
      return m_libCec != NULL;
    }

    void ConvertConfiguration(marshal_context ^context, LibCECConfiguration ^netConfig, CEC::libcec_configuration &config)
    {
      config.Clear();

      const char *strDeviceName = context->marshal_as<const char*>(netConfig->DeviceName);
      memcpy_s(config.strDeviceName, 13, strDeviceName, 13);
      for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
        config.deviceTypes.types[iPtr] = (cec_device_type)netConfig->DeviceTypes->Types[iPtr];

      config.bAutodetectAddress   = netConfig->AutodetectAddress ? 1 : 0;
      config.iPhysicalAddress     = netConfig->PhysicalAddress;
      config.baseDevice           = (cec_logical_address)netConfig->BaseDevice;
      config.iHDMIPort            = netConfig->HDMIPort;
      config.clientVersion        = (cec_client_version)netConfig->ClientVersion;
      config.bGetSettingsFromROM  = netConfig->GetSettingsFromROM ? 1 : 0;
      config.bActivateSource      = netConfig->ActivateSource ? 1 : 0;
      config.tvVendor             = (cec_vendor_id)netConfig->TvVendor;
      config.wakeDevices.Clear();
      for (int iPtr = 0; iPtr < 16; iPtr++)
      {
        if (netConfig->WakeDevices->IsSet((CecLogicalAddress)iPtr))
          config.wakeDevices.Set((cec_logical_address)iPtr);
      }
      config.powerOffDevices.Clear();
      for (int iPtr = 0; iPtr < 16; iPtr++)
      {
        if (netConfig->PowerOffDevices->IsSet((CecLogicalAddress)iPtr))
          config.powerOffDevices.Set((cec_logical_address)iPtr);
      }
      config.bPowerOffScreensaver = netConfig->PowerOffScreensaver ? 1 : 0;
      config.bPowerOffOnStandby   = netConfig->PowerOffOnStandby ? 1 : 0;

      if (netConfig->ServerVersion >= CecServerVersion::Version1_5_1)
        config.bSendInactiveSource  = netConfig->SendInactiveSource ? 1 : 0;

      if (netConfig->ServerVersion >= CecServerVersion::Version1_6_0)
      {
        config.bPowerOffDevicesOnStandby  = netConfig->PowerOffDevicesOnStandby ? 1 : 0;
        config.bShutdownOnStandby         = netConfig->ShutdownOnStandby ? 1 : 0;
      }

      if (netConfig->ServerVersion >= CecServerVersion::Version1_6_2)
      {
        const char *strDeviceLanguage = context->marshal_as<const char*>(netConfig->DeviceLanguage);
        memcpy_s(config.strDeviceLanguage, 3, strDeviceLanguage, 3);
      }

      if (netConfig->ServerVersion >= CecServerVersion::Version1_6_3)
        config.bMonitorOnly = netConfig->MonitorOnlyClient ? 1 : 0;

      if (netConfig->ServerVersion >= CecServerVersion::Version1_8_0)
        config.cecVersion = (cec_version)netConfig->CECVersion;

      if (netConfig->ServerVersion >= CecServerVersion::Version2_1_0)
        config.bPowerOnScreensaver  = netConfig->PowerOnScreensaver ? 1 : 0;

      config.callbacks = &g_cecCallbacks;
    }


    ICECAdapter *        m_libCec;
    CecCallbackMethods ^ m_callbacks;
  };
}

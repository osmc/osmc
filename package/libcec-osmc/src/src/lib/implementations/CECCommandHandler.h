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

#include <vector>
#include <string>
#include <map>
#include "lib/platform/threads/mutex.h"

namespace CEC
{
  #define COMMAND_HANDLED 0xFF

  class CCECProcessor;
  class CCECBusDevice;

  class CCECCommandHandler
  {
    friend class CCECBusDevice;

  public:
    CCECCommandHandler(CCECBusDevice *busDevice,
                       int32_t iTransmitTimeout = CEC_DEFAULT_TRANSMIT_TIMEOUT,
                       int32_t iTransmitWait = CEC_DEFAULT_TRANSMIT_WAIT,
                       int8_t iTransmitRetries = CEC_DEFAULT_TRANSMIT_RETRIES,
                       int64_t iActiveSourcePending = 0);
    virtual ~CCECCommandHandler(void) {};

    virtual bool HandleCommand(const cec_command &command);
    virtual cec_vendor_id GetVendorId(void) { return m_vendorId; };
    virtual void SetVendorId(cec_vendor_id vendorId) { m_vendorId = vendorId; }
    static bool HasSpecificHandler(cec_vendor_id vendorId) { return vendorId == CEC_VENDOR_LG || vendorId == CEC_VENDOR_SAMSUNG || vendorId == CEC_VENDOR_PANASONIC || vendorId == CEC_VENDOR_PHILIPS || vendorId == CEC_VENDOR_SHARP;}

    virtual bool InitHandler(void) { return true; }
    virtual bool ActivateSource(bool bTransmitDelayedCommandsOnly = false);
    virtual uint8_t GetTransmitRetries(void) const { return m_iTransmitRetries; }

    virtual bool PowerOn(const cec_logical_address iInitiator, const cec_logical_address iDestination);
    virtual bool TransmitImageViewOn(const cec_logical_address iInitiator, const cec_logical_address iDestination);
    virtual bool TransmitStandby(const cec_logical_address iInitiator, const cec_logical_address iDestination);
    virtual bool TransmitRequestActiveSource(const cec_logical_address iInitiator, bool bWaitForResponse = true);
    virtual bool TransmitRequestCecVersion(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse = true);
    virtual bool TransmitRequestMenuLanguage(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse = true);
    virtual bool TransmitRequestOSDName(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse = true);
    virtual bool TransmitRequestAudioStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse = true);
    virtual bool TransmitRequestPhysicalAddress(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse = true);
    virtual bool TransmitRequestPowerStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bUpdate, bool bWaitForResponse = true);
    virtual bool TransmitRequestVendorId(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWaitForResponse = true);
    virtual bool TransmitActiveSource(const cec_logical_address iInitiator, uint16_t iPhysicalAddress, bool bIsReply);
    virtual bool TransmitCECVersion(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_version cecVersion, bool bIsReply);
    virtual bool TransmitInactiveSource(const cec_logical_address iInitiator, uint16_t iPhysicalAddress);
    virtual bool TransmitMenuState(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_menu_state menuState, bool bIsReply);
    virtual bool TransmitOSDName(const cec_logical_address iInitiator, const cec_logical_address iDestination, std::string strDeviceName, bool bIsReply);
    virtual bool TransmitOSDString(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_display_control duration, const char *strMessage, bool bIsReply);
    virtual bool TransmitPhysicalAddress(const cec_logical_address iInitiator, uint16_t iPhysicalAddress, cec_device_type type, bool bIsReply);
    virtual bool TransmitSetMenuLanguage(const cec_logical_address iInitiator, const char lang[4], bool bIsReply);
    virtual bool TransmitPoll(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bIsReply);
    virtual bool TransmitPowerState(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_power_status state, bool bIsReply);
    virtual bool TransmitVendorID(const cec_logical_address iInitiator, const cec_logical_address iDestination, uint64_t iVendorId, bool bIsReply);
    virtual bool TransmitAudioStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, uint8_t state, bool bIsReply);
    virtual bool TransmitSetSystemAudioMode(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_system_audio_status state, bool bIsReply);
    virtual bool TransmitSystemAudioModeStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_system_audio_status state, bool bIsReply);
    virtual bool TransmitDeckStatus(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_deck_info state, bool bIsReply);
    virtual bool TransmitKeypress(const cec_logical_address iInitiator, const cec_logical_address iDestination, cec_user_control_code key, bool bWait = true);
    virtual bool TransmitKeyRelease(const cec_logical_address iInitiator, const cec_logical_address iDestination, bool bWait = true);
    virtual bool TransmitSetStreamPath(uint16_t iStreamPath, bool bIsReply);
    virtual bool SendDeckStatusUpdateOnActiveSource(void) const { return m_bOPTSendDeckStatusUpdateOnActiveSource; };

    virtual void ScheduleActivateSource(uint64_t iDelay);

    virtual bool SupportsDeviceType(const cec_device_type UNUSED(type)) const { return true; };
    virtual cec_device_type GetReplacementDeviceType(const cec_device_type type) const { return type; }
    virtual bool ActiveSourcePending(void) const { return m_iActiveSourcePending != 0; }

  protected:
    virtual int HandleActiveSource(const cec_command &command);
    virtual int HandleDeckControl(const cec_command &command);
    virtual int HandleDeviceCecVersion(const cec_command &command);
    virtual int HandleDeviceVendorCommandWithId(const cec_command &command);
    virtual int HandleDeviceVendorId(const cec_command &command);
    virtual int HandleFeatureAbort(const cec_command &command);
    virtual int HandleGetCecVersion(const cec_command &command);
    virtual int HandleGiveAudioStatus(const cec_command &command);
    virtual int HandleGiveDeckStatus(const cec_command &command);
    virtual int HandleGiveDevicePowerStatus(const cec_command &command);
    virtual int HandleGiveDeviceVendorId(const cec_command &command);
    virtual int HandleGiveOSDName(const cec_command &command);
    virtual int HandleGivePhysicalAddress(const cec_command &command);
    virtual int HandleGiveMenuLanguage(const cec_command &command);
    virtual int HandleGiveSystemAudioModeStatus(const cec_command &command);
    virtual int HandleImageViewOn(const cec_command &command);
    virtual int HandleMenuRequest(const cec_command &command);
    virtual bool HandlePoll(const cec_command &command);
    virtual int HandleReportAudioStatus(const cec_command &command);
    virtual int HandleReportPhysicalAddress(const cec_command &command);
    virtual int HandleReportPowerStatus(const cec_command &command);
    virtual int HandleRequestActiveSource(const cec_command &command);
    virtual int HandleRoutingChange(const cec_command &command);
    virtual int HandleRoutingInformation(const cec_command &command);
    virtual int HandleSetMenuLanguage(const cec_command &command);
    virtual int HandleSetOSDName(const cec_command &command);
    virtual int HandleSetStreamPath(const cec_command &command);
    virtual int HandleSystemAudioModeRequest(const cec_command &command);
    virtual int HandleStandby(const cec_command &command);
    virtual int HandleSystemAudioModeStatus(const cec_command &command);
    virtual int HandleSetSystemAudioMode(const cec_command &command);
    virtual int HandleTextViewOn(const cec_command &command);
    virtual int HandleUserControlPressed(const cec_command &command);
    virtual int HandleUserControlRelease(const cec_command &command);
    virtual int HandleVendorCommand(const cec_command &command);
    virtual int HandleVendorRemoteButtonDown(const cec_command& command);
    virtual int HandleVendorRemoteButtonUp(const cec_command& command) { return HandleUserControlRelease(command); }
    virtual void UnhandledCommand(const cec_command &command, const cec_abort_reason reason);
    virtual void RequestEmailFromCustomer(const cec_command& command);

    virtual void VendorPreActivateSourceHook(void) {};

    virtual size_t GetMyDevices(std::vector<CCECBusDevice *> &devices) const;
    virtual CCECBusDevice *GetDevice(cec_logical_address iLogicalAddress) const;
    virtual CCECBusDevice *GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress) const;

    virtual bool SetVendorId(const cec_command &command);
    virtual void SetPhysicalAddress(cec_logical_address iAddress, uint16_t iNewAddress);

    virtual bool Transmit(cec_command &command, bool bSuppressWait, bool bIsReply);

    virtual bool SourceSwitchAllowed(void) { return true; }

    CCECBusDevice *  m_busDevice;
    CCECProcessor *  m_processor;
    int32_t          m_iTransmitTimeout;
    int32_t          m_iTransmitWait;
    int8_t           m_iTransmitRetries;
    bool            m_bHandlerInited;
    bool            m_bOPTSendDeckStatusUpdateOnActiveSource;
    cec_vendor_id    m_vendorId;
    int64_t          m_iActiveSourcePending;
    PLATFORM::CMutex m_mutex;
    int64_t          m_iPowerStatusRequested;
    std::map<cec_opcode, std::vector<cec_command> > m_logsRequested;
  };
};

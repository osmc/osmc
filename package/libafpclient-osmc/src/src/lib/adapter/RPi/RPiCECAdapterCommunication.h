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

#include "env.h"
#if defined(HAVE_RPI_API)

#include "lib/adapter/AdapterCommunication.h"
#include "lib/platform/threads/threads.h"

#define RPI_ADAPTER_VID 0x2708
#define RPI_ADAPTER_PID 0x1001

extern "C" {
#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vchiq_arm/vchiq_if.h>
}

namespace CEC
{
  class CRPiCECAdapterMessageQueue;

  class CRPiCECAdapterCommunication : public IAdapterCommunication
  {
  public:
    /*!
     * @brief Create a new USB-CEC communication handler.
     * @param callback The callback to use for incoming CEC commands.
     */
    CRPiCECAdapterCommunication(IAdapterCommunicationCallback *callback);
    virtual ~CRPiCECAdapterCommunication(void);

    /** @name IAdapterCommunication implementation */
    ///{
    bool Open(uint32_t iTimeoutMs = CEC_DEFAULT_CONNECT_TIMEOUT, bool bSkipChecks = false, bool bStartListening = true);
    void Close(void);
    bool IsOpen(void) { return m_bInitialised; };
    std::string GetError(void) const;
    cec_adapter_message_state Write(const cec_command &data, bool &bRetry, uint8_t iLineTimeout, bool bIsReply);

    bool SetLineTimeout(uint8_t UNUSED(iTimeout)) { return true; };
    bool StartBootloader(void) { return false; };
    bool SetLogicalAddresses(const cec_logical_addresses &addresses);
    cec_logical_addresses GetLogicalAddresses(void);
    bool PingAdapter(void) { return m_bInitialised; };
    uint16_t GetFirmwareVersion(void);
    uint32_t GetFirmwareBuildDate(void) { return 0; };
    bool IsRunningLatestFirmware(void) { return true; };
    bool PersistConfiguration(const libcec_configuration & UNUSED(configuration)) { return false; };
    bool GetConfiguration(libcec_configuration & UNUSED(configuration)) { return false; };
    std::string GetPortName(void) { std::string strReturn("RPI"); return strReturn; };
    uint16_t GetPhysicalAddress(void);
    bool SetControlledMode(bool UNUSED(controlled)) { return true; };
    cec_vendor_id GetVendorId(void) { return CEC_VENDOR_BROADCOM; }
    bool SupportsSourceLogicalAddress(const cec_logical_address address) { return address > CECDEVICE_TV && address < CECDEVICE_BROADCAST; }
    cec_adapter_type GetAdapterType(void) { return ADAPTERTYPE_RPI; };
    uint16_t GetAdapterVendorId(void) const { return RPI_ADAPTER_VID; }
    uint16_t GetAdapterProductId(void) const { return RPI_ADAPTER_PID; }
    void SetActiveSource(bool UNUSED(bSetTo), bool UNUSED(bClientUnregistered)) {}
    ///}

    bool IsInitialised(void);
    void OnDataReceived(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);
    void OnTVServiceCallback(uint32_t reason, uint32_t p0, uint32_t p1);

    static void InitHost(void);

  private:
    cec_logical_address GetLogicalAddress(void);
    bool UnregisterLogicalAddress(void);
    bool RegisterLogicalAddress(const cec_logical_address address);
    int InitHostCEC(void);

    bool m_bInitialised;   /**< true when the connection is initialised, false otherwise */
    std::string m_strError; /**< current error message */
    CRPiCECAdapterMessageQueue *m_queue;
    cec_logical_address         m_logicalAddress;

    bool                        m_bLogicalAddressChanged;
    PLATFORM::CCondition<bool>  m_logicalAddressCondition;
    PLATFORM::CMutex            m_mutex;
    VCHI_INSTANCE_T             m_vchi_instance;
    VCHI_CONNECTION_T *         m_vchi_connection;
    cec_logical_address         m_previousLogicalAddress;
    bool                        m_bLogicalAddressRegistered;
  };
};

#endif

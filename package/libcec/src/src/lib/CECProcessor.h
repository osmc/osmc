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

#include "platform/threads/threads.h"
#include "platform/util/buffer.h"

#include "adapter/AdapterCommunication.h"
#include "devices/CECDeviceMap.h"
#include "CECInputBuffer.h"

namespace CEC
{
  class CLibCEC;
  class IAdapterCommunication;
  class CCECBusDevice;
  class CCECAudioSystem;
  class CCECPlaybackDevice;
  class CCECRecordingDevice;
  class CCECTuner;
  class CCECTV;
  class CCECClient;
  class CCECProcessor;
  class CCECStandbyProtection;

  class CCECAllocateLogicalAddress : public PLATFORM::CThread
  {
  public:
    CCECAllocateLogicalAddress(CCECProcessor* processor, CCECClient* client);
    void* Process(void);

  private:
    CCECProcessor* m_processor;
    CCECClient*    m_client;
  };

  class CCECProcessor : public PLATFORM::CThread, public IAdapterCommunicationCallback
  {
    public:
      CCECProcessor(CLibCEC *libcec);
      virtual ~CCECProcessor(void);

      bool Start(const char *strPort, uint16_t iBaudRate = CEC_SERIAL_DEFAULT_BAUDRATE, uint32_t iTimeoutMs = CEC_DEFAULT_CONNECT_TIMEOUT);
      void *Process(void);
      void Close(void);

      bool RegisterClient(CCECClient *client);
      bool UnregisterClient(CCECClient *client);
      void UnregisterClients(void);
      uint16_t GetPhysicalAddressFromEeprom(void);
      CCECClient *GetPrimaryClient(void);
      CCECClient *GetClient(const cec_logical_address address);

      bool                  OnCommandReceived(const cec_command &command);
      void                  HandleLogicalAddressLost(cec_logical_address oldAddress);
      void                  HandlePhysicalAddressChanged(uint16_t iNewAddress);

      CCECBusDevice *       GetDevice(cec_logical_address address) const;
      CCECAudioSystem *     GetAudioSystem(void) const;
      CCECPlaybackDevice *  GetPlaybackDevice(cec_logical_address address) const;
      CCECRecordingDevice * GetRecordingDevice(cec_logical_address address) const;
      CCECTuner *           GetTuner(cec_logical_address address) const;
      CCECTV *              GetTV(void) const;

      CCECBusDevice *       GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress, bool bSuppressUpdate = true);
      CCECBusDevice *       GetPrimaryDevice(void);
      cec_logical_address   GetLogicalAddress(void);
      cec_logical_addresses GetLogicalAddresses(void);
      bool                  IsPresentDevice(cec_logical_address address);
      bool                  IsPresentDeviceType(cec_device_type type);
      uint16_t              GetDetectedPhysicalAddress(void) const;
      uint64_t              GetLastTransmission(void) const { return m_iLastTransmission; }
      cec_logical_address   GetActiveSource(bool bRequestActiveSource = true);
      bool                  IsActiveSource(cec_logical_address iAddress);
      bool                  CECInitialised(void);

      bool                  StandbyDevices(const cec_logical_address initiator, const CECDEVICEVEC &devices);
      bool                  StandbyDevice(const cec_logical_address initiator, cec_logical_address address);
      bool                  PowerOnDevices(const cec_logical_address initiator, const CECDEVICEVEC &devices);
      bool                  PowerOnDevice(const cec_logical_address initiator, cec_logical_address address);

      bool SetDeckInfo(cec_deck_info info, bool bSendUpdate = true);
      bool ActivateSource(uint16_t iStreamPath);
      void SetActiveSource(bool bSetTo, bool bClientUnregistered);
      bool PollDevice(cec_logical_address iAddress);
      void SetStandardLineTimeout(uint8_t iTimeout);
      uint8_t GetStandardLineTimeout(void);
      void SetRetryLineTimeout(uint8_t iTimeout);
      uint8_t GetRetryLineTimeout(void);
      bool CanPersistConfiguration(void);
      bool PersistConfiguration(const libcec_configuration &configuration);
      void RescanActiveDevices(void);

      bool SetLineTimeout(uint8_t iTimeout);

      bool Transmit(const cec_command &data, bool bIsReply);
      void TransmitAbort(cec_logical_address source, cec_logical_address destination, cec_opcode opcode, cec_abort_reason reason = CEC_ABORT_REASON_UNRECOGNIZED_OPCODE);

      bool StartBootloader(const char *strPort = NULL);
      bool PingAdapter(void);
      void HandlePoll(cec_logical_address initiator, cec_logical_address destination);
      bool HandleReceiveFailed(cec_logical_address initiator);

      bool GetDeviceInformation(const char *strPort, libcec_configuration *config, uint32_t iTimeoutMs = CEC_DEFAULT_CONNECT_TIMEOUT);

      bool TransmitPendingActiveSourceCommands(void);

      CCECDeviceMap *GetDevices(void) const { return m_busDevices; }
      CLibCEC *GetLib(void) const { return m_libcec; }

      bool IsHandledByLibCEC(const cec_logical_address address) const;

      bool TryLogicalAddress(cec_logical_address address, cec_version libCECSpecVersion = CEC_VERSION_1_4);

      bool IsRunningLatestFirmware(void);
      void SwitchMonitoring(bool bSwitchTo);

      bool AllocateLogicalAddresses(CCECClient* client);

      uint16_t GetAdapterVendorId(void) const;
      uint16_t GetAdapterProductId(void) const;
    private:
      bool OpenConnection(const char *strPort, uint16_t iBaudRate, uint32_t iTimeoutMs, bool bStartListening = true);
      void SetCECInitialised(bool bSetTo = true);

      void ReplaceHandlers(void);
      bool PhysicalAddressInUse(uint16_t iPhysicalAddress);

      bool ClearLogicalAddresses(void);
      bool SetLogicalAddresses(const cec_logical_addresses &addresses);

      void LogOutput(const cec_command &data);
      void ProcessCommand(const cec_command &command);

      void ResetMembers(void);

      bool                                        m_bInitialised;
      PLATFORM::CMutex                            m_mutex;
      IAdapterCommunication *                     m_communication;
      CLibCEC*                                    m_libcec;
      uint8_t                                     m_iStandardLineTimeout;
      uint8_t                                     m_iRetryLineTimeout;
      uint64_t                                    m_iLastTransmission;
      CCECInputBuffer                             m_inBuffer;
      CCECDeviceMap *                             m_busDevices;
      std::map<cec_logical_address, CCECClient *> m_clients;
      bool                                        m_bMonitor;
      CCECAllocateLogicalAddress*                 m_addrAllocator;
      bool                                        m_bStallCommunication;
      CCECStandbyProtection*                      m_connCheck;
  };

  class CCECStandbyProtection : public PLATFORM::CThread
  {
  public:
    CCECStandbyProtection(CCECProcessor* processor);
    virtual ~CCECStandbyProtection(void);
    void* Process(void);

  private:
    CCECProcessor* m_processor;
  };
};

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

#include "lib/platform/threads/threads.h"
#include "lib/adapter/AdapterCommunication.h"
#include "USBCECAdapterMessage.h"

namespace PLATFORM
{
  class ISocket;
}

namespace CEC
{
  class CCECProcessor;
  class CAdapterPingThread;
  class CAdapterEepromWriteThread;
  class CUSBCECAdapterCommands;
  class CCECAdapterMessageQueue;
  class CCECAdapterMessage;

  class CUSBCECAdapterCommunication : public IAdapterCommunication, public PLATFORM::CThread
  {
    friend class CUSBCECAdapterCommands;
    friend class CCECAdapterMessageQueue;
    friend class CAdapterEepromWriteThread;

  public:
    /*!
     * @brief Create a new USB-CEC communication handler.
     * @param callback The callback to use for incoming CEC commands.
     * @param strPort The name of the com port to use.
     * @param iBaudRate The baudrate to use on the com port connection.
     */
    CUSBCECAdapterCommunication(IAdapterCommunicationCallback *callback, const char *strPort, uint16_t iBaudRate = CEC_SERIAL_DEFAULT_BAUDRATE);
    virtual ~CUSBCECAdapterCommunication(void);

    /** @name IAdapterCommunication implementation */
    ///{
    bool Open(uint32_t iTimeoutMs = CEC_DEFAULT_CONNECT_TIMEOUT, bool bSkipChecks = false, bool bStartListening = true);
    void Close(void);
    bool IsOpen(void);
    std::string GetError(void) const;
    cec_adapter_message_state Write(const cec_command &data, bool &bRetry, uint8_t iLineTimeout, bool bIsReply);

    bool StartBootloader(void);
    bool SetLogicalAddresses(const cec_logical_addresses &addresses);
    cec_logical_addresses GetLogicalAddresses(void);
    bool PingAdapter(void);
    uint16_t GetFirmwareVersion(void);
    uint32_t GetFirmwareBuildDate(void);
    bool IsRunningLatestFirmware(void);
    bool PersistConfiguration(const libcec_configuration &configuration);
    bool GetConfiguration(libcec_configuration &configuration);
    std::string GetPortName(void);
    uint16_t GetPhysicalAddress(void);
    bool SetControlledMode(bool controlled);
    cec_vendor_id GetVendorId(void) { return CEC_VENDOR_UNKNOWN; }
    bool SupportsSourceLogicalAddress(const cec_logical_address UNUSED(address)) { return true; }
    cec_adapter_type GetAdapterType(void);
    uint16_t GetAdapterVendorId(void) const;
    uint16_t GetAdapterProductId(void) const;
    void SetActiveSource(bool bSetTo, bool bClientUnregistered);
    ///}

    bool ProvidesExtendedResponse(void);

    void *Process(void);

  private:
    /*!
     * @brief Clear all input bytes.
     * @param iTimeout Timeout when anything was received.
     */
    void ClearInputBytes(uint32_t iTimeout = CEC_CLEAR_INPUT_DEFAULT_WAIT);

    /*!
     * @brief Change the current CEC line timeout.
     * @param iTimeout The new timeout.
     * @return True when acked by the controller, false otherwise.
     */
    bool SetLineTimeout(uint8_t iTimeout);

    /*!
     * @brief Send a command to the controller and wait for an ack.
     * @param msgCode The command to send.
     * @param params The parameters to the command.
     * @param bIsRetry True when this command is being retried, false otherwise.
     * @return The message. Delete when done with it.
     */
    CCECAdapterMessage *SendCommand(cec_adapter_messagecode msgCode, CCECAdapterMessage &params, bool bIsRetry = false);

    /*!
     * @brief Change the "initialised" status.
     * @param bSetTo The new value.
     */
    void SetInitialised(bool bSetTo = true);

    /*!
     * @return True when initialised, false otherwise.
     */
    bool IsInitialised(void);

    /*!
     * @brief Pings the adapter, checks the firmware version and sets controlled mode.
     * @param iTimeoutMs The timeout after which this fails if no proper data was received.
     * @return True when the checks passed, false otherwise.
     */
    bool CheckAdapter(uint32_t iTimeoutMs = CEC_DEFAULT_CONNECT_TIMEOUT);

    /*!
     * @brief Handle a poll message inside the adapter message (checks if one is present).
     * @param msg The adapter message to parse.
     * @return True when the message resulted in a CEC error, false otherwise.
     */
    bool HandlePoll(const CCECAdapterMessage &msg);

    /*!
     * @brief Read data from the device.
     * @param iTimeout The read timeout to use.
     * @param iSize The maximum read size.
     * @return True when something was read, false otherwise.
     */
    bool ReadFromDevice(uint32_t iTimeout, size_t iSize = 256);

    /*!
     * @brief Writes a message to the serial port.
     * @param message The message to write.
     * @return True when written, false otherwise.
     */
    bool WriteToDevice(CCECAdapterMessage *message);

    /*!
     * @brief Called before sending a CEC command over the line, so we know we're expecting an ack.
     * @param dest The destination of the CEC command.
     */
    void MarkAsWaiting(const cec_logical_address dest);

    /*!
     * @brief Clear and reset the message queue.
     */
    void ResetMessageQueue(void);

    PLATFORM::ISocket *                          m_port;                 /**< the com port connection */
    PLATFORM::CMutex                             m_mutex;                /**< mutex for changes in this class */
    uint8_t                                      m_iLineTimeout;         /**< the current line timeout on the CEC line */
    cec_logical_address                          m_lastPollDestination;  /**< the destination of the last poll message that was received */
    bool                                         m_bInitialised;         /**< true when the connection is initialised, false otherwise */
    bool                                         m_bWaitingForAck[15];   /**< array in which we store from which devices we're expecting acks */
    CAdapterPingThread *                         m_pingThread;           /**< ping thread, that pings the adapter every 15 seconds */
    CAdapterEepromWriteThread *                  m_eepromWriteThread;    /**< eeprom writes are done async */
    CUSBCECAdapterCommands *                     m_commands;             /**< commands that can be sent to the adapter */
    CCECAdapterMessageQueue *                    m_adapterMessageQueue;  /**< the incoming and outgoing message queue */
    cec_logical_addresses                        m_logicalAddresses;     /**< the logical address list that this instance is using */
    PLATFORM::CMutex                             m_waitingMutex;
  };

  class CAdapterEepromWriteThread : public PLATFORM::CThread
  {
  public:
    CAdapterEepromWriteThread(CUSBCECAdapterCommunication *com) :
        m_com(com),
        m_bWrite(false),
        m_iLastEepromWrite(0),
        m_iScheduleEepromWrite(0) {}
    virtual ~CAdapterEepromWriteThread(void) {}

    bool Write(void);
    void* Process(void);
    void Stop(void);
  private:
    CUSBCECAdapterCommunication *m_com;
    bool                         m_bWrite;
    PLATFORM::CCondition<bool>   m_condition;
    PLATFORM::CMutex             m_mutex;
    int64_t                      m_iLastEepromWrite;     /**< last time that this instance did an eeprom write */
    int64_t                      m_iScheduleEepromWrite; /**< in case there were more than 2 changes within 30 seconds, do another write at this time */
  };

  class CAdapterPingThread : public PLATFORM::CThread
  {
  public:
    CAdapterPingThread(CUSBCECAdapterCommunication *com, uint32_t iTimeout) :
        m_com(com),
        m_timeout(iTimeout){}
    virtual ~CAdapterPingThread(void) {}

    void* Process(void);
  private:
    CUSBCECAdapterCommunication *m_com;
    PLATFORM::CTimeout           m_timeout;
  };
};

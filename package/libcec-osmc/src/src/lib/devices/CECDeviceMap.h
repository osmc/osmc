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

#include <map>
#include <vector>

namespace CEC
{
  class CCECBusDevice;

  typedef std::map<cec_logical_address, CCECBusDevice *> CECDEVICEMAP;
  typedef std::vector<CCECBusDevice *>                   CECDEVICEVEC;

  class CCECProcessor;

  class CCECDeviceMap
  {
  public:
    CCECDeviceMap(CCECProcessor *processor);
    virtual ~CCECDeviceMap(void);
    CECDEVICEMAP::iterator  Begin(void);
    CECDEVICEMAP::iterator  End(void);
    void                    ResetDeviceStatus(void);
    CCECBusDevice *         operator[] (cec_logical_address iAddress) const;
    CCECBusDevice *         operator[] (uint8_t iAddress) const;
    CCECBusDevice *         At(cec_logical_address iAddress) const;
    CCECBusDevice *         At(uint8_t iAddress) const;
    CCECBusDevice *         GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress, bool bSuppressUpdate = true);

    void Get(CECDEVICEVEC &devices) const;
    void GetLibCECControlled(CECDEVICEVEC &devices) const;
    void GetByLogicalAddresses(CECDEVICEVEC &devices, const cec_logical_addresses &addresses);
    void GetActive(CECDEVICEVEC &devices) const;
    void GetByType(const cec_device_type type, CECDEVICEVEC &devices) const;
    void GetChildrenOf(CECDEVICEVEC& devices, CCECBusDevice* device) const;
    void SignalAll(cec_opcode opcode);

    void GetPowerOffDevices(const libcec_configuration &configuration, CECDEVICEVEC &devices) const;
    void GetWakeDevices(const libcec_configuration &configuration, CECDEVICEVEC &devices) const;

    CCECBusDevice *GetActiveSource(void) const;

    static void FilterLibCECControlled(CECDEVICEVEC &devices);
    static void FilterActive(CECDEVICEVEC &devices);
    static void FilterTypes(const cec_device_type_list &types, CECDEVICEVEC &devices);
    static void FilterType(const cec_device_type type, CECDEVICEVEC &devices);
    static cec_logical_addresses ToLogicalAddresses(const CECDEVICEVEC &devices);
  private:
    void Clear(void);

    CECDEVICEMAP   m_busDevices;
    CCECProcessor *m_processor;
  };
}

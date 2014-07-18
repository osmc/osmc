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
#include "CECDeviceMap.h"

#include "CECAudioSystem.h"
#include "CECPlaybackDevice.h"
#include "CECRecordingDevice.h"
#include "CECTuner.h"
#include "CECTV.h"
#include "lib/CECProcessor.h"
#include "lib/CECTypeUtils.h"

using namespace std;
using namespace CEC;

CCECDeviceMap::CCECDeviceMap(CCECProcessor *processor) :
    m_processor(processor)
{
  for (uint8_t iPtr = CECDEVICE_TV; iPtr <= CECDEVICE_BROADCAST; iPtr++)
  {
    switch(iPtr)
    {
    case CECDEVICE_AUDIOSYSTEM:
      m_busDevices.insert(make_pair<cec_logical_address, CCECBusDevice *>((cec_logical_address)iPtr, new CCECAudioSystem(processor, (cec_logical_address) iPtr)));
      break;
    case CECDEVICE_PLAYBACKDEVICE1:
    case CECDEVICE_PLAYBACKDEVICE2:
    case CECDEVICE_PLAYBACKDEVICE3:
      m_busDevices.insert(make_pair<cec_logical_address, CCECBusDevice *>((cec_logical_address)iPtr, new CCECPlaybackDevice(processor, (cec_logical_address) iPtr)));
      break;
    case CECDEVICE_RECORDINGDEVICE1:
    case CECDEVICE_RECORDINGDEVICE2:
    case CECDEVICE_RECORDINGDEVICE3:
      m_busDevices.insert(make_pair<cec_logical_address, CCECBusDevice *>((cec_logical_address)iPtr, new CCECRecordingDevice(processor, (cec_logical_address) iPtr)));
      break;
    case CECDEVICE_TUNER1:
    case CECDEVICE_TUNER2:
    case CECDEVICE_TUNER3:
    case CECDEVICE_TUNER4:
      m_busDevices.insert(make_pair<cec_logical_address, CCECBusDevice *>((cec_logical_address)iPtr, new CCECTuner(processor, (cec_logical_address) iPtr)));
      break;
    case CECDEVICE_TV:
      m_busDevices.insert(make_pair<cec_logical_address, CCECBusDevice *>((cec_logical_address)iPtr, new CCECTV(processor, (cec_logical_address) iPtr)));
      break;
    default:
      m_busDevices.insert(make_pair<cec_logical_address, CCECBusDevice *>((cec_logical_address)iPtr, new CCECBusDevice(processor, (cec_logical_address) iPtr)));
      break;
    }
  }
}
CCECDeviceMap::~CCECDeviceMap(void)
{
  Clear();
}

CECDEVICEMAP::iterator CCECDeviceMap::Begin(void)
{
  return m_busDevices.begin();
}

CECDEVICEMAP::iterator CCECDeviceMap::End(void)
{
  return m_busDevices.end();
}

void CCECDeviceMap::ResetDeviceStatus(void)
{
  for (CECDEVICEMAP::iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
    it->second->ResetDeviceStatus();
}

CCECBusDevice *CCECDeviceMap::operator[] (cec_logical_address iAddress) const
{
  return At(iAddress);
}

CCECBusDevice *CCECDeviceMap::operator[] (uint8_t iAddress) const
{
  return At(iAddress);
}

CCECBusDevice *CCECDeviceMap::At(cec_logical_address iAddress) const
{
  return At((uint8_t) iAddress);
}

CCECBusDevice *CCECDeviceMap::At(uint8_t iAddress) const
{
  CECDEVICEMAP::const_iterator it = m_busDevices.find((cec_logical_address)iAddress);
  if (it != m_busDevices.end())
    return it->second;
  return NULL;
}

void CCECDeviceMap::Clear(void)
{
  for (CECDEVICEMAP::iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
    delete it->second;
  m_busDevices.clear();
}

CCECBusDevice *CCECDeviceMap::GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress, bool bSuppressUpdate /* = true */)
{
  CCECBusDevice *device(NULL);

  // check each device until we found a match
  for (CECDEVICEMAP::iterator it = m_busDevices.begin(); !device && it != m_busDevices.end(); it++)
  {
    if (it->second->GetPhysicalAddress(m_processor->GetLogicalAddress(), bSuppressUpdate) == iPhysicalAddress)
      device = it->second;
  }

  return device;
}

void CCECDeviceMap::Get(CECDEVICEVEC &devices) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
    devices.push_back(it->second);
}

void CCECDeviceMap::GetByLogicalAddresses(CECDEVICEVEC &devices, const cec_logical_addresses &addresses)
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
  {
    if (addresses.IsSet(it->first))
      devices.push_back(it->second);
  }
}

void CCECDeviceMap::GetByType(const cec_device_type type, CECDEVICEVEC &devices) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
    if (it->second->GetType() == type)
      devices.push_back(it->second);
}

void CCECDeviceMap::GetLibCECControlled(CECDEVICEVEC &devices) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
    if (it->second->IsHandledByLibCEC())
      devices.push_back(it->second);
}

void CCECDeviceMap::GetActive(CECDEVICEVEC &devices) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
  {
    cec_bus_device_status status = it->second->GetStatus();
    if (status == CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC ||
        status == CEC_DEVICE_STATUS_PRESENT)
      devices.push_back(it->second);
  }
}

void CCECDeviceMap::GetPowerOffDevices(const libcec_configuration &configuration, CECDEVICEVEC &devices) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
  {
    if (configuration.powerOffDevices[(uint8_t)it->first])
      devices.push_back(it->second);
  }
}

void CCECDeviceMap::GetWakeDevices(const libcec_configuration &configuration, CECDEVICEVEC &devices) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
  {
    if (configuration.wakeDevices[(uint8_t)it->first])
      devices.push_back(it->second);
  }
}

CCECBusDevice *CCECDeviceMap::GetActiveSource(void) const
{
  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
  {
    if (it->second->IsActiveSource())
      return it->second;
  }
  return NULL;
}

void CCECDeviceMap::FilterLibCECControlled(CECDEVICEVEC &devices)
{
  CECDEVICEVEC newDevices;
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
  {
    if ((*it)->IsHandledByLibCEC())
      newDevices.push_back(*it);
  }
  devices = newDevices;
}

void CCECDeviceMap::FilterActive(CECDEVICEVEC &devices)
{
  CECDEVICEVEC newDevices;
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
  {
    cec_bus_device_status status = (*it)->GetCurrentStatus();
    if (status == CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC ||
        status == CEC_DEVICE_STATUS_PRESENT)
      newDevices.push_back(*it);
  }
  devices = newDevices;
}

void CCECDeviceMap::FilterTypes(const cec_device_type_list &types, CECDEVICEVEC &devices)
{
  cec_device_type_list t(types);//silly, but needed to retain abi
  CECDEVICEVEC newDevices;
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
  {
    if (t.IsSet((*it)->GetType()))
      newDevices.push_back(*it);
  }
  devices = newDevices;
}

void CCECDeviceMap::FilterType(const cec_device_type type, CECDEVICEVEC &devices)
{
  CECDEVICEVEC newDevices;
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
  {
    if ((*it)->GetType() == type)
      newDevices.push_back(*it);
  }
  devices = newDevices;
}

cec_logical_addresses CCECDeviceMap::ToLogicalAddresses(const CECDEVICEVEC &devices)
{
  cec_logical_addresses addresses;
  addresses.Clear();
  for (CECDEVICEVEC::const_iterator it = devices.begin(); it != devices.end(); it++)
    addresses.Set((*it)->GetLogicalAddress());
  return addresses;
}

void CCECDeviceMap::GetChildrenOf(CECDEVICEVEC& devices, CCECBusDevice* device) const
{
  devices.clear();
  if (!device)
    return;

  uint16_t iPA = device->GetCurrentPhysicalAddress();

  for (CECDEVICEMAP::const_iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
  {
    uint16_t iCurrentPA = it->second->GetCurrentPhysicalAddress();
    if (CCECTypeUtils::PhysicalAddressIsIncluded(iPA, iCurrentPA))
      devices.push_back(it->second);
  }
}

void CCECDeviceMap::SignalAll(cec_opcode opcode)
{
  for (CECDEVICEMAP::iterator it = m_busDevices.begin(); it != m_busDevices.end(); it++)
    it->second->SignalOpcode(opcode);
}

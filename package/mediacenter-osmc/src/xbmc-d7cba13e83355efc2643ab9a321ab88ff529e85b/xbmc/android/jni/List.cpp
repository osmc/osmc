/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "List.h"
#include "View.h"
#include "ScanResult.h"
#include "WifiConfiguration.h"
#include "ApplicationInfo.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

template <typename T>
T CJNIList<T>::get(int index)
{
  return (T)call_method<jhobject>(m_object,
    "get", "(I)Ljava/lang/Object;",
    index);
}

template <typename T>
int CJNIList<T>::size()
{
  return m_object.get() ? call_method<jint>(m_object,
    "size", "()I") : 0;
}

template class CJNIList<CJNIScanResult>;
template class CJNIList<CJNIWifiConfiguration>;
template class CJNIList<CJNIApplicationInfo>;
template class CJNIList<CJNIViewInputDeviceMotionRange>;

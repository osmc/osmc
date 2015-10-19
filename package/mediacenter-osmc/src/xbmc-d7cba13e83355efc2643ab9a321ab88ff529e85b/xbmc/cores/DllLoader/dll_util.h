#ifndef _DLL_UTIL_H_
#define _DLL_UTIL_H_

#include <stdint.h>

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifdef _cplusplus
extern "C"
{
#endif

uintptr_t create_dummy_function(const char* strDllName, const char* strFunctionName);
uintptr_t get_win_function_address(const char* strDllName, const char* strFunctionName);

#ifdef _cplusplus
}
#endif

#endif // _DLL_UTIL_H_

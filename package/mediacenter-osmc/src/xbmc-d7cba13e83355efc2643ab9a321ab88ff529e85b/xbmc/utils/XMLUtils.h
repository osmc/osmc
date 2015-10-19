#pragma once

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

#include <string>
#include <stdint.h>
#include <vector>
#include "utils/XBMCTinyXML.h"

class CDateTime;

class XMLUtils
{
public:
  static bool HasChild(const TiXmlNode* pRootNode, const char* strTag);

  static bool GetHex(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwHexValue);
  static bool GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwUIntValue);
  static bool GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value);
  static bool GetDouble(const TiXmlNode* pRootNode, const char* strTag, double &value);
  static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);
  
  /*! \brief Get a string value from the xml tag
   If the specified tag isn't found strStringvalue is not modified and will contain whatever
   value it had before the method call.

   \param[in]     pRootNode the xml node that contains the tag
   \param[in]     strTag  the xml tag to read from
   \param[in,out] strStringValue  where to store the read string
   \return true on success, false if the tag isn't found
   */
  static bool GetString(const TiXmlNode* pRootNode, const char* strTag, std::string& strStringValue);

  /*! \brief Get a string value from the xml tag
      
   \param[in]  pRootNode the xml node that contains the tag
   \param[in]  strTag the tag to read from
   
   \return the value in the specified tag or an empty string if the tag isn't found
   */
  static std::string GetString(const TiXmlNode* pRootNode, const char* strTag);
  /*! \brief Get multiple tags, concatenating the values together.
   Transforms
     <tag>value1</tag>
     <tag clear="true">value2</tag>
     ...
     <tag>valuen</tag>
   into value2<sep>...<sep>valuen, appending it to the value string. Note that <value1> is overwritten by the clear="true" tag.

   \param rootNode    the parent containing the <tag>'s.
   \param tag         the <tag> in question.
   \param separator   the separator to use when concatenating values.
   \param value [out] the resulting string. Remains untouched if no <tag> is available, else is appended (or cleared based on the clear parameter).
   \param clear       if true, clears the string prior to adding tags, if tags are available. Defaults to false.
   */
  static bool GetAdditiveString(const TiXmlNode* rootNode, const char* tag, const std::string& separator, std::string& value, bool clear = false);
  static bool GetStringArray(const TiXmlNode* rootNode, const char* tag, std::vector<std::string>& arrayValue, bool clear = false, const std::string& separator = "");
  static bool GetPath(const TiXmlNode* pRootNode, const char* strTag, std::string& strStringValue);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value, const float min, const float max);
  static bool GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwUIntValue, const uint32_t min, const uint32_t max);
  static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue, const int min, const int max);
  static bool GetDate(const TiXmlNode* pRootNode, const char* strTag, CDateTime& date);
  static bool GetDateTime(const TiXmlNode* pRootNode, const char* strTag, CDateTime& dateTime);
  /*! \brief Fetch a std::string copy of an attribute, if it exists.  Cannot distinguish between empty and non-existent attributes.
   \param element the element to query.
   \param tag the name of the attribute.
   \return the attribute, if it exists, else an empty string
   */
  static std::string GetAttribute(const TiXmlElement *element, const char *tag);

  static void SetString(TiXmlNode* pRootNode, const char *strTag, const std::string& strValue);
  static void SetAdditiveString(TiXmlNode* pRootNode, const char *strTag, const std::string& strSeparator, const std::string& strValue);
  static void SetStringArray(TiXmlNode* pRootNode, const char *strTag, const std::vector<std::string>& arrayValue);
  static void SetInt(TiXmlNode* pRootNode, const char *strTag, int value);
  static void SetFloat(TiXmlNode* pRootNode, const char *strTag, float value);
  static void SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value);
  static void SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value);
  static void SetPath(TiXmlNode* pRootNode, const char *strTag, const std::string& strValue);
  static void SetLong(TiXmlNode* pRootNode, const char *strTag, long iValue);
  static void SetDate(TiXmlNode* pRootNode, const char *strTag, const CDateTime& date);
  static void SetDateTime(TiXmlNode* pRootNode, const char *strTag, const CDateTime& dateTime);

  static const int path_version = 1;
};


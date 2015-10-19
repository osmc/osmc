/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "ScraperParser.h"

#include "addons/AddonManager.h"
#include "RegExp.h"
#include "HTMLUtil.h"
#include "addons/Scraper.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "log.h"
#include "CharsetConverter.h"
#include "utils/XSLTUtils.h"
#include "utils/XMLUtils.h"
#include <sstream>
#include <cstring>

using namespace ADDON;
using namespace XFILE;

CScraperParser::CScraperParser()
{
  m_pRootElement = NULL;
  m_document = NULL;
  m_SearchStringEncoding = "UTF-8";
  m_scraper = NULL;
  m_isNoop = true;
}

CScraperParser::CScraperParser(const CScraperParser& parser)
{
  m_pRootElement = NULL;
  m_document = NULL;
  m_SearchStringEncoding = "UTF-8";
  m_scraper = NULL;
  m_isNoop = true;
  *this = parser;
}

CScraperParser &CScraperParser::operator=(const CScraperParser &parser)
{
  if (this != &parser)
  {
    Clear();
    if (parser.m_document)
    {
      m_scraper = parser.m_scraper;
      m_document = new CXBMCTinyXML(*parser.m_document);
      LoadFromXML();
    }
    else
      m_scraper = NULL;
  }
  return *this;
}

CScraperParser::~CScraperParser()
{
  Clear();
}

void CScraperParser::Clear()
{
  m_pRootElement = NULL;
  delete m_document;

  m_document = NULL;
  m_strFile.clear();
}

bool CScraperParser::Load(const std::string& strXMLFile)
{
  Clear();

  m_document = new CXBMCTinyXML();

  if (!m_document)
    return false;

  m_strFile = strXMLFile;

  if (m_document->LoadFile(strXMLFile))
    return LoadFromXML();

  delete m_document;
  m_document = NULL;
  return false;
}

bool CScraperParser::LoadFromXML()
{
  if (!m_document)
    return false;

  m_pRootElement = m_document->RootElement();
  std::string strValue = m_pRootElement->ValueStr();
  if (strValue == "scraper")
  {
    TiXmlElement* pChildElement = m_pRootElement->FirstChildElement("CreateSearchUrl");
    if (pChildElement)
    {
      m_isNoop = false;
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }

    pChildElement = m_pRootElement->FirstChildElement("CreateArtistSearchUrl");
    if (pChildElement)
    {
      m_isNoop = false;
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }
    pChildElement = m_pRootElement->FirstChildElement("CreateAlbumSearchUrl");
    if (pChildElement)
    {
      m_isNoop = false;
      if (!(m_SearchStringEncoding = pChildElement->Attribute("SearchStringEncoding")))
        m_SearchStringEncoding = "UTF-8";
    }

    return true;
  }

  delete m_document;
  m_document = NULL;
  m_pRootElement = NULL;
  return false;
}

void CScraperParser::ReplaceBuffers(std::string& strDest)
{
  // insert buffers
  size_t iIndex;
  for (int i=MAX_SCRAPER_BUFFERS-1; i>=0; i--)
  {
    iIndex = 0;
    std::string temp = StringUtils::Format("$$%i",i+1);
    while ((iIndex = strDest.find(temp,iIndex)) != std::string::npos)
    {
      strDest.replace(strDest.begin()+iIndex,strDest.begin()+iIndex+temp.size(),m_param[i]);
      iIndex += m_param[i].length();
    }
  }
  // insert settings
  iIndex = 0;
  while ((iIndex = strDest.find("$INFO[", iIndex)) != std::string::npos)
  {
    size_t iEnd = strDest.find("]", iIndex);
    std::string strInfo = strDest.substr(iIndex+6, iEnd - iIndex - 6);
    std::string strReplace;
    if (m_scraper)
      strReplace = m_scraper->GetSetting(strInfo);
    strDest.replace(strDest.begin()+iIndex,strDest.begin()+iEnd+1,strReplace);
    iIndex += strReplace.length();
  }
  // insert localize strings
  iIndex = 0;
  while ((iIndex = strDest.find("$LOCALIZE[", iIndex)) != std::string::npos)
  {
    size_t iEnd = strDest.find("]", iIndex);
    std::string strInfo = strDest.substr(iIndex+10, iEnd - iIndex - 10);
    std::string strReplace;
    if (m_scraper)
      strReplace = m_scraper->GetString(strtol(strInfo.c_str(),NULL,10));
    strDest.replace(strDest.begin()+iIndex,strDest.begin()+iEnd+1,strReplace);
    iIndex += strReplace.length();
  }
  iIndex = 0;
  while ((iIndex = strDest.find("\\n",iIndex)) != std::string::npos)
    strDest.replace(strDest.begin()+iIndex,strDest.begin()+iIndex+2,"\n");
}

void CScraperParser::ParseExpression(const std::string& input, std::string& dest, TiXmlElement* element, bool bAppend)
{
  std::string strOutput = XMLUtils::GetAttribute(element, "output");

  TiXmlElement* pExpression = element->FirstChildElement("expression");
  if (pExpression)
  {
    bool bInsensitive=true;
    const char* sensitive = pExpression->Attribute("cs");
    if (sensitive)
      if (stricmp(sensitive,"yes") == 0)
        bInsensitive=false; // match case sensitive

    CRegExp::utf8Mode eUtf8 = CRegExp::autoUtf8;
    const char* const strUtf8 = pExpression->Attribute("utf8");
    if (strUtf8)
    {
      if (stricmp(strUtf8, "yes") == 0)
        eUtf8 = CRegExp::forceUtf8;
      else if (stricmp(strUtf8, "no") == 0)
        eUtf8 = CRegExp::asciiOnly;
      else if (stricmp(strUtf8, "auto") == 0)
        eUtf8 = CRegExp::autoUtf8;
    }

    CRegExp reg(bInsensitive, eUtf8);
    std::string strExpression;
    if (pExpression->FirstChild())
      strExpression = pExpression->FirstChild()->Value();
    else
      strExpression = "(.*)";
    ReplaceBuffers(strExpression);
    ReplaceBuffers(strOutput);

    if (!reg.RegComp(strExpression.c_str()))
    {
      return;
    }

    bool bRepeat = false;
    const char* szRepeat = pExpression->Attribute("repeat");
    if (szRepeat)
      if (stricmp(szRepeat,"yes") == 0)
        bRepeat = true;

    const char* szClear = pExpression->Attribute("clear");
    if (szClear)
      if (stricmp(szClear,"yes") == 0)
        dest=""; // clear no matter if regexp fails

    bool bClean[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bClean,pExpression->Attribute("noclean"),true);

    bool bTrim[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bTrim,pExpression->Attribute("trim"),false);

    bool bFixChars[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bFixChars,pExpression->Attribute("fixchars"),false);

    bool bEncode[MAX_SCRAPER_BUFFERS];
    GetBufferParams(bEncode,pExpression->Attribute("encode"),false);

    int iOptional = -1;
    pExpression->QueryIntAttribute("optional",&iOptional);

    int iCompare = -1;
    pExpression->QueryIntAttribute("compare",&iCompare);
    if (iCompare > -1)
      StringUtils::ToLower(m_param[iCompare-1]);
    std::string curInput = input;
    for (int iBuf=0;iBuf<MAX_SCRAPER_BUFFERS;++iBuf)
    {
      if (bClean[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!CLEAN!!!");
      if (bTrim[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!TRIM!!!");
      if (bFixChars[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!FIXCHARS!!!");
      if (bEncode[iBuf])
        InsertToken(strOutput,iBuf+1,"!!!ENCODE!!!");
    }
    int i = reg.RegFind(curInput.c_str());
    while (i > -1 && (i < (int)curInput.size() || curInput.size() == 0))
    {
      if (!bAppend)
      {
        dest = "";
        bAppend = true;
      }
      std::string strCurOutput=strOutput;

      if (iOptional > -1) // check that required param is there
      {
        char temp[4];
        sprintf(temp,"\\%i",iOptional);
        std::string szParam = reg.GetReplaceString(temp);
        CRegExp reg2;
        reg2.RegComp("(.*)(\\\\\\(.*\\\\2.*)\\\\\\)(.*)");
        int i2=reg2.RegFind(strCurOutput.c_str());
        while (i2 > -1)
        {
          std::string szRemove(reg2.GetMatch(2));
          int iRemove = szRemove.size();
          int i3 = strCurOutput.find(szRemove);
          if (!szParam.empty())
          {
            strCurOutput.erase(i3+iRemove,2);
            strCurOutput.erase(i3,2);
          }
          else
            strCurOutput.replace(strCurOutput.begin()+i3,strCurOutput.begin()+i3+iRemove+2,"");

          i2 = reg2.RegFind(strCurOutput.c_str());
        }
      }

      int iLen = reg.GetFindLen();
      // nasty hack #1 - & means \0 in a replace string
      StringUtils::Replace(strCurOutput, "&","!!!AMPAMP!!!");
      std::string result = reg.GetReplaceString(strCurOutput.c_str());
      if (!result.empty())
      {
        std::string strResult(result);
        StringUtils::Replace(strResult, "!!!AMPAMP!!!","&");
        Clean(strResult);
        ReplaceBuffers(strResult);
        if (iCompare > -1)
        {
          std::string strResultNoCase = strResult;
          StringUtils::ToLower(strResultNoCase);
          if (strResultNoCase.find(m_param[iCompare-1]) != std::string::npos)
            dest += strResult;
        }
        else
          dest += strResult;
      }
      if (bRepeat && iLen > 0)
      {
        curInput.erase(0,i+iLen>(int)curInput.size()?curInput.size():i+iLen);
        i = reg.RegFind(curInput.c_str());
      }
      else
        i = -1;
    }
  }
}

void CScraperParser::ParseXSLT(const std::string& input, std::string& dest, TiXmlElement* element, bool bAppend)
{
  TiXmlElement* pSheet = element->FirstChildElement();
  if (pSheet)
  {
    XSLTUtils xsltUtils;
    std::string strXslt;
    strXslt << *pSheet;
    ReplaceBuffers(strXslt);

    if (!xsltUtils.SetInput(input))
      CLog::Log(LOGDEBUG, "could not parse input XML");

    if (!xsltUtils.SetStylesheet(strXslt))
      CLog::Log(LOGDEBUG, "could not parse stylesheet XML");

    xsltUtils.XSLTTransform(dest);
  }
}

TiXmlElement *FirstChildScraperElement(TiXmlElement *element)
{
  for (TiXmlElement *child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    if (child->ValueStr() == "RegExp" || child->ValueStr() == "XSLT")
      return child;
  }
  return NULL;
}

TiXmlElement *NextSiblingScraperElement(TiXmlElement *element)
{
  for (TiXmlElement *next = element->NextSiblingElement(); next; next = next->NextSiblingElement())
  {
    if (next->ValueStr() == "RegExp" || next->ValueStr() == "XSLT")
      return next;
  }
  return NULL;
}

void CScraperParser::ParseNext(TiXmlElement* element)
{
  TiXmlElement* pReg = element;
  while (pReg)
  {
    TiXmlElement* pChildReg = FirstChildScraperElement(pReg);
    if (pChildReg)
      ParseNext(pChildReg);
    else
    {
      TiXmlElement* pChildReg = pReg->FirstChildElement("clear");
      if (pChildReg)
        ParseNext(pChildReg);
    }

    int iDest = 1;
    bool bAppend = false;
    const char* szDest = pReg->Attribute("dest");
    if (szDest && strlen(szDest))
    {
      if (szDest[strlen(szDest)-1] == '+')
        bAppend = true;

      iDest = atoi(szDest);
    }

    const char *szInput = pReg->Attribute("input");
    std::string strInput;
    if (szInput)
    {
      strInput = szInput;
      ReplaceBuffers(strInput);
    }
    else
      strInput = m_param[0];

    const char* szConditional = pReg->Attribute("conditional");
    bool bExecute = true;
    if (szConditional)
    {
      bool bInverse=false;
      if (szConditional[0] == '!')
      {
        bInverse = true;
        szConditional++;
      }
      std::string strSetting;
      if (m_scraper && m_scraper->HasSettings())
        strSetting = m_scraper->GetSetting(szConditional);
      bExecute = bInverse != (strSetting == "true");
    }

    if (bExecute)
    {
      if (iDest-1 < MAX_SCRAPER_BUFFERS && iDest-1 > -1)
      {
        if (pReg->ValueStr() == "XSLT")
          ParseXSLT(strInput, m_param[iDest - 1], pReg, bAppend);
        else
          ParseExpression(strInput, m_param[iDest - 1],pReg,bAppend);
      }
      else
        CLog::Log(LOGERROR,"CScraperParser::ParseNext: destination buffer "
                           "out of bounds, skipping expression");
    }
    pReg = NextSiblingScraperElement(pReg);
  }
}

const std::string CScraperParser::Parse(const std::string& strTag,
                                       CScraper* scraper)
{
  TiXmlElement* pChildElement = m_pRootElement->FirstChildElement(strTag.c_str());
  if(pChildElement == NULL)
  {
    CLog::Log(LOGERROR,"%s: Could not find scraper function %s",__FUNCTION__,strTag.c_str());
    return "";
  }
  int iResult = 1; // default to param 1
  pChildElement->QueryIntAttribute("dest",&iResult);
  TiXmlElement* pChildStart = FirstChildScraperElement(pChildElement);
  m_scraper = scraper;
  ParseNext(pChildStart);
  std::string tmp = m_param[iResult-1];

  const char* szClearBuffers = pChildElement->Attribute("clearbuffers");
  if (!szClearBuffers || stricmp(szClearBuffers,"no") != 0)
    ClearBuffers();

  return tmp;
}

void CScraperParser::Clean(std::string& strDirty)
{
  size_t i = 0;
  std::string strBuffer;
  while ((i = strDirty.find("!!!CLEAN!!!",i)) != std::string::npos)
  {
    size_t i2;
    if ((i2 = strDirty.find("!!!CLEAN!!!",i+11)) != std::string::npos)
    {
      strBuffer = strDirty.substr(i+11,i2-i-11);
      std::string strConverted(strBuffer);
      HTML::CHTMLUtil::RemoveTags(strConverted);
      StringUtils::Trim(strConverted);
      strDirty.replace(i, i2-i+11, strConverted);
      i += strConverted.size();
    }
    else
      break;
  }
  i=0;
  while ((i = strDirty.find("!!!TRIM!!!",i)) != std::string::npos)
  {
    size_t i2;
    if ((i2 = strDirty.find("!!!TRIM!!!",i+10)) != std::string::npos)
    {
      strBuffer = strDirty.substr(i+10,i2-i-10);
      StringUtils::Trim(strBuffer);
      strDirty.replace(i, i2-i+10, strBuffer);
      i += strBuffer.size();
    }
    else
      break;
  }
  i=0;
  while ((i = strDirty.find("!!!FIXCHARS!!!",i)) != std::string::npos)
  {
    size_t i2;
    if ((i2 = strDirty.find("!!!FIXCHARS!!!",i+14)) != std::string::npos)
    {
      strBuffer = strDirty.substr(i+14,i2-i-14);
      std::wstring wbuffer;
      g_charsetConverter.utf8ToW(strBuffer, wbuffer, false, false, false);
      std::wstring wConverted;
      HTML::CHTMLUtil::ConvertHTMLToW(wbuffer,wConverted);
      g_charsetConverter.wToUTF8(wConverted, strBuffer, false);
      StringUtils::Trim(strBuffer);
      ConvertJSON(strBuffer);
      strDirty.replace(i, i2-i+14, strBuffer);
      i += strBuffer.size();
    }
    else
      break;
  }
  i=0;
  while ((i=strDirty.find("!!!ENCODE!!!",i)) != std::string::npos)
  {
    size_t i2;
    if ((i2 = strDirty.find("!!!ENCODE!!!",i+12)) != std::string::npos)
    {
      strBuffer = CURL::Encode(strDirty.substr(i + 12, i2 - i - 12));
      strDirty.replace(i, i2-i+12, strBuffer);
      i += strBuffer.size();
    }
    else
      break;
  }
}

void CScraperParser::ConvertJSON(std::string &string)
{
  CRegExp reg;
  reg.RegComp("\\\\u([0-f]{4})");
  while (reg.RegFind(string.c_str()) > -1)
  {
    int pos = reg.GetSubStart(1);
    std::string szReplace(reg.GetMatch(1));

    std::string replace = StringUtils::Format("&#x%s;", szReplace.c_str());
    string.replace(string.begin()+pos-2, string.begin()+pos+4, replace);
  }

  CRegExp reg2;
  reg2.RegComp("\\\\x([0-9]{2})([^\\\\]+;)");
  while (reg2.RegFind(string.c_str()) > -1)
  {
    int pos1 = reg2.GetSubStart(1);
    int pos2 = reg2.GetSubStart(2);
    std::string szHexValue(reg2.GetMatch(1));

    std::string replace = StringUtils::Format("%li", strtol(szHexValue.c_str(), NULL, 16));
    string.replace(string.begin()+pos1-2, string.begin()+pos2+reg2.GetSubLength(2), replace);
  }

  StringUtils::Replace(string, "\\\"","\"");
}

void CScraperParser::ClearBuffers()
{
  //clear all m_param strings
  for (int i=0;i<MAX_SCRAPER_BUFFERS;++i)
    m_param[i].clear();
}

void CScraperParser::GetBufferParams(bool* result, const char* attribute, bool defvalue)
{
  for (int iBuf=0;iBuf<MAX_SCRAPER_BUFFERS;++iBuf)
    result[iBuf] = defvalue;
  if (attribute)
  {
    std::vector<std::string> vecBufs;
    StringUtils::Tokenize(attribute,vecBufs,",");
    for (size_t nToken=0; nToken < vecBufs.size(); nToken++)
    {
      int index = atoi(vecBufs[nToken].c_str())-1;
      if (index < MAX_SCRAPER_BUFFERS)
        result[index] = !defvalue;
    }
  }
}

void CScraperParser::InsertToken(std::string& strOutput, int buf, const char* token)
{
  char temp[4];
  sprintf(temp,"\\%i",buf);
  size_t i2=0;
  while ((i2 = strOutput.find(temp,i2)) != std::string::npos)
  {
    strOutput.insert(i2,token);
    i2 += strlen(token) + strlen(temp);
    strOutput.insert(i2,token);
  }
}

void CScraperParser::AddDocument(const CXBMCTinyXML* doc)
{
  const TiXmlNode* node = doc->RootElement()->FirstChild();
  while (node)
  {
    m_pRootElement->InsertEndChild(*node);
    node = node->NextSibling();
  }
}


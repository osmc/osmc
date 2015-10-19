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

#include <iostream>
#include <string>

#include "PlayListWPL.h"
#include "Util.h"
#include "utils/XBMCTinyXML.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

using namespace XFILE;
using namespace PLAYLIST;

/* ------------------------ example wpl playlist file ---------------------------------
  <?wpl version="1.0"?>
  <smil>
      <head>
          <meta name="Generator" content="Microsoft Windows Media Player -- 10.0.0.3646"/>
          <author/>
          <title>Playlist</title>
      </head>
      <body>
          <seq>
              <media src="E:\MP3\Track1.mp3"/>
              <media src="E:\MP3\Track2.mp3"/>
              <media src="E:\MP3\track3.mp3"/>
          </seq>
      </body>
  </smil>
------------------------ end of example wpl playlist file ---------------------------------*/
//Note: File is utf-8 encoded by default

CPlayListWPL::CPlayListWPL(void)
{}

CPlayListWPL::~CPlayListWPL(void)
{}


bool CPlayListWPL::LoadData(std::istream& stream)
{
  CXBMCTinyXML xmlDoc;

  stream >> xmlDoc;
  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "Unable to parse B4S info Error: %s", xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement ) return false;

  TiXmlElement* pHeadElement = pRootElement->FirstChildElement("head");
  if (pHeadElement )
  {
    TiXmlElement* pTitelElement = pHeadElement->FirstChildElement("title");
    if (pTitelElement )
      m_strPlayListName = pTitelElement->Value();
  }

  TiXmlElement* pBodyElement = pRootElement->FirstChildElement("body");
  if (!pBodyElement ) return false;

  TiXmlElement* pSeqElement = pBodyElement->FirstChildElement("seq");
  if (!pSeqElement ) return false;

  TiXmlElement* pMediaElement = pSeqElement->FirstChildElement("media");

  if (!pMediaElement) return false;
  while (pMediaElement)
  {
    std::string strFileName = XMLUtils::GetAttribute(pMediaElement, "src");
    if (!strFileName.empty())
    {
      std::string strFileNameClean = URIUtils::SubstitutePath(strFileName);
      CUtil::GetQualifiedFilename(m_strBasePath, strFileNameClean);
      std::string strDescription = URIUtils::GetFileName(strFileNameClean);
      CFileItemPtr newItem(new CFileItem(strDescription));
      newItem->SetPath(strFileNameClean);
      Add(newItem);
    }
    pMediaElement = pMediaElement->NextSiblingElement();
  }
  return true;
}

void CPlayListWPL::Save(const std::string& strFileName) const
{
  if (!m_vecItems.size()) return ;
  std::string strPlaylist = CUtil::MakeLegalPath(strFileName);
  CFile file;
  if (!file.OpenForWrite(strPlaylist, true))
  {
    CLog::Log(LOGERROR, "Could not save WPL playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  std::string write;
  write += StringUtils::Format("<?wpl version=%c1.0%c>\n", 34, 34);
  write += StringUtils::Format("<smil>\n");
  write += StringUtils::Format("    <head>\n");
  write += StringUtils::Format("        <meta name=%cGenerator%c content=%cMicrosoft Windows Media Player -- 10.0.0.3646%c/>\n", 34, 34, 34, 34);
  write += StringUtils::Format("        <author/>\n");
  write += StringUtils::Format("        <title>%s</title>\n", m_strPlayListName.c_str());
  write += StringUtils::Format("    </head>\n");
  write += StringUtils::Format("    <body>\n");
  write += StringUtils::Format("        <seq>\n");
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    write += StringUtils::Format("            <media src=%c%s%c/>", 34, item->GetPath().c_str(), 34);
  }
  write += StringUtils::Format("        </seq>\n");
  write += StringUtils::Format("    </body>\n");
  write += StringUtils::Format("</smil>\n");
  file.Write(write.c_str(), write.size());
  file.Close();
}

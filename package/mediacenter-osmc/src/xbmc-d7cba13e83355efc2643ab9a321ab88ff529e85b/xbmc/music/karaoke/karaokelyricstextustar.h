#ifndef KARAOKELYRICSTEXTUSTAR_H
#define KARAOKELYRICSTEXTUSTAR_H

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

// C++ Interface: karaokelyricstextlrc

#include "karaokelyricstext.h"


//! This class loads UltraStar format lyrics
class CKaraokeLyricsTextUStar : public CKaraokeLyricsText
{
  public:
    CKaraokeLyricsTextUStar( const std::string & lyricsFile );
    ~CKaraokeLyricsTextUStar();

    //! Parses the lyrics or song file, and loads the lyrics into memory. Returns true if the
    //! lyrics are successfully loaded, false otherwise.
    bool Load();

    static bool isValidFile( const std::string & lyricsFile );

  private:
    static std::vector<std::string> readFile( const std::string & lyricsFile, bool report_errors );

  private:
    std::string     m_lyricsFile;
};

#endif

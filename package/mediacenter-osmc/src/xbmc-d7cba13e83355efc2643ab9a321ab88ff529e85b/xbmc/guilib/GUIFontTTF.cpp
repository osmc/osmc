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

#include "GUIFont.h"
#include "GUIFontTTF.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "URL.h"
#include "filesystem/File.h"
#include "threads/SystemClock.h"

#include <math.h>
#include <memory>
#include <queue>

// stuff for freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_STROKER_H

#define USE_RELEASE_LIBS

#ifdef TARGET_WINDOWS
#pragma comment(lib, "freetype246MT.lib")
#endif

#define CHARS_PER_TEXTURE_LINE 20 // number of characters to cache per texture line
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)


class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary()
  {
    m_library = NULL;
  }

  virtual ~CFreeTypeLibrary()
  {
    if (m_library)
      FT_Done_FreeType(m_library);
  }

  FT_Face GetFont(const std::string &filename, float size, float aspect, XUTILS::auto_buffer& memoryBuf)
  {
    // don't have it yet - create it
    if (!m_library)
      FT_Init_FreeType(&m_library);
    if (!m_library)
    {
      CLog::Log(LOGERROR, "Unable to initialize freetype library");
      return NULL;
    }

    FT_Face face;

    // ok, now load the font face
    CURL realFile(CSpecialProtocol::TranslatePath(filename));
    if (realFile.GetFileName().empty())
      return NULL;

    memoryBuf.clear();
#ifndef TARGET_WINDOWS
    if (!realFile.GetProtocol().empty())
#endif // ! TARGET_WINDOWS
    {
      // load file into memory if it is not on local drive
      // in case of win32: always load file into memory as filename is in UTF-8,
      //                   but freetype expect filename in ANSI encoding
      XFILE::CFile f;
      if (f.LoadFile(realFile, memoryBuf) <= 0)
        return NULL;
      if (FT_New_Memory_Face(m_library, (const FT_Byte*)memoryBuf.get(), memoryBuf.size(), 0, &face) != 0)
        return NULL;
    }
#ifndef TARGET_WINDOWS
    else if (FT_New_Face( m_library, realFile.GetFileName().c_str(), 0, &face ))
      return NULL;
#endif // ! TARGET_WINDOWS

    unsigned int ydpi = 72; // 72 points to the inch is the freetype default
    unsigned int xdpi = (unsigned int)MathUtils::round_int(ydpi * aspect);

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size( face, 0, (int)(size*64 + 0.5f), xdpi, ydpi ))
    {
      FT_Done_Face(face);
      return NULL;
    }

    return face;
  };
  
  FT_Stroker GetStroker()
  {
    if (!m_library)
      return NULL;

    FT_Stroker stroker;
    if (FT_Stroker_New(m_library, &stroker))
      return NULL;

    return stroker;
  };

  static void ReleaseFont(FT_Face face)
  {
    assert(face);
    FT_Done_Face(face);
  };
  
  static void ReleaseStroker(FT_Stroker stroker)
  {
    assert(stroker);
    FT_Stroker_Done(stroker);
  }

private:
  FT_Library   m_library;
};

XBMC_GLOBAL_REF(CFreeTypeLibrary, g_freeTypeLibrary); // our freetype library
#define g_freeTypeLibrary XBMC_GLOBAL_USE(CFreeTypeLibrary)

CGUIFontTTFBase::CGUIFontTTFBase(const std::string& strFileName) : m_staticCache(*this), m_dynamicCache(*this)
{
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_nestedBeginCount = 0;

  m_vertex.reserve(4*1024);

  m_face = NULL;
  m_stroker = NULL;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_strFileName = strFileName;
  m_referenceCount = 0;
  m_originX = m_originY = 0.0f;
  m_cellBaseLine = m_cellHeight = 0;
  m_numChars = 0;
  m_posX = m_posY = 0;
  m_textureHeight = m_textureWidth = 0;
  m_textureScaleX = m_textureScaleY = 0.0;
  m_ellipsesWidth = m_height = 0.0f;
  m_color = 0;
  m_nTexture = 0;
}

CGUIFontTTFBase::~CGUIFontTTFBase(void)
{
  Clear();
}

void CGUIFontTTFBase::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTFBase::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}


void CGUIFontTTFBase::ClearCharacterCache()
{
  delete(m_texture);

  DeleteHardwareTexture();

  m_texture = NULL;
  delete[] m_char;
  m_char = new Character[CHAR_CHUNK];
  memset(m_charquick, 0, sizeof(m_charquick));
  m_numChars = 0;
  m_maxChars = CHAR_CHUNK;
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)GetTextureLineHeight();
  m_textureHeight = 0;
}

void CGUIFontTTFBase::Clear()
{
  delete(m_texture);
  m_texture = NULL;
  delete[] m_char;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_posX = 0;
  m_posY = 0;
  m_nestedBeginCount = 0;

  if (m_face)
    g_freeTypeLibrary.ReleaseFont(m_face);
  m_face = NULL;
  if (m_stroker)
    g_freeTypeLibrary.ReleaseStroker(m_stroker);
  m_stroker = NULL;

  m_vertexTrans.clear();
  m_vertex.clear();

  m_strFileName.clear();
  m_fontFileInMemory.clear();
}

bool CGUIFontTTFBase::Load(const std::string& strFilename, float height, float aspect, float lineSpacing, bool border)
{
  // we now know that this object is unique - only the GUIFont objects are non-unique, so no need
  // for reference tracking these fonts
  m_face = g_freeTypeLibrary.GetFont(strFilename, height, aspect, m_fontFileInMemory);

  if (!m_face)
    return false;

  /*
   the values used are described below

      XBMC coords                                     Freetype coords

                0  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  bbox.yMax, ascender
                        A                 \
                       A A                |
                      A   A               |
                      AAAAA  pppp   cellAscender
                      A   A  p   p        |
                      A   A  p   p        |
   m_cellBaseLine  _ _A_ _A_ pppp_ _ _ _ _/_ _ _ _ _  0, base line.
                             p            \
                             p      cellDescender
     m_cellHeight  _ _ _ _ _ p _ _ _ _ _ _/_ _ _ _ _  bbox.yMin, descender

   */
  int cellDescender = std::min<int>(m_face->bbox.yMin, m_face->descender);
  int cellAscender  = std::max<int>(m_face->bbox.yMax, m_face->ascender);

  if (border)
  {
    /*
     add on the strength of any border - the non-bordered font needs
     aligning with the bordered font by utilising GetTextBaseLine()
     */
    FT_Pos strength = FT_MulFix( m_face->units_per_EM, m_face->size->metrics.y_scale) / 12;
    if (strength < 128)
      strength = 128;

    cellDescender -= strength;
    cellAscender  += strength;

    m_stroker = g_freeTypeLibrary.GetStroker();
    if (m_stroker)
      FT_Stroker_Set(m_stroker, strength, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
  }

  // scale to pixel sizing, rounding so that maximal extent is obtained
  float scaler  = height / m_face->units_per_EM;
  cellDescender = MathUtils::round_int(cellDescender * scaler - 0.5f);   // round down
  cellAscender  = MathUtils::round_int(cellAscender  * scaler + 0.5f);   // round up

  m_cellBaseLine = cellAscender;
  m_cellHeight   = cellAscender - cellDescender;

  m_height = height;

  delete(m_texture);
  m_texture = NULL;
  delete[] m_char;
  m_char = NULL;

  m_maxChars = 0;
  m_numChars = 0;

  m_strFilename = strFilename;

  m_textureHeight = 0;
  m_textureWidth = ((m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;

  m_textureWidth = CBaseTexture::PadPow2(m_textureWidth);

  if (m_textureWidth > g_Windowing.GetMaxTextureSize())
    m_textureWidth = g_Windowing.GetMaxTextureSize();
  m_textureScaleX = 1.0f / m_textureWidth;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)GetTextureLineHeight();

  // cache the ellipses width
  Character *ellipse = GetCharacter(L'.');
  if (ellipse) m_ellipsesWidth = ellipse->advance;

  return true;
}

void CGUIFontTTFBase::Begin()
{
  if (m_nestedBeginCount == 0 && m_texture != NULL && FirstBegin())
  {
    m_vertexTrans.clear();
    m_vertex.clear();
  }
  // Keep track of the nested begin/end calls.
  m_nestedBeginCount++;
}

void CGUIFontTTFBase::End()
{
  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

  LastEnd();
}

void CGUIFontTTFBase::DrawTextInternal(float x, float y, const vecColors &colors, const vecText &text, uint32_t alignment, float maxPixelWidth, bool scrolling)
{
  Begin();

  uint32_t rawAlignment = alignment;
  bool dirtyCache(false);
  bool hardwareClipping = g_Windowing.ScissorsCanEffectClipping();
  CGUIFontCacheStaticPosition staticPos(x, y);
  CGUIFontCacheDynamicPosition dynamicPos;
  if (hardwareClipping)
  {
    dynamicPos = CGUIFontCacheDynamicPosition(g_graphicsContext.ScaleFinalXCoord(x, y),
                                              g_graphicsContext.ScaleFinalYCoord(x, y),
                                              g_graphicsContext.ScaleFinalZCoord(x, y));
  }
  CVertexBuffer unusedVertexBuffer;
  CVertexBuffer &vertexBuffer = hardwareClipping ?
      m_dynamicCache.Lookup(dynamicPos,
                            colors, text,
                            alignment, maxPixelWidth,
                            scrolling,
                            XbmcThreads::SystemClockMillis(),
                            dirtyCache) :
      unusedVertexBuffer;
  std::shared_ptr<std::vector<SVertex> > tempVertices = std::make_shared<std::vector<SVertex> >();
  std::shared_ptr<std::vector<SVertex> > &vertices = hardwareClipping ?
      tempVertices :
      static_cast<std::shared_ptr<std::vector<SVertex> >&>(m_staticCache.Lookup(staticPos,
                           colors, text,
                           alignment, maxPixelWidth,
                           scrolling,
                           XbmcThreads::SystemClockMillis(),
                           dirtyCache));
  if (dirtyCache)
  {
    // save the origin, which is scaled separately
    m_originX = x;
    m_originY = y;

    // Check if we will really need to truncate or justify the text
    if ( alignment & XBFONT_TRUNCATED )
    {
      if ( maxPixelWidth <= 0.0f || GetTextWidthInternal(text.begin(), text.end()) <= maxPixelWidth)
        alignment &= ~XBFONT_TRUNCATED;
    }
    else if ( alignment & XBFONT_JUSTIFIED )
    {
      if ( maxPixelWidth <= 0.0f )
        alignment &= ~XBFONT_JUSTIFIED;
    }

    // calculate sizing information
    float startX = 0;
    float startY = (alignment & XBFONT_CENTER_Y) ? -0.5f*m_cellHeight : 0;  // vertical centering

    if ( alignment & (XBFONT_RIGHT | XBFONT_CENTER_X) )
    {
      // Get the extent of this line
      float w = GetTextWidthInternal( text.begin(), text.end() );

      if ( alignment & XBFONT_TRUNCATED && w > maxPixelWidth + 0.5f ) // + 0.5f due to rounding issues
        w = maxPixelWidth;

      if ( alignment & XBFONT_CENTER_X)
        w *= 0.5f;
      // Offset this line's starting position
      startX -= w;
    }

    float spacePerSpaceCharacter = 0; // for justification effects
    if ( alignment & XBFONT_JUSTIFIED )
    {
      // first compute the size of the text to render in both characters and pixels
      unsigned int numSpaces = 0;
      float linePixels = 0;
      for (vecText::const_iterator pos = text.begin(); pos != text.end(); ++pos)
      {
        Character *ch = GetCharacter(*pos);
        if (ch)
        {
          if ((*pos & 0xffff) == L' ')
            numSpaces +=  1;
          linePixels += ch->advance;
        }
      }
      if (numSpaces > 0)
        spacePerSpaceCharacter = (maxPixelWidth - linePixels) / numSpaces;
    }

    float cursorX = 0; // current position along the line

    // Collect all the Character info in a first pass, in case any of them
    // are not currently cached and cause the texture to be enlarged, which
    // would invalidate the texture coordinates.
    std::queue<Character> characters;
    if (alignment & XBFONT_TRUNCATED)
      GetCharacter(L'.');
    for (vecText::const_iterator pos = text.begin(); pos != text.end(); ++pos)
    {
      Character *ch = GetCharacter(*pos);
      if (!ch)
      {
        Character null = { 0 };
        characters.push(null);
        continue;
      }
      characters.push(*ch);

      if (maxPixelWidth > 0 &&
          cursorX + (alignment & XBFONT_TRUNCATED ? ch->advance + 3 * m_ellipsesWidth : 0) > maxPixelWidth)
        break;
      cursorX += ch->advance;
    }
    cursorX = 0;

    for (vecText::const_iterator pos = text.begin(); pos != text.end(); ++pos)
    {
      // If starting text on a new line, determine justification effects
      // Get the current letter in the CStdString
      color_t color = (*pos & 0xff0000) >> 16;
      if (color >= colors.size())
        color = 0;
      color = colors[color];

      // grab the next character
      Character *ch = &characters.front();
      if (ch->letterAndStyle == 0)
      {
        characters.pop();
        continue;
      }

      if ( alignment & XBFONT_TRUNCATED )
      {
        // Check if we will be exceeded the max allowed width
        if ( cursorX + ch->advance + 3 * m_ellipsesWidth > maxPixelWidth )
        {
          // Yup. Let's draw the ellipses, then bail
          // Perhaps we should really bail to the next line in this case??
          Character *period = GetCharacter(L'.');
          if (!period)
            break;

          for (int i = 0; i < 3; i++)
          {
            RenderCharacter(startX + cursorX, startY, period, color, !scrolling, *tempVertices);
            cursorX += period->advance;
          }
          break;
        }
      }
      else if (maxPixelWidth > 0 && cursorX > maxPixelWidth)
        break;  // exceeded max allowed width - stop rendering

      RenderCharacter(startX + cursorX, startY, ch, color, !scrolling, *tempVertices);
      if ( alignment & XBFONT_JUSTIFIED )
      {
        if ((*pos & 0xffff) == L' ')
          cursorX += ch->advance + spacePerSpaceCharacter;
        else
          cursorX += ch->advance;
      }
      else
        cursorX += ch->advance;
      characters.pop();
    }
    if (hardwareClipping)
    {
      CVertexBuffer &vertexBuffer = m_dynamicCache.Lookup(dynamicPos,
                                                          colors, text,
                                                          rawAlignment, maxPixelWidth,
                                                          scrolling,
                                                          XbmcThreads::SystemClockMillis(),
                                                          dirtyCache);
      CVertexBuffer newVertexBuffer = CreateVertexBuffer(*tempVertices);
      vertexBuffer = newVertexBuffer;
      m_vertexTrans.push_back(CTranslatedVertices(0, 0, 0, &vertexBuffer, g_graphicsContext.GetClipRegion()));
    }
    else
    {
      m_staticCache.Lookup(staticPos,
                           colors, text,
                           rawAlignment, maxPixelWidth,
                           scrolling,
                           XbmcThreads::SystemClockMillis(),
                           dirtyCache) = *static_cast<CGUIFontCacheStaticValue *>(&tempVertices);
      /* Append the new vertices to the set collected since the first Begin() call */
      m_vertex.insert(m_vertex.end(), tempVertices->begin(), tempVertices->end());
    }
  }
  else
  {
    if (hardwareClipping)
      m_vertexTrans.push_back(CTranslatedVertices(dynamicPos.m_x, dynamicPos.m_y, dynamicPos.m_z, &vertexBuffer, g_graphicsContext.GetClipRegion()));
    else
      /* Append the vertices from the cache to the set collected since the first Begin() call */
      m_vertex.insert(m_vertex.end(), vertices->begin(), vertices->end());
  }

  End();
}

// this routine assumes a single line (i.e. it was called from GUITextLayout)
float CGUIFontTTFBase::GetTextWidthInternal(vecText::const_iterator start, vecText::const_iterator end)
{
  float width = 0;
  while (start != end)
  {
    Character *c = GetCharacter(*start++);
    if (c)
    {
      // If last character in line, we want to add render width
      // and not advance distance - this makes sure that italic text isn't
      // choped on the end (as render width is larger than advance then).
      if (start == end)
        width += std::max(c->right - c->left + c->offsetX, c->advance);
      else
        width += c->advance;
    }
  }
  return width;
}

float CGUIFontTTFBase::GetCharWidthInternal(character_t ch)
{
  Character *c = GetCharacter(ch);
  if (c) return c->advance;
  return 0;
}

float CGUIFontTTFBase::GetTextHeight(float lineSpacing, int numLines) const
{
  return (float)(numLines - 1) * GetLineHeight(lineSpacing) + m_cellHeight;
}

float CGUIFontTTFBase::GetLineHeight(float lineSpacing) const
{
  if (m_face)
    return lineSpacing * m_face->size->metrics.height / 64.0f;
  return 0.0f;
}

const unsigned int CGUIFontTTFBase::spacing_between_characters_in_texture = 1;

unsigned int CGUIFontTTFBase::GetTextureLineHeight() const
{
  return m_cellHeight + spacing_between_characters_in_texture;
}

CGUIFontTTFBase::Character* CGUIFontTTFBase::GetCharacter(character_t chr)
{
  wchar_t letter = (wchar_t)(chr & 0xffff);
  character_t style = (chr & 0x7000000) >> 24;

  // ignore linebreaks
  if (letter == L'\r')
    return NULL;

  // quick access to ascii chars
  if (letter < 255)
  {
    character_t ch = (style << 8) | letter;
    if (m_charquick[ch])
      return m_charquick[ch];
  }

  // letters are stored based on style and letter
  character_t ch = (style << 16) | letter;

  int low = 0;
  int high = m_numChars - 1;
  while (low <= high)
  {
    int mid = (low + high) >> 1;
    if (ch > m_char[mid].letterAndStyle)
      low = mid + 1;
    else if (ch < m_char[mid].letterAndStyle)
      high = mid - 1;
    else
      return &m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  // increase the size of the buffer if we need it
  if (m_numChars >= m_maxChars)
  { // need to increase the size of the buffer
    Character *newTable = new Character[m_maxChars + CHAR_CHUNK];
    if (m_char)
    {
      memcpy(newTable, m_char, low * sizeof(Character));
      memcpy(newTable + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
      delete[] m_char;
    }
    m_char = newTable;
    m_maxChars += CHAR_CHUNK;

  }
  else
  { // just move the data along as necessary
    memmove(m_char + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
  }
  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  unsigned int nestedBeginCount = m_nestedBeginCount;
  m_nestedBeginCount = 1;
  if (nestedBeginCount) End();
  if (!CacheCharacter(letter, style, m_char + low))
  { // unable to cache character - try clearing them all out and starting over
    CLog::Log(LOGDEBUG, "%s: Unable to cache character.  Clearing character cache of %i characters", __FUNCTION__, m_numChars);
    ClearCharacterCache();
    low = 0;
    if (!CacheCharacter(letter, style, m_char + low))
    {
      CLog::Log(LOGERROR, "%s: Unable to cache character (out of memory?)", __FUNCTION__);
      if (nestedBeginCount) Begin();
      m_nestedBeginCount = nestedBeginCount;
      return NULL;
    }
  }
  if (nestedBeginCount) Begin();
  m_nestedBeginCount = nestedBeginCount;

  // fixup quick access
  memset(m_charquick, 0, sizeof(m_charquick));
  for(int i=0;i<m_numChars;i++)
  {
    if ((m_char[i].letterAndStyle & 0xffff) < 255)
    {
      character_t ch = ((m_char[i].letterAndStyle & 0xffff0000) >> 8) | (m_char[i].letterAndStyle & 0xff);
      m_charquick[ch] = m_char+i;
    }
  }

  return m_char + low;
}

bool CGUIFontTTFBase::CacheCharacter(wchar_t letter, uint32_t style, Character *ch)
{
  int glyph_index = FT_Get_Char_Index( m_face, letter );

  FT_Glyph glyph = NULL;
  if (FT_Load_Glyph( m_face, glyph_index, FT_LOAD_TARGET_LIGHT ))
  {
    CLog::Log(LOGDEBUG, "%s Failed to load glyph %x", __FUNCTION__, letter);
    return false;
  }
  // make bold if applicable
  if (style & FONT_STYLE_BOLD)
    EmboldenGlyph(m_face->glyph);
  // and italics if applicable
  if (style & FONT_STYLE_ITALICS)
    ObliqueGlyph(m_face->glyph);
  // and light if applicable
  if (style & FONT_STYLE_LIGHT)
    LightenGlyph(m_face->glyph);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
  {
    CLog::Log(LOGDEBUG, "%s Failed to get glyph %x", __FUNCTION__, letter);
    return false;
  }
  if (m_stroker)
    FT_Glyph_StrokeBorder(&glyph, m_stroker, 0, 1);
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
  {
    CLog::Log(LOGDEBUG, "%s Failed to render glyph %x to a bitmap", __FUNCTION__, letter);
    return false;
  }
  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  bool isEmptyGlyph = (bitmap.width == 0 || bitmap.rows == 0);

  if (!isEmptyGlyph)
  {
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    // check we have enough room for the character
    if ((m_posX + bitGlyph->left + bitmap.width) > static_cast<int>(m_textureWidth))
    { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
      m_posX = 0;
      m_posY += GetTextureLineHeight();
      if (bitGlyph->left < 0)
        m_posX += -bitGlyph->left;

      if(m_posY + GetTextureLineHeight() >= m_textureHeight)
      {
        // create the new larger texture
        unsigned int newHeight = m_posY + GetTextureLineHeight();
        // check for max height
        if (newHeight > g_Windowing.GetMaxTextureSize())
        {
          CLog::Log(LOGDEBUG, "%s: New cache texture is too large (%u > %u pixels long)", __FUNCTION__, newHeight, g_Windowing.GetMaxTextureSize());
          FT_Done_Glyph(glyph);
          return false;
        }

        CBaseTexture* newTexture = NULL;
        newTexture = ReallocTexture(newHeight);
        if(newTexture == NULL)
        {
          FT_Done_Glyph(glyph);
          CLog::Log(LOGDEBUG, "%s: Failed to allocate new texture of height %u", __FUNCTION__, newHeight);
          return false;
        }
        m_texture = newTexture;
      }
    }

    if(m_texture == NULL)
    {
      FT_Done_Glyph(glyph);
      CLog::Log(LOGDEBUG, "%s: no texture to cache character to", __FUNCTION__);
      return false;
    }
  }
  // set the character in our table
  ch->letterAndStyle = (style << 16) | letter;
  ch->offsetX = (short)bitGlyph->left;
  ch->offsetY = (short)m_cellBaseLine - bitGlyph->top;
  ch->left = isEmptyGlyph ? 0 : ((float)m_posX + ch->offsetX);
  ch->top = isEmptyGlyph ? 0 : ((float)m_posY + ch->offsetY);
  ch->right = ch->left + bitmap.width;
  ch->bottom = ch->top + bitmap.rows;
  ch->advance = (float)MathUtils::round_int( (float)m_face->glyph->advance.x / 64 );

  // we need only render if we actually have some pixels
  if (!isEmptyGlyph)
  {
    // ensure our rect will stay inside the texture (it *should* but we need to be certain)
    unsigned int x1 = std::max(m_posX + ch->offsetX, 0);
    unsigned int y1 = std::max(m_posY + ch->offsetY, 0);
    unsigned int x2 = std::min(x1 + bitmap.width, m_textureWidth);
    unsigned int y2 = std::min(y1 + bitmap.rows, m_textureHeight);
    CopyCharToTexture(bitGlyph, x1, y1, x2, y2);
  
    m_posX += spacing_between_characters_in_texture + (unsigned short)std::max(ch->right - ch->left + ch->offsetX, ch->advance);
  }
  m_numChars++;

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTFBase::RenderCharacter(float posX, float posY, const Character *ch, color_t color, bool roundX, std::vector<SVertex> &vertices)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->right - ch->left;
  const float height = ch->bottom - ch->top;
  
  // return early if nothing to render
  if (width == 0 || height == 0)
    return;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
  CRect vertex((posX + ch->offsetX) * g_graphicsContext.GetGUIScaleX(),
               (posY + ch->offsetY) * g_graphicsContext.GetGUIScaleY(),
               (posX + ch->offsetX + width) * g_graphicsContext.GetGUIScaleX(),
               (posY + ch->offsetY + height) * g_graphicsContext.GetGUIScaleY());
  vertex += CPoint(m_originX, m_originY);
  CRect texture(ch->left, ch->top, ch->right, ch->bottom);
  if (!g_Windowing.ScissorsCanEffectClipping())
    g_graphicsContext.ClipRect(vertex, texture);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x[4], y[4], z[4];

  x[0] = g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1);
  x[1] = g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1);
  x[2] = g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2);
  x[3] = g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2);

  if (roundX)
  {
    // We only round the "left" side of the character, and then use the direction of rounding to
    // move the "right" side of the character.  This ensures that a constant width is kept when rendering
    // the same letter at the same size at different places of the screen, avoiding the problem
    // of the "left" side rounding one way while the "right" side rounds the other way, thus getting
    // altering the width of thin characters substantially.  This only really works for positive
    // coordinates (due to the direction of truncation for negatives) but this is the only case that
    // really interests us anyway.
    float rx0 = (float)MathUtils::round_int(x[0]);
    float rx3 = (float)MathUtils::round_int(x[3]);
    x[1] = (float)MathUtils::truncate_int(x[1]);
    x[2] = (float)MathUtils::truncate_int(x[2]);
    if (x[0] > 0.0f && rx0 > x[0])
      x[1] += 1;
    else if (x[0] < 0.0f && rx0 < x[0])
      x[1] -= 1;
    if (x[3] > 0.0f && rx3 > x[3])
      x[2] += 1;
    else if (x[3] < 0.0f && rx3 < x[3])
      x[2] -= 1;
    x[0] = rx0;
    x[3] = rx3;
  }

  y[0] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1));
  y[1] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1));
  y[2] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2));
  y[3] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2));

  z[0] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y1));
  z[1] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y1));
  z[2] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y2));
  z[3] = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y2));

  // tex coords converted to 0..1 range
  float tl = texture.x1 * m_textureScaleX;
  float tr = texture.x2 * m_textureScaleX;
  float tt = texture.y1 * m_textureScaleY;
  float tb = texture.y2 * m_textureScaleY;

  vertices.resize(vertices.size() + 4);
  SVertex* v = &vertices[vertices.size() - 4];
  m_color = color;

#ifndef HAS_DX
  unsigned char r = GET_R(color)
              , g = GET_G(color)
              , b = GET_B(color)
              , a = GET_A(color);

  if(g_Windowing.UseLimitedColor())
  {
    r = (235 - 16) * r / 255;
    g = (235 - 16) * g / 255;
    b = (235 - 16) * b / 255;
  }
#endif

  for(int i = 0; i < 4; i++)
  {
#ifdef HAS_DX
    CD3DHelper::XMStoreColor(&v[i].col, color);
#else
    v[i].r = r;
    v[i].g = g;
    v[i].b = b;
    v[i].a = a;
#endif
  }

#if defined(HAS_GL) || defined(HAS_DX)
  for(int i = 0; i < 4; i++)
  {
    v[i].x = x[i];
    v[i].y = y[i];
    v[i].z = z[i];
  }

  v[0].u = tl;
  v[0].v = tt;

  v[1].u = tr;
  v[1].v = tt;

  v[2].u = tr;
  v[2].v = tb;

  v[3].u = tl;
  v[3].v = tb;
#else
  // GLES uses triangle strips, not quads, so have to rearrange the vertex order
  v[0].u = tl;
  v[0].v = tt;
  v[0].x = x[0];
  v[0].y = y[0];
  v[0].z = z[0];

  v[1].u = tl;
  v[1].v = tb;
  v[1].x = x[3];
  v[1].y = y[3];
  v[1].z = z[3];

  v[2].u = tr;
  v[2].v = tt;
  v[2].x = x[1];
  v[2].y = y[1];
  v[2].z = z[1];

  v[3].u = tr;
  v[3].v = tb;
  v[3].x = x[2];
  v[3].y = y[2];
  v[3].z = z[2];
#endif
}

// Oblique code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTFBase::ObliqueGlyph(FT_GlyphSlot slot)
{
  /* only oblique outline glyphs */
  if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    return;

  /* we don't touch the advance width */

  /* For italic, simply apply a shear transform, with an angle */
  /* of about 12 degrees.                                      */

  FT_Matrix    transform;
  transform.xx = 0x10000L;
  transform.yx = 0x00000L;

  transform.xy = 0x06000L;
  transform.yy = 0x10000L;

  FT_Outline_Transform( &slot->outline, &transform );
}


// Embolden code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTFBase::EmboldenGlyph(FT_GlyphSlot slot)
{
  if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix( m_face->units_per_EM,
                    m_face->size->metrics.y_scale ) / 24;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox( &slot->outline, &bbox_before );
  FT_Outline_Embolden( &slot->outline, strength );  // ignore error
  FT_Outline_Get_CBox( &slot->outline, &bbox_after );

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if ( slot->advance.x )
    slot->advance.x += dx;

  if ( slot->advance.y )
    slot->advance.y += dy;

  slot->metrics.width        += dx;
  slot->metrics.height       += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance  += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance  += dy;
}
 
// Lighten code - original taken from freetype2 (ftsynth.c)  
void CGUIFontTTFBase::LightenGlyph(FT_GlyphSlot slot)
{
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix(m_face->units_per_EM,
                              m_face->size->metrics.y_scale) / -48;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox(&slot->outline, &bbox_before);
  FT_Outline_Embolden(&slot->outline, strength);  // ignore error  
  FT_Outline_Get_CBox(&slot->outline, &bbox_after);

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if (slot->advance.x)
    slot->advance.x += dx;

  if (slot->advance.y)
    slot->advance.y += dy;

  slot->metrics.width += dx;
  slot->metrics.height += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance += dy;
}




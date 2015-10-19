/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "AEChannelInfo.h"
#include <algorithm>
#include <limits>
#include <string.h>
#include <assert.h>

CAEChannelInfo::CAEChannelInfo()
{
  Reset();
}

CAEChannelInfo::CAEChannelInfo(const enum AEChannel* rhs)
{
  *this = rhs;
}

CAEChannelInfo::CAEChannelInfo(const AEStdChLayout rhs)
{
  *this = rhs;
}

CAEChannelInfo::~CAEChannelInfo()
{
}

void CAEChannelInfo::ResolveChannels(const CAEChannelInfo& rhs)
{
  /* mono gets upmixed to dual mono */
  if (m_channelCount == 1 && m_channels[0] == AE_CH_FC)
  {
    Reset();
    *this += AE_CH_FL;
    *this += AE_CH_FR;
    return;
  }

  bool srcHasSL = false;
  bool srcHasSR = false;
  bool srcHasRL = false;
  bool srcHasRR = false;
  bool srcHasBC = false;

  bool dstHasSL = false;
  bool dstHasSR = false;
  bool dstHasRL = false;
  bool dstHasRR = false;
  bool dstHasBC = false;

  for (unsigned int c = 0; c < rhs.m_channelCount; ++c)
    switch(rhs.m_channels[c])
    {
      case AE_CH_SL: dstHasSL = true; break;
      case AE_CH_SR: dstHasSR = true; break;
      case AE_CH_BL: dstHasRL = true; break;
      case AE_CH_BR: dstHasRR = true; break;
      case AE_CH_BC: dstHasBC = true; break;
      default:
        break;
    }

  CAEChannelInfo newInfo;
  for (unsigned int i = 0; i < m_channelCount; ++i)
  {
    switch (m_channels[i])
    {
      case AE_CH_SL: srcHasSL = true; break;
      case AE_CH_SR: srcHasSR = true; break;
      case AE_CH_BL: srcHasRL = true; break;
      case AE_CH_BR: srcHasRR = true; break;
      case AE_CH_BC: srcHasBC = true; break;
      default:
        break;
    }

    bool found = false;
    for (unsigned int c = 0; c < rhs.m_channelCount; ++c)
      if (m_channels[i] == rhs.m_channels[c])
      {
        found = true;
        break;
      }

    if (found)
      newInfo += m_channels[i];
  }

  /* we need to ensure we end up with rear or side channels for downmix to work */
  if (srcHasSL && !dstHasSL && dstHasRL && !newInfo.HasChannel(AE_CH_BL))
    newInfo += AE_CH_BL;
  if (srcHasSR && !dstHasSR && dstHasRR && !newInfo.HasChannel(AE_CH_BR))
    newInfo += AE_CH_BR;
  if (srcHasRL && !dstHasRL && dstHasSL && !newInfo.HasChannel(AE_CH_SL))
    newInfo += AE_CH_SL;
  if (srcHasRR && !dstHasRR && dstHasSR && !newInfo.HasChannel(AE_CH_SR))
    newInfo += AE_CH_SR;

  // mix back center if not available in destination layout
  // prefer mixing into backs if available
  if (srcHasBC && !dstHasBC)
  {
    if (dstHasRL && !newInfo.HasChannel(AE_CH_BL))
      newInfo += AE_CH_BL;
    else if (dstHasSL && !newInfo.HasChannel(AE_CH_SL))
      newInfo += AE_CH_SL;

    if (dstHasRR && !newInfo.HasChannel(AE_CH_BR))
      newInfo += AE_CH_BR;
    else if (dstHasSR && !newInfo.HasChannel(AE_CH_SR))
      newInfo += AE_CH_SR;
  }

  *this = newInfo;
}

void CAEChannelInfo::Reset()
{
  m_channelCount = 0;
  for (unsigned int i = 0; i < AE_CH_MAX; ++i)
    m_channels[i] = AE_CH_NULL;
}

CAEChannelInfo& CAEChannelInfo::operator=(const CAEChannelInfo& rhs)
{
  if (this == &rhs)
    return *this;

  /* clone the information */
  m_channelCount = rhs.m_channelCount;
  memcpy(m_channels, rhs.m_channels, sizeof(enum AEChannel) * rhs.m_channelCount);

  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator=(const enum AEChannel* rhs)
{
  Reset();
  if (rhs == NULL)
    return *this;

  while (m_channelCount < AE_CH_MAX && rhs[m_channelCount] != AE_CH_NULL)
  {
    m_channels[m_channelCount] = rhs[m_channelCount];
    ++m_channelCount;
  }

  /* the last entry should be NULL, if not we were passed a non null terminated list */
  assert(rhs[m_channelCount] == AE_CH_NULL);

  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator=(const enum AEStdChLayout rhs)
{
  assert(rhs > AE_CH_LAYOUT_INVALID && rhs < AE_CH_LAYOUT_MAX);

  static enum AEChannel layouts[AE_CH_LAYOUT_MAX][9] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL , AE_CH_BR , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_LFE,  AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_SL , AE_CH_SR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_LFE, AE_CH_BL , AE_CH_BR , AE_CH_SL , AE_CH_SR, AE_CH_NULL}
  };

  *this = layouts[rhs];
  return *this;
}

bool CAEChannelInfo::operator==(const CAEChannelInfo& rhs) const
{
  /* if the channel count doesnt match, no need to check further */
  if (m_channelCount != rhs.m_channelCount)
    return false;

  /* make sure the channel order is the same */
  for (unsigned int i = 0; i < m_channelCount; ++i)
    if (m_channels[i] != rhs.m_channels[i])
      return false;

  return true;
}

bool CAEChannelInfo::operator!=(const CAEChannelInfo& rhs)
{
  return !(*this == rhs);
}

CAEChannelInfo& CAEChannelInfo::operator+=(const enum AEChannel& rhs)
{
  assert(m_channelCount < AE_CH_MAX);
  assert(rhs > AE_CH_NULL && rhs < AE_CH_MAX);

  m_channels[m_channelCount++] = rhs;
  return *this;
}

CAEChannelInfo& CAEChannelInfo::operator-=(const enum AEChannel& rhs)
{
  assert(rhs > AE_CH_NULL && rhs < AE_CH_MAX);

  unsigned int i = 0;
  while(i < m_channelCount && m_channels[i] != rhs)
    i++;
  if (i >= m_channelCount)
    return *this; // Channel not found

  for(; i < m_channelCount-1; i++)
    m_channels[i] = m_channels[i+1];

  m_channels[i] = AE_CH_NULL;
  m_channelCount--;
  return *this;
}

const enum AEChannel CAEChannelInfo::operator[](unsigned int i) const
{
  assert(i < m_channelCount);
  return m_channels[i];
}

CAEChannelInfo::operator std::string() const
{
  if (m_channelCount == 0)
    return "NULL";

  std::string s;
  for (unsigned int i = 0; i < m_channelCount - 1; ++i)
  {
    s.append(GetChName(m_channels[i]));
    s.append(",");
  }
  s.append(GetChName(m_channels[m_channelCount-1]));

  return s;
}

const char* CAEChannelInfo::GetChName(const enum AEChannel ch)
{
  assert(ch >= 0 && ch < AE_CH_MAX);

  static const char* channels[AE_CH_MAX] =
  {
    "RAW" ,
    "FL"  , "FR" , "FC" , "LFE", "BL"  , "BR"  , "FLOC",
    "FROC", "BC" , "SL" , "SR" , "TFL" , "TFR" , "TFC" ,
    "TC"  , "TBL", "TBR", "TBC", "BLOC", "BROC",

    /* p16v devices */
    "UNKNOWN1" , "UNKNOWN2" , "UNKNOWN3" , "UNKNOWN4" , 
    "UNKNOWN5" , "UNKNOWN6" , "UNKNOWN7" , "UNKNOWN8" ,
    "UNKNOWN9" , "UNKNOWN10", "UNKNOWN11", "UNKNOWN12",
    "UNKNOWN13", "UNKNOWN14", "UNKNOWN15", "UNKNOWN16",
    "UNKNOWN17", "UNKNOWN18", "UNKNOWN19", "UNKNOWN20",
    "UNKNOWN21", "UNKNOWN22", "UNKNOWN23", "UNKNOWN24",
    "UNKNOWN25", "UNKNOWN26", "UNKNOWN27", "UNKNOWN28",
    "UNKNOWN29", "UNKNOWN30", "UNKNOWN31", "UNKNOWN32",
    "UNKNOWN33", "UNKNOWN34", "UNKNOWN35", "UNKNOWN36", 
    "UNKNOWN37", "UNKNOWN38", "UNKNOWN39", "UNKNOWN40",
    "UNKNOWN41", "UNKNOWN42", "UNKNOWN43", "UNKNOWN44",
    "UNKNOWN45", "UNKNOWN46", "UNKNOWN47", "UNKNOWN48",
    "UNKNOWN49", "UNKNOWN50", "UNKNOWN51", "UNKNOWN52",
    "UNKNOWN53", "UNKNOWN54", "UNKNOWN55", "UNKNOWN56",
    "UNKNOWN57", "UNKNOWN58", "UNKNOWN59", "UNKNOWN60",
    "UNKNOWN61", "UNKNOWN62", "UNKNOWN63", "UNKNOWN64"
  };

  return channels[ch];
}

bool CAEChannelInfo::HasChannel(const enum AEChannel ch) const
{
  for (unsigned int i = 0; i < m_channelCount; ++i)
    if (m_channels[i] == ch)
      return true;
  return false;
}

bool CAEChannelInfo::ContainsChannels(const CAEChannelInfo& rhs) const
{
  for (unsigned int i = 0; i < rhs.m_channelCount; ++i)
  {
    if (!HasChannel(rhs.m_channels[i]))
      return false;
  }
  return true;
}

void CAEChannelInfo::ReplaceChannel(const enum AEChannel from, const enum AEChannel to)
{
  for (unsigned int i = 0; i < m_channelCount; ++i)
  {
    if (m_channels[i] == from)
    {
      m_channels[i] = to;
      break;
    }
  }
}

int CAEChannelInfo::BestMatch(const std::vector<CAEChannelInfo>& dsts, int* score) const
{
  CAEChannelInfo availableDstChannels;
  for (unsigned int i = 0; i < dsts.size(); i++)
    availableDstChannels.AddMissingChannels(dsts[i]);

  /* if we have channels not existing in any destination layout but that
   * are remappable (e.g. RC => RL+RR), do those remaps */
  CAEChannelInfo src(*this);
  src.ResolveChannels(availableDstChannels);

  bool remapped = (src != *this);
  /* good enough approximation (does not account for added channels) */
  int dropped = std::max((int)src.Count() - (int)Count(), 0);

  int bestScore = std::numeric_limits<int>::min();
  int bestMatch = -1;

  for (unsigned int i = 0; i < dsts.size(); i++)
  {
    const CAEChannelInfo& dst = dsts[i];
    int okChannels = 0;

    for (unsigned int j = 0; j < src.Count(); j++)
      okChannels += dst.HasChannel(src[j]);

    int missingChannels = src.Count() - okChannels;
    int extraChannels = dst.Count() - okChannels;

    int curScore = 0 - (missingChannels + dropped) * 1000 - extraChannels * 10 - (remapped ? 1 : 0);

    if (curScore > bestScore)
    {
      bestScore = curScore;
      bestMatch = i;
      if (curScore == 0)
        break;
    }
  }

  if (score)
    *score = bestScore;

  return bestMatch;
}

void CAEChannelInfo::AddMissingChannels(const CAEChannelInfo& rhs)
{
  for (unsigned int i = 0; i < rhs.Count(); i++)
    if (!HasChannel(rhs[i]))
      *this += rhs[i];
}

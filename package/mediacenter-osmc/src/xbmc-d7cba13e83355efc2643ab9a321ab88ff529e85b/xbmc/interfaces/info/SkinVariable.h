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
#include <vector>
#include "guilib/GUIInfoTypes.h"
#include "interfaces/info/InfoBool.h"

class TiXmlElement;

namespace INFO
{
class CSkinVariableString;

class CSkinVariable
{
public:
  static const CSkinVariableString* CreateFromXML(const TiXmlElement& node, int context);
};

class CSkinVariableString
{
public:
  const std::string& GetName() const;
  int GetContext() const;
  std::string GetValue(bool preferImage = false, const CGUIListItem *item = NULL );
private:
  CSkinVariableString();

  std::string m_name;
  int m_context;

  struct ConditionLabelPair
  {
    INFO::InfoPtr m_condition;
    CGUIInfoLabel m_label;
  };

  typedef std::vector<ConditionLabelPair> VECCONDITIONLABELPAIR;
  VECCONDITIONLABELPAIR m_conditionLabelPairs;

  friend class CSkinVariable;
};

}

/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IListProvider.h"
#include "utils/XBMCTinyXML.h"
#include "StaticProvider.h"
#include "DirectoryProvider.h"
#include "MultiProvider.h"
#include "utils/log.h"
#include "guilib/WindowIDs.h"

IListProvider *IListProvider::Create(const TiXmlNode *node, int parentID)
{
  const bool isHome = (parentID == WINDOW_HOME);
  if (isHome)
    CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: Create enter parent={}", parentID);
  const TiXmlNode *root = node->FirstChild("content");
  if (root)
  {
    const TiXmlNode *next = root->NextSibling("content");
    if (next)
    {
      if (isHome)
        CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: Create multi");
      return new CMultiProvider(root, parentID);
    }

    if (isHome)
      CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: Create single");
    return CreateSingle(root, parentID);
  }
  if (isHome)
    CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: Create none");
  return NULL;
}

IListProvider *IListProvider::CreateSingle(const TiXmlNode *content, int parentID)
{
  const bool isHome = (parentID == WINDOW_HOME);
  const TiXmlElement *item = content->FirstChildElement("item");
  if (item)
  {
    if (isHome)
      CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: CreateSingle static");
    return new CStaticListProvider(content->ToElement(), parentID);
  }

  if (!content->NoChildren())
  {
    if (isHome)
      CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: CreateSingle directory");
    return new CDirectoryProvider(content->ToElement(), parentID);
  }

  if (isHome)
    CLog::Log(LOGNOTICE, "SWITCH_PROVIDER: CreateSingle none");
  return NULL;
}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/NetworkSwitch.h"

std::vector<CNetworkInterface*>& CNetworkSwitch::GetInterfaceList(void)
{
  return m_interfaces;
}

bool CNetworkSwitch::PingHost(unsigned long, unsigned int)
{
  return false;
}

std::vector<std::string> CNetworkSwitch::GetNameServers(void)
{
  return {};
}

void CNetworkSwitch::SetNameServers(const std::vector<std::string>&)
{
}

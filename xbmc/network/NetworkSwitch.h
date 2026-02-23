/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Network.h"

class CNetworkSwitch : public CNetworkBase
{
public:
  CNetworkSwitch() = default;
  ~CNetworkSwitch() override = default;

  std::vector<CNetworkInterface*>& GetInterfaceList(void) override;
  bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;
  std::vector<std::string> GetNameServers(void) override;
  void SetNameServers(const std::vector<std::string>& nameServers) override;

private:
  std::vector<CNetworkInterface*> m_interfaces;
};

using CNetwork = CNetworkSwitch;

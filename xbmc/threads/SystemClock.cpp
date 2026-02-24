/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdint.h>

#include "SystemClock.h"
#include "utils/TimeUtils.h"

#if defined(TARGET_SWITCH) || defined(__SWITCH__)
#include <switch.h>
#endif

namespace XbmcThreads
{
  unsigned int SystemClockMillis()
  {
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
    // CLOCK_MONOTONIC_RAW is unreliable on libnx (tv_sec may not increment).
    // Use the ARM system tick counter directly — it runs at a fixed frequency
    // (~19.2 MHz) and is the canonical monotonic clock for Switch homebrew.
    static uint64_t s_startTick = 0;
    static bool s_initialized = false;
    const uint64_t ticks = svcGetSystemTick();
    if (!s_initialized)
    {
      s_startTick = ticks;
      s_initialized = true;
    }
    const uint64_t freq = armGetSystemTickFreq(); // typically 19200000
    return static_cast<unsigned int>(((ticks - s_startTick) * 1000ULL) / freq);
#else
    uint64_t now_time;
    static uint64_t start_time = 0;
    static bool start_time_set = false;

    now_time = static_cast<uint64_t>(1000 * CurrentHostCounter() / CurrentHostFrequency());

    if (!start_time_set)
    {
      start_time = now_time;
      start_time_set = true;
    }
    return (unsigned int)(now_time - start_time);
#endif
  }
  const unsigned int EndTime::InfiniteValue = std::numeric_limits<unsigned int>::max();
}

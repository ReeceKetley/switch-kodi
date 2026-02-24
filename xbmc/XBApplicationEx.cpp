/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "messaging/ApplicationMessenger.h"
#include "PlayListPlayer.h"
#include "XBApplicationEx.h"
#include "utils/log.h"
#include "threads/SystemClock.h"
#include "commons/Exception.h"
#ifdef TARGET_POSIX
#include "platform/linux/XTimeUtils.h"
#endif
#include "AppParamParser.h"

CXBApplicationEx::CXBApplicationEx()
{
  // Variables to perform app timing
  m_bStop = false;
  m_AppFocused = true;
  m_ExitCode = EXITCODE_QUIT;
  m_renderGUI = false;
}

CXBApplicationEx::~CXBApplicationEx() = default;

/* Destroy the app */
void CXBApplicationEx::Destroy()
{
  CLog::Log(LOGNOTICE, "XBApplicationEx: destroying...");
  // Perform app-specific cleanup
  Cleanup();
}

/* Function that runs the application */
int CXBApplicationEx::Run(const CAppParamParser &params)
{
  CLog::Log(LOGNOTICE, "Running the application... [LOOPDBGv2]" );

  unsigned int lastFrameTime = 0;
  unsigned int frameTime = 0;
  const unsigned int noRenderFrameTime = 15;  // Simulates ~66fps
  uint64_t runLoopCount = 0;

  if (params.GetPlaylist().Size() > 0)
  {
    CServiceBroker::GetPlaylistPlayer().Add(0, params.GetPlaylist());
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(0);
    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_PLAYLISTPLAYER_PLAY, -1);
  }

  // Per-phase frame timing (Switch perf probe).
  // Logs a breakdown every ~1 second so we can identify which phase is slow
  // without spamming the log every frame.
  unsigned int s_phaseProcess   = 0;
  unsigned int s_phaseFrameMove = 0;
  unsigned int s_phaseRender    = 0;
  unsigned int s_frameCount     = 0;
  unsigned int s_reportTimer    = XbmcThreads::SystemClockMillis();

  // Run xbmc
  CLog::Log(LOGNOTICE, "LOOPDBG: entering main while");
  while (!m_bStop)
  {
    ++runLoopCount;
    const bool traceTick = (runLoopCount <= 20);
    if (traceTick)
      CLog::Log(LOGNOTICE, "LOOPDBG: tick={} begin", static_cast<unsigned long long>(runLoopCount));
    if ((runLoopCount % 300) == 0)
    {
      CLog::Log(LOGNOTICE, "LOOPDBG: tick={} renderGUI={} focused={} stop={}",
                static_cast<unsigned long long>(runLoopCount),
                m_renderGUI ? 1 : 0,
                m_AppFocused ? 1 : 0,
                m_bStop ? 1 : 0);
    }

    //-----------------------------------------
    // Animate and render a frame
    //-----------------------------------------

    lastFrameTime = XbmcThreads::SystemClockMillis();
    unsigned int t0, t1, t2, t3;

    t0 = XbmcThreads::SystemClockMillis();
    if (traceTick)
      CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Process begin", static_cast<unsigned long long>(runLoopCount));
    Process();
    t1 = XbmcThreads::SystemClockMillis();
    if (traceTick)
      CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Process done ({}ms)", static_cast<unsigned long long>(runLoopCount), t1 - t0);

    if (!m_bStop)
    {
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} FrameMove begin", static_cast<unsigned long long>(runLoopCount));
      FrameMove(true, m_renderGUI);
      t2 = XbmcThreads::SystemClockMillis();
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} FrameMove done ({}ms)", static_cast<unsigned long long>(runLoopCount), t2 - t1);
    }
    else
    {
      t2 = t1;
    }

    if (m_renderGUI && !m_bStop)
    {
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Render begin", static_cast<unsigned long long>(runLoopCount));
      Render();
      t3 = XbmcThreads::SystemClockMillis();
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Render done ({}ms)", static_cast<unsigned long long>(runLoopCount), t3 - t2);
    }
    else
    {
      t3 = t2;
      if (!m_renderGUI)
      {
        frameTime = t3 - lastFrameTime;
        if(frameTime < noRenderFrameTime)
          Sleep(noRenderFrameTime - frameTime);
      }
    }

    // Accumulate phase times and report once per second.
    s_phaseProcess   += (t1 - t0);
    s_phaseFrameMove += (t2 - t1);
    s_phaseRender    += (t3 - t2);
    ++s_frameCount;

    unsigned int now = XbmcThreads::SystemClockMillis();
    if (now - s_reportTimer >= 1000)
    {
      unsigned int totalMs = now - s_reportTimer;
      CLog::Log(LOGNOTICE,
                "SWITCH_PERF: frames={} total={}ms  Process={}ms  FrameMove={}ms  Render={}ms  fps~={:.1f}",
                s_frameCount, totalMs,
                s_phaseProcess, s_phaseFrameMove, s_phaseRender,
                s_frameCount > 0 ? (s_frameCount * 1000.0f / totalMs) : 0.0f);
      s_phaseProcess = s_phaseFrameMove = s_phaseRender = s_frameCount = 0;
      s_reportTimer = now;
    }

  }
  Destroy();

  CLog::Log(LOGNOTICE, "XBApplicationEx: application stopped!" );
  return m_ExitCode;
}

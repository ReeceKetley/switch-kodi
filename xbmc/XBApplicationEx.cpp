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
    if (traceTick)
      CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Process begin", static_cast<unsigned long long>(runLoopCount));
    Process();
    if (traceTick)
      CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Process done", static_cast<unsigned long long>(runLoopCount));

    if (!m_bStop)
    {
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} FrameMove begin", static_cast<unsigned long long>(runLoopCount));
      FrameMove(true, m_renderGUI);
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} FrameMove done", static_cast<unsigned long long>(runLoopCount));
    }

    if (m_renderGUI && !m_bStop)
    {
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Render begin", static_cast<unsigned long long>(runLoopCount));
      Render();
      if (traceTick)
        CLog::Log(LOGNOTICE, "LOOPDBG: tick={} Render done", static_cast<unsigned long long>(runLoopCount));
    }
    else if (!m_renderGUI)
    {
      frameTime = XbmcThreads::SystemClockMillis() - lastFrameTime;
      if(frameTime < noRenderFrameTime)
        Sleep(noRenderFrameTime - frameTime);
    }

  }
  Destroy();

  CLog::Log(LOGNOTICE, "XBApplicationEx: application stopped!" );
  return m_ExitCode;
}

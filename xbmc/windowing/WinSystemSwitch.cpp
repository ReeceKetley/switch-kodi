/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "windowing/WinSystem.h"

#include "rendering/gles/RenderSystemGLES.h"
#include "threads/SingleLock.h"
#include "utils/EGLUtils.h"
#include "utils/log.h"

#include <switch.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
struct SwitchGfxDebugConfig
{
  bool loaded = false;
  bool present_log = false;
  int present_every = 120;
  bool skip_swap = false;
};

bool SwitchGfxCfgContains(const char* text, const char* key)
{
  return text != nullptr && key != nullptr && std::strstr(text, key) != nullptr;
}

int SwitchGfxCfgReadInt(const char* text, const char* key, int fallback)
{
  if (!text || !key)
    return fallback;

  const char* p = std::strstr(text, key);
  if (!p)
    return fallback;
  p += std::strlen(key);
  if (*p != '=')
    return fallback;
  ++p;

  int value = 0;
  if (std::sscanf(p, "%d", &value) != 1)
    return fallback;
  return value;
}

SwitchGfxDebugConfig& GetSwitchGfxDebugConfig()
{
  static SwitchGfxDebugConfig cfg;
  if (cfg.loaded)
    return cfg;

  cfg.loaded = true;

  const char* paths[] = {
    "sdmc:/switch/kodi/trace.cfg",
    "sdmc:/switch/kodi-switch-slim-debug/trace.cfg",
    "sdmc:/switch/kodi-switch/trace.cfg",
    "sdmc:/switch/trace.cfg",
  };

  for (const char* p : paths)
  {
    FILE* f = fopen(p, "rb");
    if (!f)
      continue;

    char buf[4096];
    const size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    if (SwitchGfxCfgContains(buf, "gfx_present_log=1"))
      cfg.present_log = true;
    if (SwitchGfxCfgContains(buf, "gfx_skip_swap=1"))
      cfg.skip_swap = true;
    cfg.present_every = SwitchGfxCfgReadInt(buf, "gfx_present_every", cfg.present_every);
    if (cfg.present_every < 1)
      cfg.present_every = 1;
    break;
  }

  return cfg;
}
} // namespace

class CWinSystemSwitch final : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CRenderSystemBase* GetRenderSystem() override { return this; }
  bool MessagePump() override
  {
    const bool alive = appletMainLoop();
    if (!alive && !m_appletExitLogged)
    {
      m_appletExitLogged = true;
      CLog::Log(LOGNOTICE, "SWITCH_GFX: appletMainLoop returned false");
    }

    return alive;
  }

  bool InitWindowSystem() override
  {
    if (!CWinSystemBase::InitWindowSystem())
      return false;

    if (!m_eglContext.CreateDisplay(EGL_DEFAULT_DISPLAY))
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: CreateDisplay failed");
      return false;
    }

    if (!m_eglContext.InitializeDisplay(EGL_OPENGL_ES_API))
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: InitializeDisplay failed");
      return false;
    }

    if (!m_eglContext.ChooseConfig(EGL_OPENGL_ES2_BIT))
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: ChooseConfig failed");
      return false;
    }

    CEGLAttributesVec contextAttribs;
    contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

    if (!m_eglContext.CreateContext(contextAttribs))
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: CreateContext failed");
      return false;
    }

    CLog::Log(LOGNOTICE, "SWITCH_GFX: InitWindowSystem ok");
    return true;
  }

  bool DestroyWindowSystem() override
  {
    m_eglContext.Destroy();
    return CWinSystemBase::DestroyWindowSystem();
  }

  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override
  {
    (void)name;
    m_eglContext.DestroySurface();

    m_bFullScreen = fullScreen;
    m_nWidth = res.iWidth > 0 ? res.iWidth : 1280;
    m_nHeight = res.iHeight > 0 ? res.iHeight : 720;
    m_fRefreshRate = res.fRefreshRate;

    m_nativeWindow = nwindowGetDefault();
    if (m_nativeWindow == nullptr)
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: nwindowGetDefault returned null");
      return false;
    }

    nwindowSetDimensions(m_nativeWindow, m_nWidth, m_nHeight);

    if (!m_eglContext.CreateSurface(static_cast<EGLNativeWindowType>(m_nativeWindow)))
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: CreateSurface failed");
      return false;
    }

    if (!m_eglContext.BindContext())
    {
      CLog::Log(LOGERROR, "SWITCH_GFX: BindContext failed");
      return false;
    }

    const char* eglVendor = eglQueryString(m_eglContext.GetEGLDisplay(), EGL_VENDOR);
    const char* eglVersion = eglQueryString(m_eglContext.GetEGLDisplay(), EGL_VERSION);
    CLog::Log(LOGNOTICE, "SWITCH_GFX: EGL vendor={} version={}",
              eglVendor ? eglVendor : "(null)",
              eglVersion ? eglVersion : "(null)");

    m_bWindowCreated = true;
    CLog::Log(LOGNOTICE, "SWITCH_GFX: CreateNewWindow ok {}x{}", m_nWidth, m_nHeight);
    return true;
  }

  bool DestroyWindow() override
  {
    m_eglContext.DestroySurface();
    m_bWindowCreated = false;
    return true;
  }

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override
  {
    (void)newLeft;
    (void)newTop;

    m_nWidth = newWidth;
    m_nHeight = newHeight;
    if (m_nativeWindow != nullptr)
      nwindowSetDimensions(m_nativeWindow, m_nWidth, m_nHeight);
    CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
    return true;
  }

  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override
  {
    (void)blankOtherDisplays;
    m_bFullScreen = fullScreen;
    if (!CreateNewWindow("", fullScreen, res))
      return false;
    CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);
    return true;
  }

  void Register(IDispResource *resource) override
  {
    CSingleLock lock(m_resourceSection);
    m_resources.push_back(resource);
  }

  void Unregister(IDispResource *resource) override
  {
    CSingleLock lock(m_resourceSection);
    auto it = std::find(m_resources.begin(), m_resources.end(), resource);
    if (it != m_resources.end())
      m_resources.erase(it);
  }

protected:
  void SetVSyncImpl(bool enable) override
  {
    if (!m_eglContext.SetVSync(enable))
      CLog::Log(LOGWARNING, "SWITCH_GFX: SetVSync failed");
  }

  void PresentRenderImpl(bool rendered) override
  {
    ++m_presentCount;
    const auto& cfg = GetSwitchGfxDebugConfig();
    if (cfg.present_log && (m_presentCount % static_cast<uint64_t>(cfg.present_every) == 0 || !rendered))
    {
      CLog::Log(LOGNOTICE, "SWITCH_GFX: present tick={} rendered={} size={}x{}",
                static_cast<unsigned long long>(m_presentCount),
                rendered ? 1 : 0,
                m_nWidth,
                m_nHeight);
    }

    if (!rendered)
      return;

    if (cfg.present_log && (m_presentCount % static_cast<uint64_t>(cfg.present_every) == 0))
      CLog::Log(LOGNOTICE, "SWITCH_GFX: pre-swap tick={}", static_cast<unsigned long long>(m_presentCount));

    if (cfg.skip_swap)
    {
      if (cfg.present_log && (m_presentCount % static_cast<uint64_t>(cfg.present_every) == 0))
        CLog::Log(LOGNOTICE, "SWITCH_GFX: skip-swap tick={}", static_cast<unsigned long long>(m_presentCount));
      return;
    }

    if (!m_eglContext.TrySwapBuffers())
      CEGLUtils::LogError("SWITCH_GFX: eglSwapBuffers failed");
    else if (cfg.present_log && (m_presentCount % static_cast<uint64_t>(cfg.present_every) == 0))
      CLog::Log(LOGNOTICE, "SWITCH_GFX: post-swap tick={}", static_cast<unsigned long long>(m_presentCount));
  }

private:
  CEGLContextUtils m_eglContext;
  NWindow* m_nativeWindow = nullptr;
  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  uint64_t m_presentCount = 0;
  bool m_appletExitLogged = false;
};

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  return std::unique_ptr<CWinSystemBase>(new CWinSystemSwitch());
}

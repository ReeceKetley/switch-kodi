/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "imagefactory.h"
#include "guilib/FFmpegImage.h"
#include "addons/ImageDecoder.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <algorithm>
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
#include <cstdio>
#include <cstring>
#endif

CCriticalSection ImageFactory::m_createSec;

using namespace ADDON;

#if defined(TARGET_SWITCH) || defined(__SWITCH__)
static bool SwitchImgCfgHas(const char* key)
{
  if (!key || !*key)
    return false;

  const char* paths[] = {
    "sdmc:/switch/kodi-switch-slim-debug/trace.cfg",
    "sdmc:/switch/kodi-switch/trace.cfg",
    "sdmc:/switch/trace.cfg",
  };

  for (const char* p : paths)
  {
    FILE* f = fopen(p, "rb");
    if (!f)
      continue;
    char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    return std::strstr(buf, key) != nullptr;
  }
  return false;
}
#else
static inline bool SwitchImgCfgHas(const char* key) { return false; }
#endif

IImage* ImageFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IImage* ImageFactory::CreateLoader(const CURL& url)
{
  if(!url.GetFileType().empty())
    return CreateLoaderFromMimeType("image/"+url.GetFileType());

  return CreateLoaderFromMimeType(CMime::GetMimeType(url));
}

IImage* ImageFactory::CreateLoaderFromMimeType(const std::string& strMimeType)
{
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  const bool traceImageFactory = SwitchImgCfgHas("trace_imagefactory=1");
  const bool forceFfmpegOnly = SwitchImgCfgHas("force_ffmpeg_image_loader=1");
  if (traceImageFactory)
    CLog::Log(LOGNOTICE, "SWITCH_IMGFACT: begin mime='{}' forceFfmpegOnly={}", strMimeType,
              forceFfmpegOnly ? 1 : 0);
  if (forceFfmpegOnly)
  {
    if (traceImageFactory)
      CLog::Log(LOGNOTICE, "SWITCH_IMGFACT: force ffmpeg loader mime='{}'", strMimeType);
    return new CFFmpegImage(strMimeType);
  }
#endif
  BinaryAddonBaseList addonInfos;

#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  if (traceImageFactory)
    CLog::Log(LOGNOTICE, "SWITCH_IMGFACT: GetAddonInfos begin");
#endif
  CServiceBroker::GetBinaryAddonManager().GetAddonInfos(addonInfos, true, ADDON_IMAGEDECODER);
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  if (traceImageFactory)
    CLog::Log(LOGNOTICE, "SWITCH_IMGFACT: GetAddonInfos done count={}", static_cast<int>(addonInfos.size()));
#endif
  for (auto addonInfo : addonInfos)
  {
    std::vector<std::string> mime = StringUtils::Split(addonInfo->Type(ADDON_IMAGEDECODER)->GetValue("@mimetype").asString(), "|");
    if (std::find(mime.begin(), mime.end(), strMimeType) != mime.end())
    {
      CSingleLock lock(m_createSec);
      CImageDecoder* result = new CImageDecoder(addonInfo);
      result->Create(strMimeType);
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
      if (traceImageFactory)
        CLog::Log(LOGNOTICE, "SWITCH_IMGFACT: addon loader selected mime='{}'", strMimeType);
#endif
      return result;
    }
  }

#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  if (traceImageFactory)
    CLog::Log(LOGNOTICE, "SWITCH_IMGFACT: ffmpeg fallback mime='{}'", strMimeType);
#endif
  return new CFFmpegImage(strMimeType);
}

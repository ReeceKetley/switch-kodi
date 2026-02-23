/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  Extension by Reece Ketley
 */


#include "SwitchFile.h"

#include "URL.h"
#include "utils/AliasShortcutUtils.h"
#include "utils/StringUtils.h"
#include "filesystem/SpecialProtocol.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

using namespace XFILE;

CSwitchFile::~CSwitchFile()
{
  Close();
}

std::string CSwitchFile::MapPath(const CURL& url) const
{
  std::string path(url.GetFileName());
  if (IsAliasShortcut(path, false))
    TranslateAliasShortcut(path);

  if (StringUtils::StartsWithNoCase(path, "special://"))
    path = CSpecialProtocol::TranslatePathConvertCase(path);

  // Normalize slashes for robustness across call sites.
  std::replace(path.begin(), path.end(), '\\', '/');

  // Force absolute POSIX-style paths onto the sdmc device namespace.
  if (!path.empty() && path[0] == '/' &&
      !StringUtils::StartsWithNoCase(path, "/romfs/") &&
      !StringUtils::StartsWithNoCase(path, "sdmc:/"))
  {
    path = "sdmc:" + path;
  }

  return path;
}

bool CSwitchFile::Open(const CURL& url)
{
  if (m_fp != nullptr)
    return false;

  std::string filename = MapPath(url);
  if (filename.empty())
    return false;

  m_fp = fopen(filename.c_str(), "rb");
  if (!m_fp)
    return false;

  m_filePos = 0;
  m_allowWrite = false;
  m_eof = false;
  return true;
}

bool CSwitchFile::OpenForWrite(const CURL& url, bool bOverWrite /* = false */)
{
  if (m_fp != nullptr)
    return false;

  std::string filename = MapPath(url);
  if (filename.empty())
    return false;

  m_fp = fopen(filename.c_str(), bOverWrite ? "wb+" : "rb+");
  if (!m_fp)
    m_fp = fopen(filename.c_str(), "wb+");
  if (!m_fp)
    return false;

  m_filePos = 0;
  m_allowWrite = true;
  m_eof = false;
  return true;
}

void CSwitchFile::Close()
{
  if (m_fp)
  {
    fclose(m_fp);
    m_fp = nullptr;
  }

  m_filePos = -1;
  m_allowWrite = false;
  m_eof = false;
}

ssize_t CSwitchFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_fp)
    return -1;
  if ((lpBuf == nullptr && uiBufSize != 0))
    return -1;
  if (uiBufSize == 0)
    return 0;
  if (m_eof)
    return 0;

  const size_t toRead = std::min(uiBufSize, static_cast<size_t>(SSIZE_MAX));
  const size_t n = fread(lpBuf, 1, toRead, m_fp);

  if (n > 0)
  {
    m_filePos += static_cast<int64_t>(n);
    return static_cast<ssize_t>(n);
  }

  if (feof(m_fp))
  {
    m_eof = true;
    return 0;
  }

  if (ferror(m_fp))
  {
    clearerr(m_fp);
    return -1;
  }

  return 0;
}

ssize_t CSwitchFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_fp || !m_allowWrite)
    return -1;
  if ((lpBuf == nullptr && uiBufSize != 0))
    return -1;
  if (uiBufSize == 0)
    return 0;

  const size_t toWrite = std::min(uiBufSize, static_cast<size_t>(SSIZE_MAX));
  const size_t n = fwrite(lpBuf, 1, toWrite, m_fp);

  if (n > 0)
  {
    m_filePos += static_cast<int64_t>(n);
    m_eof = false;
    return static_cast<ssize_t>(n);
  }

  if (ferror(m_fp))
  {
    clearerr(m_fp);
    return -1;
  }

  return 0;
}

int64_t CSwitchFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET */)
{
  if (!m_fp)
    return -1;

  if (fseeko(m_fp, static_cast<off_t>(iFilePosition), iWhence) != 0)
    return -1;

  const off_t pos = ftello(m_fp);
  if (pos < 0)
    return -1;

  m_filePos = static_cast<int64_t>(pos);
  m_eof = false;
  return m_filePos;
}

int CSwitchFile::Truncate(int64_t size)
{
  if (!m_fp)
    return -1;

  const int fd = fileno(m_fp);
  if (fd < 0)
    return -1;

  return ftruncate(fd, static_cast<off_t>(size));
}

int64_t CSwitchFile::GetPosition()
{
  if (!m_fp)
    return -1;

  if (m_filePos < 0)
  {
    const off_t pos = ftello(m_fp);
    if (pos < 0)
      return -1;
    m_filePos = static_cast<int64_t>(pos);
  }
  return m_filePos;
}

int64_t CSwitchFile::GetLength()
{
  if (!m_fp)
    return -1;

  struct __stat64 st;
  if (Stat(&st) != 0)
    return -1;
  return st.st_size;
}

void CSwitchFile::Flush()
{
  if (m_fp)
    fflush(m_fp);
}

bool CSwitchFile::Delete(const CURL& url)
{
  const std::string filename = MapPath(url);
  if (filename.empty())
    return false;
  return unlink(filename.c_str()) == 0;
}

bool CSwitchFile::Rename(const CURL& url, const CURL& urlnew)
{
  const std::string from = MapPath(url);
  const std::string to = MapPath(urlnew);
  if (from.empty() || to.empty())
    return false;
  if (from == to)
    return true;
  return rename(from.c_str(), to.c_str()) == 0;
}

bool CSwitchFile::Exists(const CURL& url)
{
  const std::string filename = MapPath(url);
  if (filename.empty())
    return false;

  struct __stat64 st;
  return stat64(filename.c_str(), &st) == 0 && !S_ISDIR(st.st_mode);
}

int CSwitchFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!buffer)
    return -1;
  const std::string filename = MapPath(url);
  if (filename.empty())
    return -1;
  return stat64(filename.c_str(), buffer);
}

int CSwitchFile::Stat(struct __stat64* buffer)
{
  if (!buffer || !m_fp)
    return -1;

  const int fd = fileno(m_fp);
  if (fd < 0)
    return -1;
  return fstat64(fd, buffer);
}

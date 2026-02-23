/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

namespace XFILE
{

class CSwitchFile : public IFile
{
public:
  ~CSwitchFile() override;

  bool Open(const CURL& url) override;
  bool OpenForWrite(const CURL& url, bool bOverWrite = false) override;
  void Close() override;

  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  ssize_t Write(const void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  int Truncate(int64_t size) override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  void Flush() override;

  bool Delete(const CURL& url) override;
  bool Rename(const CURL& url, const CURL& urlnew) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  int Stat(struct __stat64* buffer) override;

private:
  std::string MapPath(const CURL& url) const;

  FILE* m_fp = nullptr;
  int64_t m_filePos = -1;
  bool m_allowWrite = false;
  bool m_eof = false;
};

} // namespace XFILE


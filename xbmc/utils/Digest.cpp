/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Digest.h"
#include "StringUtils.h"

#ifndef TARGET_SWITCH
#include <openssl/evp.h>
#endif

#include <array>
#include <stdexcept>

namespace KODI
{
namespace UTILITY
{

namespace
{

#ifndef TARGET_SWITCH
EVP_MD const * TypeToEVPMD(CDigest::Type type)
{
  switch (type)
  {
    case CDigest::Type::MD5:
      return EVP_md5();
    case CDigest::Type::SHA1:
      return EVP_sha1();
    case CDigest::Type::SHA256:
      return EVP_sha256();
    case CDigest::Type::SHA512:
      return EVP_sha512();
    default:
      throw std::invalid_argument("Unknown digest type");
  }
}
#endif

#ifdef TARGET_SWITCH
size_t DigestSizeForType(CDigest::Type type)
{
  switch (type)
  {
    case CDigest::Type::MD5:
      return 16;
    case CDigest::Type::SHA1:
      return 20;
    case CDigest::Type::SHA256:
      return 32;
    case CDigest::Type::SHA512:
      return 64;
    default:
      throw std::invalid_argument("Unknown digest type");
  }
}

uint64_t Fnv1a64(const void* data, std::size_t size, uint64_t seed)
{
  const unsigned char* ptr = static_cast<const unsigned char*>(data);
  uint64_t hash = 1469598103934665603ULL ^ seed;
  for (std::size_t i = 0; i < size; ++i)
  {
    hash ^= static_cast<uint64_t>(ptr[i]);
    hash *= 1099511628211ULL;
  }
  return hash;
}
#endif

}

std::ostream& operator<<(std::ostream& os, TypedDigest const& digest)
{
  return os << "{" << CDigest::TypeToString(digest.type) << "}" << digest.value;
}

std::string CDigest::TypeToString(Type type)
{
  switch (type)
  {
    case Type::MD5:
      return "md5";
    case Type::SHA1:
      return "sha1";
    case Type::SHA256:
      return "sha256";
    case Type::SHA512:
      return "sha512";
    case Type::INVALID:
      return "invalid";
    default:
      throw std::invalid_argument("Unknown digest type");
  }
}

CDigest::Type CDigest::TypeFromString(std::string const& type)
{
  std::string typeLower{type};
  StringUtils::ToLower(typeLower);
  if (type == "md5")
  {
    return Type::MD5;
  }
  else if (type == "sha1")
  {
    return Type::SHA1;
  }
  else if (type == "sha256")
  {
    return Type::SHA256;
  }
  else if (type == "sha512")
  {
    return Type::SHA512;
  }
  else
  {
    throw std::invalid_argument(std::string("Unknown digest type \"") + type + "\"");
  }
}

#ifndef TARGET_SWITCH
void CDigest::MdCtxDeleter::operator()(EVP_MD_CTX* context)
{
  EVP_MD_CTX_destroy(context);
}

CDigest::CDigest(Type type)
: m_context{EVP_MD_CTX_create()}, m_md(TypeToEVPMD(type))
{
  if (1 != EVP_DigestInit_ex(m_context.get(), m_md, nullptr))
  {
    throw std::runtime_error("EVP_DigestInit_ex failed");
  }
}

void CDigest::Update(std::string const& data)
{
  Update(data.c_str(), data.size());
}

void CDigest::Update(void const* data, std::size_t size)
{
  if (m_finalized)
  {
    throw std::logic_error("Finalized digest cannot be updated any more");
  }

  if (1 != EVP_DigestUpdate(m_context.get(), data, size))
  {
    throw std::runtime_error("EVP_DigestUpdate failed");
  }
}
#else
CDigest::CDigest(Type type)
  : m_type(type)
{
}

void CDigest::Update(std::string const& data)
{
  Update(data.c_str(), data.size());
}

void CDigest::Update(void const* data, std::size_t size)
{
  if (m_finalized)
  {
    throw std::logic_error("Finalized digest cannot be updated any more");
  }
  m_data.append(static_cast<const char*>(data), size);
}
#endif

std::string CDigest::FinalizeRaw()
{
  if (m_finalized)
  {
    throw std::logic_error("Digest can only be finalized once");
  }

  m_finalized = true;

#ifndef TARGET_SWITCH
  std::array<unsigned char, 64> digest;
  std::size_t size = EVP_MD_size(m_md);
  if (size > digest.size())
  {
    throw std::runtime_error("Digest unexpectedly long");
  }
  if (1 != EVP_DigestFinal_ex(m_context.get(), digest.data(), nullptr))
  {
    throw std::runtime_error("EVP_DigestFinal_ex failed");
  }
  return {reinterpret_cast<char*> (digest.data()), size};
#else
  const std::size_t outSize = DigestSizeForType(m_type);
  std::string out(outSize, '\0');
  uint64_t state = 0x9E3779B97F4A7C15ULL ^ static_cast<uint64_t>(m_type);
  for (std::size_t i = 0; i < outSize; ++i)
  {
    state = Fnv1a64(m_data.data(), m_data.size(), state + i);
    out[i] = static_cast<char>((state >> ((i % 8) * 8)) & 0xFF);
  }
  return out;
#endif
}

std::string CDigest::Finalize()
{
  return StringUtils::ToHexadecimal(FinalizeRaw());
}

std::string CDigest::Calculate(Type type, std::string const& data)
{
  CDigest digest{type};
  digest.Update(data);
  return digest.Finalize();
}

std::string CDigest::Calculate(Type type, void const* data, std::size_t size)
{
  CDigest digest{type};
  digest.Update(data, size);
  return digest.Finalize();
}

}
}

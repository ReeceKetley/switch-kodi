/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUILabel.h"
#include "utils/log.h"
#include <cmath>
#include <limits>

#if defined(TARGET_SWITCH) || defined(__SWITCH__)
#include <cstdio>
#include <cstring>
#include <cstdlib>

static bool SwitchLabelCfgContains(const char* key)
{
  if (!key || !*key)
    return false;

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

    char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    return std::strstr(buf, key) != nullptr;
  }

  return false;
}

static bool SwitchLabelCfgContainsInt(const char* key, int value)
{
  if (!key || !*key)
    return false;

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

    char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    const size_t keyLen = std::strlen(key);
    const char* cur = buf;
    while (cur && *cur)
    {
      const char* hit = std::strstr(cur, key);
      if (!hit)
        break;
      const char* val = hit + keyLen;
      if (*val == '=')
      {
        ++val;
        if (std::atoi(val) == value)
          return true;
      }
      cur = hit + 1;
    }
    return false;
  }

  return false;
}
#endif

CGUILabel::CGUILabel(float posX, float posY, float width, float height, const CLabelInfo& labelInfo, CGUILabel::OVER_FLOW overflow)
    : m_label(labelInfo)
    , m_textLayout(labelInfo.font, overflow == OVER_FLOW_WRAP, height)
    , m_scrolling(overflow == OVER_FLOW_SCROLL)
    , m_overflowType(overflow)
    , m_scrollInfo(50, 0, labelInfo.scrollSpeed, labelInfo.scrollSuffix)
    , m_renderRect()
    , m_maxRect(posX, posY, posX + width, posY + height)
    , m_invalid(true)
    , m_color(COLOR_TEXT)
{
}

CGUILabel::~CGUILabel(void) = default;

bool CGUILabel::SetScrolling(bool scrolling)
{
  bool changed = m_scrolling != scrolling;

  m_scrolling = scrolling;
  if (changed)
    m_scrollInfo.Reset();

  return changed;
}

bool CGUILabel::SetOverflow(OVER_FLOW overflow)
{
  bool changed = m_overflowType != overflow;

  m_overflowType = overflow;

  return changed;
}

bool CGUILabel::SetColor(CGUILabel::COLOR color)
{
  bool changed = m_color != color;

  m_color = color;

  return changed;
}

UTILS::Color CGUILabel::GetColor() const
{
  switch (m_color)
  {
    case COLOR_SELECTED:
      return m_label.selectedColor;
    case COLOR_DISABLED:
      return m_label.disabledColor;
    case COLOR_FOCUSED:
      return m_label.focusedColor ? m_label.focusedColor : m_label.textColor;
    case COLOR_INVALID:
      return m_label.invalidColor ? m_label.invalidColor : m_label.textColor;
    default:
      break;
  }
  return m_label.textColor;
}

bool CGUILabel::Process(unsigned int currentTime)
{
  //! @todo Add the correct processing

  bool overFlows = (m_renderRect.Width() + 0.5f < m_textLayout.GetTextWidth()); // 0.5f to deal with floating point rounding issues
  bool renderSolid = (m_color == COLOR_DISABLED);

  if (overFlows && m_scrolling && !renderSolid)
  {
    if (m_maxScrollLoops < m_scrollInfo.m_loopCount)
      SetScrolling(false);
    else
      return m_textLayout.UpdateScrollinfo(m_scrollInfo);
  }

  return false;
}

void CGUILabel::Render()
{
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  static uint64_t s_switchLabelRenderCalls = 0;
  ++s_switchLabelRenderCalls;
  const bool traceLabel = SwitchLabelCfgContains("trace_label_render=1") && (s_switchLabelRenderCalls <= 500);
  const bool skipLabel = SwitchLabelCfgContainsInt("skip_label_render", 1);
  if (traceLabel)
    CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: begin call={} maxRect=({},{})->({},{}) renderRect=({},{})->({},{})",
              static_cast<unsigned long long>(s_switchLabelRenderCalls),
              m_maxRect.x1, m_maxRect.y1, m_maxRect.x2, m_maxRect.y2,
              m_renderRect.x1, m_renderRect.y1, m_renderRect.x2, m_renderRect.y2);
  if (skipLabel)
  {
    if (traceLabel)
      CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: skipped");
    return;
  }
#endif
  if (!std::isfinite(m_renderRect.x1) || !std::isfinite(m_renderRect.y1) ||
      !std::isfinite(m_renderRect.x2) || !std::isfinite(m_renderRect.y2))
  {
    static unsigned int s_nonFiniteRectWarns = 0;
    if (s_nonFiniteRectWarns < 64)
    {
      const char* fontName = (m_label.font ? m_label.font->GetFontName().c_str() : "<null>");
      CLog::Log(LOGWARNING,
                "SWITCH_LABEL_RENDER: non-finite renderRect font={} rect=({},{})->({},{}) maxRect=({},{})->({},{})",
                fontName, m_renderRect.x1, m_renderRect.y1, m_renderRect.x2, m_renderRect.y2,
                m_maxRect.x1, m_maxRect.y1, m_maxRect.x2, m_maxRect.y2);
      ++s_nonFiniteRectWarns;
    }
    m_renderRect.x1 = std::isfinite(m_renderRect.x1) ? m_renderRect.x1 : 0.0f;
    m_renderRect.y1 = std::isfinite(m_renderRect.y1) ? m_renderRect.y1 : 0.0f;
    m_renderRect.x2 = std::isfinite(m_renderRect.x2) ? m_renderRect.x2 : m_renderRect.x1;
    m_renderRect.y2 = std::isfinite(m_renderRect.y2) ? m_renderRect.y2 : m_renderRect.y1;
  }

  UTILS::Color color = GetColor();
  bool renderSolid = (m_color == COLOR_DISABLED);
  float renderWidth = m_renderRect.Width();
  if (!std::isfinite(renderWidth) || renderWidth < 0.0f)
    renderWidth = 0.0f;
  float textWidth = m_textLayout.GetTextWidth();
  if (!std::isfinite(textWidth) || textWidth < 0.0f)
  {
    static unsigned int s_nonFiniteTextWidthWarns = 0;
    if (s_nonFiniteTextWidthWarns < 64)
    {
      const char* fontName = (m_label.font ? m_label.font->GetFontName().c_str() : "<null>");
      CLog::Log(LOGWARNING, "SWITCH_LABEL_RENDER: non-finite textWidth font={} textWidth={}", fontName, textWidth);
      ++s_nonFiniteTextWidthWarns;
    }
    textWidth = 0.0f;
  }
  bool overFlows = (renderWidth + 0.5f < textWidth); // 0.5f to deal with floating point rounding issues
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  if (traceLabel)
    CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: state overflows={} scrolling={} solid={} width={} textWidth={}",
              overFlows ? 1 : 0, m_scrolling ? 1 : 0, renderSolid ? 1 : 0, renderWidth, textWidth);
#endif
  if (overFlows && m_scrolling && !renderSolid)
  {
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
    if (traceLabel)
      CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: scrolling begin");
#endif
    m_textLayout.RenderScrolling(m_renderRect.x1, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, 0, renderWidth, m_scrollInfo);
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
    if (traceLabel)
      CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: scrolling done");
#endif
  }
  else
  {
    float posX = std::isfinite(m_renderRect.x1) ? m_renderRect.x1 : 0.0f;
    float posY = std::isfinite(m_renderRect.y1) ? m_renderRect.y1 : 0.0f;
    uint32_t align = 0;
    if (!overFlows)
    { // hack for right and centered multiline text, as GUITextLayout::Render() treats posX as the right hand
      // or center edge of the text (see GUIFontTTF::DrawTextInternal), and this has already been taken care of
      // in UpdateRenderRect(), but we wish to still pass the horizontal alignment info through (so that multiline text
      // is aligned correctly), so we must undo the UpdateRenderRect() changes for horizontal alignment.
      if (m_label.align & XBFONT_RIGHT)
        posX += renderWidth;
      else if (m_label.align & XBFONT_CENTER_X)
        posX += renderWidth * 0.5f;
      if (m_label.align & XBFONT_CENTER_Y) // need to pass a centered Y so that <angle> will rotate around the correct point.
      {
        const float renderHeight = (std::isfinite(m_renderRect.Height()) && m_renderRect.Height() > 0.0f) ? m_renderRect.Height() : 0.0f;
        posY += renderHeight * 0.5f;
      }
      align = m_label.align;
    }
    else
      align |= XBFONT_TRUNCATED;
    float clipWidth = (m_overflowType == OVER_FLOW_CLIP) ? textWidth : renderWidth;
    if (!std::isfinite(clipWidth) || clipWidth < 0.0f)
      clipWidth = 0.0f;
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
    if (traceLabel)
      CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: layout done align={} pos=({}, {}) clipw={}",
                static_cast<unsigned int>(align), posX, posY, clipWidth);
    if (SwitchLabelCfgContainsInt("skip_label_draw", 1))
    {
      if (traceLabel)
        CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: draw skipped");
      return;
    }
    if (traceLabel)
      CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: draw begin");
#endif
    m_textLayout.Render(posX, posY, m_label.angle, color, m_label.shadowColor, align, clipWidth, renderSolid);
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
    if (traceLabel)
      CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: draw done");
#endif
  }
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
  if (traceLabel)
    CLog::Log(LOGNOTICE, "SWITCH_LABEL_RENDER: end");
#endif
}

void CGUILabel::SetInvalid()
{
  m_invalid = true;
}

bool CGUILabel::UpdateColors()
{
  return m_label.UpdateColors();
}

bool CGUILabel::SetMaxRect(float x, float y, float w, float h)
{
  CRect oldRect = m_maxRect;

  m_maxRect.SetRect(x, y, x + w, y + h);
  UpdateRenderRect();

  return oldRect != m_maxRect;
}

bool CGUILabel::SetAlign(uint32_t align)
{
  bool changed = m_label.align != align;

  m_label.align = align;
  UpdateRenderRect();

  return changed;
}

bool CGUILabel::SetStyledText(const vecText &text, const std::vector<UTILS::Color> &colors)
{
  m_textLayout.UpdateStyled(text, colors, m_maxRect.Width());
  m_invalid = false;
  return true;
}

bool CGUILabel::SetText(const std::string &label)
{
  if (m_textLayout.Update(label, m_maxRect.Width(), m_invalid))
  { // needed an update - reset scrolling and update our text layout
    m_scrollInfo.Reset();
    UpdateRenderRect();
    m_invalid = false;
    return true;
  }
  else
    return false;
}

bool CGUILabel::SetTextW(const std::wstring &label)
{
  if (m_textLayout.UpdateW(label, m_maxRect.Width(), m_invalid))
  {
    m_scrollInfo.Reset();
    UpdateRenderRect();
    m_invalid = false;
    return true;
  }
  else
    return false;
}

void CGUILabel::UpdateRenderRect()
{
  // recalculate our text layout
  float width, height;
  m_textLayout.GetTextExtent(width, height);
  if (!std::isfinite(width) || width < 0.0f || !std::isfinite(height) || height < 0.0f)
  {
    static unsigned int s_badExtentWarns = 0;
    if (s_badExtentWarns < 64)
    {
      const char* fontName = (m_label.font ? m_label.font->GetFontName().c_str() : "<null>");
      CLog::Log(LOGWARNING, "SWITCH_LABEL_LAYOUT: non-finite extent font={} width={} height={}",
                fontName, width, height);
      ++s_badExtentWarns;
    }
    if (!std::isfinite(width) || width < 0.0f)
      width = 0.0f;
    if (!std::isfinite(height) || height < 0.0f)
      height = 0.0f;
  }
  width = std::min(width, GetMaxWidth());
  if (m_label.align & XBFONT_CENTER_Y)
    m_renderRect.y1 = m_maxRect.y1 + (m_maxRect.Height() - height) * 0.5f;
  else
    m_renderRect.y1 = m_maxRect.y1 + m_label.offsetY;
  if (m_label.align & XBFONT_RIGHT)
    m_renderRect.x1 = m_maxRect.x2 - width - m_label.offsetX;
  else if (m_label.align & XBFONT_CENTER_X)
    m_renderRect.x1 = m_maxRect.x1 + (m_maxRect.Width() - width) * 0.5f;
  else
    m_renderRect.x1 = m_maxRect.x1 + m_label.offsetX;
  m_renderRect.x2 = m_renderRect.x1 + width;
  m_renderRect.y2 = m_renderRect.y1 + height;
}

float CGUILabel::GetMaxWidth() const
{
  if (m_label.width)
    return (std::isfinite(m_label.width) && m_label.width > 0.0f) ? m_label.width : 0.0f;
  const float maxRectWidth = (std::isfinite(m_maxRect.Width()) && m_maxRect.Width() > 0.0f) ? m_maxRect.Width() : 0.0f;
  const float offsetX = std::isfinite(m_label.offsetX) ? m_label.offsetX : 0.0f;
  const float w = maxRectWidth - 2 * offsetX;
  return (std::isfinite(w) && w > 0.0f) ? w : 0.0f;
}

bool CGUILabel::CheckAndCorrectOverlap(CGUILabel &label1, CGUILabel &label2)
{
  CRect rect(label1.m_renderRect);
  if (rect.Intersect(label2.m_renderRect).IsEmpty())
    return false; // nothing to do (though it could potentially encroach on the min_space requirement)

  // overlap vertically and horizontally - check alignment
  CGUILabel &left = label1.m_renderRect.x1 <= label2.m_renderRect.x1 ? label1 : label2;
  CGUILabel &right = label1.m_renderRect.x1 <= label2.m_renderRect.x1 ? label2 : label1;
  if ((left.m_label.align & 3) == 0 && right.m_label.align & XBFONT_RIGHT)
  {
    static const float min_space = 10;
    float chopPoint = (left.m_maxRect.x1 + left.GetMaxWidth() + right.m_maxRect.x2 - right.GetMaxWidth()) * 0.5f;
    // [1       [2...[2  1].|..........1]         2]
    // [1       [2.....[2   |      1]..1]         2]
    // [1       [2..........|.[2   1]..1]         2]
    if (right.m_renderRect.x1 > chopPoint)
      chopPoint = right.m_renderRect.x1 - min_space;
    else if (left.m_renderRect.x2 < chopPoint)
      chopPoint = left.m_renderRect.x2 + min_space;
    left.m_renderRect.x2 = chopPoint - min_space;
    right.m_renderRect.x1 = chopPoint + min_space;
    return true;
  }
  return false;
}

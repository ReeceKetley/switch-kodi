/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlGroup.h"
#include "GUIMessage.h"
#include "utils/log.h"
#include "WindowIDs.h"

#include <cassert>
#include <utility>
#if defined(TARGET_SWITCH) || defined(__SWITCH__)
#include <cstdio>
#include <cstring>
#include <cstdlib>
#endif

#if defined(TARGET_SWITCH) || defined(__SWITCH__)
static bool SwitchGroupCfgContainsInt(const char* key, int value)
{
  if (!key || !*key)
    return false;

  static bool s_loaded = false;
  static bool s_traceEnabled = false;
  static char s_cfg[8192] = {0};
  if (!s_loaded)
  {
    s_loaded = true;
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

      size_t n = fread(s_cfg, 1, sizeof(s_cfg) - 1, f);
      fclose(f);
      s_cfg[n] = '\0';
      s_traceEnabled = std::strstr(s_cfg, "trace=1") != nullptr;
      break;
    }
  }

  if (!s_cfg[0] || !s_traceEnabled)
    return false;

  const size_t keyLen = std::strlen(key);
  const char* cur = s_cfg;
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

static bool SwitchGroupCfgContainsPrefix(const char* key, int value)
{
  if (!key || !*key || value < 0)
    return false;

  char valueBuf[32];
  std::snprintf(valueBuf, sizeof(valueBuf), "%d", value);

  static bool s_loaded = false;
  static bool s_traceEnabled = false;
  static char s_cfg[8192] = {0};
  if (!s_loaded)
  {
    s_loaded = true;
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
      size_t n = fread(s_cfg, 1, sizeof(s_cfg) - 1, f);
      fclose(f);
      s_cfg[n] = '\0';
      s_traceEnabled = std::strstr(s_cfg, "trace=1") != nullptr;
      break;
    }
  }

  if (!s_cfg[0] || !s_traceEnabled)
    return false;

  const size_t keyLen = std::strlen(key);
  const char* cur = s_cfg;
  while (cur && *cur)
  {
    const char* hit = std::strstr(cur, key);
    if (!hit)
      break;
    const char* val = hit + keyLen;
    if (*val == '=')
    {
      ++val;
      const char* end = val;
      while (*end && *end != '\n' && *end != '\r')
        ++end;
      const size_t prefixLen = static_cast<size_t>(end - val);
      if (prefixLen > 0 && std::strncmp(valueBuf, val, prefixLen) == 0)
        return true;
    }
    cur = hit + 1;
  }
  return false;
}

static bool SwitchGroupCfgContainsSuffix(const char* key, int value)
{
  if (!key || !*key || value < 0)
    return false;

  char valueBuf[32];
  std::snprintf(valueBuf, sizeof(valueBuf), "%d", value);
  const size_t valueLen = std::strlen(valueBuf);

  static bool s_loaded = false;
  static bool s_traceEnabled = false;
  static char s_cfg[8192] = {0};
  if (!s_loaded)
  {
    s_loaded = true;
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
      size_t n = fread(s_cfg, 1, sizeof(s_cfg) - 1, f);
      fclose(f);
      s_cfg[n] = '\0';
      s_traceEnabled = std::strstr(s_cfg, "trace=1") != nullptr;
      break;
    }
  }

  if (!s_cfg[0] || !s_traceEnabled)
    return false;

  const size_t keyLen = std::strlen(key);
  const char* cur = s_cfg;
  while (cur && *cur)
  {
    const char* hit = std::strstr(cur, key);
    if (!hit)
      break;
    const char* val = hit + keyLen;
    if (*val == '=')
    {
      ++val;
      const char* end = val;
      while (*end && *end != '\n' && *end != '\r')
        ++end;
      const size_t suffixLen = static_cast<size_t>(end - val);
      if (suffixLen > 0 && suffixLen <= valueLen &&
          std::strncmp(valueBuf + (valueLen - suffixLen), val, suffixLen) == 0)
        return true;
    }
    cur = hit + 1;
  }
  return false;
}
#else
static inline bool SwitchGroupCfgContainsInt(const char* key, int value) { return false; }
static inline bool SwitchGroupCfgContainsPrefix(const char* key, int value) { return false; }
static inline bool SwitchGroupCfgContainsSuffix(const char* key, int value) { return false; }
#endif

CGUIControlGroup::CGUIControlGroup()
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height)
: CGUIControlLookup(parentID, controlID, posX, posY, width, height)
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(const CGUIControlGroup &from)
: CGUIControlLookup(from)
{
  m_defaultControl = from.m_defaultControl;
  m_defaultAlways = from.m_defaultAlways;
  m_renderFocusedLast = from.m_renderFocusedLast;

  // run through and add our controls
  for (auto *i : from.m_children)
    AddControl(i->Clone());

  // defaults
  m_focusedControl = 0;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::~CGUIControlGroup(void)
{
  ClearAll();
}

void CGUIControlGroup::AllocResources()
{
  CGUIControl::AllocResources();
  const bool traceAllocAll = SwitchGroupCfgContainsInt("trace_alloc_all", 1);
  const bool traceAllocWindow = SwitchGroupCfgContainsInt("trace_alloc_window", GetParentID());
  const bool traceGroup = (GetParentID() == WINDOW_HOME) || traceAllocAll || traceAllocWindow;
  const bool skipScope = (GetParentID() == WINDOW_HOME) ||
                         SwitchGroupCfgContainsInt("skip_alloc_all", 1) ||
                         SwitchGroupCfgContainsInt("skip_alloc_window", GetParentID());
  if (traceGroup)
    CLog::Log(LOGNOTICE, "SWITCH_GRP: AllocResources begin parent={} children={}", GetParentID(), static_cast<int>(m_children.size()));
  int childIndex = 0;
  for (auto *control : m_children)
  {
    if (traceGroup)
      CLog::Log(LOGNOTICE, "SWITCH_GRP: child inspect parent={} idx={} id={} type={}", GetParentID(), childIndex, control->GetID(), control->GetControlType());
    ++childIndex;
    if (!control->IsDynamicallyAllocated())
    {
      const bool skipById = skipScope && control->GetID() > 0 &&
                            SwitchGroupCfgContainsInt("skip_alloc_id", control->GetID());
      const bool skipByType = skipScope && SwitchGroupCfgContainsInt("skip_alloc_type", control->GetControlType());
      const bool skipByPrefix = skipScope && SwitchGroupCfgContainsPrefix("skip_alloc_prefix", control->GetID());
      const bool skipBySuffix = skipScope && SwitchGroupCfgContainsSuffix("skip_alloc_suffix", control->GetID());
      if (skipById || skipByType || skipByPrefix || skipBySuffix)
      {
        CLog::Log(LOGNOTICE,
                  "SWITCH_GRP: child alloc skipped id={} type={} reason={}",
                  control->GetID(),
                  control->GetControlType(),
                  skipById ? "id" : (skipByType ? "type" : (skipByPrefix ? "prefix" : "suffix")));
        continue;
      }
      if (traceGroup)
        CLog::Log(LOGNOTICE, "SWITCH_GRP: child alloc begin id={} type={}", control->GetID(), control->GetControlType());
      control->AllocResources();
      if (traceGroup)
        CLog::Log(LOGNOTICE, "SWITCH_GRP: child alloc done id={}", control->GetID());
    }
  }
  if (traceGroup)
    CLog::Log(LOGNOTICE, "SWITCH_GRP: AllocResources done parent={}", GetParentID());
}

void CGUIControlGroup::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  for (auto *control : m_children)
  {
    control->FreeResources(immediately);
  }
}

void CGUIControlGroup::DynamicResourceAlloc(bool bOnOff)
{
  for (auto *control : m_children)
  {
    control->DynamicResourceAlloc(bOnOff);
  }
}

void CGUIControlGroup::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  const bool traceProcessWindow = SwitchGroupCfgContainsInt("trace_process_steps", 1) &&
                                  (GetParentID() == WINDOW_DIALOG_VOLUME_BAR || GetParentID() == WINDOW_HOME);
  if (traceProcessWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: begin parent={} children={}", GetParentID(),
              static_cast<int>(m_children.size()));
  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);

  CRect rect;
  int childIndex = 0;
  for (auto *control : m_children)
  {
    if (traceProcessWindow)
      CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: child begin parent={} idx={} id={} type={}",
                GetParentID(), childIndex, control->GetID(), control->GetControlType());

    const bool skipProcessScope = (GetParentID() == WINDOW_HOME) ||
                                  (GetParentID() == WINDOW_DIALOG_VOLUME_BAR) ||
                                  SwitchGroupCfgContainsInt("skip_process_window", GetParentID()) ||
                                  SwitchGroupCfgContainsInt("skip_process_all", 1);
    const bool skipProcessById = skipProcessScope && control->GetID() > 0 &&
                                 SwitchGroupCfgContainsInt("skip_process_id", control->GetID());
    const bool skipProcessByType = skipProcessScope && SwitchGroupCfgContainsInt("skip_process_type", control->GetControlType());
    if (skipProcessById || skipProcessByType)
    {
      if (traceProcessWindow)
        CLog::Log(LOGNOTICE,
                  "SWITCH_GRP_PROC: child process skipped parent={} idx={} id={} type={} reason={}",
                  GetParentID(),
                  childIndex,
                  control->GetID(),
                  control->GetControlType(),
                  skipProcessById ? "id" : "type");
      ++childIndex;
      continue;
    }

    control->UpdateVisibility(nullptr);
    if (traceProcessWindow)
      CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: child update_visibility done idx={} id={}",
                childIndex, control->GetID());
    unsigned int oldDirty = dirtyregions.size();
    control->DoProcess(currentTime, dirtyregions);
    if (traceProcessWindow)
      CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: child do_process done idx={} id={} dirty_delta={}",
                childIndex, control->GetID(),
                static_cast<int>(dirtyregions.size() - oldDirty));
    if (control->IsVisible() || (oldDirty != dirtyregions.size())) // visible or dirty (was visible?)
      rect.Union(control->GetRenderRegion());
    ++childIndex;
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
  if (traceProcessWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: restore_origin done parent={}", GetParentID());
  CGUIControl::Process(currentTime, dirtyregions);
  if (traceProcessWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: base process done parent={}", GetParentID());
  m_renderRegion = rect;
  if (traceProcessWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_PROC: done parent={}", GetParentID());
}

void CGUIControlGroup::Render()
{
  const bool traceRenderSteps = SwitchGroupCfgContainsInt("trace_render_steps", 1);
  const bool traceRenderByWindow = SwitchGroupCfgContainsInt("trace_render_window", GetParentID());
  const bool traceRenderWindow = traceRenderSteps &&
                                 (GetParentID() == WINDOW_HOME || GetParentID() == WINDOW_DIALOG_VOLUME_BAR || traceRenderByWindow);
  if (traceRenderWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: begin parent={} children={}", GetParentID(), static_cast<int>(m_children.size()));

  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);
  CGUIControl *focusedControl = NULL;
  int childIndex = 0;
  for (auto *control : m_children)
  {
    const bool skipRenderById = control->GetID() > 0 &&
                                SwitchGroupCfgContainsInt("skip_render_id", control->GetID());
    const bool skipRenderByType = SwitchGroupCfgContainsInt("skip_render_type", control->GetControlType());
    if (skipRenderById || skipRenderByType)
    {
      if (traceRenderWindow)
      {
        CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: child skipped parent={} idx={} id={} type={} reason={}",
                  GetParentID(), childIndex, control->GetID(), control->GetControlType(),
                  skipRenderById ? "id" : "type");
      }
      ++childIndex;
      continue;
    }

    if (traceRenderWindow)
      CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: child begin parent={} idx={} id={} type={}",
                GetParentID(), childIndex, control->GetID(), control->GetControlType());
    if (m_renderFocusedLast && control->HasFocus())
      focusedControl = control;
    else
    {
      control->DoRender();
      if (traceRenderWindow)
        CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: child done parent={} idx={} id={}",
                  GetParentID(), childIndex, control->GetID());
    }
    ++childIndex;
  }
  if (focusedControl)
  {
    if (traceRenderWindow)
      CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: focused child begin parent={} id={}",
                GetParentID(), focusedControl->GetID());
    focusedControl->DoRender();
    if (traceRenderWindow)
      CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: focused child done parent={} id={}",
                GetParentID(), focusedControl->GetID());
  }
  if (traceRenderWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: base Render begin parent={}", GetParentID());
  CGUIControl::Render();
  if (traceRenderWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: base Render done parent={}", GetParentID());
  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
  if (traceRenderWindow)
    CLog::Log(LOGNOTICE, "SWITCH_GRP_RENDER: done parent={}", GetParentID());
}

void CGUIControlGroup::RenderEx()
{
  for (auto *control : m_children)
    control->RenderEx();
  CGUIControl::RenderEx();
}

bool CGUIControlGroup::OnAction(const CAction &action)
{
  assert(false);  // unimplemented
  return false;
}

bool CGUIControlGroup::HasFocus() const
{
  for (auto *control : m_children)
  {
    if (control->HasFocus())
      return true;
  }
  return false;
}

bool CGUIControlGroup::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage() )
  {
  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == GetID())
      {
        m_focusedControl = message.GetParam1();
        return true;
      }
      break;
    }
  case GUI_MSG_ITEM_SELECTED:
    {
      if (message.GetControlId() == GetID())
      {
        message.SetParam1(m_focusedControl);
        return true;
      }
      break;
    }
  case GUI_MSG_FOCUSED:
    { // a control has been focused
      m_focusedControl = message.GetControlId();
      SetFocus(true);
      // tell our parent thatwe have focus
      if (m_parentControl)
        m_parentControl->OnMessage(message);
      return true;
    }
  case GUI_MSG_SETFOCUS:
    {
      // first try our last focused control...
      if (!m_defaultAlways && m_focusedControl)
      {
        CGUIControl *control = GetFirstFocusableControl(m_focusedControl);
        if (control)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
          return control->OnMessage(msg);
        }
      }
      // ok, no previously focused control, try the default control first
      if (m_defaultControl)
      {
        CGUIControl *control = GetFirstFocusableControl(m_defaultControl);
        if (control)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
          return control->OnMessage(msg);
        }
      }
      // no success with the default control, so just find one to focus
      CGUIControl *control = GetFirstFocusableControl(0);
      if (control)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
        return control->OnMessage(msg);
      }
      // unsuccessful
      return false;
      break;
    }
  case GUI_MSG_LOSTFOCUS:
    {
      // set all subcontrols unfocused
      for (auto *control : m_children)
        control->SetFocus(false);
      if (!GetControl(message.GetParam1()))
      { // we don't have the new id, so unfocus
        SetFocus(false);
        if (m_parentControl)
          m_parentControl->OnMessage(message);
      }
      return true;
    }
    break;
  case GUI_MSG_PAGE_CHANGE:
  case GUI_MSG_REFRESH_THUMBS:
  case GUI_MSG_REFRESH_LIST:
  case GUI_MSG_WINDOW_RESIZE:
    { // send to all child controls (make sure the target is the control id)
      for (auto *control : m_children)
      {
        CGUIMessage msg(message.GetMessage(), message.GetSenderId(), control->GetID(), message.GetParam1());
        control->OnMessage(msg);
      }
      return true;
    }
    break;
  case GUI_MSG_REFRESH_TIMER:
    if (!IsVisible() || !IsVisibleFromSkin())
      return true;
    break;
  }
  bool handled(false);
  //not intented for any specific control, send to all childs and our base handler.
  if (message.GetControlId() == 0)
  {
    for (auto *control : m_children)
    {
      handled |= control->OnMessage(message);
    }
    return CGUIControl::OnMessage(message) || handled;
  }
  // if it's intended for us, then so be it
  if (message.GetControlId() == GetID())
    return CGUIControl::OnMessage(message);

  return SendControlMessage(message);
}

bool CGUIControlGroup::SendControlMessage(CGUIMessage &message)
{
  IDCollector collector(m_idCollector);

  CGUIControl *ctrl(GetControl(message.GetControlId(), collector.m_collector));
  // see if a child matches, and send to the child control if so
  if (ctrl && ctrl->OnMessage(message))
    return true;

  // Unhandled - send to all matching invisible controls as well
  bool handled(false);
  for (auto *control : *collector.m_collector)
    if (control->OnMessage(message))
      handled = true;

  return handled;
}

bool CGUIControlGroup::CanFocus() const
{
  if (!CGUIControl::CanFocus()) return false;
  // see if we have any children that can be focused
  for (auto *control : m_children)
  {
    if (control->CanFocus())
      return true;
  }
  return false;
}

void CGUIControlGroup::SetInitialVisibility()
{
  CGUIControl::SetInitialVisibility();
  for (auto *control : m_children)
    control->SetInitialVisibility();
}

void CGUIControlGroup::QueueAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::QueueAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (auto *control : m_children)
      control->QueueAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::ResetAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (auto *control : m_children)
      control->ResetAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimations()
{ // resets all animations, regardless of condition
  CGUIControl::ResetAnimations();
  for (auto *control : m_children)
    control->ResetAnimations();
}

bool CGUIControlGroup::IsAnimating(ANIMATION_TYPE animType)
{
  if (CGUIControl::IsAnimating(animType))
    return true;

  if (IsVisible())
  {
    for (auto *control : m_children)
    {
      if (control->IsAnimating(animType))
        return true;
    }
  }
  return false;
}

bool CGUIControlGroup::HasAnimation(ANIMATION_TYPE animType)
{
  if (CGUIControl::HasAnimation(animType))
    return true;

  if (IsVisible())
  {
    for (auto *control : m_children)
    {
      if (control->HasAnimation(animType))
        return true;
    }
  }
  return false;
}

EVENT_RESULT CGUIControlGroup::SendMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  // transform our position into child coordinates
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);

  if (CGUIControl::CanFocus())
  {
    CPoint pos(GetPosition());
    // run through our controls in reverse order (so that last rendered is checked first)
    for (rControls i = m_children.rbegin(); i != m_children.rend(); ++i)
    {
      CGUIControl *child = *i;
      EVENT_RESULT ret = child->SendMouseEvent(childPoint - pos, event);
      if (ret)
      { // we've handled the action, and/or have focused an item
        return ret;
      }
    }
    // none of our children want the event, but we may want it.
    EVENT_RESULT ret;
    if (HitTest(childPoint) && (ret = OnMouseEvent(childPoint, event)))
      return ret;
  }
  m_focusedControl = 0;
  return EVENT_RESULT_UNHANDLED;
}

void CGUIControlGroup::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  controlCoords -= GetPosition();
  for (auto *child : m_children)
  {
    child->UnfocusFromPoint(controlCoords);
  }
  CGUIControl::UnfocusFromPoint(point);
}

int CGUIControlGroup::GetFocusedControlID() const
{
  if (m_focusedControl) return m_focusedControl;
  CGUIControl *control = GetFocusedControl();
  if (control) return control->GetID();
  return 0;
}

CGUIControl *CGUIControlGroup::GetFocusedControl() const
{
  // try lookup first
  if (m_focusedControl)
  {
    // we may have multiple controls with same id - we pick first that has focus
    std::pair<LookupMap::const_iterator, LookupMap::const_iterator> range = GetLookupControls(m_focusedControl);

    for (LookupMap::const_iterator i = range.first; i != range.second; ++i)
    {
      if (i->second->HasFocus())
        return i->second;
    }
  }

  // if lookup didn't find focused control, iterate m_children to find it
  for (auto *control : m_children)
  {
    // Avoid calling HasFocus() on control group as it will (possibly) recursively
    // traverse entire group tree just to check if there is focused control.
    // We are recursively traversing it here so no point in doing it twice.
    CGUIControlGroup *groupControl(dynamic_cast<CGUIControlGroup*>(control));
    if (groupControl)
    {
      CGUIControl* focusedControl = groupControl->GetFocusedControl();
      if (focusedControl)
        return focusedControl;
    }
    else if (control->HasFocus())
      return control;
  }
  return NULL;
}

// in the case of id == 0, we don't match id
CGUIControl *CGUIControlGroup::GetFirstFocusableControl(int id)
{
  if (!CanFocus()) return NULL;
  if (id && id == GetID()) return this; // we're focusable and they want us
  for (auto *pControl : m_children)
  {
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(pControl));
    if (group)
    {
      CGUIControl *control = group->GetFirstFocusableControl(id);
      if (control) return control;
    }
    if ((!id || pControl->GetID() == id) && pControl->CanFocus())
      return pControl;
  }
  return NULL;
}

void CGUIControlGroup::AddControl(CGUIControl *control, int position /* = -1*/)
{
  if (!control) return;
  if (position < 0 || position > (int)m_children.size())
    position = (int)m_children.size();
  m_children.insert(m_children.begin() + position, control);
  control->SetParentControl(this);
  control->SetControlStats(m_controlStats);
  control->SetPushUpdates(m_pushedUpdates);
  AddLookup(control);
  SetInvalid();
}

bool CGUIControlGroup::InsertControl(CGUIControl *control, const CGUIControl *insertPoint)
{
  // find our position
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    CGUIControl *child = m_children[i];
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(child));
    if (group && group->InsertControl(control, insertPoint))
      return true;
    else if (child == insertPoint)
    {
      AddControl(control, i);
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::SaveStates(std::vector<CControlState> &states)
{
  // save our state, and that of our children
  states.push_back(CControlState(GetID(), m_focusedControl));
  for (auto *control : m_children)
    control->SaveStates(states);
}

// Note: This routine doesn't delete the control.  It just removes it from the control list
bool CGUIControlGroup::RemoveControl(const CGUIControl *control)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(child));
    if (group && group->RemoveControl(control))
      return true;
    if (control == child)
    {
      m_children.erase(it);
      RemoveLookup(child);
      SetInvalid();
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::ClearAll()
{
  // first remove from the lookup table
  RemoveLookup();

  // and delete all our children
  for (auto *control : m_children)
  {
    delete control;
  }
  m_focusedControl = 0;
  m_children.clear();
  ClearLookup();
  SetInvalid();
}

#ifdef _DEBUG
void CGUIControlGroup::DumpTextureUse()
{
  for (auto *control : m_children)
    control->DumpTextureUse();
}
#endif

// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's session restore (C API). Wren.h includes this header; see
// Wren.h for threading and handles.
//
// Chromium records the Browsers of a persistent or ephemeral Profile as the
// app uses them: their tabs, each tab's navigation history, pinned tabs,
// groups and the active tab. It never restores them by itself. The app asks,
// when it wants to: at launch, from a "Restore" button, or never.
// - The last session is what the Profile's Browsers held when the app last
//   quit or crashed. A window the user closed before quitting is not part
//   of it, except the last one.
// - A launch that opens a Browser and never restores replaces the last
//   session with its own when it ends.
// - Chromium also keeps the tabs and windows closed lately, across launches,
//   for Chrome's Reopen Closed Tab.
// An off-the-record Profile has neither.

#ifndef WREN_PUBLIC_WREN_SESSION_H_
#define WREN_PUBLIC_WREN_SESSION_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- The last session -------------------------------------------------------

typedef struct WrenSessionState {
  // Windows with tabs, and their tabs, that a restore would bring back.
  size_t window_count;
  size_t tab_count;
  // The app did not quit: it crashed or was killed. Chrome offers to restore
  // then, instead of restoring.
  bool crashed;
} WrenSessionState;

// `state` is valid during the call, and NULL unless `result` is WREN_OK.
typedef void (*WrenSessionCallback)(void* context,
                                    const WrenSessionState* state,
                                    WrenResult result);

// Reads what `profile`'s last session holds, without restoring it.
// WREN_ERROR_NOT_ALLOWED for an off-the-record Profile.
WREN_API void Wren_GetLastSession(WrenProfile* profile,
                                  WrenSessionCallback callback,
                                  void* context);

// `tab_count` tabs were restored; 0 if the last session held none.
typedef void (*WrenRestoreCallback)(void* context,
                                    WrenResult result,
                                    size_t tab_count);

// Restores `profile`'s last session. Each of its windows is a new Browser
// that reaches on_browser_opened, as any Browser Chromium opens does, with
// its tabs in place; a tab loads its page when it first becomes active.
// `callback` runs after the last of them. Restoring twice opens the session
// twice. WREN_ERROR_NOT_ALLOWED for an off-the-record Profile, or while a
// restore of the Profile is under way.
WREN_API void Wren_RestoreLastSession(WrenProfile* profile,
                                      WrenRestoreCallback callback,
                                      void* context);

// --- Closed tabs ------------------------------------------------------------

// Whether Chromium has a closed tab, group or window to reopen for
// `browser`'s Profile, as Chrome's Reopen Closed Tab is enabled;
// WREN_CHANGE_CLOSED_TABS reports each change once the app has read it.
// False for an off-the-record Profile's Browser, and once the Browser has
// closed. The first read after a launch loads what was closed before it, and
// is true until that has loaded.
WREN_API bool Wren_CanReopenClosedTab(WrenBrowser* browser);

// Reopens what was closed last, as Chrome's Reopen Closed Tab does: a tab
// returns to its Browser at its old place with its history, or to `browser`
// if its own has closed; a group returns with its tabs; a window is a new
// Browser that reaches on_browser_opened. Right after a launch it takes
// effect once the closed tabs have loaded. WREN_ERROR_NOT_ALLOWED if there is
// nothing to reopen.
WREN_API WrenResult Wren_ReopenClosedTab(WrenBrowser* browser);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_SESSION_H_

// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: restoring sessions and closed tabs (C API). Wren.h includes this
// header. Wren.h also explains threads and handles.
//
// Chromium records each Browser of a persistent or ephemeral Profile as the
// app uses it. The record holds the Browser's tabs, each tab's history, which
// tabs are pinned, the groups and the active tab.
//
// Chromium never restores a session by itself. The app decides when to ask:
// at launch, from a Restore button, or never.
//
// - The last session is what the Profile's Browsers held when the app last
//   quit or crashed. It leaves out windows the user closed before quitting,
//   except the last one closed.
// - If a launch opens a Browser and never restores, its own session replaces
//   the last session when the app quits.
// - Chromium also keeps a list of recently closed tabs and windows, across
//   launches, for Reopen Closed Tab.
// - The app can tag each Browser with a short string. Chromium keeps the tag
//   with the Browser in the session and in the list of closed windows.
// - Each tab has a persistent ID, which a restored tab has again.
//
// An off-the-record Profile has no last session and no closed tabs.

#ifndef WREN_PUBLIC_WREN_SESSION_H_
#define WREN_PUBLIC_WREN_SESSION_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- The last session -------------------------------------------------------

typedef struct WrenSessionState {
  // The number of windows with tabs, and of their tabs, that a restore would
  // bring back.
  size_t window_count;
  size_t tab_count;
  // True if the app did not quit but crashed or was killed. In that case
  // Chrome offers to restore instead of restoring.
  bool crashed;
  // True if the last session ended in a relaunch: Chromium's own, from
  // Wren_Relaunch or About's Relaunch, or one handed to the app
  // (Wren_LeaveRelaunchToApp). Chrome restores after a relaunch. The same for
  // every Profile.
  bool relaunched;
} WrenSessionState;

// `state` is valid only during the call. It is NULL unless `result` is
// WREN_OK.
typedef void (*WrenSessionCallback)(void* context,
                                    const WrenSessionState* state,
                                    WrenResult result);

// Reads what `profile`'s last session holds, without restoring it. Returns
// WREN_ERROR_NOT_ALLOWED for an off-the-record Profile.
WREN_API void Wren_GetLastSession(WrenProfile* profile,
                                  WrenSessionCallback callback,
                                  void* context);

// `tab_count` is the number of tabs restored. It is 0 if the last session
// held none.
typedef void (*WrenRestoreCallback)(void* context,
                                    WrenResult result,
                                    size_t tab_count);

// Restores `profile`'s last session.
//
// Each window of the session becomes a new Browser with its tabs in place.
// Each one reaches on_browser_opened, like any Browser Chromium opens. A
// restored tab loads its page the first time it becomes active. `callback`
// runs after the last Browser has opened.
//
// A launch restores a Profile's last session once, as Chrome does. Once a
// restore has started, later calls for the same Profile return
// WREN_ERROR_NOT_ALLOWED until the app quits, even if the Profile closes and
// opens again. A call that returns another error, or WREN_ERROR_NOT_ALLOWED for
// an off-the-record Profile or from inside a callback, restored nothing, and
// can be made again.
//
// A quit asked while a restore is under way waits for it. `callback` runs, and
// the restored Browsers open, before the quit goes on.
WREN_API void Wren_RestoreLastSession(WrenProfile* profile,
                                      WrenRestoreCallback callback,
                                      void* context);

// --- A Browser's tag --------------------------------------------------------
//
// A Browser can carry a short string of the app's, its session tag. Chromium
// saves the tag with the Browser in the session, and with the window in the
// list of closed windows. A Browser that a restore or Reopen Closed Tab brings
// back has its tag already when it reaches on_browser_opened.
//
// Use it to put a window back together. A window that shows several Browsers
// in turn ("Several Browsers in one window", WrenBrowser.h) tags each of them
// with the window and the Browser's place in it. When restored Browsers reach
// on_browser_opened, the app reads their tags and attaches each one to the
// window it names.
//
// Chromium records each Browser as a window of its own. A restore brings back
// all of them, but Reopen Closed Tab brings back one Browser at a time, from
// the list of the Browser's own Profile.

// Sets `browser`'s tag, or removes it if `tag` is "". A Browser has no tag
// until the app sets one, unless a restore or Reopen Closed Tab brought it back
// with one. A Browser Chromium opens from a tagged one, such as a new window or
// a popup, has no tag.
//
// The tag is at most 128 bytes of UTF-8, not counting the terminating NUL.
// That is the most Chromium keeps with a closed window across a relaunch.
// Returns WREN_ERROR_INVALID_ARGUMENT for a longer tag, for one that is not
// UTF-8, or for NULL. The Browser keeps its old tag then. Returns
// WREN_ERROR_CLOSED once the Browser has closed.
//
// The tag is saved wherever Chromium saves the Browser. An off-the-record
// Profile has no session, so its Browser keeps its tag only while it is open.
// It can be set at any time, also from inside on_popover and the other
// callbacks inside Chromium's call. No change is reported, since only the app
// sets it.
WREN_API WrenResult Wren_SetBrowserSessionTag(WrenBrowser* browser,
                                              const char* tag);
// Copies `browser`'s tag. "" if it has none, or once it has closed.
WREN_API size_t Wren_CopyBrowserSessionTag(WrenBrowser* browser,
                                           char* buffer,
                                           size_t size);

// --- A tab's persistent ID --------------------------------------------------
//
// A WrenTabID lasts one process. A tab's persistent ID lasts as long as the
// tab, across launches. Key what the app keeps for each tab by it, such as a
// name the user gave the tab, to find the record again after a restore.
//
// - A tab keeps its ID through navigation, moves, also to another Browser,
//   pinning, grouping and discards.
// - A tab that a restore brings back has its ID again, and so does one that
//   Reopen Closed Tab brings back in the same launch.
// - A new tab and a duplicate have a new ID, and so does a tab reopened from
//   the closed tabs of an earlier launch.
//
// An off-the-record Profile has no session, so its tabs' IDs last only while
// the tabs are open.
//
// Wren_CopyTabText copies a tab's persistent ID, with
// WREN_TAB_TEXT_PERSISTENT_ID (WrenTabs.h).

// --- Closed tabs ------------------------------------------------------------

// Returns true if Chromium has a closed tab, group or window to reopen for
// `browser`'s Profile. Chrome enables its Reopen Closed Tab command on the
// same condition.
//
// WREN_CHANGE_CLOSED_TABS reports each change, once the app has read this
// value. Returns false for an off-the-record Profile's Browser, and after the
// Browser has closed. Just after launch, Chromium loads the list of tabs
// closed in earlier launches. The first read starts that load, and returns
// true until it finishes.
WREN_API bool Wren_CanReopenClosedTab(WrenBrowser* browser);

// Reopens what was closed last, as Chrome's Reopen Closed Tab does.
//
// - A tab goes back to its Browser at its old place, with its history. If its
//   Browser has closed, it opens in `browser`.
// - A group comes back with its tabs.
// - A window comes back as a new Browser, which reaches on_browser_opened.
//
// Just after launch, the reopen waits until the list of closed tabs has
// loaded. Returns WREN_ERROR_NOT_ALLOWED if there is nothing to reopen.
//
// A reopened window reaches on_browser_opened before Chromium has taken it off
// the list. Do not call this again from inside that callback, or the same
// window opens twice. Call it from a later turn of the main loop.
//
// After a quit, the list also holds the windows the quit closed, as in Chrome.
WREN_API WrenResult Wren_ReopenClosedTab(WrenBrowser* browser);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_SESSION_H_

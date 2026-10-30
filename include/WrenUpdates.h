// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the app's updates and relaunching (C API). Wren.h includes this
// header. Wren.h also explains threads and handles.
//
// The app updates itself with an updater of its own, such as Sparkle. Wren
// does not check for or install updates. It lets the app:
//
// - Tell Chromium's About page (Settings > About) what its updater is doing.
//   With no updater, the page has no update line.
// - Hear when the About page asks for a check (on_update_check).
// - Relaunch as Chrome does, keeping the session, and hand a relaunch to its
//   updater, which installs before it launches the app again.
// - Learn at the next launch that the last session ended in a relaunch
//   (WrenSessionState.relaunched).
//
// About's Relaunch button, chrome://restart and chrome://flags relaunch the
// same way as Wren_Relaunch.

#ifndef WREN_PUBLIC_WREN_UPDATES_H_
#define WREN_PUBLIC_WREN_UPDATES_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// What the app's updater is doing.
typedef enum WrenUpdateStatus {
  // The app has no updater, or tells Chromium nothing. About has no update
  // line. The state at launch.
  WREN_UPDATE_NONE = 0,
  // Checking for an update.
  WREN_UPDATE_CHECKING = 1,
  // Downloading or installing an update, with `progress`.
  WREN_UPDATE_DOWNLOADING = 2,
  // The app is up to date.
  WREN_UPDATE_UP_TO_DATE = 3,
  // An update is ready, and a relaunch finishes it. About shows its Relaunch
  // button.
  WREN_UPDATE_READY = 4,
  // The check or the update failed, with `message`.
  WREN_UPDATE_FAILED = 5,
} WrenUpdateStatus;

typedef struct WrenUpdateState {
  WrenUpdateStatus status;
  // WREN_UPDATE_DOWNLOADING: done so far, from 0 to 100, or -1 if not known.
  // 0 otherwise.
  int progress;
  // WREN_UPDATE_READY: the update's version, or NULL. UTF-8.
  const char* version;
  // WREN_UPDATE_FAILED: what went wrong, as plain text, or NULL. UTF-8.
  const char* message;
} WrenUpdateState;

// Tells Chromium's About page what the app's updater is doing. Every open
// About page shows it at once, and later ones show it as they open. Wren
// copies the strings.
//
// Allowed inside Chromium's calls, as it changes no Browser. Returns
// WREN_ERROR_INVALID_ARGUMENT for an unknown status, a progress out of range,
// a string that is not UTF-8, or a field the status does not take.
WREN_API WrenResult Wren_SetUpdateState(const WrenUpdateState* state);

// Quits and launches the app again, as Chrome's Relaunch does. Each Profile's
// session is kept, session-only cookies and site data included. Pages are
// asked first with beforeunload, and one may cancel, as for Wren_Quit. A
// relaunch a page cancels is forgotten, as in Chrome. Then on_quit_committed
// runs, and Wren_IsRelaunching is true from there.
WREN_API void Wren_Relaunch(void);

// True while the app quits to relaunch, by Wren_Relaunch or by Chromium's
// own relaunch. The app's on_quit_committed reads it to tell a relaunch from
// a quit.
WREN_API bool Wren_IsRelaunching(void);

// Hands the relaunch to the app: Chromium will not launch the app again.
// Something else does, such as an updater that installs the update first.
// Everything else stays as for Chromium's relaunch: the session is kept, and
// the next launch's last session reads `relaunched`.
//
// Only inside on_quit_committed while relaunching. Returns
// WREN_ERROR_NOT_ALLOWED elsewhere. An app that hands a relaunch over must
// also tell its updater to launch the app, or the app stays quit.
WREN_API WrenResult Wren_LeaveRelaunchToApp(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_UPDATES_H_

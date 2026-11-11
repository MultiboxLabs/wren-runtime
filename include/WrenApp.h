// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the app's life after startup, and its place in macOS (C API). Wren.h
// includes this header. Wren.h also explains threads and handles.
//
// This header has quitting, with Warn Before Quitting and the app's own
// warning, relaunching and the app's updates, and being the default browser.

#ifndef WREN_PUBLIC_WREN_APP_H_
#define WREN_PUBLIC_WREN_APP_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Quitting ---------------------------------------------------------------

// Asks the app to quit. A page may cancel the quit with beforeunload. With
// downloads in progress, the user or the app may cancel it
// (on_downloads_prompt). Otherwise on_quit_committed runs, every Browser gets
// on_browser_closed, then on_will_quit runs, and Wren_Main returns
// WREN_EXIT_NORMAL.
//
// It never warns, whether or not Warn Before Quitting is on: only the Quit
// item's key does (on_quit_prompt). An app that wants the user to confirm
// asks first, then calls this.
WREN_API void Wren_Quit(void);

// Warn Before Quitting, Chrome's setting. While it is on, the Quit item's key
// warns before quitting (on_quit_prompt). It is on unless the user or policy
// turned it off. Chrome's menu item and chrome://settings change it too, and
// WREN_CHANGE_QUIT_WARNING reports every change.
typedef struct WrenQuitWarningState {
  // ⌘Q warns before quitting.
  bool on;
  // Policy sets it, and Wren_SetQuitWarning refuses to change it.
  bool managed;
} WrenQuitWarningState;

// Fills `state`. Before on_ready and after quit, it is off.
WREN_API void Wren_GetQuitWarningState(WrenQuitWarningState* state);

// Turns Warn Before Quitting on or off for every Profile, and for later
// launches. Returns WREN_ERROR_NOT_ALLOWED when policy sets it, as Chrome's
// menu item is disabled then, or inside a callback that runs inside
// Chromium's call.
WREN_API WrenResult Wren_SetQuitWarning(bool on);

// The ID of a quit prompt, unique for the life of the process.
typedef uint64_t WrenQuitPromptID;

// Chromium is about to warn before quitting (on_quit_prompt).
struct WrenQuitPrompt {
  WrenQuitPromptID prompt;
  // The key was pressed again while this prompt, which the app answered
  // WREN_POPOVER_APP, is open.
  bool again;
};

// Answers a prompt for which the app answered WREN_POPOVER_APP. With `quit`,
// the app quits, as Wren_Quit does, without a warning. Without it, the app
// keeps running.
//
// One prompt is open at a time, and Wren never withdraws it. A quit from
// elsewhere goes on while it is open. If a page's beforeunload cancels that
// quit, the prompt is still open.
//
// Returns WREN_ERROR_POPOVER_CLOSED for a prompt already answered, or once the
// app has quit, and WREN_ERROR_NOT_ALLOWED inside a callback that runs inside
// Chromium's call.
WREN_API WrenResult Wren_AnswerQuitPrompt(WrenQuitPromptID prompt, bool quit);

// --- Relaunching and updates ------------------------------------------------
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

// --- The default browser ----------------------------------------------------
//
// Whether the app is the browser macOS opens web links with, and asking the
// user to make it so, as Chrome's Settings do. The app is the default browser
// when it opens http links. For macOS to offer it, its Info.plist declares the
// http and https schemes and the HTML type (`wren bundle`'s `urlSchemes` and
// `documentTypes`).

typedef enum WrenDefaultBrowser {
  // macOS did not say, or the app has no bundle ID.
  WREN_DEFAULT_BROWSER_UNKNOWN = 0,
  WREN_DEFAULT_BROWSER_NO = 1,
  WREN_DEFAULT_BROWSER_YES = 2,
} WrenDefaultBrowser;

typedef struct WrenDefaultBrowserState {
  WrenDefaultBrowser state;
  // False where the DefaultBrowserSettingEnabled policy is off. Chrome's
  // Settings disable their button then, and Wren_SetAsDefaultBrowser refuses.
  bool can_set;
} WrenDefaultBrowserState;

// Receives the answer of Wren_GetDefaultBrowserState or
// Wren_SetAsDefaultBrowser, from the event loop. `state` is NULL unless
// `result` is WREN_OK, and borrowed until the callback returns.
typedef void (*WrenDefaultBrowserCallback)(void* context,
                                           const WrenDefaultBrowserState* state,
                                           WrenResult result);

// Asks macOS whether the app is the default browser, off the main thread, and
// gives the answer to `callback` from the event loop. The call returns at once.
// `callback` is required. It is not called before on_ready, or once the app
// quits.
//
// From the first call, or the first Wren_SetAsDefaultBrowser, Wren asks again
// each time the app becomes active, and reports WREN_CHANGE_DEFAULT_BROWSER
// whenever an answer differs from the one before. Someone who changes the
// default browser in System Settings leaves the app to do so. A change made
// while the app stays active is seen the next time it becomes active.
WREN_API void Wren_GetDefaultBrowserState(WrenDefaultBrowserCallback callback,
                                          void* context);

// Asks macOS to make the app the default browser, as Chrome's Settings do.
// macOS asks the user, and `callback` gets the answer from the event loop
// about when its prompt goes away: WREN_DEFAULT_BROWSER_YES if the user
// accepted, else what the default browser is. The call returns at once.
// `callback` is required. It is not called before on_ready, or once the app
// quits. There is no time limit: the user may leave the prompt open.
//
// - While a prompt is open, another call does not ask macOS again. It gets
//   the open prompt's answer.
// - The app's windows stay where they are. macOS's prompt comes in front.
//
// `callback` gets WREN_ERROR_NOT_ALLOWED where the DefaultBrowserSettingEnabled
// policy is off, or for a call made inside a callback that runs inside
// Chromium's call.
WREN_API void Wren_SetAsDefaultBrowser(WrenDefaultBrowserCallback callback,
                                       void* context);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_APP_H_

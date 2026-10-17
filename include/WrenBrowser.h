// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Profiles and Browsers (C API). Wren.h includes this header; see Wren.h
// for threading, handles and strings.
//
// Profiles, Browsers and their windows: several Browsers in one window, full
// screen and the side panel.

#ifndef WREN_PUBLIC_WREN_BROWSER_H_
#define WREN_PUBLIC_WREN_BROWSER_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Profiles ---------------------------------------------------------------

typedef enum WrenProfileKind {
  // Stored in user_data_dir and listed.
  WREN_PROFILE_PERSISTENT = 0,
  // Chromium's incognito Profile of a persistent one. It ends when Chromium
  // closes its last Browser; a later request starts a new session.
  WREN_PROFILE_OFF_THE_RECORD = 1,
  // Stored in user_data_dir but not listed, and deleted once its last handle
  // and Browser are gone, or at the next launch after a crash.
  WREN_PROFILE_EPHEMERAL = 2,
} WrenProfileKind;

// Receives one reference on success, or NULL and the reason.
typedef void (*WrenProfileCallback)(void* context,
                                    WrenProfile* profile,
                                    WrenResult result);

// Opens a Profile by ID: a persistent Profile (loading it if needed), or an
// open off-the-record or ephemeral one. NULL opens the startup Profile.
WREN_API void Wren_OpenProfile(const char* id,
                               WrenProfileCallback callback,
                               void* context);

// Creates a persistent Profile. An empty or NULL name lets Chromium choose.
WREN_API void Wren_CreateProfile(const char* display_name,
                                 WrenProfileCallback callback,
                                 void* context);

WREN_API void Wren_CreateEphemeralProfile(WrenProfileCallback callback,
                                          void* context);

// The off-the-record Profile of a persistent or ephemeral one. `*result`
// receives one reference.
WREN_API WrenResult Wren_GetOffTheRecordProfile(WrenProfile* original,
                                                WrenProfile** result);

// While the app holds a reference to a persistent or ephemeral Profile,
// Chromium keeps it loaded.
WREN_API void Wren_RetainProfile(WrenProfile* profile);
WREN_API void Wren_ReleaseProfile(WrenProfile* profile);

WREN_API bool Wren_IsProfileOpen(WrenProfile* profile);
WREN_API WrenProfileKind Wren_GetProfileKind(WrenProfile* profile);
// The Profile an off-the-record Profile belongs to; otherwise `profile`
// itself. Borrowed.
WREN_API WrenProfile* Wren_GetOriginalProfile(WrenProfile* profile);

// Copies a UTF-8 string and returns the length needed, including the NUL. A
// persistent or ephemeral Profile's ID is its directory name; an
// off-the-record Profile's ID is its original's ID followed by
// ":off-the-record".
WREN_API size_t Wren_CopyProfileID(WrenProfile* profile,
                                   char* buffer,
                                   size_t size);

// The persistent Profiles in user_data_dir, loaded or not.
WREN_API size_t Wren_GetProfileListCount(void);
WREN_API size_t Wren_CopyProfileListID(size_t index, char* buffer, size_t size);
WREN_API size_t Wren_CopyProfileListName(size_t index,
                                         char* buffer,
                                         size_t size);

// --- Browsers ---------------------------------------------------------------

// Receives one reference on success, or NULL and the reason.
typedef void (*WrenBrowserCallback)(void* context,
                                    WrenBrowser* browser,
                                    WrenResult result);

// Creates a Browser on `profile` with one tab showing `initial_url`, or
// Chromium's New Tab page if it is NULL or empty.
WREN_API void Wren_CreateBrowser(WrenProfile* profile,
                                 const char* initial_url,
                                 WrenBrowserCallback callback,
                                 void* context);

WREN_API void Wren_RetainBrowser(WrenBrowser* browser);
WREN_API void Wren_ReleaseBrowser(WrenBrowser* browser);

// Requests that the Browser close. Pages may cancel it with beforeunload;
// on_browser_closed reports the close. Returns WREN_ERROR_CLOSED if it has
// already closed.
WREN_API WrenResult Wren_CloseBrowser(WrenBrowser* browser);

WREN_API bool Wren_IsBrowserOpen(WrenBrowser* browser);
// WREN_NO_BROWSER once the Browser has closed.
WREN_API WrenBrowserID Wren_GetBrowserID(WrenBrowser* browser);
WREN_API WrenBrowserType Wren_GetBrowserType(WrenBrowser* browser);
// The WrenWindowFeature bits Chromium's own rule gives the Browser
// (WindowFeatureController): a page's popup has a title bar and a location
// bar, an extension's popup window or one Chromium opened itself only a
// title bar, a normal Browser everything but a title bar. It does not change
// while the Browser is open: full screen is not counted, where Chrome hides
// them all (see Full screen). 0 once the Browser has closed.
WREN_API uint32_t Wren_GetWindowFeatures(WrenBrowser* browser);
// Borrowed; NULL once the Browser has closed.
WREN_API WrenProfile* Wren_GetBrowserProfile(WrenBrowser* browser);

// The open Browsers the app knows (it created them or was told about them),
// oldest first. Borrowed.
WREN_API size_t Wren_GetBrowserCount(void);
WREN_API WrenBrowser* Wren_GetBrowserAt(size_t index);

// The screen rectangle the page asked for (window.open features), or the
// frame a restored Browser's window had (WrenSession.h), in AppKit screen
// coordinates; x and y are NAN if the page gave no position. Returns false
// if there is none.
WREN_API bool Wren_GetRequestedBounds(WrenBrowser* browser,
                                      double* x,
                                      double* y,
                                      double* width,
                                      double* height);

// Places Chromium's own browser UI (dialogs, permission and passkey prompts)
// over `ns_view`, the app view that hosts the tab views. It follows the view
// as the view or its ancestors move or resize, and shows only while the
// view's window is on screen. `ns_view` must be in a window; NULL detaches,
// as does closing the window, and attaching again moves it.
// The view's window becomes the Browser's window: the Browser is active while
// that window (or a Chromium window over it) is key, and Chromium's requests
// to activate the Browser make it key once the app has shown it. A window the
// app has not shown (one waiting for its page to draw), or has taken off
// screen itself, is left to the app. Attach every Browser you show.
// Browser-modal dialogs are sheets over the view; see Wren_HasModalDialog.
// Bubbles attach to the app's views named with Wren_SetAnchor.
WREN_API void Wren_AttachBrowserToView(WrenBrowser* browser, void* ns_view);

// Whether the Browser is active, as Chromium counts it: its window, or a
// window of Chromium's over it (a bubble, a dialog, a sheet), is key, and it
// is not in the background. Chrome draws its window's frame as in use by
// this, and not by the key window alone, so that its bars do not look left
// while page info is open; draw yours by it too. A Browser counts as active
// from when it is shown until its window's key state is known, and while
// Chromium has asked to activate one whose window the app has not shown.
// WREN_CHANGE_ACTIVE reports each change. False once the Browser has
// closed.
WREN_API bool Wren_IsBrowserActive(WrenBrowser* browser);

// Whether a browser-modal dialog of Chromium's is open for the Browser (an
// extension's install prompt or uninstall confirmation), shown or waiting
// for its window to be on screen; WREN_CHANGE_MODAL_DIALOG reports each
// change. While one is open, Chrome blocks the whole window: do the same for
// your UI (New Tab, Close Tab, the address field, switching tabs) and leave
// New Window and Quit available. Wren already refuses clicks below the
// window's title bar and disables Chromium's commands in its menus. Dialogs
// that belong to one page (HTTP auth, a page's alert) block only that page,
// and Chromium handles them. False once the Browser has closed.
WREN_API bool Wren_HasModalDialog(WrenBrowser* browser);

// --- Several Browsers in one window -----------------------------------------
//
// A window that shows several Browsers in turn, as workspaces on different
// Profiles are shown, attaches each of them to the same view and keeps all
// but one in the background. A Browser in the background still belongs to
// the window, and until it is brought forward:
// - it is never active, whatever window is key;
// - Chromium's commands in the menus do not reach it: they reach the
//   Browser of that window that is in front;
// - its browser-modal dialogs, its pages' dialogs and its bubbles are not
//   shown and block nothing. They wait, and are shown once it is in front;
// - a request of Chromium's to activate it (a notification's click, an
//   extension's chrome.windows.update, a page focusing a window it opened)
//   leaves it where it is and is reported to the app, which decides whether
//   to bring it forward.
// Its tabs stay open and loaded, as the tabs of a window behind another are.
// What its tabs' views or elements show is the app's and its page's to
// change. Keep one Browser of a window in front: Wren does not put the
// others in the background, and two in front share the window as two
// attached Browsers always have.

// Puts the Browser in the background, or brings it forward, which also
// forgets a request to come forward. A new Browser is in front.
WREN_API WrenResult Wren_SetBrowserBackgrounded(WrenBrowser* browser,
                                                bool backgrounded);
// False once the Browser has closed.
WREN_API bool Wren_IsBrowserBackgrounded(WrenBrowser* browser);

// Whether Chromium has asked for the Browser to come forward since it was
// put in the background; WREN_CHANGE_FOREGROUND_REQUEST reports the first
// such request. False once the Browser has closed.
WREN_API bool Wren_HasForegroundRequest(WrenBrowser* browser);

// --- Full screen ------------------------------------------------------------
//
// A page in full screen has the app's whole window, as in Chrome: while
// Wren_GetFullscreenTab names a tab, the app hides its tab strip and toolbar
// and gives the tab's view the window. Chromium asks the app to take the
// window into full screen and out of it (on_fullscreen), shows its own
// "Press Esc to exit full screen" notice over the page, and asks the app to
// leave when the user presses Escape. A page asks nothing while the window
// is in full screen already: it just takes the window.

// `browser`'s window entered (`fullscreen`) or left full screen, for
// on_fullscreen or by the user (the window's green button, the app's menu).
// Call it once the change has finished (windowDidEnterFullScreen:,
// windowDidExitFullScreen:), or failed. Leaving takes the page out of full
// screen too. WREN_ERROR_CLOSED once the Browser has closed.
WREN_API WrenResult Wren_SetFullscreen(WrenBrowser* browser, bool fullscreen);

// The tab whose page is in full screen in `browser`'s window, from the
// moment the app agrees to it; WREN_CHANGE_FULLSCREEN reports each change.
// WREN_NO_TAB if none, or once the Browser has closed.
WREN_API WrenTabID Wren_GetFullscreenTab(WrenBrowser* browser);

// --- Side panels ------------------------------------------------------------
//
// Chromium's side panels: Customize Chromium on the New Tab page, reading
// list, bookmarks, history, reading mode, extensions' panels. A Browser shows
// at most one at a time, as Chrome does, and the app decides where: put the
// Browser's side panel view where the panel belongs and show it while
// WrenSidePanelState.open. Wren puts each panel's page in it. Panels open
// from Chromium (the New Tab page's Customize button, Chrome's menu,
// chrome.sidePanel.open) or with Wren_ShowSidePanel; per-tab panels follow
// Chromium's rule on a tab switch (the new tab's own panel, else the window's
// panel, else none; switching back shows the tab's panel again). Panels that
// Chromium draws without a page (Lens, About this site) are not shown.

typedef struct WrenSidePanelState {
  bool open;
  // The tab a per-tab panel belongs to; WREN_NO_TAB for a window's panel.
  WrenTabID tab;
  // Chromium's width for the panel, in points, set when the panel opens.
  // Reading mode's is half its page's width at that moment.
  double default_width;
  // The width to show the panel at, in points: the one the user gave this
  // panel (Wren_SetSidePanelWidth), which Chromium remembers, else
  // default_width.
  double width;
  // Chromium's preference for the panel's side (Settings > Appearance): a
  // hint the app may follow.
  bool right_aligned;
} WrenSidePanelState;

// The Browser's side panel view (borrowed; valid while the Browser handle
// is), or NULL once the Browser has closed. The same view for every panel.
WREN_API void* Wren_GetSidePanelView(WrenBrowser* browser);
WREN_API WrenResult Wren_GetSidePanelState(WrenBrowser* browser,
                                           WrenSidePanelState* state);
// Chromium's name for the open panel ("ReadingList", "Bookmarks",
// "CustomizeChrome", "Extension" followed by the extension's ID, ...), or ""
// with none open. Copies a UTF-8 string and returns the length needed,
// including the NUL.
WREN_API size_t Wren_CopySidePanelID(WrenBrowser* browser,
                                     char* buffer,
                                     size_t size);
// The open panel's title, in Chromium's language, or "".
WREN_API size_t Wren_CopySidePanelTitle(WrenBrowser* browser,
                                        char* buffer,
                                        size_t size);
// Opens the panel with this ID, as Chromium names it, in the Browser or for
// its active tab; WREN_CHANGE_SIDE_PANEL follows once its page is ready.
// WREN_ERROR_PANEL_UNAVAILABLE if there is no such panel here now (an
// extension's panel needs the extension loaded); WREN_ERROR_NOT_ALLOWED in a
// Browser other than a normal one.
WREN_API WrenResult Wren_ShowSidePanel(WrenBrowser* browser, const char* id);
WREN_API WrenResult Wren_CloseSidePanel(WrenBrowser* browser);
// Records the width the user gave the open panel, in points, as dragging
// the edge of Chrome's side panel does: call it as the app's own resize
// handle is dragged. Chromium keeps one width per kind of panel in the
// Browser's Profile (every extension's panel shares one), across launches,
// and makes it no narrower than its narrowest panel (360 points).
// WREN_CHANGE_SIDE_PANEL follows, in every Browser of the Profile with a
// panel open; read WrenSidePanelState.width. WREN_ERROR_PANEL_UNAVAILABLE
// with no panel open.
WREN_API WrenResult Wren_SetSidePanelWidth(WrenBrowser* browser, double width);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_BROWSER_H_

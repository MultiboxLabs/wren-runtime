// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: tabs (C API). Wren.h includes this header. Wren.h also explains
// threads, handles and strings.
//
// This header covers tabs and what they show: navigation, groups, splits, a
// tab's view, and DevTools.

#ifndef WREN_PUBLIC_WREN_TABS_H_
#define WREN_PUBLIC_WREN_TABS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Tabs -------------------------------------------------------------------

// The number of the Browser's tabs. 0 once the Browser has closed.
WREN_API int Wren_GetTabCount(WrenBrowser* browser);
// The tab at `index`. WREN_NO_TAB if `index` is out of range.
WREN_API WrenTabID Wren_GetTabAt(WrenBrowser* browser, int index);
WREN_API WrenTabID Wren_GetActiveTab(WrenBrowser* browser);

// Returns the tab's Browser, borrowed. NULL if no open tab has this ID.
WREN_API WrenBrowser* Wren_GetTabBrowser(WrenTabID tab);
// Returns the tab's index in its Browser, or -1.
WREN_API int Wren_GetTabIndex(WrenTabID tab);

// Each copies a UTF-8 string and returns the length needed, including the
// NUL. A tab that is not available copies "".
WREN_API size_t Wren_CopyTabTitle(WrenTabID tab, char* buffer, size_t size);
WREN_API size_t Wren_CopyTabURL(WrenTabID tab, char* buffer, size_t size);

typedef struct WrenTabOptions {
  // Where to insert the tab. -1, or an index past the end, adds it at the end.
  int index;
  bool activate;
} WrenTabOptions;

// Adds a tab showing `url`. If `url` is NULL or empty, the tab shows
// Chromium's New Tab page. NULL `options` adds an active tab at the end. If
// `tab` is not NULL, it receives the new tab's ID.
WREN_API WrenResult Wren_AddTab(WrenBrowser* browser,
                                const char* url,
                                const WrenTabOptions* options,
                                WrenTabID* tab);

// Runs Chromium's New Tab command, as Cmd-T does in Chrome. It opens a tab in
// front, showing the New Tab page or an extension's replacement for it. A
// Browser without a tab strip, such as a popup, opens the tab in a normal
// Browser instead.
WREN_API WrenResult Wren_NewTab(WrenBrowser* browser);

WREN_API WrenResult Wren_ActivateTab(WrenTabID tab);
// Asks the tab to close. Its page may cancel the close with beforeunload
// (WrenTabState.may_ask_before_close). When
// a Browser's last tab closes, the Browser closes too (WrenBrowser.h).
WREN_API WrenResult Wren_CloseTab(WrenTabID tab);
// Loads `url` in the tab.
WREN_API WrenResult Wren_Navigate(WrenTabID tab, const char* url);

// Where a tab's DevTools is.
typedef enum WrenDevTools {
  WREN_DEVTOOLS_CLOSED = 0,
  // Docked in the tab's view, on that side of the page.
  WREN_DEVTOOLS_RIGHT = 1,
  WREN_DEVTOOLS_LEFT = 2,
  WREN_DEVTOOLS_BOTTOM = 3,
  // In a WREN_BROWSER_DEVTOOLS Browser of its own, which reaches
  // on_browser_opened.
  WREN_DEVTOOLS_UNDOCKED = 4,
} WrenDevTools;

// What Chrome's status bubble, at the bottom left of its window, would show
// for a tab. Chromium draws no status bubble in a Wren window. The app draws
// its own from this, or none.
typedef enum WrenTabStatus {
  WREN_TAB_STATUS_NONE = 0,
  // The address of the link under the pointer, or of the link with keyboard
  // focus.
  WREN_TAB_STATUS_LINK = 1,
  // What Chromium is doing for a loading page, such as "Waiting for
  // example.com…", while there is no link to show.
  WREN_TAB_STATUS_LOADING = 2,
} WrenTabStatus;

// What a tab is doing that Chrome marks on its tab. Bits of
// WrenTabState.alerts. Chrome shows one icon: the first of these that
// applies, in this order, with the muted speaker for an audible tab that is
// muted.
typedef enum WrenTabAlert {
  // The page captures a window or a screen.
  WREN_TAB_ALERT_CAPTURING_SCREEN = 1u << 0,
  // Something captures the page, such as a call it is shared to.
  WREN_TAB_ALERT_BEING_CAPTURED = 1u << 1,
  // The page records from a microphone, a camera, or both.
  WREN_TAB_ALERT_MICROPHONE = 1u << 2,
  WREN_TAB_ALERT_CAMERA = 1u << 3,
  // The page is connected to a device, or scans for Bluetooth ones.
  WREN_TAB_ALERT_BLUETOOTH = 1u << 4,
  WREN_TAB_ALERT_BLUETOOTH_SCANNING = 1u << 5,
  WREN_TAB_ALERT_USB = 1u << 6,
  WREN_TAB_ALERT_HID = 1u << 7,
  WREN_TAB_ALERT_SERIAL = 1u << 8,
  // A video of the page plays in Picture in Picture.
  WREN_TAB_ALERT_PICTURE_IN_PICTURE = 1u << 9,
  // The page plays sound, or did in the last 2 seconds, muted or not.
  WREN_TAB_ALERT_AUDIBLE = 1u << 10,
} WrenTabAlert;

// How a tab was opened from another (WrenTabState.opened_as).
typedef enum WrenTabOpenedAs {
  // No tab opened it: the app, an extension or a restore made it.
  WREN_TAB_OPENED_AS_NONE = 0,
  // A tab in front, as a link with target=_blank opens.
  WREN_TAB_OPENED_AS_FOREGROUND_TAB = 1,
  // A tab behind, as a Cmd-click, a middle click or Open Link in New Tab
  // opens.
  WREN_TAB_OPENED_AS_BACKGROUND_TAB = 2,
  // A new window, as a Shift-click or Open Link in New Window opens.
  WREN_TAB_OPENED_AS_WINDOW = 3,
  // A popup, as window.open with a size or "popup" opens.
  WREN_TAB_OPENED_AS_POPUP = 4,
} WrenTabOpenedAs;

// One on_navigation request. Unique for the life of the process, and never
// reused. WREN_NO_NAVIGATION means none.
typedef uint64_t WrenNavigationID;
#define WREN_NO_NAVIGATION 0

// Whether a tab's media plays (WrenTabMedia.playback).
typedef enum WrenMediaPlayback {
  // Nothing the person can play or pause: no media, only short sounds, media
  // that has stopped, or a call's stream. WrenTabMedia.actions may still
  // name what the page offers, such as a call's, and the devices may be set.
  WREN_MEDIA_NONE = 0,

  WREN_MEDIA_PLAYING = 1,
  WREN_MEDIA_PAUSED = 2,
} WrenMediaPlayback;

// What the page lets a person do with its media now, whatever its playback:
// an action named here can be asked for. Bits of WrenTabMedia.actions, and
// the action Wren_ControlTabMedia takes. Play,
// pause, stop and the seeks come with any media that can be played; the
// tracks, skip ad and a call's three only when the page handles them
// (navigator.mediaSession.setActionHandler); Picture in Picture when the
// page has one video, or handles it.
typedef enum WrenMediaAction {
  WREN_MEDIA_ACTION_PLAY = 1u << 0,
  WREN_MEDIA_ACTION_PAUSE = 1u << 1,
  // Ends the media, as Chrome's media controls' close button does.
  WREN_MEDIA_ACTION_STOP = 1u << 2,

  WREN_MEDIA_ACTION_PREVIOUS_TRACK = 1u << 3,
  WREN_MEDIA_ACTION_NEXT_TRACK = 1u << 4,
  // 5 seconds back or forward.
  WREN_MEDIA_ACTION_SEEK_BACKWARD = 1u << 5,
  WREN_MEDIA_ACTION_SEEK_FORWARD = 1u << 6,
  // Seeking to a position is offered (Wren_SeekTabMedia). Not an action
  // for Wren_ControlTabMedia.
  WREN_MEDIA_ACTION_SEEK_TO = 1u << 7,

  WREN_MEDIA_ACTION_SKIP_AD = 1u << 8,
  WREN_MEDIA_ACTION_ENTER_PICTURE_IN_PICTURE = 1u << 9,
  WREN_MEDIA_ACTION_EXIT_PICTURE_IN_PICTURE = 1u << 10,
  // A call's microphone and camera, on or off, and ending the call.
  WREN_MEDIA_ACTION_TOGGLE_MICROPHONE = 1u << 11,
  WREN_MEDIA_ACTION_TOGGLE_CAMERA = 1u << 12,
  WREN_MEDIA_ACTION_HANG_UP = 1u << 13,
} WrenMediaAction;

// A call's microphone or camera, as its page says
// (navigator.mediaSession.setMicrophoneActive and setCameraActive).
typedef enum WrenMediaDevice {
  // The page has not said.
  WREN_MEDIA_DEVICE_UNKNOWN = 0,

  WREN_MEDIA_DEVICE_ON = 1,
  WREN_MEDIA_DEVICE_OFF = 2,
} WrenMediaDevice;

// A tab's media (WrenTabState.media). WREN_TAB_FIELD_MEDIA reports a change.
typedef struct WrenTabMedia {
  WrenMediaPlayback playback;
  // WrenMediaAction bits.
  uint32_t actions;
  // True if Chromium knows the media's position. The rest of the position
  // is 0 without it.
  bool has_position;
  // Where the media is at the read, in milliseconds. Chromium reports a
  // change only when the position jumps or the rate changes, not as the
  // media plays: between reports, the position moves by `rate` times the
  // time since the read.
  int64_t position_ms;
  // The media's length in milliseconds, 0 for a stream with no end
  // (`live`).
  int64_t duration_ms;
  // 1 at normal speed, 0 while paused.
  double rate;
  // True for a stream with no end. Seeking is not offered.
  bool live;

  WrenMediaDevice microphone;
  WrenMediaDevice camera;
} WrenTabMedia;

typedef struct WrenTabState {
  bool loading;
  bool can_go_back;
  bool can_go_forward;
  bool pinned;
  // True if Chromium discarded the page to save memory. Activating or
  // reloading the tab loads it again.
  bool discarded;
  // All zero if the tab is in no group.
  WrenTabGroupID group;
  // Why the page is gone, if it is. A crashed tab's view shows nothing. The
  // app shows its own crashed state, and Wren_Reload loads the page again.
  WrenTabCrash crash;

  WrenDevTools devtools;
  // All zero if the tab is in no split.
  WrenSplitID split;
  // Only a tab the Browser shows has a status. That is its active tab, and
  // the other tab of the active tab's split.
  WrenTabStatus status;
  // True while the tab's page is in a <wren-view> element of a web view's
  // page ("Views in a web view", WrenWebView.h). Wren_FocusTab gives it the
  // keyboard there.
  //
  // An element gets its page only after the page's script asks for it. So
  // give a newly active tab the keyboard once this is true. It is false while
  // the page is in the tab's own view. It is also false from when the page
  // leaves the element, or is replaced by a discard, until the new page is in
  // an element.
  bool in_element;
  // When the tab was last in use, in milliseconds since 1970, UTC. While the
  // tab is shown, or is its Browser's active tab, it is the current time.
  // Otherwise it is when the tab was hidden or stopped being active,
  // whichever came later. So the active tab of a Browser whose window is
  // minimised, covered or on another Space, or that is in the background
  // (Wren_SetBrowserBackgrounded), reads the current time, as Chrome never
  // takes the active tab for idle. A restored tab keeps its time from the
  // launch before until it is shown. A tab added in the background has the
  // time it was made. No change is reported: read it when deciding.
  int64_t last_active_ms;
  // WrenTabAlert bits. WREN_TAB_FIELD_ALERTS reports a change.
  uint32_t alerts;
  // True while the tab's sound is muted: by Wren_SetTabMuted, by its site's
  // sound setting (WREN_CHROMIUM_COMMAND_MUTE_SITE) or by an extension.
  // WREN_TAB_FIELD_MUTED reports a change.
  bool muted;
  // True if closing the tab may show its page's "Leave site?" prompt: a frame
  // of the page has a beforeunload handler, and the user has used that frame.
  // The handler may still let the tab close. No change is reported: read it
  // when deciding.
  bool may_ask_before_close;
  // True while the tab is kept out of the session (Wren_SetTabSessionExcluded).
  // WREN_TAB_FIELD_SESSION_EXCLUDED reports a change.
  bool session_excluded;
  // True while the page waits to load until it is shown: a tab a restore
  // left in the background, a discarded tab, or a background tab whose
  // frame crashed. Showing, reloading or capturing the tab loads it. After a
  // restore, Chromium also loads background tabs itself, as Chrome does: at
  // least 4, and up to 20 while memory allows, of those used in the last 30
  // days. A change comes with WREN_TAB_FIELD_LOADING or
  // WREN_TAB_FIELD_DISCARDED, except a background tab's crashed frame, which
  // Chromium does not report.
  bool unloaded;
  // True while the app has set the tab to ignore input
  // (Wren_SetTabIgnoresInput). A block of Chromium's own, such as DevTools',
  // does not show here. WREN_TAB_FIELD_IGNORES_INPUT reports a change.
  bool ignores_input;
  // The tab whose page opened this one, or WREN_NO_TAB: by a link, a script,
  // a click with a key held or the page's context menu. It names the opener
  // even when a noopener link hides it from the page, and after the opener
  // closes. Open Link in Incognito Window names a tab of the original
  // Profile. It is set by the time on_changed or on_browser_opened first
  // gives the tab, never changes, and stays when the tab moves to another
  // Browser or its page is discarded. The session does not keep it.
  WrenTabID opened_by;
  // How opened_by opened it; WREN_TAB_OPENED_AS_NONE without opened_by.
  WrenTabOpenedAs opened_as;
  // True while the app has set the tab to watch its navigations
  // (Wren_SetTabWatchesNavigations). WREN_TAB_FIELD_WATCHES_NAVIGATIONS
  // reports a change.
  bool watches_navigations;
  // The on_navigation request whose WREN_NAVIGATION_OPEN_IN_NEW_TAB opened
  // this tab, or WREN_NO_NAVIGATION. Set, with opened_by, by the time
  // on_changed or on_browser_opened first gives the tab, and kept as
  // opened_by is.
  WrenNavigationID opened_for_navigation;
  // What the tab's media is doing. See "A tab's media".
  WrenTabMedia media;
} WrenTabState;

WREN_API WrenResult Wren_GetTabState(WrenTabID tab, WrenTabState* state);

// Copies the text of WrenTabState.status, or "" for none. Returns the length
// needed, including the NUL.
//
// A link's address is whole, formatted as Chrome shows it. Chrome's bubble
// shortens it to a third of the page's width. Once the pointer has rested on
// the link for 1.6 seconds, the bubble grows to fit the whole address.
WREN_API size_t Wren_CopyTabStatusText(WrenTabID tab,
                                       char* buffer,
                                       size_t size);

// Copies the tab's favicon as a PNG into `buffer` if it fits, and returns the
// PNG's size in bytes. Returns 0 if the tab has no favicon, or none yet. The
// PNG is at the largest scale Chromium keeps, 2x on a Mac. Draw it at 16
// points.
WREN_API size_t Wren_CopyTabFaviconPNG(WrenTabID tab,
                                       uint8_t* buffer,
                                       size_t size);

// How Wren_CaptureTab encodes its picture.
typedef enum WrenImageFormat {
  WREN_IMAGE_PNG = 0,
  WREN_IMAGE_JPEG = 1,
} WrenImageFormat;

typedef struct WrenCaptureOptions {
  // The largest the picture may be, in pixels. 0 sets no limit that way. The
  // picture keeps the page's aspect ratio, fits inside both limits, and is
  // never larger than the page's own pixels: its view's size at its screen's
  // scale.
  int max_width;
  int max_height;
  WrenImageFormat format;
  // For JPEG, 1 to 100. 0 means 80. PNG ignores it.
  int quality;
  // Captures even where the DisableScreenshots policy is set, as Chrome's
  // own hover-card pictures do. Set it only for a picture that stays in the
  // app's own UI, such as a tab switcher, never for one the app shares or
  // saves.
  bool ignore_policy;
} WrenCaptureOptions;

// A picture of a tab's page.
typedef struct WrenTabImage {
  // The encoded picture, borrowed until the callback returns.
  const uint8_t* data;
  size_t size;
  // In pixels.
  int width;
  int height;
} WrenTabImage;

// Receives the result of Wren_CaptureTab, from the event loop. `image` is
// NULL unless `result` is WREN_OK.
typedef void (*WrenTabImageCallback)(void* context,
                                     WrenTabID tab,
                                     const WrenTabImage* image,
                                     WrenResult result);

// Takes a picture of the tab's page, as it draws it after the call, and
// gives it to `callback` from the event loop. The call returns at once, and
// may be made anywhere, inside any callback. `callback` is required. It is
// not called before on_ready, or once the app quits. NULL `options` means a
// PNG at the page's own size.
//
// The tab may be shown or in the background, in any Browser, off the record
// too. A background page draws for the picture but stays hidden: it is not
// told it is shown, and no visibilitychange fires. A tab still loading gives
// what is drawn so far. An unloaded tab (WrenTabState.unloaded), such as one a
// restore left in the background, starts loading, as Chrome's tab previews
// do, and may give WREN_ERROR_NO_IMAGE: ask again once it has loaded. To leave
// a tab unloaded, read its state first and do not capture it. Chromium loads
// some restored tabs itself (WrenTabState.unloaded). The picture is
// the page alone: no docked DevTools, no app UI and none of Chromium's windows
// over it.
//
// `callback` gets one of these errors without a picture:
//
// - WREN_ERROR_INVALID_ARGUMENT for a negative size, an unknown format or a
//   quality past 100.
// - WREN_ERROR_TAB_UNAVAILABLE if no open tab has the ID, or it closes first.
// - WREN_ERROR_NOT_ALLOWED where the DisableScreenshots policy is set for the
//   tab's Profile, unless `options->ignore_policy` is true.
// - WREN_ERROR_NO_IMAGE for a tab that is discarded, crashed or of no size,
//   or that draws nothing within 5 seconds. WrenTabState tells which.
WREN_API void Wren_CaptureTab(WrenTabID tab,
                              const WrenCaptureOptions* options,
                              WrenTabImageCallback callback,
                              void* context);

// Moves the tab to `index` within its Browser. Chromium keeps pinned tabs
// first and a group's tabs together, so the tab may end up at another index.
// Wren_GetTabIndex gives the index it ends up at.
WREN_API WrenResult Wren_MoveTab(WrenTabID tab, int index);
// Pins or unpins the tab. Pinning moves the tab to the end of the pinned
// tabs. Unpinning moves it to the start of the others.
WREN_API WrenResult Wren_SetTabPinned(WrenTabID tab, bool pinned);
// Mutes or unmutes the tab's sound, as Chrome's speaker on a tab does. It
// unmutes a tab its site's sound setting muted, too. Chromium does not keep
// it in the session.
WREN_API WrenResult Wren_SetTabMuted(WrenTabID tab, bool muted);

// Makes the tab's page ignore input, or take it again. While it ignores,
// Chromium drops the mouse, the wheel, keys, drags and accessibility actions
// meant for the page, as under one of its own tab-modal dialogs. A screen
// reader can still read the page. The setting stays with the tab when it
// moves to another Browser, and when its page is replaced, as a discard
// does. The session does not keep it. WREN_TAB_FIELD_IGNORES_INPUT reports
// a change.
//
// The page can still take the keyboard and an input method's text. Keep the
// keyboard off the tab while it ignores: Wren_FocusTab refuses it, but the
// Tab key from a view beside it and AppKit's key view loop can still give it
// the keyboard.
//
// A block of Chromium's own, such as DevTools', is separate: lifting the
// app's leaves it.
//
// - WREN_ERROR_TAB_UNAVAILABLE if no open tab has the ID.
// - WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API WrenResult Wren_SetTabIgnoresInput(WrenTabID tab, bool ignores);

// --- A tab's media ----------------------------------------------------------
//
// What a tab plays, from Chromium's media session for the tab, as Chrome's
// media controls in its toolbar show it: WrenTabState.media, these copies,
// and calls that act on it. The app draws the controls. macOS's Now Playing,
// the keyboard's media keys and the Picture in Picture window's buttons stay
// Chromium's, and act on the same media, so every change reaches the app as
// a WREN_TAB_FIELD_MEDIA or WREN_TAB_FIELD_MEDIA_METADATA record.

// Which text Wren_CopyTabMediaText copies.
typedef enum WrenMediaText {
  // The page's title for its media, else the page's own title.
  WREN_MEDIA_TEXT_TITLE = 0,
  // "" unless the page sets them.
  WREN_MEDIA_TEXT_ARTIST = 1,
  WREN_MEDIA_TEXT_ALBUM = 2,
  // Where the media comes from, as Chrome's media controls show it: the
  // site, such as "example.com", or an app's or extension's name.
  WREN_MEDIA_TEXT_SOURCE = 3,
} WrenMediaText;

// Copies one text of the tab's media, or "" if it has none. Returns the
// length needed, including the NUL.
WREN_API size_t Wren_CopyTabMediaText(WrenTabID tab,
                                      WrenMediaText which,
                                      char* buffer,
                                      size_t size);

// Copies the artwork the page gives its media as a PNG into `buffer` if it
// fits, and returns the PNG's size in bytes. Returns 0 if the page gives
// none, or while Wren is still fetching it: WREN_TAB_FIELD_MEDIA_METADATA
// reports it when it arrives. It is the image Chrome's media controls pick,
// at least 114 pixels and as near 512 as the page has.
WREN_API size_t Wren_CopyTabMediaArtworkPNG(WrenTabID tab,
                                            uint8_t* buffer,
                                            size_t size);

// Asks the tab's media to do one action of WrenTabMedia.actions, as Chrome's
// media controls do. The call sends it to the page and returns: the result
// comes as a WREN_TAB_FIELD_MEDIA record, or as the page's own change. It
// may be made anywhere, inside any callback.
//
// - WREN_ERROR_TAB_UNAVAILABLE if no open tab has the ID.
// - WREN_ERROR_INVALID_ARGUMENT for none or more than one bit, or for
//   WREN_MEDIA_ACTION_SEEK_TO, which is Wren_SeekTabMedia.
// - WREN_ERROR_NOT_ALLOWED if the tab's media does not offer the action now.
WREN_API WrenResult Wren_ControlTabMedia(WrenTabID tab, WrenMediaAction action);

// Seeks the tab's media to `position_ms` from its start, as
// Wren_ControlTabMedia acts. A position past the end goes to the end.
//
// - WREN_ERROR_TAB_UNAVAILABLE if no open tab has the ID.
// - WREN_ERROR_INVALID_ARGUMENT for a negative position.
// - WREN_ERROR_NOT_ALLOWED unless the media offers WREN_MEDIA_ACTION_SEEK_TO.
WREN_API WrenResult Wren_SeekTabMedia(WrenTabID tab, int64_t position_ms);

// --- Navigations
// --------------------------------------------------------------

// A navigation of a watched tab, as on_navigation gets it (Wren.h). Strings
// are UTF-8, valid only during the call.
struct WrenNavigationRequest {
  // Unique for the life of the process. A tab opened for the request reads it
  // in WrenTabState.opened_for_navigation.
  WrenNavigationID id;
  WrenTabID tab;
  // Where the tab would go: at a redirect, the redirect's target.
  const char* url;
  // The origin of the page that started the navigation, "null" for an opaque
  // one, such as a sandboxed frame's, or NULL when no page started it.
  const char* initiator_origin;
  // A server redirected the navigation. It was asked about when it started.
  bool redirect;
  // A click, a key or another user action started it.
  bool user_gesture;
  // A page or an extension started it: a link, a form, a page's script, or
  // an extension's chrome.tabs.update, with initiator_origin naming it. False
  // for the browser's own commands, such as Chrome's Home.
  bool by_page;
  // The request is a POST, such as a form's. A new tab the answer opens sends
  // the same body and Content-Type, once.
  bool post;
};

// Makes the tab ask the app about its navigations, through on_navigation, or
// stop asking. Off by default.
//
// Asked: each navigation of the tab's page to another document, when it
// starts and at each redirect, whether a page or Chromium started it.
//
// Not asked:
//
// - navigations the app starts with a Wren call, such as Wren_Navigate,
//   Wren_OpenSuggestion and Wren_OpenBookmarkItem
// - a reload, going back or forward, or a discarded or restored page loading
//   again
// - a frame inside the page, a fragment or history.pushState, and about:blank
//
// A watched tab does not prerender pages: a page the tab would show at once
// from a prerender is asked about as any other. Watching stops the tab's
// prerenders.
//
// The setting stays with the tab when it moves to another Browser, and when
// its page is replaced, as a discard does. The session does not keep it.
// WREN_TAB_FIELD_WATCHES_NAVIGATIONS reports a change.
//
// - WREN_ERROR_TAB_UNAVAILABLE if no open tab has the ID.
// - WREN_ERROR_NOT_ALLOWED inside on_popover or on_navigation.
WREN_API WrenResult Wren_SetTabWatchesNavigations(WrenTabID tab, bool watches);

// Keeps the tab out of the session, or puts it back. Use it for a tab the app
// makes again from its own records each launch, which the session would
// otherwise restore a second time.
//
// - The session the next launch restores (Wren_RestoreLastSession) leaves the
//   tab out. So does Wren_GetLastSession's count. Its window keeps the other
//   tabs, in their order, and the active tab is the nearest kept tab, to the
//   right first. A split with one tab left out restores the other as a plain
//   tab. A window of only such tabs is not restored.
// - Closed, the tab is kept in memory: Reopen Closed Tab brings it back in
//   this launch, still kept out, with its persistent ID. Its closed tab is
//   never written to disk. A closed window or group leaves it out.
// - It stays out when it moves to another Browser, from a popup too, and when
//   Chromium discards its page.
// - Its history is kept, and its persistent ID's records
//   (Wren_CopyTabPersistentID) are as for any tab.
// - A tab in a group is also in Chrome's saved copy of that group, which
//   brings the group's tabs back when the group is restored, this tab too.
//   Keep such a tab out of groups.
//
// Wren cannot keep a tab out before Chromium adds it, so call this right after
// Wren_AddTab. Wren takes what was written for the tab out of the session at
// once, before it reaches disk. Each call rewrites the Profile's session, so
// a call per tab for hundreds of tabs costs more than for a few. An
// off-the-record tab may be kept out too; it has no session anyway.
WREN_API WrenResult Wren_SetTabSessionExcluded(WrenTabID tab, bool excluded);

// Moves tabs to `target`, another Browser. The tabs keep their pages, their
// history, their IDs and their views. What a tab carries comes with it: a
// dialog over its page, and DevTools docked to it.
//
// - The tabs must all be in one Browser. Both Browsers must be normal
//   (WREN_BROWSER_NORMAL) and of the same Profile. An off-the-record Profile
//   is its own, apart from its original.
// - `options->index` is where the first tab goes in `target`. The others
//   follow it, in the order they had in their Browser. -1, or an index past
//   the end, puts them at the end. NULL `options` puts them at the end and
//   activates one.
// - Pinned tabs stay pinned, and go among the target's pinned tabs. The
//   others go after those.
// - A group all of whose tabs move goes with them, keeping its ID, title,
//   colour and collapsed state. Tabs that leave others of their group behind
//   leave the group.
// - Naming either tab of a split moves the whole split, keeping its ID.
// - A tab or split that lands between two tabs of one group joins that
//   group, as with Wren_MoveTab. A whole group that would land inside another
//   goes to that group's nearer end.
// - With `options->activate`, the tabs' Browser's active tab becomes the
//   target's active tab if it moves, and otherwise the first tab moved does.
//   Without it, the target keeps its active tab. The tabs' Browser picks a
//   new active tab, as when a tab closes.
// - Wren does not bring the target's window forward.
// - If every tab of their Browser moves, that Browser closes after the call
//   returns, as when its last tab closes. on_browser_closed reports it.
//
// Nothing moves when the call fails. It returns WREN_ERROR_INVALID_ARGUMENT
// if `count` is 0, an ID repeats, the tabs are in different Browsers, or
// `target` is their own Browser. It returns WREN_ERROR_TAB_UNAVAILABLE if a
// tab is not open, and WREN_ERROR_CLOSED if either Browser has closed or is
// closing. It returns WREN_ERROR_NOT_ALLOWED if either Browser is not normal,
// or if their Profiles differ.
WREN_API WrenResult Wren_MoveTabsToBrowser(const WrenTabID* tabs,
                                           size_t count,
                                           WrenBrowser* target,
                                           const WrenTabOptions* options);
// Moves tabs to a new normal Browser of their Profile, as
// Wren_MoveTabsToBrowser moves them. The tabs' Browser's active tab becomes
// the new Browser's active tab if it moves, and otherwise the first tab moved
// does. The app shows the new Browser as it shows one it creates
// (Wren_CreateBrowser). on_browser_opened does not run for it.
//
// `callback` runs from the event loop. It receives the new Browser, with one
// reference, or NULL and the error. The errors are those of
// Wren_MoveTabsToBrowser, and WREN_ERROR_NOT_ALLOWED if the tabs would leave
// their Browser with none, or if Chromium's policy forbids a new window for
// the Profile.
WREN_API void Wren_MoveTabsToNewBrowser(const WrenTabID* tabs,
                                        size_t count,
                                        WrenBrowserCallback callback,
                                        void* context);

// Moves the tab of a popup into `target`, a normal Browser of its Profile, as
// Chrome's Show as Tab does. The page keeps its history, its state and
// window.opener, and does not load again. A dialog over the page comes with
// it, and so does its DevTools.
//
// The tab cannot keep its ID across kinds of Browser: it becomes a new tab,
// with a new ID and a new view. `*moved_tab` receives the new ID. Its
// persistent ID (Wren_CopyTabPersistentID) stays the same.
//
// - The tab's Browser must be WREN_BROWSER_POPUP or WREN_BROWSER_APP_POPUP.
// - `options` as for Wren_MoveTabsToBrowser: the tab is never pinned, and
//   joins a group it lands inside. NULL `options` appends it and activates
//   it.
// - A page in full screen leaves it.
// - Wren does not bring the target's window forward.
// - The popup, left with no tabs, closes after the call returns.
//   on_browser_closed reports it.
//
// Nothing moves when the call fails. It returns WREN_ERROR_INVALID_ARGUMENT
// if `moved_tab` is NULL or `target` is the tab's own Browser,
// WREN_ERROR_TAB_UNAVAILABLE if no open tab has the ID, and WREN_ERROR_CLOSED
// if either Browser has closed or is closing. It returns
// WREN_ERROR_NOT_ALLOWED if the tab's Browser is of another kind, or if
// `target` is not normal or is of another Profile.
WREN_API WrenResult Wren_MovePopupTabToBrowser(WrenTabID tab,
                                               WrenBrowser* target,
                                               const WrenTabOptions* options,
                                               WrenTabID* moved_tab);
// Receives the new Browser of Wren_MovePopupTabToNewBrowser, with one
// reference, and the moved tab's new ID. On an error, NULL, WREN_NO_TAB and
// the error.
typedef void (*WrenMovedTabCallback)(void* context,
                                     WrenBrowser* browser,
                                     WrenTabID tab,
                                     WrenResult result);
// Moves the tab of a popup into a new normal Browser of its Profile, as
// Wren_MovePopupTabToBrowser moves it. The tab is the new Browser's only tab,
// and active. The app shows the new Browser as it shows one it creates
// (Wren_CreateBrowser). on_browser_opened does not run for it.
//
// `callback` runs from the event loop. The errors are those of
// Wren_MovePopupTabToBrowser, and WREN_ERROR_NOT_ALLOWED if Chromium's policy
// forbids a new window for the Profile.
WREN_API void Wren_MovePopupTabToNewBrowser(WrenTabID tab,
                                            WrenMovedTabCallback callback,
                                            void* context);

// --- Navigation -------------------------------------------------------------

// Each returns WREN_ERROR_NAVIGATION_UNAVAILABLE if there is no page to go to.
WREN_API WrenResult Wren_GoBack(WrenTabID tab);
WREN_API WrenResult Wren_GoForward(WrenTabID tab);
// Reloads the page. Chromium may ask the user before sending a form again.
WREN_API WrenResult Wren_Reload(WrenTabID tab);
WREN_API WrenResult Wren_Stop(WrenTabID tab);
// Ends the renderer process of the tab's page, as Chrome's task manager's End
// Process does. The page's state reads WREN_TAB_CRASH_KILLED, and
// Wren_Reload loads it again. Every other page and frame in that process,
// other tabs' and web views' included, ends with it, each reading killed too,
// as in Chrome. Returns WREN_ERROR_NOT_ALLOWED for a tab whose page has no
// live renderer (not loaded, discarded, crashed, or still starting one), and
// inside on_popover or on_fullscreen.
WREN_API WrenResult Wren_KillTabRenderer(WrenTabID tab);

// --- Tab groups -------------------------------------------------------------

typedef enum WrenTabGroupColor {
  WREN_TAB_GROUP_GREY = 0,
  WREN_TAB_GROUP_BLUE = 1,
  WREN_TAB_GROUP_RED = 2,
  WREN_TAB_GROUP_YELLOW = 3,
  WREN_TAB_GROUP_GREEN = 4,
  WREN_TAB_GROUP_PINK = 5,
  WREN_TAB_GROUP_PURPLE = 6,
  WREN_TAB_GROUP_CYAN = 7,
  WREN_TAB_GROUP_ORANGE = 8,
} WrenTabGroupColor;

// Puts the tabs in a new group. They must all be in one Browser. `*group`
// receives the group's ID. Pinned tabs are unpinned, and the tabs move next to
// each other. Returns WREN_ERROR_NOT_ALLOWED if the Browser has no tab strip,
// such as a popup.
WREN_API WrenResult Wren_GroupTabs(const WrenTabID* tabs,
                                   size_t count,
                                   WrenTabGroupID* group);
// Adds the tabs to an existing group in their Browser.
WREN_API WrenResult Wren_AddTabsToGroup(const WrenTabID* tabs,
                                        size_t count,
                                        WrenTabGroupID group);
// Takes the tabs out of their groups. A group closes when its last tab leaves
// it.
WREN_API WrenResult Wren_UngroupTabs(const WrenTabID* tabs, size_t count);

// Moves the group's tabs together within its Browser, as dragging its header
// in Chrome's tab strip does. The group keeps its ID, title, colour and
// collapsed state, and a split in it stays a split.
//
// `index` is where the group's first tab goes, counted among the tabs that do
// not move. Wren makes it valid as Chrome's strip does: never among the pinned
// tabs or past the end, never inside another group but at its nearer end, and
// never between a split's two tabs but at the split's nearer end.
// Wren_GetTabIndex gives where the group's tabs end up.
WREN_API WrenResult Wren_MoveTabGroup(WrenTabGroupID group, int index);

// Returns the group's Browser, borrowed. NULL if no open group has this ID.
WREN_API WrenBrowser* Wren_GetTabGroupBrowser(WrenTabGroupID group);
// Copies the group's title, in UTF-8, and returns the length needed, including
// the NUL. Copies "" if the group has no title or is not available.
WREN_API size_t Wren_CopyTabGroupTitle(WrenTabGroupID group,
                                       char* buffer,
                                       size_t size);
// Reads the group's colour, and whether it is collapsed.
WREN_API WrenResult Wren_GetTabGroupVisuals(WrenTabGroupID group,
                                            WrenTabGroupColor* color,
                                            bool* collapsed);
// Sets the group's title, colour and collapsed state. `title` may be NULL or
// empty for a group with no title.
WREN_API WrenResult Wren_SetTabGroupVisuals(WrenTabGroupID group,
                                            const char* title,
                                            WrenTabGroupColor color,
                                            bool collapsed);

// --- Tab views --------------------------------------------------------------
//
// A tab's view is an NSView. The app places it in its own window, like any
// other view. It holds the tab's page. While DevTools is docked to the tab, it
// holds DevTools too, which Wren lays out on the side that DevTools' own dock
// menu chose: right, left or below the page.
//
// - Chromium draws the page only while the view is in a window, no ancestor
//   of the view is hidden, and the window is not covered. Otherwise the page
//   is hidden and stops drawing. Remove or hide the views of tabs that are not
//   on screen. Wren_CaptureTab draws a hidden page once, for its picture.
// - The page is the view's size. A view taken out of its window keeps its
//   size. While the app has given the view no size, a page that left a
//   <wren-view> element keeps the element's size. Set the view's
//   frame or its autoresizing mask.
// - Give the view, and the side panel view, a whole number of points each
//   way. Chromium sizes a page in whole points and drops any fraction. In a
//   view 600.5 points tall the page is 600 points tall, and whatever is behind
//   the view shows in a strip along its bottom edge, one pixel at 2x.
// - The page's scale, its devicePixelRatio, follows the window's screen.
// - The view stays the same for the whole life of the tab. That includes when
//   Chromium replaces the page inside it by discarding it, and when the tab
//   moves to another Browser. A popup's tab that moves to a normal Browser
//   becomes a new tab, with a new view (Wren_MovePopupTabToBrowser).

// Returns the tab's NSView, or NULL. Borrowed, and valid until the tab closes.
WREN_API void* Wren_GetTabView(WrenTabID tab);

// Gives the tab's page keyboard focus within its view's window. It restores
// focus to the element that had it, as Chrome does when switching tabs.
//
// A page that wants the address field to have focus instead, such as the New
// Tab page, gets on_focus_location_bar. The call does not make the window key.
// It does nothing while the view is in no window. A tab that a web view's page
// shows (WrenWebView.h) takes focus there, in that web view's window. A tab
// that ignores input (Wren_SetTabIgnoresInput) gets WREN_ERROR_NOT_ALLOWED.
WREN_API WrenResult Wren_FocusTab(WrenTabID tab);

// --- Split tabs -------------------------------------------------------------
//
// A split shows two tabs of one Browser side by side, as one. It is Chromium's
// split view.
//
// - Each side is an ordinary tab, with its own ID, page, history and
//   DevTools. One of the two is the active tab.
// - While either side is active, show both tabs' views, divided by the
//   ratio. The app draws its own divider, and calls Wren_SetSplitRatio as the
//   user drags it.
// - Focusing a side's page makes that side the active tab.
// - Chromium keeps a split's tabs together. Closing one side ends the split.
//   So does moving, grouping or pinning one side alone.

typedef enum WrenSplitLayout {
  // The first tab on the left, the second on the right.
  WREN_SPLIT_SIDE_BY_SIDE = 0,
  // The first tab above the second.
  WREN_SPLIT_STACKED = 1,
} WrenSplitLayout;

typedef struct WrenSplitState {
  // The first tab, on the left or at the top, then the second.
  WrenTabID tabs[2];
  WrenSplitLayout layout;
  // The first tab's share of the width, or of the height, between 0 and 1.
  double ratio;
} WrenSplitState;

// Puts two tabs of one Browser in a new split, `first` first, and moves them
// next to each other. `first`'s group and pinning then apply to both. If
// `split` is not NULL, it receives the split's ID.
//
// Returns WREN_ERROR_NOT_ALLOWED if the Browser is not a normal one, or if
// either tab is already in a split.
WREN_API WrenResult Wren_SplitTabs(WrenTabID first,
                                   WrenTabID second,
                                   WrenSplitLayout layout,
                                   WrenSplitID* split);
// Runs Chromium's New Split Tab command. It adds a new tab next to the active
// one, in a split with it, and shows Chromium's tab picker on the new side.
//
// Returns WREN_ERROR_NOT_ALLOWED if the Browser is not a normal one, or if the
// active tab is already in a split.
WREN_API WrenResult Wren_NewSplitTab(WrenBrowser* browser);
// Ends the split. Both tabs stay open, where they are.
WREN_API WrenResult Wren_Unsplit(WrenSplitID split);
WREN_API WrenResult Wren_GetSplitState(WrenSplitID split,
                                       WrenSplitState* state);
// Sets the first tab's share. `ratio` must be strictly between 0 and 1. Pass
// `dragging` as true while the user is still dragging the divider. Chromium
// saves the ratio once a call passes false.
WREN_API WrenResult Wren_SetSplitRatio(WrenSplitID split,
                                       double ratio,
                                       bool dragging);
WREN_API WrenResult Wren_SetSplitLayout(WrenSplitID split,
                                        WrenSplitLayout layout);
// Swaps the two tabs' places.
WREN_API WrenResult Wren_SwapSplitTabs(WrenSplitID split);

// Moves both tabs of the split together within its Browser, as dragging a
// split in Chrome's tab strip does. The split keeps its ID, layout, ratio and
// pinning.
//
// `index` is where the split's first tab goes, counted among the tabs that do
// not move. Wren makes it valid: among the pinned tabs for a pinned split and
// after them for another, and never between another split's two tabs but at
// its nearer end. The split's group follows Chromium's rule for a tab: it
// joins a group it lands inside, keeps its own while a tab beside it is in
// that group, and otherwise leaves it. A split that is its group's only tabs
// keeps the group and moves it. Wren_GetTabIndex gives where its tabs end up.
WREN_API WrenResult Wren_MoveTabSplit(WrenSplitID split, int index);

// --- DevTools ---------------------------------------------------------------
//
// DevTools stays with the page it inspects. Docked, it is part of the tab's
// view. DevTools' own dock menu moves it to another side, or to a window of
// its own, and back. That window is a WREN_BROWSER_DEVTOOLS Browser, which
// reaches on_browser_opened. Only tabs of a normal Browser can dock DevTools.
// For any other tab, DevTools always opens in its own window.

// Opens DevTools for the tab, docked on the side `place` names, or in its own
// window for WREN_DEVTOOLS_UNDOCKED. WREN_DEVTOOLS_CLOSED opens it where
// DevTools was last, which is on the right the first time. If DevTools is
// already open, it is shown where it is, not moved.
//
// Returns WREN_ERROR_NOT_ALLOWED if Chromium's policy forbids DevTools for the
// page.
WREN_API WrenResult Wren_OpenDevTools(WrenTabID tab, WrenDevTools place);
// Closes the tab's DevTools, docked or not. DevTools may cancel the close, as
// it can when the user clicks its own close button.
WREN_API WrenResult Wren_CloseDevTools(WrenTabID tab);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_TABS_H_

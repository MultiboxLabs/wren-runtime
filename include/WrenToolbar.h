// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the toolbar (C API). Wren.h includes this header; see Wren.h for
// threading, handles and strings.
//
// The app's toolbar: the views Chromium's bubbles hang off, page actions
// and content setting icons.

#ifndef WREN_PUBLIC_WREN_TOOLBAR_H_
#define WREN_PUBLIC_WREN_TOOLBAR_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Anchors ----------------------------------------------------------------
//
// Chrome attaches its bubbles to its toolbar: permission prompts and page
// info to the site icon in its address field, extension popups to their
// buttons, a feature's bubble to its pinned button or else below its
// location bar. The app says which of its views play those parts, per
// Browser, by the anchor's name; Chromium's bubbles then open below them, and
// follow them when the window, the view or one of its ancestors moves. The
// view must be in the window the Browser is attached to and not hidden;
// otherwise, and before the app names one, bubbles open where the anchor's
// fallback is.
//
// The names, what hangs off each, and the fallback:
//
//   "location"          The site icon at the start of the app's address
//                       field: permission prompts, page info, device
//                       choosers, safety tips. Fallback: the tab area's top
//                       left.
//   "address-field"     The app's whole address field, its icons included.
//                       Bubbles hang below its right end, as Chrome's do below
//                       its location bar: the content settings', the bookmark
//                       star's, opening the page in an installed app, and
//                       every feature below while the app has no button for
//                       it. Fallback: the tab area's top right.
//   "extensions"        The app's extensions button (Chrome's puzzle piece):
//                       the extensions menu, and popups and dialogs of actions
//                       with no button of their own. Fallback: the tab area's
//                       top right.
//   "extension:<id>"    The app's button for the action of the extension
//                       <id>: its popup and dialogs. Fallback: "extensions".
//   "passwords"         Saving and managing passwords. Fallback:
//                       "address-field", as for each feature below.
//   "addresses"         Saving addresses.
//   "payments"          Saving cards and IBANs, virtual cards.
//   "translate"         Translate (needs a Google API key).
//   "qr-code"           The page's QR code.
//   "send-tab-to-self"  Sending the page to another device.
//   "zoom"              Zoom.
//   "cast"              Cast's dialog, a page's own Cast device picker and the
//                       prompt to cast a playing video.
//   "downloads"         The app's download button: the download bubble.
//   "toast"             Chromium's toasts ("Link copied"), centred on the
//                       anchor's bottom edge, half below it, as Chrome's sit
//                       astride its page's top edge. Fallback: the tab area's
//                       top edge.
//
// An unknown name is WREN_ERROR_INVALID_ARGUMENT.

// Names `ns_view` (an NSView, held weakly) as the Browser's `anchor`; NULL
// forgets it. An anchor is one thing at a time: a view takes the place of an
// element named with Wren_SetAnchorElement, and the other way round.
WREN_API WrenResult Wren_SetAnchor(WrenBrowser* browser,
                                   const char* anchor,
                                   void* ns_view);

// Names an element of a web view's page as the Browser's `anchor`, for a
// browser whose UI is a page: the element the page marks `name` (see
// "Anchors in a web view" in WrenWebView.h). A NULL or empty name forgets
// the anchor. Bubbles hang off the element while the page has one of that
// name, shown, and the web view is in the window the Browser is attached
// to; otherwise they open where they do with no anchor. The naming lasts
// across the page's reloads, until the web view is destroyed or the
// Browser closes. WREN_ERROR_NOT_ALLOWED for a view made without
// `anchors`; WREN_ERROR_CLOSED for one whose page has ended.
WREN_API WrenResult Wren_SetAnchorElement(WrenBrowser* browser,
                                          const char* anchor,
                                          WrenWebView* view,
                                          const char* name);

typedef struct WrenAnchorState {
  // The app has named a view or an element for it, and has not forgotten it
  // or released the view.
  bool named;
  // That view or element is in the window the Browser is attached to, shown
  // and not empty: bubbles hang off it, not off the fallback.
  bool placed;
  // A bubble of Chromium's hangs off it now (draw the button pressed): its
  // own, or the bubble of an anchor that is not placed and falls back to it
  // (an extension's popup at "extensions" while its action has no button).
  // At the fallback too, while the anchor is not placed.
  bool showing;
} WrenAnchorState;

// The anchor's state; all false once the Browser has closed.
// WREN_CHANGE_ANCHORS reports its changes.
WREN_API WrenResult Wren_GetAnchorState(WrenBrowser* browser,
                                        const char* anchor,
                                        WrenAnchorState* state);

// Opens Chromium's page info (the connection, the site's permissions and
// data) for the tab, below the Browser's "location" anchor, as Chrome's
// site icon does. The tab must be its Browser's active tab
// (WREN_ERROR_NOT_ALLOWED); WREN_ERROR_NAVIGATION_UNAVAILABLE before it has
// shown a page. Asks on_popover first (WREN_POPOVER_PAGE_INFO).
WREN_API WrenResult Wren_ShowPageInfo(WrenTabID tab);

// --- Page actions -----------------------------------------------------------
//
// The icons Chrome draws at the end of its address field, for the app's
// address field: the key once a password was offered or saved, the zoom
// level while the page is zoomed, the bookmark star; and Chrome's Cast
// button. They are the Browser's active tab's. A normal or popup Browser has
// them; an app Browser has none (WREN_ERROR_NOT_ALLOWED). Their bubbles open
// at their anchors (above), else at "address-field".

// The page actions, in the order Chrome draws them from left to right.
typedef enum WrenPageAction {
  // Translate the page (needs a Google API key Chromium is configured with).
  WREN_PAGE_ACTION_TRANSLATE = 0,
  // Open the page in an installed app that handles it.
  WREN_PAGE_ACTION_INTENT_PICKER = 1,
  // The page is zoomed; the tooltip says how far.
  WREN_PAGE_ACTION_ZOOM = 2,
  // Passwords for the page: saving one, or the saved ones.
  WREN_PAGE_ACTION_PASSWORDS = 3,
  // The bookmark star; its icon is filled once the page is bookmarked.
  WREN_PAGE_ACTION_BOOKMARK = 4,
  // Chrome's Cast button, in its toolbar rather than its address field.
  // Visible while Chrome shows the button without the user pinning it:
  // while casting, while its dialog is open, by policy, or with "Always
  // show icon" chosen. Its icon shows whether the Browser's Profile is
  // casting. An app may keep a Cast button of its own at all times, as a
  // user may pin Chrome's: it may be clicked while not visible.
  WREN_PAGE_ACTION_CAST = 5,
  // Find in page, drawn before the star: visible while a find is open in the
  // tab, whichever bar shows it, and pressed; a click opens a find, as Find…
  // does (WREN_POPOVER_FIND). It has no anchor of its own: Chromium's bar
  // hangs below "address-field", as Chrome's below its location bar.
  WREN_PAGE_ACTION_FIND = 6,
} WrenPageAction;

typedef struct WrenPageActionState {
  // Chrome shows the icon now.
  bool visible;
  // Chrome shows it with its label beside it (Wren_CopyPageActionLabel).
  bool chip;
  // Its bubble is open, or what it shows is (the find bar): draw the icon
  // pressed, as Chrome highlights it.
  bool bubble_open;
} WrenPageActionState;

// The action's state; all false while the Browser has no active page.
// WREN_ERROR_INVALID_ARGUMENT for an unknown action.
WREN_API WrenResult Wren_GetPageActionState(WrenBrowser* browser,
                                            WrenPageAction action,
                                            WrenPageActionState* state);
// The label Chrome shows beside the icon as a chip, and its tooltip, or "".
// UTF-8; each returns the length needed, including the NUL.
WREN_API size_t Wren_CopyPageActionLabel(WrenBrowser* browser,
                                         WrenPageAction action,
                                         char* buffer,
                                         size_t size);
WREN_API size_t Wren_CopyPageActionTooltip(WrenBrowser* browser,
                                           WrenPageAction action,
                                           char* buffer,
                                           size_t size);
// The icon as PNG, `points` square (1 to 1024) at `scale` (up to 4), as
// Chrome draws it now (the star filled once bookmarked), in Chromium's icon
// colour for the window's appearance. Returns the PNG's size, copying it
// only if `size` is enough; 0 if there is no icon.
WREN_API size_t Wren_CopyPageActionIconPNG(WrenBrowser* browser,
                                           WrenPageAction action,
                                           int points,
                                           double scale,
                                           uint8_t* buffer,
                                           size_t size);
// As a click on Chrome's icon: passwords opens or closes its bubble (or
// Chromium's password manager when nothing is saved for the page), zoom
// opens its bubble, the star bookmarks the page and opens its bubble (as
// Chrome's Bookmark This Tab, Cmd-D, does), translate opens its bubble, the
// intent picker offers the apps, Cast opens or closes its dialog, find opens
// a find. A click
// that began while the action's bubble was open leaves it closed.
// WREN_ERROR_NOT_ALLOWED if the icon is not shown (Cast: if the Browser
// cannot cast, as Chrome disables its Cast… item). Each bubble but the
// intent picker's and Cast's asks on_popover first.
WREN_API WrenResult Wren_ClickPageAction(WrenBrowser* browser,
                                         WrenPageAction action);

// --- Content setting icons --------------------------------------------------
//
// The icons Chrome shows in its address field when the page was blocked from
// something or is using it: a blocked popup, location in use, the camera on.
// They are the Browser's active tab's, in Chrome's order; an app Browser has
// none. Their bubbles (what was blocked, allow it, manage) are Chromium's, at
// "address-field".

// The icons shown now; 0 once the Browser has closed. What an icon's bubble
// shows is readable as data (Wren_GetContentSettingContent, WrenUI.h).
WREN_API size_t Wren_GetContentSettingIconCount(WrenBrowser* browser);
// The icon at `index`: what it is about, and whether the page was blocked
// (else it is using it). WREN_ERROR_INVALID_ARGUMENT past the end.
WREN_API WrenResult Wren_GetContentSettingIcon(WrenBrowser* browser,
                                               size_t index,
                                               WrenContentSetting* setting,
                                               bool* blocked);
// Whether the icon's bubble is open, Chromium's or the app's own
// (WREN_POPOVER_CONTENT_SETTING): Chrome draws the icon highlighted
// meanwhile. Its opening and closing are WREN_CHANGE_CONTENT_SETTINGS.
WREN_API bool Wren_IsContentSettingBubbleOpen(WrenBrowser* browser,
                                              WrenContentSetting setting);
// The icon's tooltip, or "" if it is not shown. UTF-8; returns the length
// needed, including the NUL.
WREN_API size_t Wren_CopyContentSettingTooltip(WrenBrowser* browser,
                                               WrenContentSetting setting,
                                               char* buffer,
                                               size_t size);
// The icon as PNG, `points` square (1 to 1024) at `scale` (up to 4), in
// Chromium's icon colour for the window's appearance. Returns the PNG's
// size, copying it only if `size` is enough; 0 if it is not shown.
WREN_API size_t Wren_CopyContentSettingIconPNG(WrenBrowser* browser,
                                               WrenContentSetting setting,
                                               int points,
                                               double scale,
                                               uint8_t* buffer,
                                               size_t size);
// Chromium's bubble for the icon, as a click on Chrome's icon opens it. A
// click that began while it was open leaves it closed.
// WREN_ERROR_NOT_ALLOWED if the icon is not shown. Asks on_popover first
// (WREN_POPOVER_CONTENT_SETTING).
WREN_API WrenResult Wren_ShowContentSettingBubble(WrenBrowser* browser,
                                                  WrenContentSetting setting);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_TOOLBAR_H_

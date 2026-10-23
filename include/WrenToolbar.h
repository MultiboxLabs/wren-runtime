// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the toolbar (C API). Wren.h includes this header. Wren.h also explains
// threads, handles and strings.
//
// This header is about the app's toolbar. It has three parts. Anchors are the
// views Chromium's bubbles hang off. Page actions and content setting icons
// are the icons Chrome draws in and beside its address field.

#ifndef WREN_PUBLIC_WREN_TOOLBAR_H_
#define WREN_PUBLIC_WREN_TOOLBAR_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Anchors ----------------------------------------------------------------
//
// Chrome hangs its bubbles off its toolbar. Permission prompts and page info
// hang off the site icon in the address field. An extension's popup hangs off
// its button. A feature's bubble hangs off its pinned button, or else below
// the address field.
//
// A Wren app tells Chromium which of its views play those parts, per Browser,
// by naming each one as an anchor. Chromium's bubbles then open below the
// anchor's view. They follow it when the window, the view or one of the
// view's ancestors moves.
//
// The view must be in the window the Browser is attached to, and not hidden.
// Until the app names a view, and while the view is not shown, bubbles open at
// the anchor's fallback instead.
//
// The names, what hangs off each, and the fallback:
//
//   "location"          The site icon at the start of the app's address
//                       field. Permission prompts, page info, device choosers
//                       and safety tips hang off it. Fallback: the top left of
//                       the tab area.
//   "address-field"     The app's whole address field, its icons included.
//                       Bubbles hang below its right end, as Chrome's do below
//                       its address field. These are the bubbles of content
//                       settings, of the bookmark star, of opening the page in
//                       an installed app, and of every feature below while the
//                       app has no button for it. Fallback: the top right of
//                       the tab area.
//   "extensions"        The app's extensions button, Chrome's puzzle piece.
//                       The extensions menu hangs off it, and so do the popups
//                       and dialogs of actions that have no button of their
//                       own. Fallback: the top right of the tab area.
//   "extension:<id>"    The app's button for the action of the extension
//                       <id>. Its popup and its dialogs hang off it. Fallback:
//                       "extensions".
//   "passwords"         Saving and managing passwords. Fallback:
//                       "address-field". Each feature below has the same
//                       fallback.
//   "addresses"         Saving addresses.
//   "payments"          Saving cards and IBANs, and virtual cards.
//   "translate"         Translate. It needs a Google API key.
//   "qr-code"           The page's QR code.
//   "send-tab-to-self"  Sending the page to another device.
//   "zoom"              Zoom.
//   "cast"              Cast's dialog, a page's own Cast device picker, and
//                       the prompt to cast a playing video.
//   "downloads"         The app's download button. The download bubble hangs
//                       off it.
//   "toast"             Chromium's toasts, such as "Link copied". A toast sits
//                       centred on the anchor's bottom edge, half below it, as
//                       Chrome's sit across the top edge of its page.
//                       Fallback: the top edge of the tab area.
//
// An unknown name gives WREN_ERROR_INVALID_ARGUMENT.

// Names `ns_view` as the Browser's `anchor`. `ns_view` is an NSView, and Wren
// holds it weakly. NULL forgets the anchor.
//
// An anchor is one thing at a time. A view replaces an element named with
// Wren_SetAnchorElement, and an element replaces a view.
WREN_API WrenResult Wren_SetAnchor(WrenBrowser* browser,
                                   const char* anchor,
                                   void* ns_view);

// Names an element of a web view's page as the Browser's `anchor`. This is for
// a browser whose UI is a page. The element is the one the page marks `name`
// (see "Anchors in a web view" in WrenWebView.h). A NULL or empty name forgets
// the anchor.
//
// Bubbles hang off the element while three things hold: the page has an
// element of that name, the element is shown, and the web view is in the
// window the Browser is attached to. Otherwise bubbles open where they would
// with no anchor. The naming lasts across reloads of the page, until the web
// view is destroyed or the Browser closes.
//
// Returns WREN_ERROR_NOT_ALLOWED for a view made without `anchors`, and
// WREN_ERROR_CLOSED for a view whose page has ended.
WREN_API WrenResult Wren_SetAnchorElement(WrenBrowser* browser,
                                          const char* anchor,
                                          WrenWebView* view,
                                          const char* name);

typedef struct WrenAnchorState {
  // True if the app has named a view or an element for the anchor, and has
  // not forgotten it or released the view.
  bool named;
  // True if that view or element is in the window the Browser is attached
  // to, shown and not empty. Bubbles then hang off it, not off the fallback.
  bool placed;
  // True while one of Chromium's bubbles hangs off the anchor. Draw its
  // button pressed meanwhile.
  //
  // The bubble may be the anchor's own, or that of another anchor which is
  // not placed and falls back to this one. For example, an extension's popup
  // hangs off "extensions" while its action has no button. While this anchor
  // is not placed, the field is also true while a bubble hangs off its
  // fallback.
  bool showing;
} WrenAnchorState;

// Reads the anchor's state. Every field is false once the Browser has closed.
// WREN_CHANGE_ANCHORS reports each change.
WREN_API WrenResult Wren_GetAnchorState(WrenBrowser* browser,
                                        const char* anchor,
                                        WrenAnchorState* state);

// Opens Chromium's page info for the tab, below the Browser's "location"
// anchor, as Chrome's site icon does. Page info shows the connection, and the
// site's permissions and data.
//
// The tab must be its Browser's active tab. If not, the call returns
// WREN_ERROR_NOT_ALLOWED. Before the tab has shown a page it returns
// WREN_ERROR_NAVIGATION_UNAVAILABLE. Wren asks on_popover first, as
// WREN_POPOVER_PAGE_INFO.
WREN_API WrenResult Wren_ShowPageInfo(WrenTabID tab);

// --- Page actions -----------------------------------------------------------
//
// Page actions are the icons Chrome draws at the end of its address field. A
// Wren app draws them in its own address field. They include the key once a
// password was offered or saved, the zoom level while the page is zoomed, and
// the bookmark star. Chrome's Cast button is one too.
//
// Page actions belong to the Browser's active tab. A normal or popup Browser
// has them. An app Browser has none, and calls on it return
// WREN_ERROR_NOT_ALLOWED. Their bubbles open at their anchors, listed above,
// or else at "address-field".

// The page actions, in the order Chrome draws them from left to right.
typedef enum WrenPageAction {
  // Translate the page. It needs Chromium configured with a Google API key.
  WREN_PAGE_ACTION_TRANSLATE = 0,
  // Open the page in an installed app that handles it.
  WREN_PAGE_ACTION_INTENT_PICKER = 1,
  // The page is zoomed. The tooltip says by how much.
  WREN_PAGE_ACTION_ZOOM = 2,
  // Passwords for the page: saving one, or the ones saved.
  WREN_PAGE_ACTION_PASSWORDS = 3,
  // The bookmark star. Its icon is filled once the page is bookmarked.
  WREN_PAGE_ACTION_BOOKMARK = 4,
  // Chrome's Cast button, which sits in its toolbar, not its address field.
  //
  // It is visible while Chrome would show the button without the user
  // pinning it. That is while casting, while its dialog is open, by policy,
  // or when the user chose "Always show icon". Its icon shows whether the
  // Browser's Profile is casting.
  //
  // An app may keep a Cast button of its own at all times, as a user may pin
  // Chrome's. The app may click it while it is not visible.
  WREN_PAGE_ACTION_CAST = 5,
  // Find in page, drawn before the star.
  //
  // It is visible, and pressed, while a find is open in the tab, whichever
  // bar shows it. A click opens a find, as Find… does (WREN_POPOVER_FIND). It
  // has no anchor of its own. Chromium's find bar hangs below
  // "address-field", as Chrome's hangs below its address field.
  WREN_PAGE_ACTION_FIND = 6,
} WrenPageAction;

typedef struct WrenPageActionState {
  // True if Chrome shows the icon now.
  bool visible;
  // True if Chrome shows the icon with its label beside it, as a chip. See
  // Wren_CopyPageActionLabel.
  bool chip;
  // True if the icon's bubble is open, or for find, the find bar. Draw the
  // icon pressed meanwhile, as Chrome highlights it.
  bool bubble_open;
} WrenPageActionState;

// Reads the action's state. Every field is false while the Browser has no
// active page. Returns WREN_ERROR_INVALID_ARGUMENT for an unknown action.
WREN_API WrenResult Wren_GetPageActionState(WrenBrowser* browser,
                                            WrenPageAction action,
                                            WrenPageActionState* state);
// The next two calls copy the label Chrome shows beside the icon as a chip,
// and the icon's tooltip, or "". Each string is UTF-8, and each call returns
// the length needed, including the NUL.
WREN_API size_t Wren_CopyPageActionLabel(WrenBrowser* browser,
                                         WrenPageAction action,
                                         char* buffer,
                                         size_t size);
WREN_API size_t Wren_CopyPageActionTooltip(WrenBrowser* browser,
                                           WrenPageAction action,
                                           char* buffer,
                                           size_t size);
// Copies the icon as a PNG, `points` square at `scale`. `points` is 1 to 1024
// and `scale` is at most 4. The icon is drawn as Chrome draws it now, such as
// the star filled once the page is bookmarked, in Chromium's icon colour for
// the window's appearance.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 if there is no icon.
WREN_API size_t Wren_CopyPageActionIconPNG(WrenBrowser* browser,
                                           WrenPageAction action,
                                           int points,
                                           double scale,
                                           uint8_t* buffer,
                                           size_t size);
// Does what a click on Chrome's icon does:
//
// - Passwords opens or closes its bubble. With nothing saved for the page, it
//   opens Chromium's password manager instead.
// - Zoom opens its bubble.
// - The star bookmarks the page and opens its bubble, as Chrome's Bookmark
//   This Tab (Cmd-D) does.
// - Translate opens its bubble.
// - The intent picker offers the apps that handle the page.
// - Cast opens or closes its dialog.
// - Find opens a find.
//
// A click that began while the action's bubble was open leaves the bubble
// closed. Every bubble except the intent picker's and Cast's asks on_popover
// first.
//
// Returns WREN_ERROR_NOT_ALLOWED if the icon is not shown. For Cast it returns
// WREN_ERROR_NOT_ALLOWED if the Browser cannot cast, when Chrome disables its
// Cast… item.
WREN_API WrenResult Wren_ClickPageAction(WrenBrowser* browser,
                                         WrenPageAction action);

// --- Content setting icons --------------------------------------------------
//
// Content setting icons are the icons Chrome shows in its address field when
// the page was blocked from something, or is using it. Examples are a blocked
// popup, location in use, and the camera on.
//
// They belong to the Browser's active tab, and come in Chrome's order. An app
// Browser has none. Their bubbles are Chromium's, at "address-field". A bubble
// says what was blocked, and offers to allow it or to manage the setting.

// The number of icons shown now. 0 once the Browser has closed. The app can
// read what an icon's bubble shows as data, with
// Wren_GetContentSettingContent (WrenUI.h).
WREN_API size_t Wren_GetContentSettingIconCount(WrenBrowser* browser);
// Reads the icon at `index`: which setting it is about, and whether the page
// was blocked. If `blocked` is false, the page is using it. Returns
// WREN_ERROR_INVALID_ARGUMENT past the end.
WREN_API WrenResult Wren_GetContentSettingIcon(WrenBrowser* browser,
                                               size_t index,
                                               WrenContentSetting* setting,
                                               bool* blocked);
// Returns true while the icon's bubble is open, whether Chromium's or the
// app's own (WREN_POPOVER_CONTENT_SETTING). Chrome draws the icon highlighted
// meanwhile. WREN_CHANGE_CONTENT_SETTINGS reports the bubble opening and
// closing.
WREN_API bool Wren_IsContentSettingBubbleOpen(WrenBrowser* browser,
                                              WrenContentSetting setting);
// Copies the icon's tooltip, or "" if the icon is not shown. The tooltip is
// UTF-8. Returns the length needed, including the NUL.
WREN_API size_t Wren_CopyContentSettingTooltip(WrenBrowser* browser,
                                               WrenContentSetting setting,
                                               char* buffer,
                                               size_t size);
// Copies the icon as a PNG, `points` square at `scale`. `points` is 1 to 1024
// and `scale` is at most 4. The icon is in Chromium's icon colour for the
// window's appearance.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 if the icon is not shown.
WREN_API size_t Wren_CopyContentSettingIconPNG(WrenBrowser* browser,
                                               WrenContentSetting setting,
                                               int points,
                                               double scale,
                                               uint8_t* buffer,
                                               size_t size);
// Opens Chromium's bubble for the icon, as a click on Chrome's icon does. A
// click that began while the bubble was open leaves it closed. Wren asks
// on_popover first, as WREN_POPOVER_CONTENT_SETTING.
//
// Returns WREN_ERROR_NOT_ALLOWED if the icon is not shown.
WREN_API WrenResult Wren_ShowContentSettingBubble(WrenBrowser* browser,
                                                  WrenContentSetting setting);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_TOOLBAR_H_

// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's popovers as requests and data (C API). Wren.h includes
// this header. Wren.h also explains threads, handles and strings.
//
// This is the part Wren keeps the same from release to release: the
// requests, and the facts and actions behind each popover. Chrome's sentences
// and the layout of its bubbles are in WrenUnstableUI.h, which may change in
// any release. The Compatibility guide has the details.
//
// Chromium opens popovers next to its toolbar. They include page info, the
// extensions menu, zoom, the bookmark star, a content setting's bubble,
// translate, its prompts to save a password, an address or a card, and
// permission prompts, such as a page asking for location, the camera or
// notifications.
//
// Before Chromium opens a popover in a Browser the app knows, Wren asks the
// app's on_popover. It asks whatever started the popover:
//
// - the app's own call, such as Wren_ShowPageInfo or Wren_ClickPageAction
// - a Chromium command, such as Cmd-D or Cmd-+
// - Chromium on its own, such as a save prompt after a sign-in, or an
//   extension's zoom
//
// The app answers each request: Chromium's own popover, the app's, or none.
// So an app can draw its own zoom popover and leave the rest to Chromium, or
// the other way round, or decide from the request.
//
// Dialogs that stand on their own stay Chromium's, and Wren never asks about
// them. They are device choosers, passkeys, HTTP authentication, page
// dialogs, extension installs, the dialogs a popover opens, and the prompts
// of a page's <permission> elements.
//
// on_popover runs inside Chromium's call, as on_fullscreen does (Wren.h).
// Every other callback runs from the event loop. on_popover may read
// anything. But until it returns, Wren refuses calls that change Profiles,
// Browsers, tabs, navigation, anchors, extension actions, popovers or full
// screen. They return WREN_ERROR_NOT_ALLOWED, or report it to their callback.
// Wren_AttachBrowserToView does nothing. Do that work afterwards, for example
// from a dispatched block. Wren_Quit, which takes effect later, is allowed, and
// so are retaining and releasing handles.
//
// A request the app answers with WREN_POPOVER_APP stays open until the app
// closes it with Wren_ClosePopover, or Wren withdraws it. WREN_CHANGE_POPOVER
// reports either. Wren withdraws a request when Chrome would close its
// popover:
//
// - The tab is hidden or switched away from, closes, or leaves the Browser.
// - For page info and a content setting's bubble, which are about one page,
//   the page changes. Chrome's other popovers stay across navigations.
// - Chromium's model closes it, such as a prompt that is no longer needed.
// - The Browser opens another popover of the same kind, Chromium's or the
//   app's. There is one at a time, as in Chrome.
//
// While the app's popover is open, Chromium's bookkeeping says a popover is
// showing. The zoom icon stays shown, and the page action reports
// bubble_open.
//
// Wren_ClickPageAction and the other calls that open a popover have a rule: a
// click that began while the popover was open leaves it closed. That rule is
// for Chromium's popovers only. The app opens and closes its own.

#ifndef WREN_PUBLIC_WREN_UI_H_
#define WREN_PUBLIC_WREN_UI_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Popover requests -------------------------------------------------------

typedef enum WrenPopoverKind {
  // The connection, and the site's permissions and data (Wren_ShowPageInfo).
  WREN_POPOVER_PAGE_INFO = 0,
  // Every extension, with pinning and site access (Wren_ShowExtensionsMenu).
  WREN_POPOVER_EXTENSIONS_MENU = 1,
  // The zoom level, after Cmd-+, Cmd-- or Cmd-0, an extension's zoom, or the
  // zoom page action.
  //
  // Chrome closes its own zoom popover by itself. It closes 1.5 seconds after
  // opening on its own, or 5 seconds after a button in it was used, but not
  // while the pointer is over it. Do the same.
  WREN_POPOVER_ZOOM = 2,
  // The page was bookmarked, by Cmd-D or the star. The popover has its title,
  // its folder, and remove. See newly_bookmarked.
  WREN_POPOVER_BOOKMARK = 3,
  // A content setting icon's bubble (Wren_ShowContentSettingBubble). See
  // content_setting.
  WREN_POPOVER_CONTENT_SETTING = 4,
  // Translating the page.
  WREN_POPOVER_TRANSLATE = 5,
  // Saving or updating a password, the passwords saved for the site, and
  // automatic sign-in. Chromium's other password prompts stay Chromium's,
  // and Wren does not ask. Those are generated passwords, passkeys, and
  // moving a password to the account.
  WREN_POPOVER_PASSWORDS = 6,
  // Saving or updating an address.
  WREN_POPOVER_SAVE_ADDRESS = 7,
  // Saving a card on this device. Saving to Google Payments stays Chromium's,
  // and Wren does not ask.
  WREN_POPOVER_SAVE_CARD = 8,
  // A page asks for permissions, such as location, the camera or
  // notifications. Always WREN_POPOVER_AUTOMATIC. Most apps leave these to
  // Chromium.
  //
  // Switching tabs closes the request. A new request comes when the tab is
  // back, as Chrome hides its prompt and shows it again.
  WREN_POPOVER_PERMISSION = 9,
  // One of Chromium's toasts, such as "Link copied" or "Added to reading
  // list". Always WREN_POPOVER_AUTOMATIC. See toast.
  //
  // Chromium decides when the toast goes, as for its own. That is after
  // about 4 seconds, or 8 with a button or a menu. A tab's toast also goes
  // when the tab switches or navigates, but a global toast does not. Any
  // toast goes when another toast comes. The request then closes, as for any
  // popover. Show the toast without taking focus.
  WREN_POPOVER_TOAST = 10,
  // The find bar, when a find opens in the active tab.
  //
  // A find opens from Find… (Cmd-F) or Find Next, from Wren_Find, or by
  // itself when a tab with a find becomes active again. Chrome hides its bar
  // for a tab with no find. A second Find while the bar is open asks again,
  // and the app's field takes focus again, as Chrome's does.
  //
  // Answered with the app's own bar, the request stays open until the find
  // ends or its tab is switched away from. Wren_ClosePopover ends the find as
  // Escape does. See the find state. Answered with none, the find ends.
  WREN_POPOVER_FIND = 11,
  // Chrome's download bubble. It comes in three forms:
  //
  // - Its short list, WREN_POPOVER_AUTOMATIC, when all of the Profile's
  //   downloads are done, in the Profile's last active Browser. Chrome closes
  //   it 5 seconds after it opened, unless it is used. Do the same, and show
  //   it without taking focus.
  // - The full list, from Wren_ClickDownloadButton.
  // - One download's warning (`download`), from chrome://downloads.
  //
  // It is not about one tab, so it stays across tab switches. See
  // WrenDownloads.h.
  WREN_POPOVER_DOWNLOADS = 12,
} WrenPopoverKind;

typedef enum WrenPopoverTrigger {
  // The app's own call: Wren_ShowPageInfo, Wren_ShowExtensionsMenu,
  // Wren_ClickPageAction or Wren_ShowContentSettingBubble.
  WREN_POPOVER_BY_APP = 0,
  // A Chromium command or shortcut the user chose, such as Cmd-D or Cmd-+.
  WREN_POPOVER_BY_USER = 1,
  // Chromium on its own, such as a save prompt after a sign-in or a form, an
  // extension's zoom, or translate offering itself. Show it without taking
  // focus, as Chrome does.
  WREN_POPOVER_AUTOMATIC = 2,
} WrenPopoverTrigger;

// What Chromium is about to open. Valid only during on_popover.
struct WrenPopoverRequest {
  // The request's ID. If the app answers WREN_POPOVER_APP, Wren_ClosePopover
  // and WREN_CHANGE_POPOVER use it.
  WrenPopoverID popover;
  WrenPopoverKind kind;
  WrenBrowser* browser;  // borrowed
  // The tab the popover is about, which is the Browser's active tab.
  WrenTabID tab;
  WrenPopoverTrigger trigger;
  // For CONTENT_SETTING, the icon's setting.
  WrenContentSetting content_setting;
  // For BOOKMARK, true if the page was bookmarked just now, not before.
  bool newly_bookmarked;
  // For TOAST, Chromium's name for the toast, such as "LinkCopied" or
  // "AddedToReadingList". That is the toast's ID in Chromium without its
  // "k". NULL for any other kind.
  const char* toast;
  // For DOWNLOADS, the download whose warning to show, or WREN_NO_DOWNLOAD
  // for the list.
  WrenDownloadID download;
};

// Returns the open request of this kind in the Browser that the app answered
// with WREN_POPOVER_APP, or WREN_NO_POPOVER.
WREN_API WrenPopoverID Wren_GetOpenPopover(WrenBrowser* browser,
                                           WrenPopoverKind kind);

// Tells Wren the app's popover closed, such as on Escape, a click elsewhere,
// or Done. Wren tells Chromium what its own popover would tell it on closing.
// A prompt counts as dismissed without a decision, and its page action can
// open it again.
//
// WREN_CHANGE_POPOVER follows. Returns WREN_ERROR_POPOVER_CLOSED if the
// request has already closed.
WREN_API WrenResult Wren_ClosePopover(WrenPopoverID popover);

// --- State ------------------------------------------------------------------
//
// What these popovers show can be read at any time, with no popover open,
// for a Browser's active tab. An example is the site's security and
// permissions, the site state.
//
// The app can show the state as labels and buttons that are always visible,
// combine it, or draw its own popover from it when it answers a request. A
// normal or popup Browser has state. An app Browser has none, and calls on it
// return WREN_ERROR_NOT_ALLOWED.
//
// Wren builds Chromium's model on the first read, and keeps it for the active
// tab's page. Changes reach the app once it has read the state. Chrome's text
// for the state is in WrenUnstableUI.h. The app draws its own icons.

// A page a tab shows, as read with the state. The token changes with each
// navigation to a new document and each tab switch.
//
// Calls that act on a page take the token back, and return
// WREN_ERROR_STALE_PAGE if the Browser's active page is no longer that one.
// So a click that lands as the page changes never acts on the next page.
typedef uint64_t WrenPageToken;

// --- Site state (page info) -------------------------------------------------

typedef enum WrenSiteSecurity {
  // A secure connection.
  WREN_SITE_SECURE = 0,
  // Not secure: plain http, or https with errors or insecure content.
  WREN_SITE_NOT_SECURE = 1,
  // Safe Browsing or a safety tip warns about the site.
  WREN_SITE_DANGEROUS = 2,
  // A browser page (chrome://), an extension's page, a file, or a page on a
  // scheme the app declared WREN_SCHEME_SECURE. For the app's own pages,
  // Chrome's text is "You're viewing a secure <the app's name> page".
  WREN_SITE_INTERNAL = 3,
  // The administrator's policy flags the site.
  WREN_SITE_ENTERPRISE = 4,
} WrenSiteSecurity;

typedef struct WrenSiteState {
  WrenPageToken page;
  WrenSiteSecurity security;
  // True if the connection has a certificate. WREN_SITE_CERTIFICATE shows it.
  bool has_certificate;
  // True if the user let a certificate error or an http warning through for
  // this site. WREN_SITE_REVOKE_DECISIONS takes that back.
  bool can_revoke_decisions;
  // True if a change made here applies only once the page reloads. Chrome
  // offers to reload as its page info closes. WREN_SITE_RELOAD reloads.
  bool reload_needed;
  // How many sites this one let use cookies. -1 until Chromium knows.
  int cookie_sites;
  // Whether Chrome shows its third-party cookie switch for the site, and
  // whether third-party cookies are blocked here. See
  // WREN_SITE_TOGGLE_THIRD_PARTY_COOKIES.
  bool third_party_cookies_shown;
  bool third_party_cookies_blocked;
} WrenSiteState;

// Reads the site state of the Browser's active tab. Returns
// WREN_ERROR_NAVIGATION_UNAVAILABLE before the tab has shown a page.
WREN_API WrenResult Wren_GetSiteState(WrenBrowser* browser,
                                      WrenSiteState* state);
// Copies the origin of the page the state is about, in UTF-8, such as
// "https://example.com:8443" or "chrome://settings". Copies "" for a page
// with no origin of its own, such as a file or a data: URL. Returns the length
// needed, including the NUL.
WREN_API size_t Wren_CopySiteOrigin(WrenBrowser* browser,
                                    char* buffer,
                                    size_t size);

typedef enum WrenPermissionSetting {
  // The site has no setting, so the default applies.
  WREN_PERMISSION_DEFAULT = 0,

  WREN_PERMISSION_ASK = 1,
  WREN_PERMISSION_ALLOW = 2,
  WREN_PERMISSION_BLOCK = 3,
} WrenPermissionSetting;

typedef struct WrenSitePermission {
  WrenPermissionSetting setting;
  // What WREN_PERMISSION_DEFAULT means for it now: ASK, ALLOW or BLOCK.
  WrenPermissionSetting default_setting;
  // True if policy or an extension set it. The user cannot change it.
  bool managed;
  // True if it is allowed this time only.
  bool one_time;
  // True if the page is using it now.
  bool in_use;
} WrenSitePermission;

// The permissions and settings Chrome's page info lists for the site, in its
// order. They are those set for the site, those in use, and those changed
// here.
WREN_API size_t Wren_GetSitePermissionCount(WrenBrowser* browser);
WREN_API WrenResult Wren_GetSitePermission(WrenBrowser* browser,
                                           size_t index,
                                           WrenSitePermission* permission);
// Copies Chromium's name for a listed permission, such as "geolocation",
// "notifications" or "popups". It is the name Settings URLs use, and the one
// Wren_SetSitePermission takes.
WREN_API size_t Wren_CopySitePermissionID(WrenBrowser* browser,
                                          size_t index,
                                          char* buffer,
                                          size_t size);
// Sets a permission or setting for the site, by Chromium's name, whether it
// is listed or not, as Chrome's page info does.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for a name page info does not offer, or
// a setting the permission cannot have. Returns WREN_ERROR_NOT_ALLOWED for a
// managed permission.
WREN_API WrenResult Wren_SetSitePermission(WrenBrowser* browser,
                                           WrenPageToken page,
                                           const char* permission,
                                           WrenPermissionSetting setting);

// The number of devices the site may use, in Chrome's order. Devices are USB,
// HID, serial and Bluetooth.
WREN_API size_t Wren_GetSiteDeviceCount(WrenBrowser* browser);

typedef enum WrenSiteDeviceKind {
  WREN_SITE_DEVICE_USB = 0,
  WREN_SITE_DEVICE_HID = 1,
  WREN_SITE_DEVICE_SERIAL = 2,
  WREN_SITE_DEVICE_BLUETOOTH = 3,
} WrenSiteDeviceKind;

typedef struct WrenSiteDevice {
  WrenSiteDeviceKind kind;
  // True if the administrator's policy allows it. It cannot be forgotten.
  bool managed;
} WrenSiteDevice;

WREN_API WrenResult Wren_GetSiteDevice(WrenBrowser* browser,
                                       size_t index,
                                       WrenSiteDevice* device);
// Copies the device's name, as the device or its system reports it.
WREN_API size_t Wren_CopySiteDeviceName(WrenBrowser* browser,
                                        size_t index,
                                        char* buffer,
                                        size_t size);
// Takes back the site's access to the device. Returns WREN_ERROR_NOT_ALLOWED
// for a managed device.
WREN_API WrenResult Wren_ForgetSiteDevice(WrenBrowser* browser,
                                          WrenPageToken page,
                                          size_t index);

typedef enum WrenSiteAction {
  // Opens Chromium's settings for the site, in a new tab.
  WREN_SITE_SETTINGS = 0,
  // Opens Chromium's certificate viewer, a dialog.
  WREN_SITE_CERTIFICATE = 1,
  // Opens Chromium's dialog of the site's cookies and data.
  WREN_SITE_DATA = 2,

  WREN_SITE_REVOKE_DECISIONS = 3,
  WREN_SITE_TOGGLE_THIRD_PARTY_COOKIES = 4,
  // Opens the help page about the site's security state, in a new tab.
  WREN_SITE_LEARN_MORE = 5,
  // Reloads the page, after reload_needed.
  WREN_SITE_RELOAD = 6,
} WrenSiteAction;

// Does one of page info's actions.
//
// Returns WREN_ERROR_NOT_ALLOWED when the state does not offer the action now,
// as has_certificate, can_revoke_decisions and third_party_cookies_shown say.
// Also returns it for a WREN_SITE_INTERNAL page, such as a browser page, the
// app's own, a file, DevTools or an extension's page. As in Chrome, such a
// page has only its summary and no permissions.
WREN_API WrenResult Wren_SiteAction(WrenBrowser* browser,
                                    WrenPageToken page,
                                    WrenSiteAction action);

// --- Extensions state (the extensions menu) ---------------------------------
//
// What Chromium's extensions menus show about the extensions on the active
// tab's site. Only a normal Browser has it. Calls on other Browsers return
// WREN_ERROR_NOT_ALLOWED.
//
// Chromium has two designs of the menu.
//
// - Its own menu is the first design. It groups the extensions by what they
//   may do on the site: full access, access requested, or no access needed.
//   The user changes each from its context menu
//   (Wren_ShowExtensionActionMenu).
// - The redesign adds a switch to block every extension on the site, a switch
//   per extension, and a choice per extension: on click, on this site, or on
//   all sites. It also shows extensions' requests for access, and a message
//   to reload.
//
// What each extension may do, and the actions on it, are here, whichever
// design the app draws. Each design's controls are in WrenUnstableUI.h.
// Extensions are named by their IDs, as extension actions are
// (Wren_CopyExtensionActionID).

typedef enum WrenExtensionsSiteSetting {
  // Each extension's own access applies. The redesign's switch is on.
  WREN_EXTENSIONS_CUSTOMIZE = 0,
  // No extension may run on the site.
  WREN_EXTENSIONS_BLOCK_ALL = 1,
} WrenExtensionsSiteSetting;

typedef struct WrenExtensionsSite {
  WrenPageToken page;
  WrenExtensionsSiteSetting setting;
  // True if the user can change the setting. It is false when the setting is
  // managed, or the site is restricted.
  bool can_change;
  // True if a change applies only once the page reloads. See
  // Wren_ReloadForExtensions.
  bool reload_needed;
} WrenExtensionsSite;

// Returns WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a
// page.
WREN_API WrenResult Wren_GetExtensionsSite(WrenBrowser* browser,
                                           WrenExtensionsSite* site);

// Sets whether extensions may run on the site.
WREN_API WrenResult
Wren_SetExtensionsSiteSetting(WrenBrowser* browser,
                              WrenPageToken page,
                              WrenExtensionsSiteSetting setting);

// What an extension may do on the site, as Chromium's first menu groups it.
typedef enum WrenSiteInteraction {
  // It needs no access to the site ("No access needed").
  WREN_SITE_INTERACTION_NONE = 0,
  // It wants access it does not have ("Access requested").
  WREN_SITE_INTERACTION_REQUESTED = 1,
  // It has access ("Full access").
  WREN_SITE_INTERACTION_FULL = 2,
} WrenSiteInteraction;

// When an extension may read and change the site, as the redesign offers.
typedef enum WrenSiteAccess {
  // Only after the user clicks it.
  WREN_SITE_ACCESS_ON_CLICK = 0,
  // Every time on this site.
  WREN_SITE_ACCESS_ON_SITE = 1,
  // On every site.
  WREN_SITE_ACCESS_ON_ALL_SITES = 2,
} WrenSiteAccess;

// What an extension may do on the active tab's site.
typedef struct WrenExtensionOnSite {
  WrenSiteInteraction interaction;
  // Whether its access to this site can be granted or taken back now
  // (Wren_SetExtensionSiteAllowed), and whether it is granted.
  bool can_allow;
  bool allowed;
  // Its access (Wren_SetExtensionSiteAccess), and which choices are possible
  // now. None is possible for an extension that asks for no site.
  WrenSiteAccess access;
  bool can_access_on_click;
  bool can_access_on_site;
  bool can_access_on_all_sites;
  // True if it may show its requests for access in the toolbar
  // (Wren_SetExtensionShowsRequests).
  bool shows_requests;
  // True if policy installed it.
  bool enterprise;
} WrenExtensionOnSite;

// Returns WREN_ERROR_INVALID_ARGUMENT for an extension with no action here.
WREN_API WrenResult Wren_GetExtensionOnSite(WrenBrowser* browser,
                                            const char* extension_id,
                                            WrenExtensionOnSite* extension);

// Sets the extension's access, as the redesign's choice does. Returns
// WREN_ERROR_NOT_ALLOWED when that access cannot be chosen.
WREN_API WrenResult Wren_SetExtensionSiteAccess(WrenBrowser* browser,
                                                WrenPageToken page,
                                                const char* extension_id,
                                                WrenSiteAccess access);
// Grants or takes back the extension's access to this site, as the
// redesign's switch does.
WREN_API WrenResult Wren_SetExtensionSiteAllowed(WrenBrowser* browser,
                                                 WrenPageToken page,
                                                 const char* extension_id,
                                                 bool allowed);
// Sets whether the extension may show its requests for access in the
// toolbar.
WREN_API WrenResult Wren_SetExtensionShowsRequests(WrenBrowser* browser,
                                                   const char* extension_id,
                                                   bool show);

// The extensions asking for access to the site, in Chrome's order: their
// number, and the ID of each.
WREN_API size_t Wren_GetExtensionRequestCount(WrenBrowser* browser);
WREN_API size_t Wren_CopyExtensionRequestID(WrenBrowser* browser,
                                            size_t index,
                                            char* buffer,
                                            size_t size);
// Answers an extension's request. With `allow`, the extension gets access to
// the site. Without it, the request is dismissed.
WREN_API WrenResult Wren_AnswerExtensionRequest(WrenBrowser* browser,
                                                WrenPageToken page,
                                                const char* extension_id,
                                                bool allow);
// Reloads the page, after WREN_EXTENSIONS_MESSAGE_RELOAD.
WREN_API WrenResult Wren_ReloadForExtensions(WrenBrowser* browser,
                                             WrenPageToken page);

// The extensions button, Chrome's puzzle piece, which changes with the site.
// WREN_CHANGE_EXTENSION_ACTIONS reports its changes.
typedef enum WrenExtensionsButton {
  WREN_EXTENSIONS_BUTTON_DEFAULT = 0,
  // At least one extension has access to the site.
  WREN_EXTENSIONS_BUTTON_ANY_HAS_ACCESS = 1,
  // Every extension is blocked on the site.
  WREN_EXTENSIONS_BUTTON_ALL_BLOCKED = 2,
} WrenExtensionsButton;

typedef struct WrenExtensionsButtonState {
  WrenExtensionsButton button;
  // How many extensions Chrome's request-access button, next to it, lists. 0
  // hides that button.
  size_t requesting_count;
} WrenExtensionsButtonState;

WREN_API WrenResult
Wren_GetExtensionsButtonState(WrenBrowser* browser,
                              WrenExtensionsButtonState* state);
// Copies the ID of an extension the request-access button lists.
WREN_API size_t Wren_CopyRequestingExtensionID(WrenBrowser* browser,
                                               size_t index,
                                               char* buffer,
                                               size_t size);
// Does what a click on Chrome's request-access button does. Every extension
// it lists gets access to the site.
WREN_API WrenResult Wren_GrantRequestedSiteAccess(WrenBrowser* browser,
                                                  WrenPageToken page);

// --- Zoom state -------------------------------------------------------------

typedef struct WrenZoomState {
  WrenPageToken page;
  // The zoom level, as Chrome shows it. 100 is the default size.
  int percent;

  bool at_default;
  // Whether Zoom In and Zoom Out can change it, as Chrome's zoom bubble
  // enables its buttons.
  bool can_zoom_in;
  bool can_zoom_out;
} WrenZoomState;

// Reads the zoom of the Browser's active tab. Returns
// WREN_ERROR_NAVIGATION_UNAVAILABLE with no tab.
WREN_API WrenResult Wren_GetZoomState(WrenBrowser* browser,
                                      WrenZoomState* state);
// Copies the ID of the extension that set the level last, through
// chrome.tabs.setZoom, which Chrome's zoom bubble names. Copies "" if the
// user set it.
WREN_API size_t Wren_CopyZoomExtensionID(WrenBrowser* browser,
                                         char* buffer,
                                         size_t size);

typedef enum WrenZoom {
  WREN_ZOOM_IN = 0,
  WREN_ZOOM_OUT = 1,
  WREN_ZOOM_RESET = 2,
} WrenZoom;

// Zooms as the buttons of Chrome's zoom bubble do, and as Cmd-+, Cmd-- and
// Cmd-0 do.
WREN_API WrenResult Wren_Zoom(WrenBrowser* browser,
                              WrenPageToken page,
                              WrenZoom zoom);

// --- Find state -------------------------------------------------------------
//
// The find in the Browser's active tab, whichever bar shows it, Chromium's or
// the app's. Chromium finds, and the app's bar does what Chrome's does.
//
// On macOS the find text is the same in every tab. It is the system's find
// pasteboard, which Chromium follows, for example after another app's Use
// Selection for Find. Typing in the bar writes it, except off the record.
// WREN_CHANGE_FIND reports each change.

typedef struct WrenFindState {
  WrenPageToken page;
  // True if a find is open in the tab, and its bar shows.
  bool open;
  // The number of matches in the page, and the current match, counting from
  // 1. The current match is 0 when there is none. Both are -1 before there is
  // a result for the text.
  int match_count;
  int active_match;
  // True while Chromium is still counting, so the numbers may change. Chrome
  // announces only a finished count.
  bool counting;
} WrenFindState;

// Reads the find of the Browser's active tab. Returns
// WREN_ERROR_NAVIGATION_UNAVAILABLE with no tab.
WREN_API WrenResult Wren_GetFindState(WrenBrowser* browser,
                                      WrenFindState* state);
// Copies the text the bar shows. "" before a find has opened.
WREN_API size_t Wren_CopyFindString(WrenBrowser* browser,
                                    char* buffer,
                                    size_t size);

typedef enum WrenFind {
  // Opens a find, as Chrome's Find… (Cmd-F) does. The bar shows the page's
  // selection, or the find pasteboard's text. Chromium counts and highlights
  // the matches without moving to one. WREN_POPOVER_FIND asks whose bar it
  // is.
  WREN_FIND_OPEN = 0,
  // Moves to the next or the previous match, wrapping at the ends. In
  // Chrome's bar these are Return and Shift-Return, and its arrows.
  WREN_FIND_NEXT = 1,
  WREN_FIND_PREVIOUS = 2,
  // Ends the find and leaves the current match selected, as Escape and the
  // close button do.
  WREN_FIND_END = 3,
  // Ends the find by clicking the current match, if it is a link, as
  // Control-Return does.
  WREN_FIND_END_ACTIVATE = 4,
} WrenFind;

// Opens, steps or ends a find.
//
// Returns WREN_ERROR_NOT_ALLOWED where Chrome disables Find, such as in a
// DevTools Browser or with a dialog over the page. Also returns it for
// stepping or ending with no find open, and inside on_popover.
WREN_API WrenResult Wren_Find(WrenBrowser* browser,
                              WrenPageToken page,
                              WrenFind find);
// Sets the text, as typing in the bar does. Chromium counts and highlights
// the matches, the first match after the current one becomes current, and
// Wren writes the find pasteboard. "" clears the matches. Returns
// WREN_ERROR_NOT_ALLOWED with no find open.
WREN_API WrenResult Wren_SetFindString(WrenBrowser* browser,
                                       WrenPageToken page,
                                       const char* string);

// --- Bookmark state ---------------------------------------------------------
//
// The bookmark of the active tab's page, as Chrome's bookmark bubble edits
// it. The star's click (Wren_ClickPageAction) bookmarks the page. Each call
// applies its change at once, not when a popover closes.
//
// Folders are named by Chromium's IDs for them, which stay the same while the
// folder exists. The list of folders that Chrome's bubble shows is in
// WrenUnstableUI.h. For all of a Profile's bookmarks, see WrenBookmarks.h.

typedef int64_t WrenBookmarkFolderID;

typedef struct WrenBookmark {
  WrenPageToken page;
  // True if the user has bookmarked the page.
  bool bookmarked;
  // The folder holding the bookmark. 0 when the page is not bookmarked.
  WrenBookmarkFolderID folder;
} WrenBookmark;

// Returns WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a
// page.
WREN_API WrenResult Wren_GetBookmark(WrenBrowser* browser,
                                     WrenBookmark* bookmark);
// Copies the bookmark's title. "" when the page is not bookmarked.
WREN_API size_t Wren_CopyBookmarkTitle(WrenBrowser* browser,
                                       char* buffer,
                                       size_t size);

// The number of folders Chrome suggests for the bookmark, in its order. They
// are the folders used most recently and the main ones, with the user's
// Google Account's first. 0 when the page is not bookmarked.
WREN_API size_t Wren_GetBookmarkFolderSuggestionCount(WrenBrowser* browser);
// The suggested folder at `index`. 0 past the end.
WREN_API WrenBookmarkFolderID
Wren_GetBookmarkFolderSuggestion(WrenBrowser* browser, size_t index);

typedef struct WrenBookmarkFolder {
  // The folder holding this one. 0 for a folder at the top.
  WrenBookmarkFolderID parent;
  // True if it is kept in the user's Google Account, not on this device.
  bool account;
  // True for one of Chromium's own folders, such as the bookmarks bar and
  // Other bookmarks. They cannot be renamed or removed.
  bool permanent;
} WrenBookmarkFolder;

// Reads any folder of the Browser's Profile. Returns
// WREN_ERROR_INVALID_ARGUMENT for an ID that is not a folder.
WREN_API WrenResult Wren_GetBookmarkFolder(WrenBrowser* browser,
                                           WrenBookmarkFolderID folder,
                                           WrenBookmarkFolder* info);
// Copies the folder's title.
WREN_API size_t Wren_CopyBookmarkFolderTitle(WrenBrowser* browser,
                                             WrenBookmarkFolderID folder,
                                             char* buffer,
                                             size_t size);

// Each call below needs the page to be bookmarked. If it is not, the call
// returns WREN_ERROR_NOT_ALLOWED.

// Moves the bookmark to the end of a folder, as Chrome's bubble does. Returns
// WREN_ERROR_INVALID_ARGUMENT for an ID that is not a folder, or a folder the
// user cannot add to, such as a managed folder.
WREN_API WrenResult Wren_MoveBookmark(WrenBrowser* browser,
                                      WrenPageToken page,
                                      WrenBookmarkFolderID folder);
// Renames the bookmark.
WREN_API WrenResult Wren_SetBookmarkTitle(WrenBrowser* browser,
                                          WrenPageToken page,
                                          const char* title);
// Removes the bookmark.
WREN_API WrenResult Wren_RemoveBookmark(WrenBrowser* browser,
                                        WrenPageToken page);
// Opens Chromium's bookmark editor, a dialog, as Chrome's Edit… button does.
WREN_API WrenResult Wren_EditBookmark(WrenBrowser* browser, WrenPageToken page);

// --- Content setting state --------------------------------------------------
//
// What the bubble of a content setting icon (WrenToolbar.h) is about: what
// was blocked or used, and the site's value for the setting.
//
// Wren builds Chromium's model on the first read of an icon, for the active
// tab's page. WREN_CHANGE_CONTENT_SETTINGS reports changes. Calls for an icon
// that is not shown return WREN_ERROR_NOT_ALLOWED. The layout of Chrome's own
// bubble is in WrenUnstableUI.h.

// A value the bubble offers for the site.
typedef enum WrenContentSettingValue {
  WREN_CONTENT_SETTING_VALUE_ALLOW = 0,
  WREN_CONTENT_SETTING_VALUE_BLOCK = 1,
  // For a page's protocol handler only. Neither allow nor block, and ask
  // again next time.
  WREN_CONTENT_SETTING_VALUE_IGNORE = 2,
} WrenContentSettingValue;

typedef struct WrenContentSettingState {
  WrenPageToken page;
  // The number of things blocked or used, such as each blocked popup. See
  // Wren_CopyContentSettingItemURL.
  size_t item_count;
  // Whether the bubble offers a value for the site, its value now, and which
  // values it offers (Wren_SetContentSettingValue).
  bool has_value;
  WrenContentSettingValue value;
  bool can_allow;
  bool can_block;
  bool can_ignore;
  // True if policy or an extension set it. The user cannot change it.
  bool managed;
} WrenContentSettingState;

WREN_API WrenResult Wren_GetContentSettingState(WrenBrowser* browser,
                                                WrenContentSetting setting,
                                                WrenContentSettingState* state);
// Copies an item's URL, for an item that is a page, such as a blocked popup or
// redirect. Copies "" for other items.
WREN_API size_t Wren_CopyContentSettingItemURL(WrenBrowser* browser,
                                               WrenContentSetting setting,
                                               size_t index,
                                               char* buffer,
                                               size_t size);
// Sets the site's value and applies it at once, as choosing it in Chrome's
// bubble and pressing Done does. Returns WREN_ERROR_NOT_ALLOWED for a managed
// setting, and WREN_ERROR_INVALID_ARGUMENT for a value not offered.
WREN_API WrenResult Wren_SetContentSettingValue(WrenBrowser* browser,
                                                WrenPageToken page,
                                                WrenContentSetting setting,
                                                WrenContentSettingValue value);
// Opens an item, as a click on it in Chrome's bubble does. For example, a
// blocked popup opens.
WREN_API WrenResult Wren_OpenContentSettingItem(WrenBrowser* browser,
                                                WrenPageToken page,
                                                WrenContentSetting setting,
                                                size_t index);

// --- Prompt data ------------------------------------------------------------
//
// A prompt the app answered with its own popover carries what Chrome's prompt
// shows and does. That is WREN_POPOVER_PASSWORDS, _SAVE_ADDRESS, _SAVE_CARD,
// _TRANSLATE and _PERMISSION.
//
// The data lasts until the prompt closes. Every call then returns
// WREN_ERROR_POPOVER_CLOSED, or copies "". A call for another kind of request
// returns WREN_ERROR_INVALID_ARGUMENT. The prompts' texts are in
// WrenUnstableUI.h.

// Passwords, as Chrome's prompts have them: saving or updating one, the ones
// saved for the site, and signing in automatically.
//
// Passwords themselves reach the app only when Chromium's rule allows it, as
// Chrome reveals them. Right after a sign-in, Chrome shows the password at
// once. When the user reopened the prompt, it asks for Touch ID or the Mac
// password first. The app can read them until the request closes.
typedef enum WrenPasswordsPrompt {
  WREN_PASSWORDS_SAVE = 0,
  WREN_PASSWORDS_UPDATE = 1,
  // The passwords saved for the site, from the key icon or after saving.
  WREN_PASSWORDS_SAVED = 2,
  // Chromium signed in with a saved password. A short notice.
  WREN_PASSWORDS_AUTO_SIGNIN = 3,
} WrenPasswordsPrompt;

typedef struct WrenPasswordsState {
  WrenPasswordsPrompt prompt;
  // For SAVE and UPDATE: the number of passwords the form had, which Chrome
  // shows as a dropdown when there is more than one, and the one selected.
  // Then whether they are revealed, and whether revealing them asks for
  // authentication first.
  size_t candidate_count;
  size_t candidate;
  bool revealed;
  bool reveal_needs_authentication;
  // For SAVED: the number of saved credentials, as Chromium had them when the
  // request opened. A password saved a moment ago may still be on its way.
  size_t credential_count;
  // For SAVE and UPDATE: true if the password goes to the user's Google
  // Account, not to this device.
  bool account_store;
} WrenPasswordsState;

// Reads the prompt's state. Its texts come from Wren_CopyPopoverText
// (WrenUnstableUI.h).
WREN_API WrenResult Wren_GetPasswordsState(WrenPopoverID popover,
                                           WrenPasswordsState* state);
// For SAVE, UPDATE and AUTO_SIGNIN: copies the username of the credential the
// prompt is about.
WREN_API size_t Wren_CopyPasswordsUsername(WrenPopoverID popover,
                                           char* buffer,
                                           size_t size);
// Returns the length of a candidate's password, for SAVE and UPDATE, or of a
// saved credential's, for SAVED, so the app can draw its dots. 0 past the end.
WREN_API size_t Wren_GetPasswordLength(WrenPopoverID popover, size_t index);
// For SAVED: copies a credential's username, or "" if it has none.
WREN_API size_t Wren_CopySavedUsername(WrenPopoverID popover,
                                       size_t index,
                                       char* buffer,
                                       size_t size);

// Runs later, from the event loop, with whether the passwords may be shown.
typedef void (*WrenPasswordsCallback)(void* context,
                                      WrenPopoverID popover,
                                      bool allowed);

// For SAVE and UPDATE: Chrome's eye button. Asks for authentication where
// Chromium does. Once allowed, Wren_CopyPassword reads the candidates.
WREN_API WrenResult Wren_RevealPasswords(WrenPopoverID popover,
                                         WrenPasswordsCallback callback,
                                         void* context);
// For SAVED: opens a credential's details, after Touch ID or the Mac password,
// as Chrome does. Once allowed, Wren_CopyPassword reads that credential, and
// Wren_UpdateSavedPassword changes it.
WREN_API WrenResult Wren_ShowPasswordDetails(WrenPopoverID popover,
                                             size_t index,
                                             WrenPasswordsCallback callback,
                                             void* context);
// Copies a revealed candidate's password, or an opened credential's. Copies
// "" otherwise.
WREN_API size_t Wren_CopyPassword(WrenPopoverID popover,
                                  size_t index,
                                  char* buffer,
                                  size_t size);
// For SAVE and UPDATE: saves, as Chrome's Save does. It saves with the
// username as edited, where NULL keeps it, and the password the user typed,
// or else the candidate at `candidate`. The request then closes.
WREN_API WrenResult Wren_SavePassword(WrenPopoverID popover,
                                      const char* username,
                                      const char* password,
                                      size_t candidate);
// For SAVED, after Wren_ShowPasswordDetails: changes the credential's
// username, password and note. NULL keeps each one.
WREN_API WrenResult Wren_UpdateSavedPassword(WrenPopoverID popover,
                                             size_t index,
                                             const char* username,
                                             const char* password,
                                             const char* note);

typedef enum WrenPasswordsAction {
  // For SAVE and UPDATE: Chrome's Not now, or No thanks. The request closes.
  WREN_PASSWORDS_NOT_NOW = 0,
  // For SAVE: never for this site. The request closes.
  WREN_PASSWORDS_NEVER = 1,
  // Opens Chromium's password manager, in a new tab.
  WREN_PASSWORDS_MANAGE = 2,
} WrenPasswordsAction;

// Does one of the prompt's actions.
WREN_API WrenResult Wren_PasswordsAction(WrenPopoverID popover,
                                         WrenPasswordsAction action);

// Saving or updating an address.
typedef struct WrenAddressState {
  bool is_update;
} WrenAddressState;

WREN_API WrenResult Wren_GetAddressState(WrenPopoverID popover,
                                         WrenAddressState* state);

typedef enum WrenAddressAnswer {
  WREN_ADDRESS_SAVE = 0,
  WREN_ADDRESS_NO_THANKS = 1,
  // Opens Chromium's address editor, a dialog. The request closes.
  WREN_ADDRESS_EDIT = 2,
} WrenAddressAnswer;

// Answers the prompt. Every answer closes the request.
WREN_API WrenResult Wren_AnswerAddressPrompt(WrenPopoverID popover,
                                             WrenAddressAnswer answer);

// Saving a card on this device. Wren never gives the app the card's number or
// security code.
typedef enum WrenCardNetwork {
  // A network Wren does not name, or none known.
  WREN_CARD_NETWORK_OTHER = 0,

  WREN_CARD_NETWORK_AMERICAN_EXPRESS = 1,
  WREN_CARD_NETWORK_DINERS = 2,
  WREN_CARD_NETWORK_DISCOVER = 3,
  WREN_CARD_NETWORK_ELO = 4,
  WREN_CARD_NETWORK_JCB = 5,
  WREN_CARD_NETWORK_MASTERCARD = 6,
  WREN_CARD_NETWORK_MIR = 7,
  WREN_CARD_NETWORK_TROY = 8,
  WREN_CARD_NETWORK_UNIONPAY = 9,
  WREN_CARD_NETWORK_VERVE = 10,
  WREN_CARD_NETWORK_VISA = 11,
} WrenCardNetwork;

typedef struct WrenCardState {
  WrenCardNetwork network;
  int expiration_month;  // 1 to 12, or 0 if missing
  int expiration_year;   // four digits, or 0 if missing
  // True if Chrome asks for the name, or for the expiration date, before
  // saving.
  bool needs_name;
  bool needs_expiration;
} WrenCardState;

WREN_API WrenResult Wren_GetCardState(WrenPopoverID popover,
                                      WrenCardState* state);
// Copies the card's last four digits, such as "1111".
WREN_API size_t Wren_CopyCardLastDigits(WrenPopoverID popover,
                                        char* buffer,
                                        size_t size);
// Copies the name on the card, as the form gave it. "" if it gave none.
WREN_API size_t Wren_CopyCardName(WrenPopoverID popover,
                                  char* buffer,
                                  size_t size);
// Saves the card, with the name and expiration date the user gave when asked.
// Pass NULL and 0 when Chrome did not ask. The request closes.
WREN_API WrenResult Wren_SaveCard(WrenPopoverID popover,
                                  const char* name,
                                  int expiration_month,
                                  int expiration_year);
// Chrome's No thanks. The request closes.
WREN_API WrenResult Wren_DeclineCard(WrenPopoverID popover);

// Translating the page. It needs Chromium configured with a Google API key,
// and Wren adds none.
typedef enum WrenTranslateStep {
  WREN_TRANSLATE_BEFORE = 0,
  WREN_TRANSLATE_TRANSLATING = 1,
  WREN_TRANSLATE_AFTER = 2,
  WREN_TRANSLATE_ERROR = 3,
} WrenTranslateStep;

typedef struct WrenTranslateState {
  WrenTranslateStep step;
  // The page's language and the one to translate to, as indexes in their
  // lists, and the length of each list.
  size_t source;
  size_t target;
  size_t source_count;
  size_t target_count;

  bool always_translate;
  bool never_language;
  bool never_site;
  // True if the site can be put on the never list.
  bool can_never_site;
} WrenTranslateState;

WREN_API WrenResult Wren_GetTranslateState(WrenPopoverID popover,
                                           WrenTranslateState* state);
// Copies a language's code, from the list of source languages, or of target
// languages if `target` is true. Codes are such as "fr" and "zh-TW", and "und"
// is the source list's unknown language.
WREN_API size_t Wren_CopyTranslateLanguageCode(WrenPopoverID popover,
                                               bool target,
                                               size_t index,
                                               char* buffer,
                                               size_t size);
// Chooses the source and target languages, by their indexes in their lists.
WREN_API WrenResult Wren_SetTranslateLanguages(WrenPopoverID popover,
                                               size_t source,
                                               size_t target);

typedef enum WrenTranslateAction {
  WREN_TRANSLATE_PAGE = 0,
  WREN_TRANSLATE_REVERT = 1,
  // Not this time. The request closes.
  WREN_TRANSLATE_DECLINE = 2,
  // The next three turn a setting on or off, as `on` says.
  WREN_TRANSLATE_ALWAYS = 3,
  WREN_TRANSLATE_NEVER_LANGUAGE = 4,
  WREN_TRANSLATE_NEVER_SITE = 5,
} WrenTranslateAction;

// Does one of the prompt's actions.
WREN_API WrenResult Wren_TranslateAction(WrenPopoverID popover,
                                         WrenTranslateAction action,
                                         bool on);

// A permission request.
typedef enum WrenPermissionQuiet {
  // Chromium wants its full prompt.
  WREN_PERMISSION_NOT_QUIET = 0,
  // The other values mean Chromium wants a quiet prompt, an icon in the
  // address field. This one is because the user chose quieter messaging.
  WREN_PERMISSION_QUIET_BY_SETTING = 1,
  // Because the site is known for abusive requests or content.
  WREN_PERMISSION_QUIET_ABUSIVE = 2,
  // Because Chromium predicts the user will not allow it.
  WREN_PERMISSION_QUIET_PREDICTED = 3,
  // Because the page asked without a user gesture.
  WREN_PERMISSION_QUIET_NO_GESTURE = 4,
} WrenPermissionQuiet;

typedef struct WrenPermissionPromptState {
  // The number of permissions asked for together, such as the camera and the
  // microphone.
  size_t count;

  WrenPermissionQuiet quiet;
  // True if Chrome offers "Allow this time" for every one of them.
  bool can_allow_this_time;
} WrenPermissionPromptState;

WREN_API WrenResult
Wren_GetPermissionPromptState(WrenPopoverID popover,
                              WrenPermissionPromptState* state);
// Copies a requested permission's Chromium name, as Settings URLs use it,
// such as "geolocation" or "media-stream-camera".
WREN_API size_t Wren_CopyPermissionID(WrenPopoverID popover,
                                      size_t index,
                                      char* buffer,
                                      size_t size);

typedef enum WrenPermissionAnswer {
  WREN_PERMISSION_ANSWER_ALLOW = 0,
  WREN_PERMISSION_ANSWER_ALLOW_THIS_TIME = 1,
  WREN_PERMISSION_ANSWER_BLOCK = 2,
  // Closed without deciding, as with the close button of Chrome's prompt.
  WREN_PERMISSION_ANSWER_DISMISS = 3,
} WrenPermissionAnswer;

// Answers the request, which then closes.
//
// Chrome ignores a click on its prompt that comes too soon after the prompt
// appeared, within the system's double-click interval. So an Allow that soon
// after the request returns WREN_ERROR_NOT_ALLOWED.
WREN_API WrenResult Wren_AnswerPermissionPrompt(WrenPopoverID popover,
                                                WrenPermissionAnswer answer);
// For a quiet request: Chromium asks again for its full prompt, as a new
// request. This request closes.
WREN_API WrenResult Wren_ShowFullPermissionPrompt(WrenPopoverID popover);

// --- Toasts -----------------------------------------------------------------
//
// A toast the app answered with its own (WREN_POPOVER_TOAST). Its text and its
// button's text are Chrome's (Wren_CopyPopoverText, WrenUnstableUI.h). Its
// close button calls Wren_ClosePopover.
//
// As with the prompts above, every call returns WREN_ERROR_POPOVER_CLOSED
// once the toast has closed, and WREN_ERROR_INVALID_ARGUMENT for another kind
// of request.

typedef struct WrenToastState {
  // Whether Chrome's toast has an action button, which Wren_PressToastButton
  // presses, a close button, and a menu (Wren_GetToastMenuItemCount).
  bool has_button;
  bool has_close_button;
  bool has_menu;
  // True if a spinner takes the place of its icon, because something is
  // under way.
  bool has_throbber;
  // True if the toast is about the Browser, not its active tab. It then stays
  // across tab switches and navigations.
  bool global;
} WrenToastState;

WREN_API WrenResult Wren_GetToastState(WrenPopoverID popover,
                                       WrenToastState* state);
// Copies the toast's name, as in WrenPopoverRequest.toast, in UTF-8. Returns
// the length needed, including the NUL.
WREN_API size_t Wren_CopyToastName(WrenPopoverID popover,
                                   char* buffer,
                                   size_t size);
// Copies the toast's icon as a PNG, `points` square at `scale`. `points` is 1
// to 1024 and `scale` is at most 4. The icon is in Chromium's colour for the
// window's appearance.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 when there is no icon, as with a spinner.
WREN_API size_t Wren_CopyToastIconPNG(WrenPopoverID popover,
                                      int points,
                                      double scale,
                                      uint8_t* buffer,
                                      size_t size);
// Presses Chrome's action button. It does what it says, and the request
// closes. Returns WREN_ERROR_NOT_ALLOWED for a toast without one.
WREN_API WrenResult Wren_PressToastButton(WrenPopoverID popover);

// One item of the toast's menu, as Chrome's menu shows it. The labels are in
// WrenUnstableUI.h.
typedef struct WrenToastMenuItem {
  bool separator;
  bool enabled;
  bool checked;
} WrenToastMenuItem;

// The number of items in the toast's menu. 0 without a menu.
WREN_API size_t Wren_GetToastMenuItemCount(WrenPopoverID popover);
WREN_API WrenResult Wren_GetToastMenuItem(WrenPopoverID popover,
                                          size_t index,
                                          WrenToastMenuItem* item);
// Chooses the item, as in Chrome's menu, and the request closes. Returns
// WREN_ERROR_NOT_ALLOWED for a separator or a disabled item.
WREN_API WrenResult Wren_ChooseToastMenuItem(WrenPopoverID popover,
                                             size_t index);

// --- The app's own toasts ---------------------------------------------------
//
// A toast of the app's, such as "Address copied", shown as Chromium shows its
// own. It goes through the Browser's toast queue. So it replaces the toast
// showing, and goes after about 4 seconds, when the tab switches or
// navigates, or when the next toast comes.
//
// Before it shows, Wren asks on_popover as for Chromium's toasts, as
// WREN_POPOVER_TOAST with the toast "Embedder". The answer may be Chromium's,
// the app's own, or none. The app's own toast reads its text and icon as any
// toast does.

typedef struct WrenToastOptions {
  // Required, and not empty. UTF-8.
  const char* text;
  // Optional. A square PNG, drawn as given in place of Chromium's info icon,
  // which is drawn in the toast's text colour. Any size works, but 32 pixels
  // across or more looks sharp on a Retina display.
  const void* icon_png;
  size_t icon_png_size;
} WrenToastOptions;

// Shows one of the app's own toasts in the Browser.
//
// Returns WREN_ERROR_CLOSED for a closed Browser, and
// WREN_ERROR_INVALID_ARGUMENT for no text or an icon that is not a square PNG.
// Returns WREN_ERROR_NOT_ALLOWED from on_popover, and when the user's
// settings, as Chrome's, show only toasts with a button.
WREN_API WrenResult Wren_ShowToast(WrenBrowser* browser,
                                   const WrenToastOptions* options);

#ifdef __cplusplus
}  // extern "C"
#endif

// The header that completes these as Chrome draws them. It comes after the
// types it uses.
#include "WrenUnstableUI.h"

#endif  // WREN_PUBLIC_WREN_UI_H_

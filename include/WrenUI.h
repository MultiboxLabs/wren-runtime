// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's popovers as requests and data (C API). Wren.h includes
// this header; see Wren.h for threading, handles and strings.
//
// This is the part Wren keeps from release to release: the requests, and
// the facts and actions behind each popover. Chrome's sentences and the
// layout of its bubbles are in WrenUnstableUI.h, which may change in any
// release (docs/apps/compatibility.md).
//
// Chromium opens popovers next to its toolbar: page info, the extensions
// menu, zoom, the bookmark star, a content setting's bubble, translate, its
// prompts to save a password, an address or a card, and permission prompts
// (a page asking for location, the camera, notifications). Before it opens one
// in a Browser the app knows, Wren asks the app's on_popover, whatever
// started it: the app's own call (Wren_ShowPageInfo, Wren_ClickPageAction),
// a Chromium command (Cmd-D, Cmd-+), or Chromium on its own (a save prompt
// after a sign-in, an extension's zoom). The app answers per request:
// Chromium's own popover, the app's, or none. So an app can draw its own
// zoom popover and leave the rest to Chromium, or the other way round, or
// decide from the request.
//
// Dialogs that stand on their own stay Chromium's and are never asked about:
// device choosers, passkeys, HTTP auth, page dialogs, extension installs,
// the dialogs a popover opens, and the prompts of a page's <permission>
// elements.
//
// on_popover runs inside Chromium's call, as on_fullscreen does (Wren.h),
// unlike every other callback. It may read anything, but until it returns,
// calls that change Profiles, Browsers, tabs, navigation, anchors, extension
// actions, popovers or full screen are refused: they return
// WREN_ERROR_NOT_ALLOWED, report it to their callback, or, for
// Wren_AttachBrowserToView, do nothing. Do that work afterwards, for example
// from a dispatched block. Wren_Quit, which takes effect later, and retaining
// and releasing handles are allowed.
//
// A request the app answers with WREN_POPOVER_APP stays open until the app
// closes it (Wren_ClosePopover) or Wren withdraws it, which
// WREN_CHANGE_POPOVER reports. Wren withdraws a request, as Chrome closes
// its popover, when:
// - the tab is hidden or switched away from, or closes or leaves the
//   Browser; for page info and a content setting's bubble, which are about
//   one page, also when the page changes (Chrome's other popovers stay
//   across navigations);
// - Chromium's model closes it (a prompt that is no longer needed);
// - the Browser opens another popover of the same kind, Chromium's or the
//   app's: one at a time, as in Chrome.
// While it is open, Chromium's bookkeeping says a popover is showing: the
// zoom icon stays shown and the page action reports bubble_open. The click
// rule of Wren_ClickPageAction and the other calls (a click that began while
// Chromium's popover was open leaves it closed) is for Chromium's popovers
// only: the app toggles its own.

#ifndef WREN_PUBLIC_WREN_UI_H_
#define WREN_PUBLIC_WREN_UI_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Popover requests -------------------------------------------------------

typedef enum WrenPopoverKind {
  // The connection, the site's permissions and data (Wren_ShowPageInfo).
  WREN_POPOVER_PAGE_INFO = 0,
  // Every extension, pinning and site access (Wren_ShowExtensionsMenu).
  WREN_POPOVER_EXTENSIONS_MENU = 1,
  // The zoom level, after Cmd-+, Cmd-- or Cmd-0, an extension's zoom, or the
  // zoom page action. Chrome closes its own by itself 1.5 seconds after it
  // opened automatically, or 5 seconds after a button in it was used, and
  // not while the pointer is over it; do the same.
  WREN_POPOVER_ZOOM = 2,
  // The page was bookmarked (Cmd-D, the star): its title, its folder,
  // remove. See newly_bookmarked.
  WREN_POPOVER_BOOKMARK = 3,
  // A content setting icon's bubble (Wren_ShowContentSettingBubble). See
  // content_setting.
  WREN_POPOVER_CONTENT_SETTING = 4,
  // Translating the page.
  WREN_POPOVER_TRANSLATE = 5,
  // Saving or updating a password, the passwords saved for the site, and
  // automatic sign-in. Chromium's other password prompts (generated
  // passwords, passkeys, moving to the account) stay Chromium's without
  // asking.
  WREN_POPOVER_PASSWORDS = 6,
  // Saving or updating an address.
  WREN_POPOVER_SAVE_ADDRESS = 7,
  // Saving a card on this device. Saving to Google Payments stays
  // Chromium's without asking.
  WREN_POPOVER_SAVE_CARD = 8,
  // A page asks for permissions (location, the camera, notifications…),
  // always WREN_POPOVER_AUTOMATIC. Most apps leave these to Chromium.
  // Switching tabs closes the request and a new one comes when the tab is
  // back, as Chrome hides its prompt and shows it again.
  WREN_POPOVER_PERMISSION = 9,
  // One of Chromium's toasts ("Link copied", "Added to reading list"),
  // always WREN_POPOVER_AUTOMATIC; see toast. Chromium decides when it goes,
  // as for its own: about 4 seconds, 8 with a button or a menu; when the tab
  // switches or navigates for a tab's toast (not for a global one); and when
  // another toast comes. The request then closes, as for any popover. Show
  // it without taking focus.
  WREN_POPOVER_TOAST = 10,
  // The find bar, as a find opens in the active tab: Find… (Cmd-F) or Find
  // Next, Wren_Find, or automatically when a tab with a find becomes active
  // again (Chrome hides its bar for a tab with none). Asked again by a
  // second Find while the bar is open: the app's field takes focus again,
  // as Chrome's does. Answered with the app's own bar, it stays open until
  // the find ends (Wren_ClosePopover ends it as Escape does) or its tab is
  // switched away from; see the find state. Answered none, the find ends.
  WREN_POPOVER_FIND = 11,
  // Chrome's download bubble: its short list, WREN_POPOVER_AUTOMATIC, when
  // all of the Profile's downloads are done, in its last active Browser
  // (Chrome closes it 5 seconds after it opened unless it is used: do the
  // same, and show it without taking focus); the full list from
  // Wren_ClickDownloadButton; or one download's warning (`download`), from
  // chrome://downloads. Not about one tab: it stays across tab switches. See
  // WrenDownloads.h.
  WREN_POPOVER_DOWNLOADS = 12,
} WrenPopoverKind;

typedef enum WrenPopoverTrigger {
  // The app's own call: Wren_ShowPageInfo, Wren_ShowExtensionsMenu,
  // Wren_ClickPageAction, Wren_ShowContentSettingBubble.
  WREN_POPOVER_BY_APP = 0,
  // A Chromium command or shortcut the user chose (Cmd-D, Cmd-+).
  WREN_POPOVER_BY_USER = 1,
  // Chromium on its own: a save prompt after a sign-in or a form, an
  // extension's zoom, translate offering itself. Show it without taking
  // focus, as Chrome does.
  WREN_POPOVER_AUTOMATIC = 2,
} WrenPopoverTrigger;

// What Chromium is about to open. Valid during on_popover.
struct WrenPopoverRequest {
  // The request's ID: Wren_ClosePopover and WREN_CHANGE_POPOVER use it if
  // the app answers WREN_POPOVER_APP.
  WrenPopoverID popover;
  WrenPopoverKind kind;
  WrenBrowser* browser;  // borrowed
  // The tab it is about: the Browser's active tab.
  WrenTabID tab;
  WrenPopoverTrigger trigger;
  // CONTENT_SETTING: the icon's setting.
  WrenContentSetting content_setting;
  // BOOKMARK: the page was bookmarked just now, rather than already.
  bool newly_bookmarked;
  // TOAST: Chromium's name for the toast, such as "LinkCopied" or
  // "AddedToReadingList" (a toast's ID in Chromium, without its "k"). NULL
  // for any other kind.
  const char* toast;
  // DOWNLOADS: the download whose warning to show, or WREN_NO_DOWNLOAD for
  // the list.
  WrenDownloadID download;
};

// The open request of this kind in the Browser that the app answered with
// WREN_POPOVER_APP, or WREN_NO_POPOVER.
WREN_API WrenPopoverID Wren_GetOpenPopover(WrenBrowser* browser,
                                           WrenPopoverKind kind);

// The app's popover closed (Escape, a click elsewhere, Done). Chromium is
// told what its own popover tells it when it closes: a prompt counts as
// dismissed without a decision, and its page action can open it again.
// WREN_CHANGE_POPOVER follows. WREN_ERROR_POPOVER_CLOSED if the request has
// already closed.
WREN_API WrenResult Wren_ClosePopover(WrenPopoverID popover);

// --- State
// --------------------------------------------------------------------
//
// What these popovers show, readable at any time without a popover, for a
// Browser's active tab: the site's security and permissions (site state).
// The app can show it as always-visible labels and buttons, combine it, or
// draw its own popover from it when it answers a request. A normal or popup
// Browser has it; an app Browser has none (WREN_ERROR_NOT_ALLOWED). Wren
// builds Chromium's model on the first read and keeps it for the active
// tab's page; a change reaches the app once it has read the state. Chrome's
// text for it is in WrenUnstableUI.h; the app draws its own icons.

// A page a tab shows, as read with the state: it changes with each
// navigation to a new document and each tab switch. Actions take it back and
// return WREN_ERROR_STALE_PAGE if the Browser's active page is no longer
// that one, so a click as a page changes never acts on the next page.
typedef uint64_t WrenPageToken;

// --- Site state (page info) -------------------------------------------------

typedef enum WrenSiteSecurity {
  // A secure connection.
  WREN_SITE_SECURE = 0,
  // Not secure: plain http, or https with errors or insecure content.
  WREN_SITE_NOT_SECURE = 1,
  // Safe Browsing or a safety tip warns about the site.
  WREN_SITE_DANGEROUS = 2,
  // A browser page (chrome://), a page on a scheme the app declared
  // WREN_SCHEME_SECURE (the app's own: "You're viewing a secure <the app's
  // name> page"), an extension's page or a file.
  WREN_SITE_INTERNAL = 3,
  // The administrator's policy flags the site.
  WREN_SITE_ENTERPRISE = 4,
} WrenSiteSecurity;

typedef struct WrenSiteState {
  WrenPageToken page;
  WrenSiteSecurity security;
  // The connection has a certificate: WREN_SITE_CERTIFICATE shows it.
  bool has_certificate;
  // The user let a certificate error or an http warning through for this
  // site: WREN_SITE_REVOKE_DECISIONS takes it back.
  bool can_revoke_decisions;
  // A change made here applies once the page reloads (Chrome offers to
  // reload as its page info closes): WREN_SITE_RELOAD.
  bool reload_needed;
  // How many sites this one let use cookies; -1 until Chromium knows.
  int cookie_sites;
  // Chrome's third-party cookie switch for the site is shown, and whether
  // third-party cookies are blocked here
  // (WREN_SITE_TOGGLE_THIRD_PARTY_COOKIES).
  bool third_party_cookies_shown;
  bool third_party_cookies_blocked;
} WrenSiteState;

// The site state of the Browser's active tab. WREN_ERROR_NAVIGATION_UNAVAILABLE
// before the tab has shown a page.
WREN_API WrenResult Wren_GetSiteState(WrenBrowser* browser,
                                      WrenSiteState* state);
// The page's origin, which the state is about: "https://example.com:8443",
// or "chrome://settings"; "" for a page with no origin of its own (a file,
// data:). UTF-8; returns the length needed, including the NUL.
WREN_API size_t Wren_CopySiteOrigin(WrenBrowser* browser,
                                    char* buffer,
                                    size_t size);

typedef enum WrenPermissionSetting {
  // No setting for the site: the default applies.
  WREN_PERMISSION_DEFAULT = 0,
  WREN_PERMISSION_ASK = 1,
  WREN_PERMISSION_ALLOW = 2,
  WREN_PERMISSION_BLOCK = 3,
} WrenPermissionSetting;

typedef struct WrenSitePermission {
  WrenPermissionSetting setting;
  // What WREN_PERMISSION_DEFAULT means for it now: ASK, ALLOW or BLOCK.
  WrenPermissionSetting default_setting;
  // Set by policy or an extension; the user cannot change it.
  bool managed;
  // Allowed this time only.
  bool one_time;
  // The page is using it now.
  bool in_use;
} WrenSitePermission;

// The permissions and settings Chrome's page info lists for the site, in its
// order: those set for the site, in use, or changed here.
WREN_API size_t Wren_GetSitePermissionCount(WrenBrowser* browser);
WREN_API WrenResult Wren_GetSitePermission(WrenBrowser* browser,
                                           size_t index,
                                           WrenSitePermission* permission);
// Chromium's name for a listed permission, as in Settings URLs and as
// Wren_SetSitePermission takes it: "geolocation", "notifications", "popups".
WREN_API size_t Wren_CopySitePermissionID(WrenBrowser* browser,
                                          size_t index,
                                          char* buffer,
                                          size_t size);
// Sets a permission or setting for the site, by Chromium's name, listed or
// not, as Chrome's page info does. WREN_ERROR_INVALID_ARGUMENT for a name
// page info does not offer, or a setting it cannot have;
// WREN_ERROR_NOT_ALLOWED for a managed one.
WREN_API WrenResult Wren_SetSitePermission(WrenBrowser* browser,
                                           WrenPageToken page,
                                           const char* permission,
                                           WrenPermissionSetting setting);

// The devices the site may use (USB, HID, serial, Bluetooth), in Chrome's
// order.
WREN_API size_t Wren_GetSiteDeviceCount(WrenBrowser* browser);

typedef enum WrenSiteDeviceKind {
  WREN_SITE_DEVICE_USB = 0,
  WREN_SITE_DEVICE_HID = 1,
  WREN_SITE_DEVICE_SERIAL = 2,
  WREN_SITE_DEVICE_BLUETOOTH = 3,
} WrenSiteDeviceKind;

typedef struct WrenSiteDevice {
  WrenSiteDeviceKind kind;
  // Allowed by the administrator's policy: it cannot be forgotten.
  bool managed;
} WrenSiteDevice;

WREN_API WrenResult Wren_GetSiteDevice(WrenBrowser* browser,
                                       size_t index,
                                       WrenSiteDevice* device);
// The device's name, as the device or its system reports it.
WREN_API size_t Wren_CopySiteDeviceName(WrenBrowser* browser,
                                        size_t index,
                                        char* buffer,
                                        size_t size);
// Takes back the site's access to the device. WREN_ERROR_NOT_ALLOWED for a
// managed one.
WREN_API WrenResult Wren_ForgetSiteDevice(WrenBrowser* browser,
                                          WrenPageToken page,
                                          size_t index);

typedef enum WrenSiteAction {
  // Chromium's settings for the site, in a new tab.
  WREN_SITE_SETTINGS = 0,
  // Chromium's certificate viewer, a dialog.
  WREN_SITE_CERTIFICATE = 1,
  // Chromium's dialog of the site's cookies and data.
  WREN_SITE_DATA = 2,
  WREN_SITE_REVOKE_DECISIONS = 3,
  WREN_SITE_TOGGLE_THIRD_PARTY_COOKIES = 4,
  // The help page about the site's security state, in a new tab.
  WREN_SITE_LEARN_MORE = 5,
  // Reloads the page, after reload_needed.
  WREN_SITE_RELOAD = 6,
} WrenSiteAction;

// WREN_ERROR_NOT_ALLOWED when the state does not offer it now
// (has_certificate, can_revoke_decisions, third_party_cookies_shown), and for
// a WREN_SITE_INTERNAL page (a browser page, the app's own, a file, DevTools,
// an extension's page), which, as in Chrome, has only its summary and no
// permissions.
WREN_API WrenResult Wren_SiteAction(WrenBrowser* browser,
                                    WrenPageToken page,
                                    WrenSiteAction action);

// --- Extensions state (the extensions menu) ---------------------------------
//
// What Chromium's extensions menus show about the extensions on the active
// tab's site, for a normal Browser (others: WREN_ERROR_NOT_ALLOWED).
// Chromium has two designs of the menu. Its own menu is the first: the
// extensions grouped by what they may do on the site (full access, access
// requested, no access needed), each changed from its context menu
// (Wren_ShowExtensionActionMenu). The redesign adds a switch to block every
// extension on the site, a switch per extension, a choice per extension (on
// click, on this site, on all sites), extensions' requests for access, and a
// message to reload. What each extension may do and the actions on it are
// here, whichever design the app draws; each design's controls are in
// WrenUnstableUI.h.
// Extensions are named by their IDs, as extension actions are
// (Wren_CopyExtensionActionID).

typedef enum WrenExtensionsSiteSetting {
  // Each extension's own access applies (the redesign's switch on).
  WREN_EXTENSIONS_CUSTOMIZE = 0,
  // No extension may run on the site.
  WREN_EXTENSIONS_BLOCK_ALL = 1,
} WrenExtensionsSiteSetting;

typedef struct WrenExtensionsSite {
  WrenPageToken page;
  WrenExtensionsSiteSetting setting;
  // The user can change the setting (not managed, not a restricted site).
  bool can_change;
  // A change applies once the page reloads (Wren_ReloadForExtensions).
  bool reload_needed;
} WrenExtensionsSite;

// WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a page.
WREN_API WrenResult Wren_GetExtensionsSite(WrenBrowser* browser,
                                           WrenExtensionsSite* site);

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
  // Its access to this site can be granted or taken back now
  // (Wren_SetExtensionSiteAllowed), and whether it is granted.
  bool can_allow;
  bool allowed;
  // Its access (Wren_SetExtensionSiteAccess) and the ones that can be chosen
  // now; none can for an extension that asks for no site.
  WrenSiteAccess access;
  bool can_access_on_click;
  bool can_access_on_site;
  bool can_access_on_all_sites;
  // It may show its requests for access in the toolbar
  // (Wren_SetExtensionShowsRequests).
  bool shows_requests;
  // Installed by policy.
  bool enterprise;
} WrenExtensionOnSite;

// WREN_ERROR_INVALID_ARGUMENT for an extension with no action here.
WREN_API WrenResult Wren_GetExtensionOnSite(WrenBrowser* browser,
                                            const char* extension_id,
                                            WrenExtensionOnSite* extension);

// As the redesign's choice. WREN_ERROR_NOT_ALLOWED when it cannot be chosen.
WREN_API WrenResult Wren_SetExtensionSiteAccess(WrenBrowser* browser,
                                                WrenPageToken page,
                                                const char* extension_id,
                                                WrenSiteAccess access);
// As the redesign's switch: grants or takes back access to this site.
WREN_API WrenResult Wren_SetExtensionSiteAllowed(WrenBrowser* browser,
                                                 WrenPageToken page,
                                                 const char* extension_id,
                                                 bool allowed);
WREN_API WrenResult Wren_SetExtensionShowsRequests(WrenBrowser* browser,
                                                   const char* extension_id,
                                                   bool show);

// The extensions asking for access to the site, in Chrome's order.
WREN_API size_t Wren_GetExtensionRequestCount(WrenBrowser* browser);
WREN_API size_t Wren_CopyExtensionRequestID(WrenBrowser* browser,
                                            size_t index,
                                            char* buffer,
                                            size_t size);
// Allow grants the extension access to the site; otherwise the request is
// dismissed.
WREN_API WrenResult Wren_AnswerExtensionRequest(WrenBrowser* browser,
                                                WrenPageToken page,
                                                const char* extension_id,
                                                bool allow);
// Reloads the page, after WREN_EXTENSIONS_MESSAGE_RELOAD.
WREN_API WrenResult Wren_ReloadForExtensions(WrenBrowser* browser,
                                             WrenPageToken page);

// The extensions button (Chrome's puzzle piece), which changes with the
// site. WREN_CHANGE_EXTENSION_ACTIONS reports its changes.
typedef enum WrenExtensionsButton {
  WREN_EXTENSIONS_BUTTON_DEFAULT = 0,
  // At least one extension has access to the site.
  WREN_EXTENSIONS_BUTTON_ANY_HAS_ACCESS = 1,
  // Every extension is blocked on the site.
  WREN_EXTENSIONS_BUTTON_ALL_BLOCKED = 2,
} WrenExtensionsButton;

typedef struct WrenExtensionsButtonState {
  WrenExtensionsButton button;
  // How many extensions Chrome's request-access button next to it lists;
  // 0 hides it.
  size_t requesting_count;
} WrenExtensionsButtonState;

WREN_API WrenResult
Wren_GetExtensionsButtonState(WrenBrowser* browser,
                              WrenExtensionsButtonState* state);
WREN_API size_t Wren_CopyRequestingExtensionID(WrenBrowser* browser,
                                               size_t index,
                                               char* buffer,
                                               size_t size);
// As a click on Chrome's request-access button: every extension it lists
// gets access to the site.
WREN_API WrenResult Wren_GrantRequestedSiteAccess(WrenBrowser* browser,
                                                  WrenPageToken page);

// --- Zoom state
// ---------------------------------------------------------------

typedef struct WrenZoomState {
  WrenPageToken page;
  // The zoom level, as Chrome shows it: 100 is the default size.
  int percent;
  bool at_default;
  // Zoom In, Zoom Out and Reset can change it, as Chrome's zoom bubble
  // enables its buttons.
  bool can_zoom_in;
  bool can_zoom_out;
} WrenZoomState;

// The zoom of the Browser's active tab. WREN_ERROR_NAVIGATION_UNAVAILABLE
// with no tab.
WREN_API WrenResult Wren_GetZoomState(WrenBrowser* browser,
                                      WrenZoomState* state);
// The ID of the extension that set the level last (chrome.tabs.setZoom),
// which Chrome's zoom bubble names; "" if the user set it.
WREN_API size_t Wren_CopyZoomExtensionID(WrenBrowser* browser,
                                         char* buffer,
                                         size_t size);

typedef enum WrenZoom {
  WREN_ZOOM_IN = 0,
  WREN_ZOOM_OUT = 1,
  WREN_ZOOM_RESET = 2,
} WrenZoom;

// As Chrome's zoom bubble's buttons and Cmd-+, Cmd--, Cmd-0.
WREN_API WrenResult Wren_Zoom(WrenBrowser* browser,
                              WrenPageToken page,
                              WrenZoom zoom);

// --- Find state -------------------------------------------------------------
//
// The find in the Browser's active tab, whichever bar shows it, Chromium's or
// the app's: Chromium finds, and the app's bar does what Chrome's does. Its
// string is one in every tab on macOS: the system's find pasteboard, which
// Chromium follows (another app's Use Selection for Find) and which typing
// writes, except off the record. WREN_CHANGE_FIND reports each change.

typedef struct WrenFindState {
  WrenPageToken page;
  // A find is open in the tab: its bar shows.
  bool open;
  // The matches of the string in the page, and the current one, from 1; 0
  // for none current. Both -1 before a result for the string.
  int match_count;
  int active_match;
  // Chromium is still counting; the numbers may change. Chrome announces
  // only a count that is done.
  bool counting;
} WrenFindState;

// The find of the Browser's active tab. WREN_ERROR_NAVIGATION_UNAVAILABLE
// with no tab.
WREN_API WrenResult Wren_GetFindState(WrenBrowser* browser,
                                      WrenFindState* state);
// The string the bar shows; "" before a find has opened.
WREN_API size_t Wren_CopyFindString(WrenBrowser* browser,
                                    char* buffer,
                                    size_t size);

typedef enum WrenFind {
  // Opens a find, as Chrome's Find… (Cmd-F): the bar shows the page's
  // selection, or the find pasteboard's string, and Chromium counts and
  // highlights its matches without moving to one. WREN_POPOVER_FIND asks
  // whose bar it is, as the app's.
  WREN_FIND_OPEN = 0,
  // To the next or the previous match, wrapping at the ends, as Return and
  // Shift-Return in Chrome's bar and its arrows.
  WREN_FIND_NEXT = 1,
  WREN_FIND_PREVIOUS = 2,
  // Ends the find, the current match left selected, as Escape and the close
  // button.
  WREN_FIND_END = 3,
  // Ends it by clicking the current match, a link, as Control-Return.
  WREN_FIND_END_ACTIVATE = 4,
} WrenFind;

// WREN_ERROR_NOT_ALLOWED where Chrome's Find is disabled (a DevTools
// Browser, a dialog over the page), to step or end with no find open, and
// inside on_popover.
WREN_API WrenResult Wren_Find(WrenBrowser* browser,
                              WrenPageToken page,
                              WrenFind find);
// As typing in the bar: the string's matches are counted and highlighted,
// the first after the current one becomes current, and the find pasteboard
// is written. "" clears them. WREN_ERROR_NOT_ALLOWED with no find open.
WREN_API WrenResult Wren_SetFindString(WrenBrowser* browser,
                                       WrenPageToken page,
                                       const char* string);

// --- Bookmark state ---------------------------------------------------------
//
// The bookmark of the active tab's page, as Chrome's bookmark bubble edits
// it. The star's click (Wren_ClickPageAction) bookmarks the page. Changes
// are applied by the call, not when a popover closes. Folders are named by
// Chromium's IDs for them, which hold while the folder exists. Chrome's
// bubble's own folder list is in WrenUnstableUI.h.

typedef int64_t WrenBookmarkFolderID;

typedef struct WrenBookmark {
  WrenPageToken page;
  // The page is bookmarked by the user.
  bool bookmarked;
  // The folder holding the bookmark; 0 when not bookmarked.
  WrenBookmarkFolderID folder;
} WrenBookmark;

// WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a page.
WREN_API WrenResult Wren_GetBookmark(WrenBrowser* browser,
                                     WrenBookmark* bookmark);
// The bookmark's title; "" when not bookmarked.
WREN_API size_t Wren_CopyBookmarkTitle(WrenBrowser* browser,
                                       char* buffer,
                                       size_t size);

// The folders Chrome suggests for the bookmark, in its order: those used
// most recently and the main ones, the user's Google Account's first. 0 when
// the page is not bookmarked.
WREN_API size_t Wren_GetBookmarkFolderSuggestionCount(WrenBrowser* browser);
// 0 past the end.
WREN_API WrenBookmarkFolderID
Wren_GetBookmarkFolderSuggestion(WrenBrowser* browser, size_t index);

typedef struct WrenBookmarkFolder {
  // The folder holding it; 0 for a top-level one.
  WrenBookmarkFolderID parent;
  // Kept in the user's Google Account rather than on this device.
  bool account;
  // One of Chromium's own (the bookmarks bar, other bookmarks): it cannot be
  // renamed or removed.
  bool permanent;
} WrenBookmarkFolder;

// Any folder of the Browser's Profile. WREN_ERROR_INVALID_ARGUMENT for an ID
// that is not a folder.
WREN_API WrenResult Wren_GetBookmarkFolder(WrenBrowser* browser,
                                           WrenBookmarkFolderID folder,
                                           WrenBookmarkFolder* info);
WREN_API size_t Wren_CopyBookmarkFolderTitle(WrenBrowser* browser,
                                             WrenBookmarkFolderID folder,
                                             char* buffer,
                                             size_t size);

// Each needs the page bookmarked (WREN_ERROR_NOT_ALLOWED).

// Moves the bookmark to the end of a folder, as Chrome's bubble does.
// WREN_ERROR_INVALID_ARGUMENT for an ID that is not a folder, or one the
// user cannot add to (a managed folder).
WREN_API WrenResult Wren_MoveBookmark(WrenBrowser* browser,
                                      WrenPageToken page,
                                      WrenBookmarkFolderID folder);
WREN_API WrenResult Wren_SetBookmarkTitle(WrenBrowser* browser,
                                          WrenPageToken page,
                                          const char* title);
WREN_API WrenResult Wren_RemoveBookmark(WrenBrowser* browser,
                                        WrenPageToken page);
// Chromium's bookmark editor, a dialog (Chrome's Edit… button).
WREN_API WrenResult Wren_EditBookmark(WrenBrowser* browser, WrenPageToken page);

// --- Content setting state --------------------------------------------------
//
// What a content setting icon's bubble is about (WrenToolbar.h's content
// setting icons): what was blocked or used, and the site's value for the
// setting. Wren builds Chromium's model on the first read of an icon, for the
// active tab's page. Changes are WREN_CHANGE_CONTENT_SETTINGS. An icon that is
// not shown is WREN_ERROR_NOT_ALLOWED. Chrome's bubble's own layout is in
// WrenUnstableUI.h.

// A value the bubble offers for the site.
typedef enum WrenContentSettingValue {
  WREN_CONTENT_SETTING_VALUE_ALLOW = 0,
  WREN_CONTENT_SETTING_VALUE_BLOCK = 1,
  // A page's protocol handler only: neither, and ask again next time.
  WREN_CONTENT_SETTING_VALUE_IGNORE = 2,
} WrenContentSettingValue;

typedef struct WrenContentSettingState {
  WrenPageToken page;
  // What was blocked or used, such as each blocked popup
  // (Wren_CopyContentSettingItemURL).
  size_t item_count;
  // The bubble offers a value for the site: its value now, and the values
  // offered (Wren_SetContentSettingValue).
  bool has_value;
  WrenContentSettingValue value;
  bool can_allow;
  bool can_block;
  bool can_ignore;
  // Set by policy or an extension: the user cannot change it.
  bool managed;
} WrenContentSettingState;

WREN_API WrenResult Wren_GetContentSettingState(WrenBrowser* browser,
                                                WrenContentSetting setting,
                                                WrenContentSettingState* state);
// An item's page, for an item that is one (a blocked popup or redirect); ""
// for others.
WREN_API size_t Wren_CopyContentSettingItemURL(WrenBrowser* browser,
                                               WrenContentSetting setting,
                                               size_t index,
                                               char* buffer,
                                               size_t size);
// Sets the site's value and applies it at once, as choosing it in Chrome's
// bubble and Done. WREN_ERROR_NOT_ALLOWED for a managed setting,
// WREN_ERROR_INVALID_ARGUMENT for a value not offered.
WREN_API WrenResult Wren_SetContentSettingValue(WrenBrowser* browser,
                                                WrenPageToken page,
                                                WrenContentSetting setting,
                                                WrenContentSettingValue value);
// Opens an item, as a click on it in Chrome's bubble: a blocked popup opens.
WREN_API WrenResult Wren_OpenContentSettingItem(WrenBrowser* browser,
                                                WrenPageToken page,
                                                WrenContentSetting setting,
                                                size_t index);

// --- Prompt data
// --------------------------------------------------------------
//
// A prompt the app answered with its own (WREN_POPOVER_PASSWORDS,
// _SAVE_ADDRESS, _SAVE_CARD, _TRANSLATE, _PERMISSION) carries what Chrome's
// prompt shows and does, until it closes: then every call returns
// WREN_ERROR_POPOVER_CLOSED or copies "". A call for another kind of request is
// WREN_ERROR_INVALID_ARGUMENT. The prompts' texts are in WrenUnstableUI.h.

// Passwords, as Chrome's prompts: saving or updating one, the ones saved for
// the site, and signing in automatically. Passwords themselves cross only
// after Chromium's rule allows it, as Chrome reveals them: right after a
// sign-in Chrome shows the password at once; when the user reopened the
// prompt it asks for Touch ID or the Mac password first. They can be read
// until the request closes.
typedef enum WrenPasswordsPrompt {
  WREN_PASSWORDS_SAVE = 0,
  WREN_PASSWORDS_UPDATE = 1,
  // The passwords saved for the site (the key, or after saving).
  WREN_PASSWORDS_SAVED = 2,
  // Chromium signed in with a saved password: a short notice.
  WREN_PASSWORDS_AUTO_SIGNIN = 3,
} WrenPasswordsPrompt;

typedef struct WrenPasswordsState {
  WrenPasswordsPrompt prompt;
  // SAVE, UPDATE: the passwords the form had (Chrome's dropdown when more
  // than one), the one selected, whether they are revealed, and whether
  // revealing them asks for authentication first.
  size_t candidate_count;
  size_t candidate;
  bool revealed;
  bool reveal_needs_authentication;
  // SAVED: the saved credentials, as Chromium had them when the request
  // opened (a password saved a moment ago may still be on its way).
  size_t credential_count;
  // SAVE, UPDATE: to the user's Google Account rather than this device.
  bool account_store;
} WrenPasswordsState;

// Its texts are Wren_CopyPopoverText's (WrenUnstableUI.h).
WREN_API WrenResult Wren_GetPasswordsState(WrenPopoverID popover,
                                           WrenPasswordsState* state);
// SAVE, UPDATE, AUTO_SIGNIN: the username of the credential it is about.
WREN_API size_t Wren_CopyPasswordsUsername(WrenPopoverID popover,
                                           char* buffer,
                                           size_t size);
// The length of a candidate's password (SAVE, UPDATE) or of a saved
// credential's (SAVED), for drawing its dots; 0 past the end.
WREN_API size_t Wren_GetPasswordLength(WrenPopoverID popover, size_t index);
// SAVED: a credential's username ("" if it has none).
WREN_API size_t Wren_CopySavedUsername(WrenPopoverID popover,
                                       size_t index,
                                       char* buffer,
                                       size_t size);

// Runs later, from the event loop, with whether the passwords may be shown.
typedef void (*WrenPasswordsCallback)(void* context,
                                      WrenPopoverID popover,
                                      bool allowed);

// SAVE, UPDATE: Chrome's eye button. Asks for authentication where Chromium
// does; once allowed, Wren_CopyPassword reads the candidates.
WREN_API WrenResult Wren_RevealPasswords(WrenPopoverID popover,
                                         WrenPasswordsCallback callback,
                                         void* context);
// SAVED: opens a credential's details, after Touch ID or the Mac password as
// Chrome does; once allowed, Wren_CopyPassword reads that credential and
// Wren_UpdateSavedPassword changes it.
WREN_API WrenResult Wren_ShowPasswordDetails(WrenPopoverID popover,
                                             size_t index,
                                             WrenPasswordsCallback callback,
                                             void* context);
// A revealed candidate's or an opened credential's password; "" otherwise.
WREN_API size_t Wren_CopyPassword(WrenPopoverID popover,
                                  size_t index,
                                  char* buffer,
                                  size_t size);
// SAVE, UPDATE: saves, as Chrome's Save, with the username edited (NULL
// keeps it) and the password the user typed, or else the candidate at
// `candidate`. The request then closes.
WREN_API WrenResult Wren_SavePassword(WrenPopoverID popover,
                                      const char* username,
                                      const char* password,
                                      size_t candidate);
// SAVED, after Wren_ShowPasswordDetails: changes the credential's username,
// password and note (NULL keeps each).
WREN_API WrenResult Wren_UpdateSavedPassword(WrenPopoverID popover,
                                             size_t index,
                                             const char* username,
                                             const char* password,
                                             const char* note);

typedef enum WrenPasswordsAction {
  // SAVE, UPDATE: Chrome's Not now (or No thanks). The request closes.
  WREN_PASSWORDS_NOT_NOW = 0,
  // SAVE: never for this site. The request closes.
  WREN_PASSWORDS_NEVER = 1,
  // Chromium's password manager, in a new tab.
  WREN_PASSWORDS_MANAGE = 2,
} WrenPasswordsAction;

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
  // Opens Chromium's address editor, a dialog; the request closes.
  WREN_ADDRESS_EDIT = 2,
} WrenAddressAnswer;

// Every answer closes the request.
WREN_API WrenResult Wren_AnswerAddressPrompt(WrenPopoverID popover,
                                             WrenAddressAnswer answer);

// Saving a card on this device. Never the card's number or security code.
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
  // Chrome asks for the name, or the expiration date, before saving.
  bool needs_name;
  bool needs_expiration;
} WrenCardState;

WREN_API WrenResult Wren_GetCardState(WrenPopoverID popover,
                                      WrenCardState* state);
// The card's last four digits ("1111").
WREN_API size_t Wren_CopyCardLastDigits(WrenPopoverID popover,
                                        char* buffer,
                                        size_t size);
// The name on the card, as the form gave it; "" if it gave none.
WREN_API size_t Wren_CopyCardName(WrenPopoverID popover,
                                  char* buffer,
                                  size_t size);
// Saves the card, with the name and expiration the user gave when asked for
// (NULL and 0 otherwise). The request closes.
WREN_API WrenResult Wren_SaveCard(WrenPopoverID popover,
                                  const char* name,
                                  int expiration_month,
                                  int expiration_year);
// Chrome's No thanks. The request closes.
WREN_API WrenResult Wren_DeclineCard(WrenPopoverID popover);

// Translating the page. It needs a Google API key Chromium is configured
// with; Wren adds none.
typedef enum WrenTranslateStep {
  WREN_TRANSLATE_BEFORE = 0,
  WREN_TRANSLATE_TRANSLATING = 1,
  WREN_TRANSLATE_AFTER = 2,
  WREN_TRANSLATE_ERROR = 3,
} WrenTranslateStep;

typedef struct WrenTranslateState {
  WrenTranslateStep step;
  // The page's language and the one to translate to, as indexes in their
  // lists.
  size_t source;
  size_t target;
  size_t source_count;
  size_t target_count;
  bool always_translate;
  bool never_language;
  bool never_site;
  // The site can be put on the never list.
  bool can_never_site;
} WrenTranslateState;

WREN_API WrenResult Wren_GetTranslateState(WrenPopoverID popover,
                                           WrenTranslateState* state);
// A language's code, in the source list or the target list: "fr", "zh-TW",
// and "und" for the source list's unknown language.
WREN_API size_t Wren_CopyTranslateLanguageCode(WrenPopoverID popover,
                                               bool target,
                                               size_t index,
                                               char* buffer,
                                               size_t size);
WREN_API WrenResult Wren_SetTranslateLanguages(WrenPopoverID popover,
                                               size_t source,
                                               size_t target);

typedef enum WrenTranslateAction {
  WREN_TRANSLATE_PAGE = 0,
  WREN_TRANSLATE_REVERT = 1,
  // Not this time. The request closes.
  WREN_TRANSLATE_DECLINE = 2,
  // Turn each on or off with `on`.
  WREN_TRANSLATE_ALWAYS = 3,
  WREN_TRANSLATE_NEVER_LANGUAGE = 4,
  WREN_TRANSLATE_NEVER_SITE = 5,
} WrenTranslateAction;

WREN_API WrenResult Wren_TranslateAction(WrenPopoverID popover,
                                         WrenTranslateAction action,
                                         bool on);

// A permission request.
typedef enum WrenPermissionQuiet {
  // Chromium wants its full prompt.
  WREN_PERMISSION_NOT_QUIET = 0,
  // Chromium wants a quiet prompt, an icon in the address field, because the
  // user chose quieter messaging,
  WREN_PERMISSION_QUIET_BY_SETTING = 1,
  // because the site is known for abusive requests or content,
  WREN_PERMISSION_QUIET_ABUSIVE = 2,
  // because the user is predicted not to allow it,
  WREN_PERMISSION_QUIET_PREDICTED = 3,
  // or because the page asked without a user gesture.
  WREN_PERMISSION_QUIET_NO_GESTURE = 4,
} WrenPermissionQuiet;

typedef struct WrenPermissionPromptState {
  // Permissions asked for together, such as the camera and the microphone.
  size_t count;
  WrenPermissionQuiet quiet;
  // Chrome offers "Allow this time" for every one of them.
  bool can_allow_this_time;
} WrenPermissionPromptState;

WREN_API WrenResult
Wren_GetPermissionPromptState(WrenPopoverID popover,
                              WrenPermissionPromptState* state);
// A requested permission's Chromium name, as in Settings URLs:
// "geolocation", "media-stream-camera".
WREN_API size_t Wren_CopyPermissionID(WrenPopoverID popover,
                                      size_t index,
                                      char* buffer,
                                      size_t size);

typedef enum WrenPermissionAnswer {
  WREN_PERMISSION_ANSWER_ALLOW = 0,
  WREN_PERMISSION_ANSWER_ALLOW_THIS_TIME = 1,
  WREN_PERMISSION_ANSWER_BLOCK = 2,
  // Closed without deciding, as Chrome's prompt's close button.
  WREN_PERMISSION_ANSWER_DISMISS = 3,
} WrenPermissionAnswer;

// Answers the request, which then closes. As Chrome ignores a click on its
// prompt that comes too soon after the prompt appeared (within the system's
// double-click interval), an Allow that soon after the request is
// WREN_ERROR_NOT_ALLOWED.
WREN_API WrenResult Wren_AnswerPermissionPrompt(WrenPopoverID popover,
                                                WrenPermissionAnswer answer);
// For a quiet request: Chromium asks again for its full prompt, as a new
// request; this one closes.
WREN_API WrenResult Wren_ShowFullPermissionPrompt(WrenPopoverID popover);

// --- Toasts -----------------------------------------------------------------
//
// A toast the app answered with its own (WREN_POPOVER_TOAST). Its text and
// its button's are Chrome's (Wren_CopyPopoverText, WrenUnstableUI.h); its
// close button is Wren_ClosePopover. Like the prompts above, every call
// returns WREN_ERROR_POPOVER_CLOSED once it has closed, and
// WREN_ERROR_INVALID_ARGUMENT for another kind of request.

typedef struct WrenToastState {
  // Chrome's toast has an action button, which Wren_PressToastButton
  // presses; a close button; a menu (Wren_GetToastMenuItemCount).
  bool has_button;
  bool has_close_button;
  bool has_menu;
  // A spinner in place of its icon: something is under way.
  bool has_throbber;
  // About the Browser rather than its active tab: it stays across tab
  // switches and navigations.
  bool global;
} WrenToastState;

WREN_API WrenResult Wren_GetToastState(WrenPopoverID popover,
                                       WrenToastState* state);
// Its name, as WrenPopoverRequest.toast. UTF-8; returns the length needed,
// including the NUL.
WREN_API size_t Wren_CopyToastName(WrenPopoverID popover,
                                   char* buffer,
                                   size_t size);
// Its icon as PNG, `points` square (1 to 1024) at `scale` (up to 4), in
// Chromium's colour for the window's appearance. Returns the PNG's size,
// copying it only if `size` is enough; 0 with no icon (a throbber).
WREN_API size_t Wren_CopyToastIconPNG(WrenPopoverID popover,
                                      int points,
                                      double scale,
                                      uint8_t* buffer,
                                      size_t size);
// As Chrome's action button: does what it says, and the request closes.
// WREN_ERROR_NOT_ALLOWED without one.
WREN_API WrenResult Wren_PressToastButton(WrenPopoverID popover);

// The toast's menu, item by item, as Chrome's shows it; its labels are in
// WrenUnstableUI.h.
typedef struct WrenToastMenuItem {
  bool separator;
  bool enabled;
  bool checked;
} WrenToastMenuItem;

// 0 without a menu.
WREN_API size_t Wren_GetToastMenuItemCount(WrenPopoverID popover);
WREN_API WrenResult Wren_GetToastMenuItem(WrenPopoverID popover,
                                          size_t index,
                                          WrenToastMenuItem* item);
// As choosing the item in Chrome's menu, and the request closes.
// WREN_ERROR_NOT_ALLOWED for a separator or a disabled item.
WREN_API WrenResult Wren_ChooseToastMenuItem(WrenPopoverID popover,
                                             size_t index);

// --- The app's own toasts ----------------------------------------------------
//
// A toast of the app's ("Address copied"), shown as Chromium shows its own:
// through the Browser's toast queue, so it replaces the toast showing, goes
// after about 4 seconds, and goes when the tab switches or navigates or the
// next toast comes. Before it shows, on_popover is asked as for Chromium's
// (WREN_POPOVER_TOAST, toast "Embedder"): Chromium's, the app's own or none;
// the app's own reads its text and icon as any toast's.

typedef struct WrenToastOptions {
  // Required, not empty. UTF-8.
  const char* text;
  // Optional: a square PNG, drawn as given in place of Chromium's info icon,
  // which is drawn in the toast's text colour. Any size; 32 or more pixels
  // across looks sharp on a Retina display.
  const void* icon_png;
  size_t icon_png_size;
} WrenToastOptions;

// WREN_ERROR_CLOSED for a closed Browser; WREN_ERROR_INVALID_ARGUMENT for no
// text, or an icon that is not a square PNG; WREN_ERROR_NOT_ALLOWED from
// on_popover, or when the user's settings (as Chrome's) show only toasts
// with a button.
WREN_API WrenResult Wren_ShowToast(WrenBrowser* browser,
                                   const WrenToastOptions* options);

#ifdef __cplusplus
}  // extern "C"
#endif

// What completes these as Chrome draws them, after the types it uses.
#include "WrenUnstableUI.h"

#endif  // WREN_PUBLIC_WREN_UI_H_

// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's popovers as Chrome draws them (C API). UNSTABLE. Wren.h
// includes this header; see Wren.h for threading, handles and strings, and
// WrenUI.h for the requests and the state these complete.
//
// What is here follows Chrome's bubbles, which Chrome redesigns: Chrome's
// sentences (WrenUIText and every Wren_Copy...Text call, in Chromium's
// language), and the layout of one bubble (the extensions menu's two designs,
// the bookmark bubble's folder list, the content setting bubble), each of
// which has its facts in WrenUI.h too. It may change in any release of Wren,
// minor ones
// included, and WREN_UNSTABLE_UI_VERSION changes with it. Wren_Main refuses
// an app built against another version (WrenStartup.unstable_ui_version), as
// it refuses another WREN_API_VERSION. What may not change is in WrenUI.h;
// see docs/apps/compatibility.md.

#ifndef WREN_PUBLIC_WREN_UNSTABLE_UI_H_
#define WREN_PUBLIC_WREN_UNSTABLE_UI_H_

#include "Wren.h"

// The version of this header. Set WrenStartup.unstable_ui_version to it.
#define WREN_UNSTABLE_UI_VERSION 4

#ifdef __cplusplus
extern "C" {
#endif

// The text a state or request item offers. Each call says which it has; any
// other copies "".
typedef enum WrenUIText {
  // A one-line summary, such as "Connection is secure".
  WREN_UI_TEXT_TITLE = 0,
  // More about it, a sentence or two.
  WREN_UI_TEXT_DETAILS = 1,
  // The site, as Chromium names it: its host (and port, if not the
  // scheme's), or an organization's name.
  WREN_UI_TEXT_SITE = 2,
  // Chromium's name for an item, for code: "geolocation", "usb-chooser-data".
  WREN_UI_TEXT_ID = 3,
  // The item's name for people: "Location", a device's name.
  WREN_UI_TEXT_LABEL = 4,
  // The item's state for people: "Allowed", "Ask (default)", "Using now".
  WREN_UI_TEXT_STATE = 5,
  // What a pointer resting on the item shows.
  WREN_UI_TEXT_TOOLTIP = 6,
  // A longer message, such as what was blocked.
  WREN_UI_TEXT_MESSAGE = 7,
  // A link's text.
  WREN_UI_TEXT_LINK = 8,
  // The text of a manage button or checkbox, such as "Manage".
  WREN_UI_TEXT_MANAGE = 9,
  // The text of the button that accepts (Done, Save), and of the one that
  // declines (Cancel, No thanks).
  WREN_UI_TEXT_ACCEPT = 10,
  WREN_UI_TEXT_DECLINE = 11,
  // A username, or the name on a card.
  WREN_UI_TEXT_USERNAME = 12,
  // A footer line, such as where a password or address is saved.
  WREN_UI_TEXT_FOOTER = 13,
} WrenUIText;

// --- Site state (page info) -------------------------------------------------

// WREN_UI_TEXT_SITE, _TITLE (the security summary) and _DETAILS. UTF-8;
// returns the length needed, including the NUL.
WREN_API size_t Wren_CopySiteText(WrenBrowser* browser,
                                  WrenUIText text,
                                  char* buffer,
                                  size_t size);

// WREN_UI_TEXT_ID (Chromium's content setting name, as in Settings URLs:
// "geolocation", "notifications", "popups"), _LABEL ("Location") and _STATE
// ("Allowed", "Ask (default)", "Using now").
WREN_API size_t Wren_CopySitePermissionText(WrenBrowser* browser,
                                            size_t index,
                                            WrenUIText text,
                                            char* buffer,
                                            size_t size);

// WREN_UI_TEXT_ID (the kind's content setting name) and _LABEL (the
// device's name).
WREN_API size_t Wren_CopySiteDeviceText(WrenBrowser* browser,
                                        size_t index,
                                        WrenUIText text,
                                        char* buffer,
                                        size_t size);

// --- Extensions state (the extensions menu) ---------------------------------

typedef enum WrenExtensionsMessage {
  WREN_EXTENSIONS_MESSAGE_NONE = 0,
  // Extensions ask for access to the site (Wren_GetExtensionRequestCount).
  WREN_EXTENSIONS_MESSAGE_REQUESTS = 1,
  // A change applies once the page reloads (Wren_ReloadForExtensions).
  WREN_EXTENSIONS_MESSAGE_RELOAD = 2,
} WrenExtensionsMessage;

typedef struct WrenExtensionsSiteState {
  WrenPageToken page;
  WrenExtensionsSiteSetting setting;
  // The user can change the setting here (not managed, not a restricted
  // site).
  bool setting_can_change;
  // What the redesign shows below the extensions.
  WrenExtensionsMessage message;
} WrenExtensionsSiteState;

// WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a page.
WREN_API WrenResult Wren_GetExtensionsSiteState(WrenBrowser* browser,
                                                WrenExtensionsSiteState* state);
// WREN_UI_TEXT_TITLE: the site setting's label, as the redesign shows it.
WREN_API size_t Wren_CopyExtensionsSiteText(WrenBrowser* browser,
                                            WrenUIText text,
                                            char* buffer,
                                            size_t size);

typedef struct WrenExtensionSiteAccess {
  WrenSiteInteraction interaction;
  // The redesign's switch for this site: shown, usable, and on.
  bool switch_shown;
  bool switch_enabled;
  bool switch_on;
  // The redesign's choice of access can be shown for it, its current choice
  // and which choices are possible.
  bool can_choose;
  WrenSiteAccess access;
  bool on_click_enabled;
  bool on_site_enabled;
  bool on_all_sites_enabled;
  // It may show its requests for access in the toolbar.
  bool shows_requests;
  // Installed by policy.
  bool enterprise;
} WrenExtensionSiteAccess;

// WREN_ERROR_INVALID_ARGUMENT for an extension with no action here.
WREN_API WrenResult
Wren_GetExtensionSiteAccess(WrenBrowser* browser,
                            const char* extension_id,
                            WrenExtensionSiteAccess* access);
// WREN_UI_TEXT_TOOLTIP: the redesign's switch's tooltip.
WREN_API size_t Wren_CopyExtensionSiteAccessText(WrenBrowser* browser,
                                                 const char* extension_id,
                                                 WrenUIText text,
                                                 char* buffer,
                                                 size_t size);

// WREN_UI_TEXT_TOOLTIP: the button's tooltip; _DETAILS: the request-access
// button's.
WREN_API size_t Wren_CopyExtensionsButtonText(WrenBrowser* browser,
                                              WrenUIText text,
                                              char* buffer,
                                              size_t size);

// --- Zoom state -------------------------------------------------------------

// The extension that set the level last (chrome.tabs.setZoom), as Chrome's
// zoom bubble names it: WREN_UI_TEXT_ID its ID, _LABEL its name; "" if the
// user set it.
WREN_API size_t Wren_CopyZoomText(WrenBrowser* browser,
                                  WrenUIText text,
                                  char* buffer,
                                  size_t size);

// --- Bookmark state ---------------------------------------------------------

typedef struct WrenBookmarkState {
  WrenPageToken page;
  // The page is bookmarked by the user.
  bool bookmarked;
  // The folder holding the bookmark, as an index in the folder list.
  size_t folder;
} WrenBookmarkState;

// WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a page.
WREN_API WrenResult Wren_GetBookmarkState(WrenBrowser* browser,
                                          WrenBookmarkState* state);
// WREN_UI_TEXT_TITLE: the bookmark's title; "" when not bookmarked.
WREN_API size_t Wren_CopyBookmarkText(WrenBrowser* browser,
                                      WrenUIText text,
                                      char* buffer,
                                      size_t size);

// The folder list of Chrome's bookmark bubble: recently used folders and the
// main ones, under headings when the user has account and device bookmarks,
// then a separator and "Choose another folder…".
typedef enum WrenBookmarkFolderKind {
  WREN_BOOKMARK_FOLDER = 0,
  WREN_BOOKMARK_HEADING = 1,
  WREN_BOOKMARK_SEPARATOR = 2,
  // Opens Chromium's bookmark editor to pick another folder.
  WREN_BOOKMARK_CHOOSE_ANOTHER = 3,
} WrenBookmarkFolderKind;

// 0 when the page is not bookmarked.
WREN_API size_t Wren_GetBookmarkFolderCount(WrenBrowser* browser);
WREN_API WrenBookmarkFolderKind Wren_GetBookmarkFolderKind(WrenBrowser* browser,
                                                           size_t index);
WREN_API size_t Wren_CopyBookmarkFolderText(WrenBrowser* browser,
                                            size_t index,
                                            char* buffer,
                                            size_t size);

// Moves the bookmark to the folder at `index`; a heading or separator is
// WREN_ERROR_INVALID_ARGUMENT, and WREN_BOOKMARK_CHOOSE_ANOTHER opens
// Chromium's editor.
WREN_API WrenResult Wren_SetBookmarkFolder(WrenBrowser* browser,
                                           WrenPageToken page,
                                           size_t index);

// --- Content setting content ------------------------------------------------
//
// What a content setting icon's bubble shows (WrenToolbar.h's content setting
// icons), as data: the blocked popups, the choice to allow them, its links.
// Wren builds Chromium's model on the first read of an icon, for the active
// tab's page, and keeps the app's choice until the page changes. Changes are
// WREN_CHANGE_CONTENT_SETTINGS. An icon that is not shown is
// WREN_ERROR_NOT_ALLOWED.

typedef enum WrenManageStyle {
  WREN_MANAGE_NONE = 0,
  WREN_MANAGE_BUTTON = 1,
  WREN_MANAGE_CHECKBOX = 2,
  // A row that opens the settings; Chrome then shows no Done button.
  WREN_MANAGE_LINK = 3,
} WrenManageStyle;

typedef struct WrenContentSettingContent {
  WrenPageToken page;
  // Listed items, such as each blocked popup.
  size_t item_count;
  // The choices ("Always allow pop-ups and redirects from …", "Continue
  // blocking"), the selected one, and whether the user may change it (not
  // set by policy).
  size_t choice_count;
  size_t choice;
  bool choice_enabled;
  bool has_link;
  WrenManageStyle manage;
  bool has_learn_more;
} WrenContentSettingContent;

WREN_API WrenResult
Wren_GetContentSettingContent(WrenBrowser* browser,
                              WrenContentSetting setting,
                              WrenContentSettingContent* content);
// WREN_UI_TEXT_TITLE, _DETAILS (the subtitle), _MESSAGE, _LINK, _MANAGE,
// _TOOLTIP (the manage control's), _ACCEPT (Done) and _DECLINE (Cancel; ""
// if none).
WREN_API size_t Wren_CopyContentSettingText(WrenBrowser* browser,
                                            WrenContentSetting setting,
                                            WrenUIText text,
                                            char* buffer,
                                            size_t size);
// An item's WREN_UI_TEXT_TITLE and _DETAILS.
WREN_API size_t Wren_CopyContentSettingItemText(WrenBrowser* browser,
                                                WrenContentSetting setting,
                                                size_t index,
                                                WrenUIText text,
                                                char* buffer,
                                                size_t size);
WREN_API size_t Wren_CopyContentSettingChoiceText(WrenBrowser* browser,
                                                  WrenContentSetting setting,
                                                  size_t index,
                                                  char* buffer,
                                                  size_t size);
// Selects a choice; it applies on WREN_CONTENT_SETTING_DONE, or when the
// app's popover for the icon closes, as in Chrome.
WREN_API WrenResult Wren_SelectContentSettingChoice(WrenBrowser* browser,
                                                    WrenPageToken page,
                                                    WrenContentSetting setting,
                                                    size_t index);

typedef enum WrenContentSettingAction {
  // Opens the item at `item`, such as a blocked popup.
  WREN_CONTENT_SETTING_ITEM = 0,
  WREN_CONTENT_SETTING_LINK = 1,
  // The manage button or row (Chromium's settings), or ticks the checkbox.
  WREN_CONTENT_SETTING_MANAGE = 2,
  WREN_CONTENT_SETTING_LEARN_MORE = 3,
  // Applies the choice, as Chrome's Done.
  WREN_CONTENT_SETTING_DONE = 4,
  WREN_CONTENT_SETTING_CANCEL = 5,
} WrenContentSettingAction;

WREN_API WrenResult Wren_ContentSettingAction(WrenBrowser* browser,
                                              WrenPageToken page,
                                              WrenContentSetting setting,
                                              WrenContentSettingAction action,
                                              size_t item);

// --- Prompt data ------------------------------------------------------------

// The request's texts:
// - every kind: WREN_UI_TEXT_TITLE;
// - passwords: _SITE (when Chrome names the site), _USERNAME (SAVE, UPDATE,
//   AUTO_SIGNIN);
// - an address: _DETAILS (the explanation), _MESSAGE (the address, in
//   Chromium's lines), _ACCEPT, _DECLINE, _FOOTER; for an update, _LABEL is
//   the address as saved now;
// - a card: _MESSAGE (the explanation), _LABEL (the network, such as "Visa"),
//   _DETAILS (the last four digits), _USERNAME (the name on it), _ACCEPT and
//   _DECLINE;
// - a permission request: _TITLE ("example.com wants to") and _SITE (the site
//   as Chrome names it there);
// - a toast: _TITLE (its text, "Link copied") and _ACCEPT (its button's, ""
//   with none).
WREN_API size_t Wren_CopyPopoverText(WrenPopoverID popover,
                                     WrenUIText text,
                                     char* buffer,
                                     size_t size);

// Translating the page.

// A language's name, in the source list or the target list.
WREN_API size_t Wren_CopyTranslateLanguage(WrenPopoverID popover,
                                           bool target,
                                           size_t index,
                                           char* buffer,
                                           size_t size);

// A permission request.

// A requested permission's WREN_UI_TEXT_ID (Chromium's content setting name:
// "geolocation") and _LABEL (Chrome's line for it: "Know your location").
WREN_API size_t Wren_CopyPermissionText(WrenPopoverID popover,
                                        size_t index,
                                        WrenUIText text,
                                        char* buffer,
                                        size_t size);

// A toast's menu item's label ("" for a separator).
WREN_API size_t Wren_CopyToastMenuItemText(WrenPopoverID popover,
                                           size_t index,
                                           char* buffer,
                                           size_t size);

// --- Downloads (WrenDownloads.h) --------------------------------------------

// WREN_UI_TEXT_STATE: Chrome's status line for it ("2.1/10 MB, 3 secs
// left", "Failed - Network error", "Done"); _MESSAGE: its warning, for one
// Chrome warns about ("This file is dangerous, so Chromium has blocked it").
WREN_API size_t Wren_CopyDownloadText(WrenDownloadID download,
                                      WrenUIText text,
                                      char* buffer,
                                      size_t size);

// Chrome's sentences for a prompt before closing or quitting with downloads
// in progress, valid until it is answered: WREN_UI_TEXT_TITLE ("Download is
// in progress"), _DETAILS ("Quit Incognito mode anyway?"), _ACCEPT (the
// button that cancels them: "Quit") and _DECLINE (the one that keeps them:
// "Continue Downloading").
WREN_API size_t Wren_CopyDownloadsPromptText(WrenDownloadsPromptID prompt,
                                             WrenUIText text,
                                             char* buffer,
                                             size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_UNSTABLE_UI_H_

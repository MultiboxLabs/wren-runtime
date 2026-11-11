// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's popovers as Chrome draws them (C API). UNSTABLE. Wren.h
// includes this header. Wren.h also explains threads, handles and strings.
// WrenUI.h has the requests and the state that this header adds to.
//
// What is here follows Chrome's bubbles, and Chrome redesigns its bubbles. It
// has two kinds of thing:
//
// - Chrome's sentences, in Chromium's language. They are WrenUIText and every
//   Wren_Copy...Text call.
// - The layout of particular bubbles: the two designs of the extensions menu,
//   the bookmark bubble's list of folders, and the content setting bubble.
//   Each also has its facts in WrenUI.h.
//
// This header may change in any release of Wren, minor releases included.
// WREN_UNSTABLE_UI_VERSION changes with it. Wren_Main refuses an app built
// against another version (WrenStartup.unstable_ui_version), as it refuses
// another WREN_API_VERSION. What may not change is in WrenUI.h. The
// Compatibility guide has the details.

#ifndef WREN_PUBLIC_WREN_UNSTABLE_UI_H_
#define WREN_PUBLIC_WREN_UNSTABLE_UI_H_

#include "Wren.h"

// The version of this header. Set WrenStartup.unstable_ui_version to it.
#define WREN_UNSTABLE_UI_VERSION 5

#ifdef __cplusplus
extern "C" {
#endif

// The texts a state or request item offers. Each call below says which texts
// it has. Asking it for any other copies "".
typedef enum WrenUIText {
  // A one-line summary, such as "Connection is secure".
  WREN_UI_TEXT_TITLE = 0,
  // More about it, in a sentence or two.
  WREN_UI_TEXT_DETAILS = 1,
  // The site, as Chromium names it. That is its host, with the port if it is
  // not the scheme's default, or an organization's name.
  WREN_UI_TEXT_SITE = 2,
  // Chromium's name for an item, for code, such as "geolocation" or
  // "usb-chooser-data".
  WREN_UI_TEXT_ID = 3,
  // The item's name for people, such as "Location" or a device's name.
  WREN_UI_TEXT_LABEL = 4,
  // The item's state for people, such as "Allowed", "Ask (default)" or "Using
  // now".
  WREN_UI_TEXT_STATE = 5,
  // What shows when the pointer rests on the item.
  WREN_UI_TEXT_TOOLTIP = 6,
  // A longer message, such as what was blocked.
  WREN_UI_TEXT_MESSAGE = 7,
  // A link's text.
  WREN_UI_TEXT_LINK = 8,
  // The text of a manage button or checkbox, such as "Manage".
  WREN_UI_TEXT_MANAGE = 9,
  // The text of the button that accepts, such as Done or Save, and of the one
  // that declines, such as Cancel or No thanks.
  WREN_UI_TEXT_ACCEPT = 10,
  WREN_UI_TEXT_DECLINE = 11,
  // A username, or the name on a card.
  WREN_UI_TEXT_USERNAME = 12,
  // A footer line, such as where a password or an address is saved.
  WREN_UI_TEXT_FOOTER = 13,
} WrenUIText;

// --- Site state (page info) -------------------------------------------------

// Copies the site's WREN_UI_TEXT_SITE, _TITLE or _DETAILS, in UTF-8. _TITLE is
// the security summary. Returns the length needed, including the NUL.
WREN_API size_t Wren_CopySiteUIText(WrenBrowser* browser,
                                    WrenUIText text,
                                    char* buffer,
                                    size_t size);

// Copies a permission's WREN_UI_TEXT_ID, _LABEL or _STATE.
//
// - _ID is Chromium's content setting name, as in Settings URLs, such as
//   "geolocation", "notifications" or "popups".
// - _LABEL is its name for people, such as "Location".
// - _STATE is its state for people, such as "Allowed", "Ask (default)" or
//   "Using now".
WREN_API size_t Wren_CopySitePermissionUIText(WrenBrowser* browser,
                                              size_t index,
                                              WrenUIText text,
                                              char* buffer,
                                              size_t size);

// Copies a device's WREN_UI_TEXT_ID, which is the content setting name of
// its kind, or its _LABEL, which is the device's name.
WREN_API size_t Wren_CopySiteDeviceUIText(WrenBrowser* browser,
                                          size_t index,
                                          WrenUIText text,
                                          char* buffer,
                                          size_t size);

// --- Extensions state (the extensions menu) ---------------------------------

typedef enum WrenExtensionsMessage {
  WREN_EXTENSIONS_MESSAGE_NONE = 0,
  // Extensions ask for access to the site. See
  // Wren_GetExtensionRequestCount.
  WREN_EXTENSIONS_MESSAGE_REQUESTS = 1,
  // A change applies once the page reloads. See Wren_ReloadForExtensions.
  WREN_EXTENSIONS_MESSAGE_RELOAD = 2,
} WrenExtensionsMessage;

typedef struct WrenExtensionsSiteState {
  WrenPageToken page;
  WrenExtensionsSiteSetting setting;
  // True if the user can change the setting here. It is false when the
  // setting is managed, or the site is restricted.
  bool setting_can_change;
  // The message the redesign shows below the extensions.
  WrenExtensionsMessage message;
} WrenExtensionsSiteState;

// Returns WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a
// page.
WREN_API WrenResult Wren_GetExtensionsSiteState(WrenBrowser* browser,
                                                WrenExtensionsSiteState* state);
// Copies WREN_UI_TEXT_TITLE, the site setting's label as the redesign shows
// it.
WREN_API size_t Wren_CopyExtensionsSiteUIText(WrenBrowser* browser,
                                              WrenUIText text,
                                              char* buffer,
                                              size_t size);

typedef struct WrenExtensionSiteAccess {
  WrenSiteInteraction interaction;
  // The redesign's switch for this site: whether it is shown, usable and on.
  bool switch_shown;
  bool switch_enabled;
  bool switch_on;
  // Whether the redesign can show a choice of access for the extension, its
  // current choice, and which choices are possible.
  bool can_choose;
  WrenSiteAccess access;
  bool on_click_enabled;
  bool on_site_enabled;
  bool on_all_sites_enabled;
  // True if the extension may show its requests for access in the toolbar.
  bool shows_requests;
  // True if policy installed the extension.
  bool enterprise;
} WrenExtensionSiteAccess;

// Returns WREN_ERROR_INVALID_ARGUMENT for an extension with no action here.
WREN_API WrenResult
Wren_GetExtensionSiteAccess(WrenBrowser* browser,
                            const char* extension_id,
                            WrenExtensionSiteAccess* access);
// Copies WREN_UI_TEXT_TOOLTIP, the tooltip of the redesign's switch.
WREN_API size_t Wren_CopyExtensionSiteAccessUIText(WrenBrowser* browser,
                                                   const char* extension_id,
                                                   WrenUIText text,
                                                   char* buffer,
                                                   size_t size);

// Copies WREN_UI_TEXT_TOOLTIP, the extensions button's tooltip, or _DETAILS,
// the tooltip of the button that requests access.
WREN_API size_t Wren_CopyExtensionsButtonUIText(WrenBrowser* browser,
                                                WrenUIText text,
                                                char* buffer,
                                                size_t size);

// --- Zoom state -------------------------------------------------------------

// Names the extension that set the zoom level last, through
// chrome.tabs.setZoom, as Chrome's zoom bubble names it. WREN_UI_TEXT_ID
// copies its ID, and _LABEL its name. Both copy "" if the user set the level.
WREN_API size_t Wren_CopyZoomUIText(WrenBrowser* browser,
                                    WrenUIText text,
                                    char* buffer,
                                    size_t size);

// --- Bookmark state ---------------------------------------------------------

typedef struct WrenBookmarkState {
  WrenPageToken page;
  // True if the user has bookmarked the page.
  bool bookmarked;
  // The folder holding the bookmark, as an index in the folder list.
  size_t folder;
} WrenBookmarkState;

// Returns WREN_ERROR_NAVIGATION_UNAVAILABLE before the active tab has shown a
// page.
WREN_API WrenResult Wren_GetBookmarkState(WrenBrowser* browser,
                                          WrenBookmarkState* state);
// Copies WREN_UI_TEXT_TITLE, the bookmark's title. "" when the page is not
// bookmarked.
WREN_API size_t Wren_CopyBookmarkUIText(WrenBrowser* browser,
                                        WrenUIText text,
                                        char* buffer,
                                        size_t size);

// The kinds of row in the folder list of Chrome's bookmark bubble. The list
// has recently used folders and the main ones, under headings when the user
// has both account and device bookmarks. Then come a separator and "Choose
// another folder…".
typedef enum WrenBookmarkFolderKind {
  WREN_BOOKMARK_FOLDER = 0,
  WREN_BOOKMARK_HEADING = 1,
  WREN_BOOKMARK_SEPARATOR = 2,
  // Opens Chromium's bookmark editor to pick another folder.
  WREN_BOOKMARK_CHOOSE_ANOTHER = 3,
} WrenBookmarkFolderKind;

// The number of rows in the folder list. 0 when the page is not bookmarked.
WREN_API size_t Wren_GetBookmarkFolderCount(WrenBrowser* browser);
WREN_API WrenBookmarkFolderKind Wren_GetBookmarkFolderKind(WrenBrowser* browser,
                                                           size_t index);
// Copies the row's text.
WREN_API size_t Wren_CopyBookmarkFolderText(WrenBrowser* browser,
                                            size_t index,
                                            char* buffer,
                                            size_t size);

// Moves the bookmark to the folder at `index`. WREN_BOOKMARK_CHOOSE_ANOTHER
// opens Chromium's editor instead. Returns WREN_ERROR_INVALID_ARGUMENT for a
// heading or a separator.
WREN_API WrenResult Wren_SetBookmarkFolder(WrenBrowser* browser,
                                           WrenPageToken page,
                                           size_t index);

// --- Content setting content ------------------------------------------------
//
// What the bubble of a content setting icon (WrenToolbar.h) shows, as data.
// For blocked popups, that is the popups, the choice to allow them, and the
// bubble's links.
//
// Wren builds Chromium's model on the first read of an icon, for the active
// tab's page. It keeps the app's choice until the page changes.
// WREN_CHANGE_CONTENT_SETTINGS reports changes. Calls for an icon that is not
// shown return WREN_ERROR_NOT_ALLOWED.

typedef enum WrenManageStyle {
  WREN_MANAGE_NONE = 0,
  WREN_MANAGE_BUTTON = 1,
  WREN_MANAGE_CHECKBOX = 2,
  // A row that opens the settings. Chrome then shows no Done button.
  WREN_MANAGE_LINK = 3,
} WrenManageStyle;

typedef struct WrenContentSettingContent {
  WrenPageToken page;
  // The number of listed items, such as each blocked popup.
  size_t item_count;
  // The number of choices, the selected one, and whether the user may change
  // it. The user may not when policy set it. Choices are such as "Always
  // allow pop-ups and redirects from …" and "Continue blocking".
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
// Copies one of the bubble's texts:
//
// - WREN_UI_TEXT_TITLE, and _DETAILS, which is the subtitle
// - _MESSAGE, _LINK and _MANAGE
// - _TOOLTIP, the manage control's tooltip
// - _ACCEPT, the Done button
// - _DECLINE, the Cancel button, or "" if there is none
WREN_API size_t Wren_CopyContentSettingUIText(WrenBrowser* browser,
                                              WrenContentSetting setting,
                                              WrenUIText text,
                                              char* buffer,
                                              size_t size);
// Copies an item's WREN_UI_TEXT_TITLE or _DETAILS.
WREN_API size_t Wren_CopyContentSettingItemUIText(WrenBrowser* browser,
                                                  WrenContentSetting setting,
                                                  size_t index,
                                                  WrenUIText text,
                                                  char* buffer,
                                                  size_t size);
// Copies a choice's text.
WREN_API size_t Wren_CopyContentSettingChoiceText(WrenBrowser* browser,
                                                  WrenContentSetting setting,
                                                  size_t index,
                                                  char* buffer,
                                                  size_t size);
// Selects a choice. It applies on WREN_CONTENT_SETTING_DONE, or when the
// app's popover for the icon closes, as in Chrome.
WREN_API WrenResult Wren_SelectContentSettingChoice(WrenBrowser* browser,
                                                    WrenPageToken page,
                                                    WrenContentSetting setting,
                                                    size_t index);

typedef enum WrenContentSettingAction {
  // Opens the item at `item`, such as a blocked popup.
  WREN_CONTENT_SETTING_ITEM = 0,

  WREN_CONTENT_SETTING_LINK = 1,
  // Presses the manage button or row, which opens Chromium's settings, or
  // ticks the checkbox.
  WREN_CONTENT_SETTING_MANAGE = 2,

  WREN_CONTENT_SETTING_LEARN_MORE = 3,
  // Applies the choice, as Chrome's Done does.
  WREN_CONTENT_SETTING_DONE = 4,

  WREN_CONTENT_SETTING_CANCEL = 5,
} WrenContentSettingAction;

// Does one of the bubble's actions.
WREN_API WrenResult Wren_ContentSettingAction(WrenBrowser* browser,
                                              WrenPageToken page,
                                              WrenContentSetting setting,
                                              WrenContentSettingAction action,
                                              size_t item);

// --- Prompt data ------------------------------------------------------------

// Copies one of the request's texts. Which texts a request has depends on its
// kind:
//
// - Every kind has WREN_UI_TEXT_TITLE.
// - Passwords have _SITE, when Chrome names the site, and _USERNAME, for
//   SAVE, UPDATE and AUTO_SIGNIN.
// - An address has _DETAILS, the explanation, and _MESSAGE, the address in
//   Chromium's lines. It also has _ACCEPT, _DECLINE and _FOOTER. For an
//   update, _LABEL is the address as saved now.
// - A card has _MESSAGE, the explanation, _LABEL, the network such as
//   "Visa", _DETAILS, the last four digits, and _USERNAME, the name on it. It
//   also has _ACCEPT and _DECLINE.
// - A permission request has _TITLE, such as "example.com wants to", and
//   _SITE, the site as Chrome names it there.
// - A toast has _TITLE, its text such as "Link copied", and _ACCEPT, its
//   button's text, which is "" with no button.
WREN_API size_t Wren_CopyPopoverUIText(WrenPopoverID popover,
                                       WrenUIText text,
                                       char* buffer,
                                       size_t size);

// Translating the page.

// Copies a language's name, from the list of source languages, or of target
// languages if `target` is true.
WREN_API size_t Wren_CopyTranslateLanguage(WrenPopoverID popover,
                                           bool target,
                                           size_t index,
                                           char* buffer,
                                           size_t size);

// A permission request.

// Copies a requested permission's WREN_UI_TEXT_ID or _LABEL. _ID is
// Chromium's content setting name, such as "geolocation". _LABEL is Chrome's
// line for it, such as "Know your location".
WREN_API size_t Wren_CopyPermissionUIText(WrenPopoverID popover,
                                          size_t index,
                                          WrenUIText text,
                                          char* buffer,
                                          size_t size);

// Copies the label of a toast's menu item, or "" for a separator.
WREN_API size_t Wren_CopyToastMenuItemText(WrenPopoverID popover,
                                           size_t index,
                                           char* buffer,
                                           size_t size);

// --- Downloads (WrenDownloads.h) --------------------------------------------

// Copies a download's WREN_UI_TEXT_STATE or _MESSAGE.
//
// - _STATE is Chrome's status line for it, such as "2.1/10 MB, 3 secs left",
//   "Failed - Network error" or "Done".
// - _MESSAGE is its warning, for a download Chrome warns about, such as "This
//   file is dangerous, so Chromium has blocked it".
WREN_API size_t Wren_CopyDownloadUIText(WrenDownloadID download,
                                        WrenUIText text,
                                        char* buffer,
                                        size_t size);

// Copies Chrome's sentences for a prompt before closing or quitting with
// downloads in progress. They are valid until the prompt is answered.
//
// - WREN_UI_TEXT_TITLE, such as "Download is in progress"
// - _DETAILS, such as "Quit Incognito mode anyway?"
// - _ACCEPT, the button that cancels the downloads, such as "Quit"
// - _DECLINE, the button that keeps them, such as "Continue Downloading"
WREN_API size_t Wren_CopyDownloadsPromptUIText(WrenDownloadsPromptID prompt,
                                               WrenUIText text,
                                               char* buffer,
                                               size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_UNSTABLE_UI_H_

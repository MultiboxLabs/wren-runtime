// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: context menus (C API). Wren.h includes this header. Wren.h also
// explains threads, handles and strings.
//
// Every context menu has a WrenContextMenuID. There are two kinds:
//
// - The app's own, for its own UI, such as a right-click on its tab strip.
//   The app makes one with Wren_CreateContextMenu, at a point in one of its
//   views.
// - Chromium's, for a page. Before Chromium shows a page's context menu, Wren
//   asks the app's on_context_menu (Wren.h), with what the menu is about and
//   every item Chromium built. The app answers with Chromium's menu, its own,
//   or none, as for a popover (WrenUI.h).
//
// Wren_ShowContextMenu shows a list of items for either kind, natively. For
// Chromium's, the list may hold Chromium's items by name (WREN_MENU_ITEM_PAGE,
// WrenMenus.h), mixed with the app's, so the app can leave out, add, move or
// relabel items and keep the rest as Chromium built them. Wren_CloseContextMenu
// closes either kind.
//
// Or the app draws Chromium's menu itself, in its own UI or a web view's page.
// It reads the items with Wren_ReadContextMenu and their icons with
// Wren_CopyContextMenuIconPNG, runs the one the user chooses with
// Wren_RunContextMenuItem, and hears Chromium change an item, or the request
// close, through WREN_CHANGE_CONTEXT_MENU (Wren.h).
//
// One context menu is open at a time, app-wide. Wren_CreateContextMenu, and
// each new request of Chromium's, closes the one open, the app's or Chromium's.

#ifndef WREN_PUBLIC_WREN_CONTEXT_MENUS_H_
#define WREN_PUBLIC_WREN_CONTEXT_MENUS_H_

#include "Wren.h"
#include "WrenMenus.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Every context menu -----------------------------------------------------

// Defined in WrenMenus.h. Declared here too, for a file that includes
// WrenMenus.h first, before Wren.h has included this header.
struct WrenMenuItem;

// WrenContextMenuID and WREN_NO_CONTEXT_MENU are in Wren.h, for WrenChange.

// What the user chose in a menu Wren_ShowContextMenu showed. Strings are valid
// only during the callback.
typedef struct WrenContextMenuChoice {
  WrenContextMenuID menu;
  // The ID of the APP item the user chose, or NULL.
  const char* app_item;
  // The name of Chromium's item the user chose, or NULL. Wren has run it by
  // the time the callback runs.
  const char* chromium_item;
} WrenContextMenuChoice;

// Runs once, from the event loop, after a menu Wren_ShowContextMenu showed has
// closed. Both names are NULL if the user chose nothing: they dismissed the
// menu, the app closed it, another context menu replaced it, or its page or
// view went away.
typedef void (*WrenContextMenuCallback)(void* context,
                                        const WrenContextMenuChoice* choice);

// Makes a context menu for the app's own UI, to show at (x, y), in points
// from the top left of `view`, and sets `*menu` to its ID. It closes the
// context menu that is open, the app's or Chromium's.
//
// `view` is one of the app's views in a window, an NSView on macOS. It may be
// a web view's view (Wren_GetWebViewNativeView), a tab's view
// (Wren_GetTabView), or a view of the app's own. Over a web view's or a tab's
// view, the menu belongs to the page, as Chrome's page menu does. So the
// system's own items, such as Services, act on the page. A page's clientX and
// clientY are the point at 100% zoom.
//
// The menu stays open until it is shown and closes, Wren_CloseContextMenu
// closes it, another context menu replaces it, or its page goes away.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for a view in no window or a NULL
// `menu`. Returns WREN_ERROR_NOT_ALLOWED inside on_popover, on_context_menu
// or on_fullscreen.
WREN_API WrenResult Wren_CreateContextMenu(void* view,
                                           double x,
                                           double y,
                                           WrenContextMenuID* menu);

// Shows `items` as a native context menu for `menu`, once. `menu` is one the
// app made, or a request of Chromium's the app answered with
// WREN_POPOVER_APP. The app's menu opens at the point it was made at.
// Chromium's opens where Chromium would have opened its own.
//
// The items are APP, SEPARATOR and SUBMENU items, by WrenMenuItem's rules,
// and for Chromium's menu PAGE items too. A PAGE item names one of the
// request's items in `id`:
//
// - It keeps Chromium's command, state, icon and submenu. A NULL `label`
//   keeps Chromium's label. `disabled` disables it, but nothing enables an
//   item Chromium disabled.
// - Each of Chromium's items may appear once, anywhere, at any depth. A
//   submenu of Chromium's brings its own items, which may not appear again.
// - A name that is not in the menu is left out, so a list stays valid when
//   Chromium leaves an item out.
//
// Wren leaves out a separator at the start or end of a menu, and the second
// of two in a row.
//
// The call returns at once, and the menu opens from the event loop.
// `callback` runs once, after the menu has closed and after the chosen item
// has run. The menu has closed by then.
//
// Returns WREN_ERROR_INVALID_ARGUMENT, and `callback` does not run, for any of
// these:
//
// - no items, no callback, or an item that breaks a rule above or
//   WrenMenuItem's, or an unknown kind, key or modifier
// - submenus nested more than 4 deep, or more than 1,000 items
// - a ROLE or CHROMIUM item, or a SUBMENU with a role, which belong in the
//   main menu only
// - a PAGE item in a menu the app made, or a name twice
//
// Returns WREN_ERROR_CONTEXT_MENU_CLOSED for a menu that has closed or never
// existed. Returns WREN_ERROR_NOT_ALLOWED for a menu shown already, and
// inside on_popover, on_context_menu or on_fullscreen.
WREN_API WrenResult Wren_ShowContextMenu(WrenContextMenuID menu,
                                         const struct WrenMenuItem* items,
                                         size_t count,
                                         WrenContextMenuCallback callback,
                                         void* context);

// Closes `menu`, the app's or Chromium's, without running anything. A menu
// showing goes, and its callback reports neither name. Chromium hears its own
// menu closed.
//
// Returns WREN_ERROR_CONTEXT_MENU_CLOSED for a menu that has closed or never
// existed.
WREN_API WrenResult Wren_CloseContextMenu(WrenContextMenuID menu);

// --- Chromium's context menus -----------------------------------------------
//
// Chromium asks on_context_menu before it shows a page's context menu in one
// of the app's windows: a tab's page, a PDF among them, DevTools, a side
// panel, or an extension's popup. A web view's page draws its own menu from
// its contextmenu event, so it asks only for a text field, about Wren's
// editing menu (WrenWebView.h).
//
// on_context_menu runs inside Chromium's call, with on_popover's rules
// (WrenUI.h). Wren_ShowContextMenu and Wren_CreateContextMenu are among the
// calls it refuses, so the app shows its own menu from the event loop, for
// example from a dispatched block.
//
// A request the app answers with WREN_POPOVER_APP stays open until it is
// shown and closes, or the app closes it. Wren withdraws it when Chrome would
// close its menu: the tab is hidden, switched away from, closes or leaves its
// Browser, the page goes to another document, the web view is destroyed, the
// app quits, or another context menu opens. A withdrawn request's later calls
// return WREN_ERROR_CONTEXT_MENU_CLOSED.

// Whose page the menu is about.
typedef enum WrenContextMenuHost {
  // A tab's page, a PDF in it among them.
  WREN_CONTEXT_MENU_HOST_TAB = 0,
  // A web view's text field, with Wren's editing items.
  WREN_CONTEXT_MENU_HOST_WEB_VIEW = 1,

  WREN_CONTEXT_MENU_HOST_DEVTOOLS = 2,
  WREN_CONTEXT_MENU_HOST_SIDE_PANEL = 3,
  WREN_CONTEXT_MENU_HOST_EXTENSION_POPUP = 4,
  // Another of Chromium's pages, such as a dialog's.
  WREN_CONTEXT_MENU_HOST_OTHER = 5,
} WrenContextMenuHost;

// What asked for the menu.
typedef enum WrenContextMenuTrigger {
  // A right-click or a Control-click.
  WREN_CONTEXT_MENU_BY_MOUSE = 0,

  WREN_CONTEXT_MENU_BY_KEYBOARD = 1,
  // A touch or a long press.
  WREN_CONTEXT_MENU_BY_TOUCH = 2,

  WREN_CONTEXT_MENU_BY_OTHER = 3,
} WrenContextMenuTrigger;

// What kind of element the menu is about.
typedef enum WrenContextMenuMedia {
  WREN_CONTEXT_MENU_MEDIA_NONE = 0,
  WREN_CONTEXT_MENU_MEDIA_IMAGE = 1,
  WREN_CONTEXT_MENU_MEDIA_VIDEO = 2,
  WREN_CONTEXT_MENU_MEDIA_AUDIO = 3,
  WREN_CONTEXT_MENU_MEDIA_CANVAS = 4,
  WREN_CONTEXT_MENU_MEDIA_PLUGIN = 5,
  WREN_CONTEXT_MENU_MEDIA_FILE = 6,
} WrenContextMenuMedia;

// Bits of WrenContextMenuRequest.media_flags: the state of the media element
// the menu is about.
typedef enum WrenContextMenuMediaFlag {
  WREN_CONTEXT_MENU_MEDIA_IN_ERROR = 1u << 0,
  WREN_CONTEXT_MENU_MEDIA_PAUSED = 1u << 1,
  WREN_CONTEXT_MENU_MEDIA_MUTED = 1u << 2,
  WREN_CONTEXT_MENU_MEDIA_LOOP = 1u << 3,
  WREN_CONTEXT_MENU_MEDIA_CONTROLS = 1u << 4,
  WREN_CONTEXT_MENU_MEDIA_CAN_SAVE = 1u << 5,
  WREN_CONTEXT_MENU_MEDIA_HAS_AUDIO = 1u << 6,
  WREN_CONTEXT_MENU_MEDIA_PICTURE_IN_PICTURE = 1u << 7,
} WrenContextMenuMediaFlag;

// Bits of WrenContextMenuRequest.edit_flags: what the focused text allows.
typedef enum WrenContextMenuEditFlag {
  WREN_CONTEXT_MENU_CAN_UNDO = 1u << 0,
  WREN_CONTEXT_MENU_CAN_REDO = 1u << 1,
  WREN_CONTEXT_MENU_CAN_CUT = 1u << 2,
  WREN_CONTEXT_MENU_CAN_COPY = 1u << 3,
  WREN_CONTEXT_MENU_CAN_PASTE = 1u << 4,
  WREN_CONTEXT_MENU_CAN_DELETE = 1u << 5,
  WREN_CONTEXT_MENU_CAN_SELECT_ALL = 1u << 6,
} WrenContextMenuEditFlag;

// What one of Chromium's items is.
typedef enum WrenContextMenuItemType {
  WREN_CONTEXT_MENU_ITEM_TYPE_COMMAND = 0,
  // A command with a check mark, shown while `checked`.
  WREN_CONTEXT_MENU_ITEM_TYPE_CHECK = 1,
  // One of a group of commands, the one `checked` among them.
  WREN_CONTEXT_MENU_ITEM_TYPE_RADIO = 2,

  WREN_CONTEXT_MENU_ITEM_TYPE_SEPARATOR = 3,
  WREN_CONTEXT_MENU_ITEM_TYPE_SUBMENU = 4,
} WrenContextMenuItemType;

// The names of Chromium's items. Every item but a separator has a name,
// unique in its menu, the same in every language and from release to release.
// Match items by name, never by label.
//
// Some names end in a part that varies:
//
// - "extension:<extension ID>" is an extension's submenu, and
//   "extension:<extension ID>:<item ID>" each of its items, with the item's
//   ID as the extension gave it, or its number.
// - "open-link-in-profile-<n>" is the nth entry of Open Link as, counted from
//   1, under WREN_CONTEXT_MENU_ITEM_OPEN_LINK_IN_PROFILE, which names the
//   submenu, or the one item when there is one other Profile.
// - "spelling-suggestion-<n>", "spelling-language-<n>",
//   "open-link-with-handler-<n>" and "custom-<n>", a page's own items such as
//   DevTools', are counted from 1.
//
// A name that starts "chromium-" is an item Wren has not named yet. It may
// change in any release, so never match it.

// The page and its frames.
#define WREN_CONTEXT_MENU_ITEM_BACK "back"
#define WREN_CONTEXT_MENU_ITEM_FORWARD "forward"
#define WREN_CONTEXT_MENU_ITEM_RELOAD "reload"
#define WREN_CONTEXT_MENU_ITEM_SAVE_PAGE "save-page"
#define WREN_CONTEXT_MENU_ITEM_PRINT "print"
#define WREN_CONTEXT_MENU_ITEM_CAST "cast"
#define WREN_CONTEXT_MENU_ITEM_OPEN_IN_READING_MODE "open-in-reading-mode"
#define WREN_CONTEXT_MENU_ITEM_QR_CODE "qr-code"
#define WREN_CONTEXT_MENU_ITEM_TRANSLATE "translate"
#define WREN_CONTEXT_MENU_ITEM_EXIT_FULLSCREEN "exit-fullscreen"
#define WREN_CONTEXT_MENU_ITEM_VIEW_SOURCE "view-source"
#define WREN_CONTEXT_MENU_ITEM_VIEW_FRAME_SOURCE "view-frame-source"
#define WREN_CONTEXT_MENU_ITEM_RELOAD_FRAME "reload-frame"
#define WREN_CONTEXT_MENU_ITEM_INSPECT "inspect"
// A link.
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_NEW_TAB "open-link-new-tab"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_NEW_WINDOW "open-link-new-window"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_SPLIT_VIEW "open-link-split-view"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_INCOGNITO "open-link-incognito"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_ISOLATED "open-link-isolated"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_IN_PROFILE "open-link-in-profile"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_IN_APP "open-link-in-app"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_WITH "open-link-with"
#define WREN_CONTEXT_MENU_ITEM_OPEN_LINK_WITH_SETTINGS "open-link-with-settings"
#define WREN_CONTEXT_MENU_ITEM_ADD_LINK_TO_READING_LIST \
  "add-link-to-reading-list"
#define WREN_CONTEXT_MENU_ITEM_SAVE_LINK "save-link"
#define WREN_CONTEXT_MENU_ITEM_COPY_LINK "copy-link"
#define WREN_CONTEXT_MENU_ITEM_COPY_LINK_TEXT "copy-link-text"
#define WREN_CONTEXT_MENU_ITEM_COPY_LINK_TO_HIGHLIGHT "copy-link-to-highlight"
#define WREN_CONTEXT_MENU_ITEM_RESHARE_HIGHLIGHT "reshare-highlight"
#define WREN_CONTEXT_MENU_ITEM_REMOVE_HIGHLIGHT "remove-highlight"
// An image.
#define WREN_CONTEXT_MENU_ITEM_OPEN_IMAGE_NEW_TAB "open-image-new-tab"
#define WREN_CONTEXT_MENU_ITEM_OPEN_ORIGINAL_IMAGE_NEW_TAB \
  "open-original-image-new-tab"
#define WREN_CONTEXT_MENU_ITEM_LOAD_IMAGE "load-image"
#define WREN_CONTEXT_MENU_ITEM_SAVE_IMAGE "save-image"
#define WREN_CONTEXT_MENU_ITEM_COPY_IMAGE "copy-image"
#define WREN_CONTEXT_MENU_ITEM_COPY_IMAGE_ADDRESS "copy-image-address"
#define WREN_CONTEXT_MENU_ITEM_SEARCH_IMAGE_WITH_LENS "search-image-with-lens"
#define WREN_CONTEXT_MENU_ITEM_SEARCH_IMAGE "search-image"
// Audio, video and plugins.
#define WREN_CONTEXT_MENU_ITEM_LOOP "loop"
#define WREN_CONTEXT_MENU_ITEM_SHOW_CONTROLS "show-controls"
#define WREN_CONTEXT_MENU_ITEM_PICTURE_IN_PICTURE "picture-in-picture"
#define WREN_CONTEXT_MENU_ITEM_ROTATE_CLOCKWISE "rotate-clockwise"
#define WREN_CONTEXT_MENU_ITEM_ROTATE_COUNTERCLOCKWISE "rotate-counterclockwise"
#define WREN_CONTEXT_MENU_ITEM_OPEN_MEDIA_NEW_TAB "open-media-new-tab"
#define WREN_CONTEXT_MENU_ITEM_SAVE_MEDIA "save-media"
#define WREN_CONTEXT_MENU_ITEM_COPY_MEDIA_ADDRESS "copy-media-address"
#define WREN_CONTEXT_MENU_ITEM_VIDEO_FRAME "video-frame"
#define WREN_CONTEXT_MENU_ITEM_SAVE_VIDEO_FRAME "save-video-frame"
#define WREN_CONTEXT_MENU_ITEM_COPY_VIDEO_FRAME "copy-video-frame"
#define WREN_CONTEXT_MENU_ITEM_SEARCH_VIDEO_FRAME_WITH_LENS \
  "search-video-frame-with-lens"
#define WREN_CONTEXT_MENU_ITEM_SEARCH_VIDEO_FRAME "search-video-frame"
#define WREN_CONTEXT_MENU_ITEM_SAVE_PLUGIN "save-plugin"
// Text, editable or selected.
#define WREN_CONTEXT_MENU_ITEM_UNDO "undo"
#define WREN_CONTEXT_MENU_ITEM_REDO "redo"
#define WREN_CONTEXT_MENU_ITEM_CUT "cut"
#define WREN_CONTEXT_MENU_ITEM_COPY "copy"
#define WREN_CONTEXT_MENU_ITEM_PASTE "paste"
#define WREN_CONTEXT_MENU_ITEM_PASTE_AND_MATCH_STYLE "paste-and-match-style"
#define WREN_CONTEXT_MENU_ITEM_DELETE "delete"
#define WREN_CONTEXT_MENU_ITEM_SELECT_ALL "select-all"
#define WREN_CONTEXT_MENU_ITEM_LOOK_UP "look-up"
#define WREN_CONTEXT_MENU_ITEM_EMOJI "emoji"
#define WREN_CONTEXT_MENU_ITEM_GO_TO_URL "go-to-url"
#define WREN_CONTEXT_MENU_ITEM_SEARCH_WEB "search-web"
#define WREN_CONTEXT_MENU_ITEM_SEARCH_WEB_NEW_TAB "search-web-new-tab"
#define WREN_CONTEXT_MENU_ITEM_LANGUAGE_SETTINGS "language-settings"
#define WREN_CONTEXT_MENU_ITEM_WRITING_DIRECTION "writing-direction"
#define WREN_CONTEXT_MENU_ITEM_WRITING_DIRECTION_DEFAULT \
  "writing-direction-default"
#define WREN_CONTEXT_MENU_ITEM_WRITING_DIRECTION_LEFT_TO_RIGHT \
  "writing-direction-left-to-right"
#define WREN_CONTEXT_MENU_ITEM_WRITING_DIRECTION_RIGHT_TO_LEFT \
  "writing-direction-right-to-left"
#define WREN_CONTEXT_MENU_ITEM_SPEECH "speech"
#define WREN_CONTEXT_MENU_ITEM_START_SPEAKING "start-speaking"
#define WREN_CONTEXT_MENU_ITEM_STOP_SPEAKING "stop-speaking"
// Spelling.
#define WREN_CONTEXT_MENU_ITEM_SPELLING "spelling"
#define WREN_CONTEXT_MENU_ITEM_NO_SPELLING_SUGGESTIONS "no-spelling-suggestions"
#define WREN_CONTEXT_MENU_ITEM_ADD_TO_DICTIONARY "add-to-dictionary"
#define WREN_CONTEXT_MENU_ITEM_REMOVE_FROM_DICTIONARY "remove-from-dictionary"
#define WREN_CONTEXT_MENU_ITEM_CHECK_SPELLING_WHILE_TYPING \
  "check-spelling-while-typing"
#define WREN_CONTEXT_MENU_ITEM_SPELLING_ALL_LANGUAGES "spelling-all-languages"
#define WREN_CONTEXT_MENU_ITEM_ENHANCED_SPELL_CHECK "enhanced-spell-check"
#define WREN_CONTEXT_MENU_ITEM_SPELLING_SERVICE_SUGGESTION \
  "spelling-service-suggestion"
// Passwords.
#define WREN_CONTEXT_MENU_ITEM_SELECT_PASSWORD "select-password"
#define WREN_CONTEXT_MENU_ITEM_SUGGEST_PASSWORD "suggest-password"
#define WREN_CONTEXT_MENU_ITEM_GENERATE_PASSWORD "generate-password"
#define WREN_CONTEXT_MENU_ITEM_IMPORT_PASSWORDS "import-passwords"
#define WREN_CONTEXT_MENU_ITEM_SHOW_SAVED_PASSWORDS "show-saved-passwords"

// One of Chromium's items, as the request lists it. Strings are UTF-8, and
// valid as long as the request is. Absent ones are NULL.
typedef struct WrenContextMenuItem {
  // Its name, from the list above. NULL for a separator.
  const char* name;
  WrenContextMenuItemType type;
  // Chromium's label, in the user's language. Wren promises a label, never
  // its words.
  const char* label;

  bool enabled;
  bool checked;
  // Chromium hides it, and may show it while the menu is open.
  bool hidden;
  // A key, as WrenMenuItem.key names it, with WrenMenuModifier bits, or NULL
  // for none. A context menu shows a key beside its label but does not listen
  // for it.
  const char* key;
  uint32_t modifiers;
  // It has an icon, such as an extension's. See Wren_CopyContextMenuIconPNG.
  bool has_icon;
  // An extension's item: the extension's ID, and the item's ID as the
  // extension gave it, or its number. NULL for any other item. An extension's
  // submenu has the extension's ID only.
  const char* extension_id;
  const char* extension_item_id;
  // SUBMENU: its items, `submenu_count` of them.
  const struct WrenContextMenuItem* submenu;
  size_t submenu_count;
} WrenContextMenuItem;

// What Chromium is about to show, for on_context_menu and
// Wren_ReadContextMenu. Valid only during the call. Strings are UTF-8. Absent
// ones are NULL.
struct WrenContextMenuRequest {
  // The request's ID, for the calls below and Wren_ShowContextMenu.
  WrenContextMenuID menu;
  WrenContextMenuHost host;
  // Borrowed. The Browser of the tab, of the tab DevTools inspects, or of the
  // window holding the side panel or the popup, or NULL.
  WrenBrowser* browser;
  // TAB: the tab. Otherwise WREN_NO_TAB.
  WrenTabID tab;
  // WEB_VIEW: the web view. Otherwise NULL.
  WrenWebView* web_view;
  // Where Chromium opens its menu, in points from the top left of the page's
  // view.
  double x;
  double y;
  WrenContextMenuTrigger trigger;
  // The page's URL, and the frame's when it is not the main frame.
  const char* page_url;
  const char* frame_url;
  // The link under the pointer, and its text.
  const char* link_url;
  const char* link_text;
  // The element under the pointer: what kind it is, the URL of the image or
  // the media, and WrenContextMenuMediaFlag bits.
  WrenContextMenuMedia media;
  const char* src_url;
  uint32_t media_flags;
  // The selected text.
  const char* selected_text;
  // A text field: whether one is under the pointer, WrenContextMenuEditFlag
  // bits, and the misspelled word under the pointer.
  bool editable;
  uint32_t edit_flags;
  const char* misspelled_word;
  // Chromium's menu, as it is now: `count` items, with their submenus.
  const WrenContextMenuItem* items;
  size_t count;
};

// --- A menu the app draws ---------------------------------------------------
//
// These calls are for a request of Chromium's the app answered with
// WREN_POPOVER_APP and draws itself, instead of showing a list with
// Wren_ShowContextMenu. A menu the app made has none of Chromium's items, so
// they refuse it.
//
// While the request is open, Chromium may change an item, as it turns Copy
// Link to Highlight on once it has made the link. WREN_CHANGE_CONTEXT_MENU
// then reports the request, and Wren_ReadContextMenu gives the menu as it is
// now. The same change reports that the request closed, however it closed.

// Runs Chromium's item `name`, as a click on it in Chromium's menu would, and
// closes the request. As in Chromium's own menu, Chromium hears the menu close
// first, then runs the item.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for a menu the app made, a name the
// menu does not have, or a submenu. Returns WREN_ERROR_NOT_ALLOWED for an item
// that is off or hidden, for a request Wren_ShowContextMenu has shown, and
// inside on_popover, on_context_menu or on_fullscreen. Returns
// WREN_ERROR_CONTEXT_MENU_CLOSED for a menu that has closed or never existed.
WREN_API WrenResult Wren_RunContextMenuItem(WrenContextMenuID menu,
                                            const char* name);

// Gets the request as it is now, with `context`. Runs before
// Wren_ReadContextMenu returns. The request is valid only during the call.
typedef void (*WrenContextMenuReadCallback)(
    void* context,
    const WrenContextMenuRequest* request);

// Calls `read` with the request as it is now, as on_context_menu got it but
// for Chromium's changes since.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for a menu the app made, or no `read`.
// Returns WREN_ERROR_CONTEXT_MENU_CLOSED for a menu that has closed or never
// existed.
WREN_API WrenResult Wren_ReadContextMenu(WrenContextMenuID menu,
                                         WrenContextMenuReadCallback read,
                                         void* context);

// Copies the icon of Chromium's item `name` as a PNG into `buffer` if it fits,
// and returns the PNG's size in bytes. The PNG is at 2x, to draw at 16 points,
// in the page's colours. Returns 0 for an item without an icon, a name the
// menu does not have, a menu the app made, or a menu that has closed.
WREN_API size_t Wren_CopyContextMenuIconPNG(WrenContextMenuID menu,
                                            const char* name,
                                            uint8_t* buffer,
                                            size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_CONTEXT_MENUS_H_

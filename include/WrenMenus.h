// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: menus the app describes as data (C API). Wren.h includes this
// header; see Wren.h for threading and handles.
//
// A menu is an array of WrenMenuItem that Wren copies, with submenus as
// arrays of their own. Wren shows it natively and tells the app which of
// its items was chosen, by the ID the app gave it. A main menu may also
// hold the system's items (ROLE) and Chromium's commands (CHROMIUM).

#ifndef WREN_PUBLIC_WREN_MENUS_H_
#define WREN_PUBLIC_WREN_MENUS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum WrenMenuItemKind {
  // The app's own: reported by its ID when chosen.
  WREN_MENU_ITEM_APP = 0,
  WREN_MENU_ITEM_SEPARATOR = 1,
  WREN_MENU_ITEM_SUBMENU = 2,
  // Main menu only: what the system does (`role`), such as Copy or Quit.
  WREN_MENU_ITEM_ROLE = 3,
  // Main menu only: a Chromium command (`command`) for the Browser in the
  // key window. Wren enables it as Chrome enables its own; with no
  // Browser in the key window it is off.
  WREN_MENU_ITEM_CHROMIUM = 4,
} WrenMenuItemKind;

// What a ROLE item does, or which of the system's menus a SUBMENU is.
typedef enum WrenMenuRole {
  WREN_MENU_ROLE_NONE = 0,
  // The standard About panel.
  WREN_MENU_ROLE_ABOUT = 1,
  WREN_MENU_ROLE_HIDE = 2,
  WREN_MENU_ROLE_HIDE_OTHERS = 3,
  WREN_MENU_ROLE_SHOW_ALL = 4,
  WREN_MENU_ROLE_QUIT = 5,
  // These act on the focused text: a page's field, or the app's own.
  WREN_MENU_ROLE_UNDO = 6,
  WREN_MENU_ROLE_REDO = 7,
  WREN_MENU_ROLE_CUT = 8,
  WREN_MENU_ROLE_COPY = 9,
  WREN_MENU_ROLE_PASTE = 10,
  WREN_MENU_ROLE_PASTE_AND_MATCH_STYLE = 11,
  WREN_MENU_ROLE_DELETE = 12,
  WREN_MENU_ROLE_SELECT_ALL = 13,
  // These act on the key window.
  WREN_MENU_ROLE_CLOSE_WINDOW = 14,
  WREN_MENU_ROLE_MINIMIZE = 15,
  WREN_MENU_ROLE_ZOOM = 16,
  WREN_MENU_ROLE_TOGGLE_FULL_SCREEN = 17,
  WREN_MENU_ROLE_BRING_ALL_TO_FRONT = 18,
  WREN_MENU_ROLE_START_SPEAKING = 19,
  WREN_MENU_ROLE_STOP_SPEAKING = 20,
  // SUBMENU only: the menus the system fills or searches. SERVICES is
  // given no items; WINDOW lists the app's windows after its own items;
  // HELP gets the system's search field.
  WREN_MENU_ROLE_SERVICES = 21,
  WREN_MENU_ROLE_WINDOW = 22,
  WREN_MENU_ROLE_HELP = 23,
} WrenMenuRole;

// A Chromium command for a CHROMIUM item: Chrome's own main menu's, for
// one Browser. Commands that open a Browser of Chromium's (New Window) are
// not here: the app makes its Browsers.
typedef enum WrenChromiumCommand {
  WREN_CHROMIUM_COMMAND_NONE = 0,
  // Tabs.
  WREN_CHROMIUM_COMMAND_NEW_TAB = 1,
  WREN_CHROMIUM_COMMAND_NEW_TAB_TO_RIGHT = 2,
  WREN_CHROMIUM_COMMAND_CLOSE_TAB = 3,
  WREN_CHROMIUM_COMMAND_REOPEN_CLOSED_TAB = 4,
  WREN_CHROMIUM_COMMAND_DUPLICATE_TAB = 5,
  // Pin, mute and group the active tab; checked while it is.
  WREN_CHROMIUM_COMMAND_PIN_TAB = 6,
  WREN_CHROMIUM_COMMAND_MUTE_SITE = 7,
  WREN_CHROMIUM_COMMAND_GROUP_TAB = 8,
  WREN_CHROMIUM_COMMAND_SELECT_NEXT_TAB = 9,
  WREN_CHROMIUM_COMMAND_SELECT_PREVIOUS_TAB = 10,
  WREN_CHROMIUM_COMMAND_CLOSE_OTHER_TABS = 11,
  WREN_CHROMIUM_COMMAND_CLOSE_TABS_TO_RIGHT = 12,
  WREN_CHROMIUM_COMMAND_NEW_SPLIT_TAB = 13,
  // The active tab's page.
  WREN_CHROMIUM_COMMAND_BACK = 14,
  WREN_CHROMIUM_COMMAND_FORWARD = 15,
  WREN_CHROMIUM_COMMAND_RELOAD = 16,
  WREN_CHROMIUM_COMMAND_FORCE_RELOAD = 17,
  WREN_CHROMIUM_COMMAND_STOP = 18,
  WREN_CHROMIUM_COMMAND_HOME = 19,
  WREN_CHROMIUM_COMMAND_ACTUAL_SIZE = 20,
  WREN_CHROMIUM_COMMAND_ZOOM_IN = 21,
  WREN_CHROMIUM_COMMAND_ZOOM_OUT = 22,
  WREN_CHROMIUM_COMMAND_BOOKMARK_THIS_TAB = 23,
  WREN_CHROMIUM_COMMAND_VIEW_SOURCE = 24,
  // Opens DevTools, or closes it where it is open.
  WREN_CHROMIUM_COMMAND_DEVELOPER_TOOLS = 25,
  WREN_CHROMIUM_COMMAND_JAVASCRIPT_CONSOLE = 26,
  WREN_CHROMIUM_COMMAND_INSPECT_ELEMENTS = 27,
  // Chromium's own pages, in a new tab.
  WREN_CHROMIUM_COMMAND_HISTORY = 28,
  WREN_CHROMIUM_COMMAND_DOWNLOADS = 29,
  WREN_CHROMIUM_COMMAND_BOOKMARK_MANAGER = 30,
  WREN_CHROMIUM_COMMAND_EXTENSIONS = 31,
  WREN_CHROMIUM_COMMAND_CLEAR_BROWSING_DATA = 32,
  WREN_CHROMIUM_COMMAND_TASK_MANAGER = 33,
  // Cast…: opens Cast's dialog for the active tab, as Chrome's View menu
  // does; disabled where Chrome disables it (Cast turned off by policy, a
  // dialog open over the page).
  WREN_CHROMIUM_COMMAND_CAST = 34,
  // Find…, Find Next and Find Previous, as in Chrome's Edit menu. Each
  // opens a find (WREN_POPOVER_FIND); the next two also step it.
  WREN_CHROMIUM_COMMAND_FIND = 35,
  WREN_CHROMIUM_COMMAND_FIND_NEXT = 36,
  WREN_CHROMIUM_COMMAND_FIND_PREVIOUS = 37,
} WrenChromiumCommand;

// Bits of WrenMenuItem.modifiers.
typedef enum WrenMenuModifier {
  // Command on macOS, Control elsewhere.
  WREN_MENU_MODIFIER_PRIMARY = 1u << 0,
  WREN_MENU_MODIFIER_SHIFT = 1u << 1,
  // Option on macOS.
  WREN_MENU_MODIFIER_ALT = 1u << 2,
  // Control on macOS, beside WREN_MENU_MODIFIER_PRIMARY.
  WREN_MENU_MODIFIER_CONTROL = 1u << 3,
} WrenMenuModifier;

// One item. Zero-filled fields are the default: enabled, unchecked, no key.
// Strings are UTF-8. Fields an item's kind does not use are ignored.
typedef struct WrenMenuItem {
  WrenMenuItemKind kind;
  // APP: required, unique in the menu, 1 to 256 bytes. Reported back as
  // given when the item is chosen.
  const char* id;
  // APP and SUBMENU: required, shown as given. ROLE, CHROMIUM and a SUBMENU
  // with a role: NULL takes the label the system's or Chrome's own menu
  // gives it, in the user's language.
  const char* label;
  bool disabled;
  // APP: shown with a check mark.
  bool checked;
  // APP, ROLE and CHROMIUM: a key, as a page's KeyboardEvent.key names
  // it: one character ("t", "[", "+") or a key's name ("Enter",
  // "ArrowLeft", "F5"), with `modifiers`. An upper-case letter is the
  // letter with Shift. NULL is no key for an APP item, and the system's or
  // Chrome's own key for the others. A main menu's key runs its item,
  // while a page has focus too; a context menu shows it beside the label
  // and does not listen for it.
  const char* key;
  // WrenMenuModifier bits.
  uint32_t modifiers;
  // SUBMENU: its items; NULL with a count of 0 is an empty submenu.
  const struct WrenMenuItem* submenu;
  size_t submenu_count;
  // ROLE: what it does, ABOUT to STOP_SPEAKING. SUBMENU: NONE, or one of
  // SERVICES, WINDOW and HELP.
  WrenMenuRole role;
  // CHROMIUM.
  WrenChromiumCommand command;
} WrenMenuItem;

// --- Context menus ----------------------------------------------------------

// A context menu closed. `chosen_id` is the chosen APP item's ID, valid
// during the call, or NULL if none was chosen: the user dismissed it,
// another context menu replaced it, or its page went away.
typedef void (*WrenMenuCallback)(void* context, const char* chosen_id);

// Shows `items` as a native context menu at (x, y), in points from the top
// left of `view`: one of the app's views in a window (an NSView on macOS),
// such as a web view's (Wren_GetWebViewNativeView), a tab's
// (Wren_GetTabView) or the app's own. Over a web view's or a tab's view the
// menu belongs to its page, as Chrome's page menu does, so the system's own
// items (Services) see the page; a page's clientX and clientY are the point
// at 100% zoom.
//
// Returns at once; the menu opens from the event loop. `callback` runs
// once, from the event loop, after the menu closed and after the chosen
// item, if any, was reported. One context menu is shown at a time: a second
// call closes the first, whose callback reports NULL. If the web view is
// destroyed or the tab closes while the menu is up, it closes and reports
// NULL.
//
// WREN_ERROR_INVALID_ARGUMENT, and `callback` does not run, for a view in no
// window, no items, an item that breaks a rule above, an unknown kind, key
// or modifier, submenus nested more than 4 deep, or more than 1,000 items.
// ROLE and CHROMIUM items, and a SUBMENU with a role, are main menu only.
// WREN_ERROR_NOT_ALLOWED inside on_popover or on_fullscreen.
WREN_API WrenResult Wren_ShowContextMenu(void* view,
                                         const WrenMenuItem* items,
                                         size_t count,
                                         double x,
                                         double y,
                                         WrenMenuCallback callback,
                                         void* context);

// --- The main menu ----------------------------------------------------------

// Makes `items` the app's main menu (the menu bar on macOS), in place of
// the one Wren installed (WrenStartup.menu) or the last one set. Each item
// is a SUBMENU; the first is the app's own menu, whose title the system
// shows as the app's name. Copied. If a menu is open, the change waits
// until it closes. When an APP item is chosen, by a click or its key,
// on_menu_command runs. After this the app does not edit NSApp.mainMenu
// itself.
//
// WREN_ERROR_INVALID_ARGUMENT for an item that is not a SUBMENU at the top,
// or an item that breaks a rule of WrenMenuItem's or Wren_ShowContextMenu's
// above; the menu stays as it was. WREN_ERROR_NOT_ALLOWED inside
// on_popover or on_fullscreen.
WREN_API WrenResult Wren_SetMainMenu(const WrenMenuItem* items, size_t count);

// Changes the label, disabled, checked, key and modifiers of the main menu's
// APP item `id` to `item`'s, an APP item with the same rules, in place: an
// open menu shows the change the next time it opens. An app whose items
// follow its state (enabled while a window allows them) calls this as that
// state changes. WREN_ERROR_INVALID_ARGUMENT if no APP item of the menu
// Wren_SetMainMenu set has this ID, or for a bad `item`.
WREN_API WrenResult Wren_UpdateMainMenuItem(const char* id,
                                            const WrenMenuItem* item);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_MENUS_H_

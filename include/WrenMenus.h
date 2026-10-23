// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: menus the app describes as data (C API). Wren.h includes this header.
// Wren.h also explains threads and handles.
//
// The app describes a menu as an array of WrenMenuItem, and each submenu as an
// array of its own. Wren copies the arrays and shows the menu natively. When
// the user chooses one of the app's items, Wren reports the ID the app gave
// it. A main menu may also hold the system's items (ROLE) and Chromium's
// commands (CHROMIUM).

#ifndef WREN_PUBLIC_WREN_MENUS_H_
#define WREN_PUBLIC_WREN_MENUS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum WrenMenuItemKind {
  // The app's own item. Wren reports its ID when the user chooses it.
  WREN_MENU_ITEM_APP = 0,

  WREN_MENU_ITEM_SEPARATOR = 1,
  WREN_MENU_ITEM_SUBMENU = 2,
  // Main menu only. Does what the system does for `role`, such as Copy or
  // Quit.
  WREN_MENU_ITEM_ROLE = 3,
  // Main menu only. Runs the Chromium command `command` for the Browser in
  // the key window. Wren enables it when Chrome would enable its own. With no
  // Browser in the key window, it is disabled.
  WREN_MENU_ITEM_CHROMIUM = 4,
} WrenMenuItemKind;

// What a ROLE item does, or which of the system's menus a SUBMENU is.
typedef enum WrenMenuRole {
  WREN_MENU_ROLE_NONE = 0,
  // Shows the standard About panel.
  WREN_MENU_ROLE_ABOUT = 1,

  WREN_MENU_ROLE_HIDE = 2,
  WREN_MENU_ROLE_HIDE_OTHERS = 3,
  WREN_MENU_ROLE_SHOW_ALL = 4,
  // Quits the app as Chrome's Quit item does. It sends NSApp terminate:,
  // which Chromium handles. As with Wren_Quit, a page's beforeunload or
  // downloads in progress can stop the quit.
  //
  // While Warn Before Quitting is on (Wren_GetQuitWarningState), its key
  // warns first, as Chrome's does, and the app may show the warning itself
  // (on_quit_prompt). A click on it never warns.
  WREN_MENU_ROLE_QUIT = 5,
  // The roles from UNDO to SELECT_ALL act on the focused text. That is a
  // field in a page, or one of the app's own.
  WREN_MENU_ROLE_UNDO = 6,
  WREN_MENU_ROLE_REDO = 7,
  WREN_MENU_ROLE_CUT = 8,
  WREN_MENU_ROLE_COPY = 9,
  WREN_MENU_ROLE_PASTE = 10,
  WREN_MENU_ROLE_PASTE_AND_MATCH_STYLE = 11,
  WREN_MENU_ROLE_DELETE = 12,
  WREN_MENU_ROLE_SELECT_ALL = 13,
  // The roles from CLOSE_WINDOW to STOP_SPEAKING act on the key window.
  WREN_MENU_ROLE_CLOSE_WINDOW = 14,
  WREN_MENU_ROLE_MINIMIZE = 15,
  WREN_MENU_ROLE_ZOOM = 16,
  WREN_MENU_ROLE_TOGGLE_FULL_SCREEN = 17,
  WREN_MENU_ROLE_BRING_ALL_TO_FRONT = 18,
  WREN_MENU_ROLE_START_SPEAKING = 19,
  WREN_MENU_ROLE_STOP_SPEAKING = 20,
  // SUBMENU only. These are the menus the system fills or searches.
  //
  // - SERVICES takes no items of the app's.
  // - WINDOW lists the app's windows after the app's own items.
  // - HELP gets the system's search field.
  WREN_MENU_ROLE_SERVICES = 21,
  WREN_MENU_ROLE_WINDOW = 22,
  WREN_MENU_ROLE_HELP = 23,
} WrenMenuRole;

// A Chromium command for a CHROMIUM item. These are the commands of Chrome's
// own main menu that act on one Browser. Commands that open a Browser, such as
// New Window, are left out, because the app makes its own Browsers.
typedef enum WrenChromiumCommand {
  WREN_CHROMIUM_COMMAND_NONE = 0,
  // Commands on tabs.
  WREN_CHROMIUM_COMMAND_NEW_TAB = 1,
  WREN_CHROMIUM_COMMAND_NEW_TAB_TO_RIGHT = 2,
  WREN_CHROMIUM_COMMAND_CLOSE_TAB = 3,
  WREN_CHROMIUM_COMMAND_REOPEN_CLOSED_TAB = 4,
  WREN_CHROMIUM_COMMAND_DUPLICATE_TAB = 5,
  // These pin, mute or group the active tab. Each is checked while the tab
  // is pinned, muted or grouped.
  WREN_CHROMIUM_COMMAND_PIN_TAB = 6,
  WREN_CHROMIUM_COMMAND_MUTE_SITE = 7,
  WREN_CHROMIUM_COMMAND_GROUP_TAB = 8,
  // More commands on tabs.
  WREN_CHROMIUM_COMMAND_SELECT_NEXT_TAB = 9,
  WREN_CHROMIUM_COMMAND_SELECT_PREVIOUS_TAB = 10,
  WREN_CHROMIUM_COMMAND_CLOSE_OTHER_TABS = 11,
  WREN_CHROMIUM_COMMAND_CLOSE_TABS_TO_RIGHT = 12,
  WREN_CHROMIUM_COMMAND_NEW_SPLIT_TAB = 13,
  // Commands on the active tab's page.
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
  // Opens DevTools, or closes it if it is open.
  WREN_CHROMIUM_COMMAND_DEVELOPER_TOOLS = 25,

  WREN_CHROMIUM_COMMAND_JAVASCRIPT_CONSOLE = 26,
  WREN_CHROMIUM_COMMAND_INSPECT_ELEMENTS = 27,
  // These open Chromium's own pages in a new tab.
  WREN_CHROMIUM_COMMAND_HISTORY = 28,
  WREN_CHROMIUM_COMMAND_DOWNLOADS = 29,
  WREN_CHROMIUM_COMMAND_BOOKMARK_MANAGER = 30,
  WREN_CHROMIUM_COMMAND_EXTENSIONS = 31,
  WREN_CHROMIUM_COMMAND_CLEAR_BROWSING_DATA = 32,
  WREN_CHROMIUM_COMMAND_TASK_MANAGER = 33,
  // Cast…, as in Chrome's View menu. Opens Cast's dialog for the active tab.
  // It is disabled where Chrome disables it, such as when policy turns Cast
  // off or a dialog is open over the page.
  WREN_CHROMIUM_COMMAND_CAST = 34,
  // Find…, Find Next and Find Previous, as in Chrome's Edit menu. Each opens
  // a find (WREN_POPOVER_FIND). Find Next and Find Previous also step it.
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
  // Control on macOS, as well as WREN_MENU_MODIFIER_PRIMARY.
  WREN_MENU_MODIFIER_CONTROL = 1u << 3,
} WrenMenuModifier;

// One menu item. A zero-filled field means the default: enabled, unchecked
// and no key. Strings are UTF-8. Wren ignores the fields an item's kind does
// not use.
typedef struct WrenMenuItem {
  WrenMenuItemKind kind;
  // Required for APP. 1 to 256 bytes, and unique in the menu. Wren reports it
  // back, as given, when the user chooses the item.
  const char* id;
  // Required for APP and SUBMENU, and shown as given. For ROLE, CHROMIUM and
  // a SUBMENU with a role, NULL gives the label the system's or Chrome's own
  // menu uses, in the user's language.
  const char* label;
  bool disabled;
  // APP only. Shows a check mark.
  bool checked;
  // APP only. The item is not shown. In a main menu its key still runs it,
  // which gives a command shown by another item a second key.
  bool hidden;
  // APP, ROLE and CHROMIUM. A key, named as a page's KeyboardEvent.key names
  // it, together with `modifiers`.
  //
  // - A key is one character, such as "t", "[" or "+", or a key's name, such
  //   as "Enter", "ArrowLeft" or "F5".
  // - An upper-case letter means the letter with Shift.
  // - NULL means no key for an APP item. For ROLE and CHROMIUM it means the
  //   key the system's or Chrome's own menu uses.
  // - A main menu's key runs its item, even while a page has focus.
  // - A context menu shows the key beside the label but does not listen for
  //   it.
  const char* key;
  // WrenMenuModifier bits.
  uint32_t modifiers;
  // SUBMENU only. Its items. NULL with a count of 0 makes an empty submenu.
  const struct WrenMenuItem* submenu;
  size_t submenu_count;
  // For ROLE, what the item does, from ABOUT to STOP_SPEAKING. For SUBMENU,
  // NONE, or one of SERVICES, WINDOW and HELP.
  WrenMenuRole role;
  // CHROMIUM only.
  WrenChromiumCommand command;
} WrenMenuItem;

// --- Context menus ----------------------------------------------------------

// Runs when a context menu has closed. `chosen_id` is the ID of the APP item
// the user chose, valid only during the call. It is NULL if the user chose
// nothing: they dismissed the menu, another context menu replaced it, or its
// page went away.
typedef void (*WrenMenuCallback)(void* context, const char* chosen_id);

// Shows `items` as a native context menu at (x, y), in points from the top
// left of `view`.
//
// `view` is one of the app's views in a window, an NSView on macOS. It may be
// a web view's view (Wren_GetWebViewNativeView), a tab's view
// (Wren_GetTabView), or a view of the app's own. Over a web view's or a tab's
// view, the menu belongs to the page, as Chrome's page menu does. So the
// system's own items, such as Services, act on the page. A page's clientX and
// clientY are the point at 100% zoom.
//
// The call returns at once, and the menu opens from the event loop.
// `callback` runs once, from the event loop, after the menu has closed and
// after Wren has reported the chosen item, if any.
//
// Only one context menu shows at a time. A second call closes the first, and
// the first one's callback reports NULL. If the web view is destroyed or the
// tab closes while the menu is open, the menu closes and reports NULL.
//
// Returns WREN_ERROR_INVALID_ARGUMENT, and `callback` does not run, for any of
// these:
//
// - a view in no window, or no items
// - an item that breaks a rule above, or an unknown kind, key or modifier
// - submenus nested more than 4 deep, or more than 1,000 items
// - a ROLE or CHROMIUM item, or a SUBMENU with a role, which belong in the
//   main menu only
//
// Returns WREN_ERROR_NOT_ALLOWED inside on_popover or on_fullscreen.
WREN_API WrenResult Wren_ShowContextMenu(void* view,
                                         const WrenMenuItem* items,
                                         size_t count,
                                         double x,
                                         double y,
                                         WrenMenuCallback callback,
                                         void* context);

// --- The main menu ----------------------------------------------------------

// Makes `items` the app's main menu, the menu bar on macOS. It replaces the
// menu Wren installed (WrenStartup.menu), or the one set last.
//
// Every item at the top is a SUBMENU. The first is the app's own menu, and the
// system shows the app's name as its title. Wren copies the items. If a menu
// is open, the change waits until it closes. When the user chooses an APP
// item, by a click or by its key, on_menu_command runs.
//
// Once the app has called this, it should not edit NSApp.mainMenu itself.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for an item at the top that is not a
// SUBMENU, or an item that breaks a rule of WrenMenuItem or
// Wren_ShowContextMenu. The menu then stays as it was. Returns
// WREN_ERROR_NOT_ALLOWED inside on_popover or on_fullscreen.
WREN_API WrenResult Wren_SetMainMenu(const WrenMenuItem* items, size_t count);

// Updates the main menu's APP item `id` in place. Its label, disabled,
// checked, hidden, key and modifiers become those of `item`, which is an APP
// item with the same rules. A menu that is open shows the change the next time
// it opens.
//
// An app whose items follow its state calls this as the state changes, for
// example to enable an item only while a window allows it.
//
// Returns WREN_ERROR_INVALID_ARGUMENT if no APP item of the menu
// Wren_SetMainMenu set has this ID, or if `item` breaks a rule.
WREN_API WrenResult Wren_UpdateMainMenuItem(const char* id,
                                            const WrenMenuItem* item);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_MENUS_H_

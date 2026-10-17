# Menus

Your menus are data. You give Wren a list of items; Wren shows a native menu and
tells you which of your items was chosen, by the ID you gave it. The same list
works on every platform Wren runs on, so a browser drawn in HTML can describe
its menus in the page and leave the host only to pass them on. The C side is
[`WrenMenus.h`](../include/WrenMenus.h).

## Items

| Item         | Swift                                  | Rust                                                  | What it is                                         |
| ------------ | -------------------------------------- | ----------------------------------------------------- | -------------------------------------------------- |
| Your own     | `.item(id:label:enabled:checked:key:)` | `MenuItem::Item { id, label, enabled, checked, key }` | Reported by `id` when chosen                       |
| A separator  | `.separator`                           | `MenuItem::Separator`                                 |                                                    |
| A submenu    | `.submenu(label:role:items:)`          | `MenuItem::Submenu { label, role, items }`            | Up to four deep                                    |
| The system's | `.role(_:label:key:enabled:)`          | `MenuItem::Role { role, label, key, enabled }`        | Main menu only: Copy, Quit, Minimize, and the like |
| Chromium's   | `.chromium(_:label:key:enabled:)`      | `MenuItem::Chromium { command, label, key, enabled }` | Main menu only: a command of Chrome's own menu     |

An item's `key` is named as a page's `KeyboardEvent.key` names it: one character
(`"t"`, `"["`, `"+"`) or a key's name (`"Enter"`, `"ArrowLeft"`, `"F5"`), with
modifiers. `.primary` (`MenuModifiers::PRIMARY`) is ⌘ on macOS and Control
elsewhere; an upper-case letter is the letter with Shift. IDs are unique in a
menu, and labels are shown as given.

## The main menu

```swift
try WrenApp.setMainMenu([
    .submenu(label: "Wren Browser", items: [
        .role(.about), .separator,
        .submenu(label: nil, role: .services, items: []), .separator,
        .role(.hide), .role(.hideOthers), .role(.showAll), .separator,
        .role(.quit),
    ]),
    .submenu(label: "File", items: [
        .item(id: "new-window", label: "New Window", key: MenuKey("n")),
        .chromium(.newTab),
        .chromium(.closeTab),
        .chromium(.reopenClosedTab),
    ]),
    .submenu(label: "Edit", items: [
        .role(.undo), .role(.redo), .separator,
        .role(.cut), .role(.copy), .role(.paste), .role(.selectAll),
    ]),
    .submenu(label: "View", items: [
        .chromium(.reload), .chromium(.developerTools), .separator,
        .chromium(.actualSize), .chromium(.zoomIn), .chromium(.zoomOut),
    ]),
    .submenu(label: nil, role: .window, items: [.role(.minimize), .role(.zoom)]),
])

appDelegate.menuCommand = { id, browser in
    if id == "new-window" { openWindow() }
}
```

In Rust the calls are `MainThread`'s `set_main_menu`, `update_main_menu_item`
and `show_context_menu`, and items reach `App::menu_command`. `MenuItem::item`,
`submenu`, `role` and `chromium` make the common items:
`MenuItem::item("new-window", "New Window")`, `MenuItem::role(MenuRole::Quit)`,
`MenuItem::chromium(ChromiumCommand::NewTab)`.

- **Your items** reach `menuCommand` with their ID and the Browser in the key
  window, if any. Change one in place with
  `WrenApp.updateMainMenuItem(.item(id:…))`: a new label, enabled or checked
  state, or key.
- **The system's** (`.role`) do what the platform does for them: Copy copies in
  whatever has the keyboard, a page or your own field; Quit quits. A submenu
  with the role `.services`, `.window` or `.help` is the system's own: it fills
  Services, lists your windows in Window, and puts its search field in Help.
- **Chromium's** (`.chromium`) are Chrome's own menu's commands, for the Browser
  in the key window, and are enabled as Chrome enables them; with no Browser in
  the key window they are off. Find, Find Next and Find Previous open a find
  (the find bar, Chromium's or yours). Commands whose UI Wren does not have
  (Print) are not offered, nor those that would open a Browser you did not ask
  for (New Window).
- **Labels and keys** of the last two are Chrome's own, in the user's language,
  unless you give yours.
- **Keys** run their item while a page has focus, too: a key the page does not
  use goes to the menu. Give each key to one item: AppKit takes the second
  item's key away if another item already has it.

The call replaces the menu Wren installed at startup (`Startup.menu`), and after
it you do not edit `NSApp.mainMenu` yourself. An app that would rather build an
`NSMenu` still may, and does not call it. If a menu is open, the change waits
until it closes.

## Context menus

```swift
try WrenApp.showContextMenu([
    .item(id: "pin", label: tab.pinned ? "Unpin Tab" : "Pin Tab"),
    .submenu(label: "Add to Group", items: groups.map {
        .item(id: "group:\($0.id)", label: $0.title)
    }),
    .separator,
    .item(id: "close", label: "Close Tab", key: MenuKey("w")),
], in: webView.view, at: point) { chosen in
    // chosen is "pin", "group:…", "close", or nil.
}
```

- **Where:** `point` is in points from the top left of the view you name: a web
  view's (`webView.view`), a tab's (`tab.view`) or one of your own, in a window.
  A web view's page reads its `contextmenu` event's `clientX` and `clientY`,
  which are that point at 100% zoom. Over a web view's or a tab's view the menu
  belongs to its page, as Chrome's page menu does, so the system's own items
  (Services) see the page.
- **When:** the call returns at once and the menu opens from the event loop. The
  closure runs once, after the menu closed and after the chosen item was
  reported: with its ID, or nil if the user dismissed the menu, another context
  menu replaced it, or its web view was destroyed or its tab closed.
- **Keys** are shown beside the label, as a hint; a context menu does not listen
  for them.

A web view's page cannot call Wren itself. It sends its items and the point to
your app as a [message](messages.md); your handler shows the menu and answers
with the choice, so the page awaits it: a few lines on each side.

A text field in a web view has Wren's own editing menu, as before
([Web views](web-views.md#a-bare-page)), and a tab's page has Chromium's.

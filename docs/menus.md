# Menus

Your menus are data. You give Wren a list of items. Wren shows a native menu,
and tells you which of your items was chosen, by the ID you gave it. The same
list works on every platform Wren runs on. So a browser drawn in HTML can
describe its menus in the page, and leave the host only to pass them on. The C
side is
[`WrenMenus.h`](../include/WrenMenus.h).

## Items

| Item         | Swift                                         | Rust                                                          | What it is                                        |
| ------------ | --------------------------------------------- | ------------------------------------------------------------- | ------------------------------------------------- |
| Your own     | `.item(id:label:enabled:checked:hidden:key:)` | `MenuItem::Item { id, label, enabled, checked, hidden, key }` | Reported by `id` when chosen                      |
| A separator  | `.separator`                                  | `MenuItem::Separator`                                         |                                                   |
| A submenu    | `.submenu(label:role:items:)`                 | `MenuItem::Submenu { label, role, items }`                    | Up to four deep                                   |
| The system's | `.role(_:label:key:enabled:)`                 | `MenuItem::Role { role, label, key, enabled }`                | Main menu only. Copy, Quit, Minimize and the like |
| Chromium's   | `.chromium(_:label:key:enabled:)`             | `MenuItem::Chromium { command, label, key, enabled }`         | Main menu only. A command of Chrome's own menu    |

An item's `key` is named as a page's `KeyboardEvent.key` names it, with
modifiers. That is one character, such as `"t"`, `"["` or `"+"`, or a key's
name, such as `"Enter"`, `"ArrowLeft"` or `"F5"`. An upper-case letter means the
letter with Shift. The modifier `.primary`, `MenuModifiers::PRIMARY` in Rust, is
⌘ on macOS and Control elsewhere. IDs are unique in a menu, and labels are shown
as given.

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

In Rust the calls are on `MainThread`: `set_main_menu`, `update_main_menu_item`
and `show_context_menu`. Chosen items reach `App::menu_command`.
`MenuItem::item`, `submenu`, `role` and `chromium` make the common items, such
as `MenuItem::item("new-window", "New Window")`,
`MenuItem::role(MenuRole::Quit)` and
`MenuItem::chromium(ChromiumCommand::NewTab)`.

- **Your items** reach `menuCommand` with their ID, and the Browser in the key
  window if there is one. To change one in place, call
  `WrenApp.updateMainMenuItem(.item(id:…))` with a new label, enabled, checked
  or hidden state, or key. A `hidden` item is not shown, but its key still runs
  it. That gives a command shown by another item a second key.
- **The system's items** do what the platform does for them. Copy copies in
  whatever has the keyboard, a page or your own field. Quit quits. A submenu
  with the role `.services`, `.window` or `.help` is the system's own. The
  system fills Services, lists your windows in Window, and puts its search field
  in Help.
- **Chromium's items** are the commands of Chrome's own menu, for the Browser in
  the key window. Wren enables them when Chrome would. With no Browser in the
  key window they are disabled. Find, Find Next and Find Previous open a find,
  in Chromium's find bar or yours. Commands whose UI Wren does not have, such as
  Name Window, are not offered. Nor are commands that would open a Browser you
  did not ask for, such as New Window.
- **Labels and keys** of the system's and Chromium's items are Chrome's own, in
  the user's language, unless you give yours.
- **Keys** run their item while a page has focus too, because a key the page
  does not use goes to the menu. Give each key to one item. AppKit takes the key
  away from the second item that has it.
- **Chrome's keys outside its menu** work in a tab as in Chrome, whatever menu
  you set. ⌘← and ⌘→ go back and forward when the page does not use them. In a
  text field they move the caret. You need no item for them.

The call replaces the menu Wren installed at startup (`Startup.menu`). After it,
do not edit `NSApp.mainMenu` yourself. An app that would rather build an
`NSMenu` still may, and then does not call `setMainMenu`. If a menu is open, the
change waits until it closes.

## Asking before quitting

Chrome's Warn Before Quitting is on unless the user or policy turned it off.
While it is on, the Quit role's key, ⌘Q, warns before quitting, as in Chrome.
Choosing Quit with the pointer, the Dock's Quit, logging out and
`WrenApp.quit()` never warn. A page's `beforeunload`, or downloads in progress,
can still stop the quit afterwards.

The warning is Chromium's "Hold ⌘Q to Quit" unless you show your own. Answer
`quitPrompt` with `.app`, ask, then answer the prompt. `.none` quits without a
warning, and `.chromium`, or no `quitPrompt`, keeps Chromium's. Like `popover`,
it runs inside Chromium's call, so ask afterwards:

```swift
appDelegate.quitPrompt = { prompt in
    // ⌘Q again while the prompt is open: quit.
    if prompt.again { return .none }
    DispatchQueue.main.async {
        try? prompt.answer(quit: confirmQuit())
    }
    return .app
}
```

In Rust:

```rust
fn quit_prompt(&self, mt: MainThread, prompt: &QuitPrompt) -> PopoverAnswer {
    if prompt.again {
        return PopoverAnswer::None;
    }
    let prompt = prompt.clone();
    mt.later(move |_| {
        let _ = prompt.answer(confirm_quit());
    });
    PopoverAnswer::App
}
```

The quit waits for the answer. ⌘Q pressed again while your prompt is open asks
`quitPrompt` again, with `again` set and the same `id`, as Chrome's warning
quits on a second press. Answer `.none` to quit, and your prompt goes with the
app; any other answer keeps it open. A quit from elsewhere goes on while your
prompt is open.

The setting is state. `WrenApp.quitWarning` says whether it is on, and whether
policy sets it (`managed`). `WrenApp.setQuitWarning(_:)` changes it, and throws
`.notAllowed` when policy sets it. In Rust they are `mt.quit_warning()` and
`mt.set_quit_warning(on)`. Chrome's own menu item, in `.chromium`'s menu, and
`chrome://settings/appearance` change it too, and a `.quitWarning` change
reports every change. To offer it in your own menu, add an item of your own,
checked from `quitWarning`, as Chrome's "Warn Before Quitting (⌘Q)" above Quit.

A browser drawn in HTML can ask in its page, and send its answer back as a
[message](messages.md).

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

`point` is in points from the top left of the view you name. That view is a web
view's (`webView.view`), a tab's (`tab.view`) or one of your own, in a window. A
web view's page reads the `clientX` and `clientY` of its `contextmenu` event,
which are that point at 100% zoom. Over a web view's or a tab's view, the menu
belongs to its page, as Chrome's page menu does. So the system's own items, such
as Services, act on the page.

The call returns at once, and the menu opens from the event loop. The closure
runs once, after the menu has closed and the chosen item was reported. It gets
the item's ID, or nil if the user dismissed the menu, another context menu
replaced it, or its web view was destroyed or its tab closed.

A context menu shows keys beside the labels, as a hint. It does not listen for
them.

A web view's page cannot call Wren itself. It sends its items and the point to
your app as a [message](messages.md). Your handler shows the menu and answers
with the choice, and the page awaits it. That takes a few lines on each side.

A text field in a web view has Wren's own editing menu, as
[Web views](web-views.md#a-bare-page) describes. A tab's page has Chromium's.

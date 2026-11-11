# Menus

Your app's menus are a list of items. You give Wren Runtime the list, it shows a native menu, and it tells you which of your items was chosen. Chrome's own commands and the system's standard items can sit among yours, and work as they do in Chrome. Because a menu is only data, a browser drawn in HTML can describe its menus in the page, and the host just passes them on.

This page covers the main menu, the Dock menu, the warning before quitting, context menus, and copying and pasting from your own commands. The code is in Swift and Rust. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in C.

## Items

| Item | Swift | Rust | What it is |
| --- | --- | --- | --- |
| Your own | `.item(id:label:enabled:checked:hidden:key:)` | `MenuItem::Item` | One of your commands, reported by its `id` when chosen |
| A separator | `.separator` | `MenuItem::Separator` | A line between groups of items |
| A submenu | `.submenu(label:role:items:)` | `MenuItem::Submenu` | A menu inside a menu, up to four deep |
| The system's | `.role(_:label:key:enabled:)` | `MenuItem::Role` | A standard item, such as Copy, Minimize or Quit. Main menu only. |
| Chromium's | `.chromium(_:label:key:enabled:)` | `MenuItem::Chromium` | A command from Chrome's own menu. Main menu only. |

An item's `key` is written the way a page's `KeyboardEvent.key` names it, with modifiers. That's one character, such as `"t"`, `"["` or `"+"`, or a key's name, such as `"Enter"`, `"ArrowLeft"` or `"F5"`. An upper-case letter means the letter with Shift. The modifier `.primary` is ⌘ on a Mac, and Control elsewhere. IDs must be unique within a menu, and labels show exactly as you give them.

## The main menu

```swift
try WrenApp.setMainMenu([
    .submenu(label: "My Browser", items: [
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

delegate.menuCommand = { id, browser in
    if id == "new-window" { openWindow() }
}
```

```rust
mt.set_main_menu(&[
    MenuItem::submenu("My Browser", vec![
        MenuItem::role(MenuRole::About),
        MenuItem::Separator,
        MenuItem::role(MenuRole::Quit),
    ]),
    MenuItem::submenu("File", vec![
        MenuItem::item("new-window", "New Window"),
        MenuItem::chromium(ChromiumCommand::NewTab),
        MenuItem::chromium(ChromiumCommand::CloseTab),
    ]),
])?;
// Chosen items reach App::menu_command.
```

- **Your items** reach `menuCommand` with their ID, and the Browser in the key window if there is one. `WrenApp.updateMainMenuItem` changes one in place, with a new label, key, or enabled, checked or hidden state. A hidden item doesn't show, but its key still works, which gives a command a second key.
- **The system's items** do what the system does for them. Copy copies from whatever has the keyboard, a page or your own text field, and Quit quits. A submenu with the role `.services`, `.window` or `.help` is the system's own, so the system fills in Services, lists your windows in Window, and adds its search field to Help.
- **Chromium's items** are the commands of Chrome's own menu, for the Browser in the key window. Wren Runtime turns them on when Chrome would, and off when no Browser is in the key window. Find, Find Next and Find Previous open Chromium's find bar or yours. Commands that need UI Wren Runtime doesn't have, such as Name Window, aren't offered, and nor are commands that would open a Browser you didn't ask for, such as New Window.
- **Labels and keys** of the system's and Chromium's items are Chrome's own, in the person's language, unless you give your own.
- **Keys work while a page has focus,** because keys a page doesn't use go on to the menu. Give each key to one item, because AppKit takes it away from the second item that has it.
- **Chrome's other keys work in tabs** as in Chrome, whatever menu you set. ⌘← and ⌘→ go back and forward when the page doesn't use them, and in a text field they move the cursor. You don't need items for them.

`setMainMenu` replaces the menu Wren Runtime installed at launch. After it, don't change `NSApp.mainMenu` yourself. If you'd rather build an `NSMenu` yourself, you still can, as long as you don't call `setMainMenu`. If a menu is open when you set a new one, the change waits until it closes.

## The Dock menu

Right-clicking your app's Dock icon shows the system's items, such as the windows, Options, Show All Windows, Hide and Quit. `WrenApp.setDockMenu` adds your own above them:

```swift
try WrenApp.setDockMenu([
    .item(id: "new-window", label: "New Window"),
    .item(id: "new-incognito-window", label: "New Incognito Window"),
])
```

- **Chosen items reach `menuCommand`** with their ID, as the main menu's do, so an item can share an ID with a main menu item to share its command. Wren Runtime doesn't activate your app, so an item that shows a window calls `NSApp.activate()` itself.
- **Only plain items and separators,** which is also what a taskbar's Jump List can show on Windows. A submenu, a system or Chromium item, a key, or a disabled, checked or hidden item throws `.invalidArgument`, and the menu stays as it was.
- **An empty list clears it.** Without one, the Dock shows only the system's items. Chrome's own Dock items would open Browsers you never asked for, so Wren Runtime leaves them out.

## Asking before quitting

Chrome's Warn Before Quitting setting is on unless someone or a policy turned it off. While it's on, pressing ⌘Q warns before quitting, as in Chrome. Choosing Quit from the menu, the Dock's Quit, logging out, and `WrenApp.quit()` never warn. A page's `beforeunload`, or downloads in progress, can still stop the quit after the warning.

The warning is Chromium's "Hold ⌘Q to Quit" unless you show your own. To show yours, answer `quitPrompt` with `.app`, ask in your own way, then answer the prompt. `.none` quits without a warning, and `.chromium`, or no `quitPrompt` at all, keeps Chromium's. Like `popover`, it runs inside Chromium's call, so ask afterwards:

```swift
delegate.quitPrompt = { prompt in
    // ⌘Q again while the prompt is open: quit.
    if prompt.again { return .none }
    DispatchQueue.main.async {
        try? prompt.answer(quit: confirmQuit())
    }
    return .app
}
```

```rust
fn quit_prompt(&self, mt: MainThread, prompt: &QuitPrompt) -> PopoverAnswer {
    // ⌘Q again while the prompt is open: quit.
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

The quit waits for your answer. If someone presses ⌘Q again while your prompt is open, `quitPrompt` runs again with `again` set and the same `id`, as a second press quits past Chrome's warning. Answer `.none` to quit, and your prompt goes with the app, or anything else to keep it open. A quit from anywhere else goes ahead while your prompt is open.

`WrenApp.quitWarning` says whether the setting is on, and whether a policy sets it, in `managed`. `WrenApp.setQuitWarning(_:)` changes it, and throws `.notAllowed` when a policy sets it. Chrome's own menu item and `chrome://settings/appearance` change it too, and `.quitWarning` changes report every change. To offer the setting in your own menu, add an item of your own, checked from `quitWarning`, as Chrome has "Warn Before Quitting (⌘Q)" above Quit.

A browser drawn in HTML can ask in its page, and send the answer back as a [message](messages.md).

## Context menus

```swift
try WrenApp.showContextMenu([
    .item(id: "pin", label: try tab.state().pinned ? "Unpin Tab" : "Pin Tab"),
    .submenu(label: "Add to Group", items: groups.map {
        .item(id: "group:\($0.id)", label: $0.title)
    }),
    .separator,
    .item(id: "close", label: "Close Tab", key: MenuKey("w")),
], in: webView.view, at: point) { chosen in
    // chosen is "pin", "group:" and an ID, "close", or nil.
}
```

`point` is in points from the top left of the view you name, which can be a web view's view, a tab's view, or one of your own, in a window. In a web view's page, the `clientX` and `clientY` of the `contextmenu` event give that point, at 100% zoom. Over a web view's or a tab's view, the menu belongs to its page, as Chrome's page menu does, so the system's items, such as Services, act on the page.

The call returns at once, and the menu opens from the event loop. The closure runs once, after the menu has closed, with the chosen item's ID. It gets nil if the menu was dismissed, another context menu replaced it, its web view ended, or its tab closed.

A context menu shows keys beside the labels as a hint, but doesn't listen for them.

A web view's page can't call Wren Runtime itself, so it sends its items and the point to your app as a [message](messages.md). Your handler shows the menu and answers with the choice, which the page awaits. That's a few lines on each side.

In a web view, text fields have Wren Runtime's own editing menu, as [Web views](web-views.md#how-a-web-view-differs-from-a-tab) describes. A tab's page has Chromium's menus.

## Copying and pasting from your own commands

Commands of your own that copy or paste, such as Copy Page Address or Paste and Go, can use Chromium's clipboard, as Chrome's own commands do. So they need no platform code, and a browser drawn in HTML can do it all in its host's shared code. The system's Cut, Copy and Paste items still act on whatever has focus.

Writing replaces what the clipboard held. Give text, a URL with its title, or both. A URL on its own is also written as text, so pasting into a plain text field gives the address. Set `offTheRecord` for a copy from an incognito window, as Chromium does for a copy from an incognito page. That keeps the copy out of clipboard history and off other devices, and macOS clears it from the clipboard after 5 minutes. Set `concealed` for a password.

```swift
func copyAddress(of tab: BrowserTab, in browser: Browser) throws {
    try WrenApp.writeClipboard(ClipboardData(
        url: tab.url, urlTitle: tab.title,
        offTheRecord: browser.profile?.kind == .offTheRecord))
}
```

```rust
fn copy_address(mt: MainThread, tab: BrowserTab, browser: &Browser) -> Result<(), Error> {
    mt.write_clipboard(&ClipboardData {
        url: Some(tab.url()),
        url_title: Some(tab.title()),
        off_the_record: browser.profile().map(|p| p.kind()) == Some(ProfileKind::OffTheRecord),
        ..Default::default()
    })
}
```

Reading answers later, in a completion. Ask only for the kinds you use, and a kind the clipboard doesn't hold comes back nil. Read only when someone asks, such as from Paste and Go, because macOS may ask the person whether your app can paste. Reading to update a menu item would ask too, so leave such an item turned on. Chrome's Paste and Go reads the text first, then a URL, and does nothing for concealed data:

```swift
WrenApp.readClipboard([.text, .url]) { result in
    guard case .success(let data) = result, !data.concealed,
        let text = data.text ?? data.url
    else { return }
    go(to: text)  // As the address field does on Return.
}
```

```rust
mt.read_clipboard(ClipboardKinds::TEXT | ClipboardKinds::URL, |result| {
    let Ok(data) = result else { return };
    if data.concealed {
        return;
    }
    if let Some(text) = data.text.or(data.url) {
        go_to(&text); // As the address field does on Return.
    }
});
```

Wren Runtime reads and writes text and URLs only. HTML, rich text, images and file names aren't available yet, and nothing tells you when the clipboard changes.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `MenuItem` | `MenuItem` | `WrenMenuItem` |
| `MenuKey`, `.primary` | `MenuKey`, `MenuModifiers::PRIMARY` | `WrenMenuItem.key` |
| `WrenApp.setMainMenu` | `MainThread::set_main_menu` | `Wren_SetMainMenu` |
| `WrenApp.updateMainMenuItem` | `MainThread::update_main_menu_item` | `Wren_UpdateMainMenuItem` |
| `AppDelegate.menuCommand` | `App::menu_command` | `on_menu_command` |
| `WrenApp.setDockMenu` | `MainThread::set_dock_menu` | `Wren_SetDockMenu` |
| `AppDelegate.quitPrompt`, `QuitPrompt` | `App::quit_prompt`, `QuitPrompt` | `on_quit_prompt`, `WrenQuitPrompt` |
| `WrenApp.quitWarning`, `WrenApp.setQuitWarning` | `MainThread::quit_warning`, `MainThread::set_quit_warning` | `Wren_GetQuitWarningState`, `Wren_SetQuitWarning` |
| `WrenApp.showContextMenu` | `MainThread::show_context_menu` | `Wren_ShowContextMenu` |
| `WrenApp.writeClipboard`, `ClipboardData` | `MainThread::write_clipboard`, `ClipboardData` | `Wren_WriteClipboard`, `WrenClipboardData` |
| `WrenApp.readClipboard` | `MainThread::read_clipboard` | `Wren_ReadClipboard` |

# Windows and tabs

This page is about how a browser on Wren shows its pages. It covers the views Chromium gives you, where they go, and the windows Chromium asks you to make. Read [How a Wren app works](app-model.md) first. The code is in Swift, and the [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

Chromium decides what exists: which tabs there are, which is active, whether DevTools is docked, and whether a split is on. You decide where it goes on screen.

## A window for a Browser

For each Browser you show, make a window and do two things:

```swift
// 1. Say which view shows this Browser's tabs.
browser.attach(to: contentArea)

// 2. Put the active tab's view in it.
if let view = browser.activeTab?.view {
    contentArea.addSubview(view)
    view.frame = contentArea.bounds
}
```

`attach` matters more than it looks. It makes that window the Browser's window.

- Chromium's dialogs, permission prompts and passkey prompts appear over the view, and follow it as it moves or resizes.
- The Browser is active while the window is key, or while one of Chromium's windows over it is key, such as a bubble or a dialog. Menu commands, extensions and `chrome.windows` go by that. Read it as `browser.isActive`, and follow `.active` in `changed`.
- Draw your bars as in use by `browser.isActive`, as Chrome draws its frame. A window drawn by its key state alone looks inactive whenever page info opens.
- When Chromium wants the Browser in front, Wren makes the window key. If your app is not active, Wren activates it first, as Chrome does. Chromium wants this when a page focuses the window, an extension calls `chrome.windows.update`, the user switches to one of its tabs from another window, or Settings opens from an incognito window.

Attach every Browser you show. The view must already be in a window. Attaching again moves the Browser to the new view, and `attach(to: nil)` detaches it.

## Tab views

`tab.view` is an `NSView` that holds the tab's page. It is the same view for the whole life of the tab, even when Chromium replaces the page inside it, or the tab moves to another Browser.

Chromium draws the page only while the view is in a window and neither it nor an ancestor is hidden. A page that is not shown stops drawing. So take the views of background tabs out of the window, or hide them. The page is the view's size, so set the view's frame or its autoresizing mask. Its scale follows the window's screen.

Three things to get right:

- **Whole points.** Chromium sizes a page in whole points and drops any fraction. That leaves a one-pixel strip of whatever is behind the view along its edge. Round the frames you give tab views and the side panel view.
- **Add, then size.** Put a tab's view in the window before giving it its frame, as the snippet above does. A background tab sized first can keep drawing at its old size.
- **Not inside a translucent view.** A page inside an `NSVisualEffectView` that blends with what is behind the window is blended too, and looks washed out. Put the effect view behind the pages, as a sibling.

### Switching tabs

When `changed` reports `.tabs` for a Browser, its list of tabs or its active tab changed. Read `browser.tabs` and `browser.activeTab` again, swap the views, and give the new page keyboard focus:

```swift
try tab.activate()          // your tab strip's click
// … later, in changed, for .tabs:
show(browser.activeTab)     // swap the views
try browser.activeTab?.focus()
```

`focus()` gives focus back to the element that had it, as Chrome does on a tab switch. On the New Tab page, Chromium wants the address field to have focus instead, and calls `focusLocationBar`. So does Search the Web in Chrome's menu, with `browser.locationFocus` set to `.search`: focus the field in the default search engine's keyword mode, querying with `browser.locationText(.searchKeyword)` as the keyword, and draw its chip with `.searchKeywordName`.

### What a tab strip draws

Everything comes from the tab and its state. Each has a field in `change.tabFields` that says when to read it again.

| To draw | Read | Changes as |
| --- | --- | --- |
| Title, address | `tab.title`, `tab.url` | `.title`, `.url` |
| Favicon | `tab.faviconPNG`, a PNG at 2x to draw at 16 points | `.favicon` |
| Spinner, back and forward | `tab.state()`: `loading`, `canGoBack`, `canGoForward` | `.loading`, `.history` |
| Pinned, grouped, discarded | `tab.state()`: `pinned`, `group`, `discarded` | `.pinned`, `.group`, `.discarded` |
| The link under the pointer | `tab.state().status`, `tab.statusText` | `.status` |

Moving and pinning, with `tab.move(to:)` and `tab.setPinned`, follow Chromium's ordering rules. Pinned tabs come first, and a group's tabs stay together. So a tab may land somewhere other than where you asked. Read `tab.index` afterwards.

### A picture of a tab

A hover card, a tab switcher or an overview needs a picture of a tab's page. `tab.capture` takes one, of a tab that is shown or in the background, and gives it to the completion later. A background page draws for the picture but is not told it is shown: no `visibilitychange` fires, and it stays hidden.

```swift
// A hover card's picture, at twice the card's width in points.
tab.capture(maxWidth: 2 * 240, format: .jpeg, ignorePolicy: true) { result in
    guard case .success(let picture) = result else { return }
    card.image = NSImage(data: picture.data)
}
```

```rust
let options = CaptureOptions {
    max_width: 2 * 240,
    format: ImageFormat::Jpeg,
    ignore_policy: true,
    ..CaptureOptions::default()
};
tab.capture(&options, move |result| {
    if let Ok(picture) = result {
        show_card_picture(&picture.data);
    }
});
```

- The picture is the page as it draws it after the call, alone: no docked DevTools, none of your UI and none of Chromium's windows over it.
- `maxWidth` and `maxHeight` are pixels. The picture keeps the page's shape, fits inside both, and is never larger than the page's own pixels: its view's size at its screen's scale. A JPEG is smaller than a PNG, which suits an HTML app's `data:` URL.
- A background page has the size of its tab's view. A tab added in the background has the active tab's size. A page that leaves a `<wren-view>` element keeps the element's size while you have given its tab's view none, as an app that shows every tab in an element never does.
- A discarded or crashed tab fails with `.noImage`, as does a page that draws nothing within 5 seconds. A tab a restore left unloaded starts loading and may fail too. Ask again once it has loaded.
- The DisableScreenshots policy refuses a picture with `.notAllowed`, as Chrome refuses extensions' screenshots. `ignorePolicy` captures anyway, as Chrome's own hover cards do. Set it only for a picture that stays in your own UI, never for one you share or save.

A tab switcher shows each tab's picture at once by keeping one per tab, as Chrome keeps its own. When the active tab changes, capture the tab that was active: its page draws once for the picture while it stays hidden, as Chrome's does for its thumbnails. Keep the picture by `tab.persistentID` if it should outlast the launch, take a new one when a card opens on the tab, keep a discarded tab's last one, and drop a closed tab's.

```swift
// In the change handler, once the active tab has changed.
if let leaving = previousActive, leaving != browser.activeTab {
    leaving.capture(maxWidth: 480, format: .jpeg, ignorePolicy: true) { result in
        if case .success(let picture) = result { pictures[leaving] = picture }
    }
}
```

Chrome on a Mac shows no picture on its hover cards. On other platforms its hover card shows one only for a tab that is not active.

### Dragging tabs

A tab strip's drag and drop is the app's own code. Wren has the calls a drop needs, and Chromium's rules decide where the tabs end up.

| A drop                      | Call                                    |
| --------------------------- | --------------------------------------- |
| Reorder within the strip    | `tab.move(to:)`, then read `tab.index`  |
| A group, by its label       | `group.move(to:)`                       |
| A split, as one             | `split.move(to:)`                       |
| Onto another tab, to split  | `tab.split(with:)`                      |
| Out of a split              | `split.unsplit()`, then `tab.move(to:)` |
| Into a group                | `group.add([tab])`                      |
| Out of its group            | `BrowserTab.ungroup([tab])`             |
| Into a new group            | `BrowserTab.group(tabs)`                |
| Onto another window's strip | `browser.moveTabs(_:at:activate:)`      |
| Outside every window        | `BrowserTab.moveToNewBrowser(_:_:)`     |

Pinning, grouping or moving one side of a split alone ends the split. Move a split with `split.move(to:)` instead, and a group with `group.move(to:)`: each keeps its ID and look. Their index counts the tabs that stay where they are, and Wren makes it valid as Chrome's strip does. A group never lands inside another group or between a split's tabs, but at its nearer end. A split keeps its pinning, and joins a group it lands inside. Wren's web sample, in a release's `samples` folder, drags tabs, splits and groups in its page's tab strip.

### Moving tabs to another window

Tabs move to another Browser of the same Profile, or out into a new one. They keep their pages, their history and their IDs, and their views. A dialog over a page and docked DevTools come with it. This is what Chrome's Move Tab to New Window and its drag between windows do. Opening the address in the other window and closing the tab would lose all of it.

```swift
// A drop on another window's strip, at the slot the drop shows.
try otherBrowser.moveTabs([tab], at: slot)
otherWindow.makeKeyAndOrderFront(nil)

// Move Tab to New Window.
BrowserTab.moveToNewBrowser([tab]) { result in
    guard case .success(let browser) = result else { return }
    openWindow(for: browser)
}
```

```rust
other_browser.move_tabs(&[tab], Some(slot), true)?;

BrowserTab::move_to_new_browser(mt, &[tab], move |result| {
    if let Ok(browser) = result {
        open_window(browser);
    }
});
```

Chromium's rules decide where the tabs land, as for a move within a strip:

- Pinned tabs stay pinned and go among the other window's pinned tabs.
- A group whose tabs all move goes with them and keeps its ID. Tabs that leave others of their group behind leave the group.
- Moving either tab of a split moves both, and the split keeps its ID.
- A tab that lands between two tabs of a group joins that group.
- With `activate`, the default, the moved tabs' active one, or else the first, becomes the other window's active tab.

Wren does not bring the other window forward. Bring it forward yourself, as Chrome does. A new Browser comes to the completion, not to `AppDelegate.browserOpened`, so show it as one you create. A window whose last tab moves out closes after the call returns, and `browserClosed` reports it. Tabs move only between normal Browsers of one Profile, and an incognito Profile is its own. A move between Profiles throws `.notAllowed`, and nothing moves.

### A popup's tab

A page's popup, or an extension's popup window, has one tab and no tab strip. Chrome's Show as Tab moves that tab into a normal window. `movePopupTab` does the same into a Browser you choose, and `movePopupTabToNewBrowser` into a new one. The page does not load again. It keeps its history, its sessionStorage and `window.opener`, and a dialog over it and its DevTools come with it.

The tab cannot keep its ID across kinds of Browser. It becomes a new tab, with a new ID and a new view, which both calls give you. Its `persistentID` stays the same, so records you keep by it still find the tab. The old ID throws `.tabUnavailable` from then on.

```swift
// Show as Tab In, with the Browser the person picked.
let moved = try browser.movePopupTab(tab)
window.makeKeyAndOrderFront(nil)

// Show as Tab.
BrowserTab.movePopupTabToNewBrowser(tab) { result in
    guard case .success(let (browser, moved)) = result else { return }
    openWindow(for: browser, showing: moved)
}
```

```rust
let moved = browser.move_popup_tab(tab, None, true)?;

BrowserTab::move_popup_tab_to_new_browser(mt, tab, move |result| {
    if let Ok((browser, moved)) = result {
        open_window(browser, moved);
    }
});
```

The tab lands as `moveTabs` puts an unpinned tab, never pinned, and joins a group it lands inside. A page in full screen leaves it. Wren does not bring the window forward, and the popup, left with no tabs, closes after the call. Only the tab of a popup or an app popup moves. Any other throws `.notAllowed`. Offer the command only while the key window is a popup, as Chrome does, and decide whether to offer it in full screen, where Chrome turns it off.

### A crashed tab

When a page's process dies, the tab's view shows nothing. Chromium's "Aw, snap!" page is part of Chrome's window, which a Wren app does not have. So you draw your own. `tab.state().crash` says why: crashed, killed, or out of memory. `.crashed` in `tabFields` says when. `tab.reload()` loads the page again.

## Windows Chromium opens

Chromium opens Browsers of its own. A page calls `window.open`, an extension opens a popup window, the user undocks DevTools, or a session is restored. Each arrives in `browserOpened`, which is why that callback is required. Show it in a window, or close it:

```swift
delegate.browserOpened = { browser, type, opener in
    let features = browser.windowFeatures
    let size = browser.requestedBounds?.size ?? CGSize(width: 800, height: 600)
    let window = makeWindow(
        size: size,
        tabStrip: features.contains(.tabStrip),
        address: features.contains(.locationBar))
    …
}
```

- `browser.windowFeatures` says which parts Chrome gives this kind of window. A page's popup has a title bar and a read-only address, and no tab strip. An extension's popup window has only a title bar. A normal Browser has everything. Go by the features, not the type, because two popups can differ.
- `browser.requestedBounds` is the size the page asked for, and perhaps the position. For a restored window, it is the frame the window had.
- `browser.isActive` says how Chromium wants it shown. If it is true, show the window and make it key. If it is false, Chromium showed it without activating it, as for a window an extension opens unfocused. Order it in behind the key window, so it does not take the keyboard: `window.order(.below, relativeTo: NSApp.keyWindow?.windowNumber ?? 0)`.
- A Browser you ignore stays open with nowhere to show. If you do not want popups, close them.
- `browser.requestedWindowState` is `.minimized` for a restored window that was minimised, or for one an extension created minimised. Show the window, then miniaturise it.

## What Chromium asks of a window

Chromium asks a Browser's window to move, resize, minimise, zoom or restore. An extension calls `chrome.windows.update`, a debugging tool sets the window's bounds, or a popup's page calls `resizeTo` or `moveTo`. The window is yours, so Wren does nothing itself. It passes each request to `windowRequest`, and you do it, adapt it or ignore it. Whatever you do, Chromium then reads the window as it is, as do extensions and DevTools.

```swift
delegate.windowRequest = { browser, request in
    guard let window = windows[browser]?.window, !browser.isBackgrounded else { return }
    switch request {
    case .frame(let frame):
        if window.styleMask.contains(.fullScreen) { window.toggleFullScreen(nil) }
        window.setFrame(frame, display: true)
    case .state(.minimized): window.miniaturize(nil)
    case .state(.zoomed): if !window.isZoomed { window.zoom(nil) }
    case .state(.normal):
        if window.styleMask.contains(.fullScreen) { window.toggleFullScreen(nil) }
        if window.isMiniaturized { window.deminiaturize(nil) }
        if window.isZoomed { window.zoom(nil) }
    }
}
```

In Rust, implement `App::window_request`, which gets a `WindowRequest`:

```rust
fn window_request(&self, _: MainThread, browser: Browser, request: WindowRequest) {
    match request {
        WindowRequest::Frame { x, y, width, height } => { /* set the frame */ }
        WindowRequest::State(WindowState::Minimized) => { /* miniaturise */ }
        _ => {}
    }
}
```

- The frame is in AppKit's screen coordinates, as `requestedBounds` is.
- Wren asks only for what the window is not already, and only for a Browser attached to a view.
- A frame, or `.normal`, for a window in full screen means leaving full screen first, as Chrome does.
- A Browser in the background shares its window with the one in front. Ignore its requests, as above, or bring it forward first.
- Without the callback, every request is ignored, as before 0.3.0.
- The request runs from the event loop, after Chromium's call has returned. So `chrome.windows.update` replies with the window as it was, and a later `chrome.windows.get` reads the change.

A window the user minimises or zooms reads so to Chromium, with nothing to do from you. The session keeps a minimised window minimised, and a window in full screen at the frame it had before.

## DevTools

DevTools stays with the page it inspects.

```swift
try tab.openDevTools()            // where it was last; the right side at first
try tab.openDevTools(.bottom)     // .right, .left, .bottom, .undocked
try tab.closeDevTools()
```

Docked, DevTools is inside the tab's view. Wren lays the page and DevTools out together, and you do nothing. DevTools' own dock menu moves it between sides, or out into a window of its own. That window is a `.devtools` Browser, which arrives in `browserOpened`. `tab.state().devtools` says where DevTools is.

Only tabs of a normal Browser dock DevTools. A popup's DevTools always opens in its own window. For a web view's DevTools, see [Web views](web-views.md).

## Split tabs

A split shows two tabs of a Browser side by side, as one. It is Chromium's split view. Each side is still an ordinary tab.

```swift
let split = try first.split(with: second)       // or browser.newSplitTab()
let state = try split.state()                   // first, second, layout, ratio
try split.setRatio(0.4, dragging: true)         // while your divider is dragged
```

While either side is the active tab, show both views, divided by `state.ratio`, and draw your own divider. `.split` in `changed` reports changes. Chromium keeps the two tabs together. Closing one side ends the split, and so does moving or pinning one side alone.

## Side panels

Chromium has side panels: the reading list, bookmarks, history, reading mode, Customize Chromium, and extensions' own panels. A Browser shows one at a time, in one view, and you decide where that view goes.

```swift
let panel = browser.sidePanelView            // the same view for every panel
let state = try browser.sidePanelState()     // open, width, rightAligned
try browser.showSidePanel("ReadingList")
try browser.closeSidePanel()
```

Place `sidePanelView` beside the pages, and show it at `state.width` while `state.open` is true. Panels also open from Chromium itself, for example from the New Tab page's Customize button, or when an extension calls `chrome.sidePanel.open`. So follow `.sidePanel` in `changed`, not only your own calls. If you draw a resize handle, report the width with `browser.setSidePanelWidth` as the user drags it. Chromium remembers the width for each kind of panel.

## Full screen

A page that calls `requestFullscreen()` wants the whole window. Chromium asks you:

```swift
delegate.fullscreen = { browser, entering in
    window(for: browser).toggleFullScreen(nil)     // start the change
    return true                                    // "I will"
}

// When AppKit finishes (windowDidEnterFullScreen / windowDidExitFullScreen):
try browser.setFullscreen(true)
```

While `browser.fullscreenTab` names a tab, hide your tab strip and toolbar, and give that tab's view the whole window. Chromium shows its "Press Esc" notice, and asks you to leave when the user presses Escape. If you return `false`, or set no handler, the page leaves full screen at once.

Also call `setFullscreen` when the user takes the window into full screen themselves, with the green button or your menu.

## Dialogs that block the window

In Chrome, some of Chromium's dialogs block the whole window, such as an extension's install prompt or an uninstall confirmation. In a Wren app they are sheets of the attached view's window, as in Chrome, and so are the file panels of Open File, Save Page and a page's file field. While a dialog is open, Wren already refuses clicks below the title bar and disables Chromium's commands.

Your own UI is yours to block. While `browser.hasModalDialog` is true, disable New Tab, Close Tab, the address field and tab switching. Leave New Window and Quit working. `.modalDialog` in `changed` reports each change.

Dialogs that belong to one page, such as an alert or HTTP authentication, block only that page. Chromium handles them.

## Several Browsers in one window

A window can show several Browsers in turn, such as workspaces each on its own Profile, drawn by one web view's page. Attach every one of them to the same view, and keep all but one in the background:

```swift
personal.attach(to: webView.view)
work.attach(to: webView.view)
work.isBackgrounded = true

func show(_ next: Browser) {
    current.isBackgrounded = true      // first, so two are never in front
    next.isBackgrounded = false
    current = next
    // then tell your page which tab to show in its <wren-view>
}
```

A Browser in the background still belongs to the window. Until you bring it forward:

- It is never active.
- The menu's Chromium commands go to the Browser in front.
- Its sheets, its pages' dialogs and its bubbles are not shown, and block nothing. They wait, and show once it is in front.
- Its tabs stay open and loaded.

What your page shows in its tab elements is yours to change. Wren only decides which Browser the window answers for.

Chromium sometimes asks for a Browser to come forward. A site's notification is clicked, an extension calls `chrome.windows.update`, a page focuses a window it opened, the user switches to one of its tabs, or Settings opens from an incognito window. A Browser in the background stays there, and Wren tells you, so you decide whether to switch:

```swift
app.changed = { changes in
    for change in changes where change.kind == .foregroundRequest {
        show(change.browser!)          // or badge the workspace's button
    }
}
```

`browser.wantsForeground` stays true until the Browser is brought forward, or until you forget the request with `browser.clearForegroundRequest()`, which leaves the Browser where it is. Chromium's next request is then reported again. A session restore activates the Browser it restores last. Put a restored Browser in the background as it opens, and you get that request too, which you may forget.

Keep exactly one Browser of a window in front. Wren does not put the others in the background for you. Two in front share the window as two attached Browsers always have. Both are active, the menu reaches the one made first, and one's sheet blocks both.

### Quitting with several Browsers

When the app quits, each Browser closes in turn, and `browserClosed` reports each. A window that keeps going when one of its Browsers closes would empty itself one Browser at a time, on screen. Hide it first, in `quitCommitted`, which runs once, after every page has agreed to the quit and before the first Browser closes:

```swift
delegate.quitCommitted = {
    for window in workspaceWindows {
        window.orderOut(nil)
    }
}
```

`quitCommitted` runs inside Chromium's call, so make no Wren calls that change Browsers there.

## Restoring the last session

Chromium records every Browser's tabs as the app runs, and restores nothing until you ask.

```swift
profile.restoreLastSession { result in
    if case .success(let tabs) = result, tabs > 0 { return }   // windows arrived
    openFirstWindow()                                         // nothing to restore
}
```

Each restored window arrives in `browserOpened`, with its tabs, their order and history. Its `requestedBounds` is the frame the window had, and its `requestedWindowState` is `.minimized` if the window was minimised. `profile.lastSession` reads what there is to restore without restoring it. It is for an app that wants to ask first, as Chrome does after a crash.

A launch restores a Profile's last session once, as Chrome does. A second `restoreLastSession` fails with `.notAllowed`, so no two windows hold the same tabs.

`browser.reopenClosedTab()` is Chrome's Reopen Closed Tab, for a tab or a whole window. Enable its menu item by `browser.canReopenClosedTab`.

### Tabs the app makes itself

An app that makes some tabs again from its own records each launch, such as pinned sites, keeps them out of the session, or the restore brings them back a second time:

```swift
let tab = try browser.addTab(url: url)
try tab.setSessionExcluded(true)   // right after adding it
```

The restore leaves such a tab out, and keeps the rest in their order, with the nearest kept tab active. Reopen Closed Tab still brings it back in the same launch, but its closed tab is never written to disk. `tab.state()` says whether it is out, and `.sessionExcluded` in a tab's changes reports when that changes. A tab in a group is also in Chrome's saved copy of the group, which brings it back when the group is restored, so keep such tabs out of groups.

### A window of several Browsers

Chromium records each Browser as a window of its own. A window that shows several Browsers in turn would come back as one window per Browser. Tag each Browser with its window and its place in it, and Chromium keeps the tag in the session:

```swift
try personal.setSessionTag("\(windowID) 0")
try work.setSessionTag("\(windowID) 1")
```

A restored Browser has its tag already in `browserOpened`. Read it there, and attach the Browser to the window it names, making that window if it is the first of its Browsers to arrive:

```swift
let parts = browser.sessionTag.split(separator: " ")
if parts.count == 2, let window = windows[String(parts[0])] ?? makeWindow(String(parts[0])) {
    window.add(browser, at: Int(parts[1]) ?? 0)
}
```

Change a tag when its Browser moves, for example to record which one is in front. A tag is at most 128 bytes of UTF-8. A Browser that Reopen Closed Tab brings back has its tag too, but it comes back alone. Chromium reopens one Browser at a time, from the list of that Browser's Profile. A Browser Chromium opens from a tagged one, such as a new window, has no tag.

### Finding a restored tab's records

A tab's `id` lasts one launch. Its `persistentID` lasts as long as the tab: a restored tab has the ID it had before the quit. Key what the app keeps for each tab by it, such as a name the user gave the tab, and look the record up when the tab comes back:

```swift
names[tab.persistentID] = "Reading list"          // as the user names it
// After a restore, in browserOpened:
for tab in browser.tabs {
    if let name = names[tab.persistentID] { show(name, for: tab) }
}
```

The ID survives navigation, moves, also to another Browser, pinning, grouping and Chromium discarding the tab to save memory. Reopen Closed Tab brings it back in the same launch. A new tab and a duplicate get a new one, and so does a tab reopened from an earlier launch's closed tabs. Compare the ID whole; its form may change.

## The rest of the browser's UI

The address field with its suggestions, the site icon, extensions' buttons, and Chromium's popovers each have their own data and calls. [The UI a browser provides](ui-checklist.md) lists every piece with the API for it.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `browser.attach(to:)` | `Browser::attach_to_view` | `Wren_AttachBrowserToView` |
| `browser.isActive` | `Browser::is_active` | `Wren_IsBrowserActive` |
| `delegate.quitCommitted` | `App::quit_committed` | `on_quit_committed` |
| `browser.tabs` | `Browser::tabs` | `Wren_GetTabCount`, `Wren_GetTabAt` |
| `browser.activeTab` | `Browser::active_tab` | `Wren_GetActiveTab` |
| `tab.view` | `BrowserTab::view` | `Wren_GetTabView` |
| `tab.activate()` | `BrowserTab::activate` | `Wren_ActivateTab` |
| `tab.focus()` | `BrowserTab::focus` | `Wren_FocusTab` |
| `tab.state()` | `BrowserTab::state` | `Wren_GetTabState` |
| `tab.faviconPNG` | `BrowserTab::favicon_png` | `Wren_CopyTabFaviconPNG` |
| `tab.statusText` | `BrowserTab::status_text` | `Wren_CopyTabStatusText` |
| `tab.move(to:)` | `BrowserTab::move_to` | `Wren_MoveTab` |
| `tab.index` | `BrowserTab::index` | `Wren_GetTabIndex` |
| `tab.setPinned` | `BrowserTab::set_pinned` | `Wren_SetTabPinned` |
| `group.move(to:)` | `TabGroup::move_to` | `Wren_MoveTabGroup` |
| `split.move(to:)` | `TabSplit::move_to` | `Wren_MoveTabSplit` |
| `tab.setSessionExcluded` | `BrowserTab::set_session_excluded` | `Wren_SetTabSessionExcluded` |
| `browser.moveTabs` | `Browser::move_tabs` | `Wren_MoveTabsToBrowser` |
| `BrowserTab.moveToNewBrowser` | `BrowserTab::move_to_new_browser` | `Wren_MoveTabsToNewBrowser` |
| `browser.movePopupTab` | `Browser::move_popup_tab` | `Wren_MovePopupTabToBrowser` |
| `BrowserTab.movePopupTabToNewBrowser` | `BrowserTab::move_popup_tab_to_new_browser` | `Wren_MovePopupTabToNewBrowser` |
| `tab.reload()` | `BrowserTab::reload` | `Wren_Reload` |
| `browser.windowFeatures` | `Browser::window_features` | `Wren_GetWindowFeatures` |
| `browser.requestedBounds` | `Browser::requested_bounds` | `Wren_GetRequestedBounds` |
| `browser.requestedWindowState` | `Browser::requested_window_state` | `Wren_GetRequestedWindowState` |
| `AppDelegate.windowRequest` | `App::window_request` | `on_window_request` |
| `browser.locationFocus` | `Browser::location_focus` | `Wren_GetLocationFocus` |
| `tab.openDevTools` | `BrowserTab::open_dev_tools` | `Wren_OpenDevTools` |
| `tab.closeDevTools()` | `BrowserTab::close_dev_tools` | `Wren_CloseDevTools` |
| `tab.split(with:)` | `BrowserTab::split_with` | `Wren_SplitTabs` |
| `browser.newSplitTab()` | `Browser::new_split_tab` | `Wren_NewSplitTab` |
| `split.setRatio` | `TabSplit::set_ratio` | `Wren_SetSplitRatio` |
| `browser.sidePanelView` | `Browser::side_panel_view` | `Wren_GetSidePanelView` |
| `browser.sidePanelState()` | `Browser::side_panel_state` | `Wren_GetSidePanelState` |
| `browser.showSidePanel` | `Browser::show_side_panel` | `Wren_ShowSidePanel` |
| `browser.setSidePanelWidth` | `Browser::set_side_panel_width` | `Wren_SetSidePanelWidth` |
| `AppDelegate.fullscreen` | `App::fullscreen` | `on_fullscreen` |
| `browser.setFullscreen` | `Browser::set_fullscreen` | `Wren_SetFullscreen` |
| `browser.fullscreenTab` | `Browser::fullscreen_tab` | `Wren_GetFullscreenTab` |
| `browser.hasModalDialog` | `Browser::has_modal_dialog` | `Wren_HasModalDialog` |
| `browser.isBackgrounded` | `Browser::set_backgrounded`, `is_backgrounded` | `Wren_SetBrowserBackgrounded` |
| `browser.wantsForeground` | `Browser::wants_foreground` | `Wren_HasForegroundRequest` |
| `browser.clearForegroundRequest()` | `Browser::clear_foreground_request` | `Wren_ClearForegroundRequest` |
| `profile.restoreLastSession` | `Profile::restore_last_session` | `Wren_RestoreLastSession` |
| `profile.lastSession` | `Profile::last_session` | `Wren_GetLastSession` |
| `browser.reopenClosedTab()` | `Browser::reopen_closed_tab` | `Wren_ReopenClosedTab` |
| `browser.canReopenClosedTab` | `Browser::can_reopen_closed_tab` | `Wren_CanReopenClosedTab` |
| `browser.sessionTag` | `Browser::session_tag` | `Wren_CopyBrowserSessionTag` |
| `browser.setSessionTag(_:)` | `Browser::set_session_tag` | `Wren_SetBrowserSessionTag` |
| `tab.persistentID` | `BrowserTab::persistent_id` | `Wren_CopyTabPersistentID` |

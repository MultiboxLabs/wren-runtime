# Windows and tabs

Your browser's windows are your own. You decide what the tab strip looks like, where tabs go, and how popups, DevTools, split tabs and side panels fit in. Chromium still decides what exists: which tabs are open, which one is active, whether DevTools is docked, and whether a split is on. This page shows how to put all of it on screen.

It builds on [How a Wren Runtime app works](app-model.md). The code is in Swift, with Rust where the two differ most. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## Giving a Browser a window

Each Browser you show needs a window, and two calls put it there:

```swift
// 1. Say which view shows this Browser's tabs.
browser.attach(to: contentArea)

// 2. Put the active tab's view in it.
if let view = browser.activeTab?.view {
    contentArea.addSubview(view)
    view.frame = contentArea.bounds
}
```

Attaching makes your window the Browser's window as far as Chromium is concerned, and that gets you a lot:

- Chromium's dialogs, permission prompts and passkey prompts appear over the view, and follow it as the window moves or resizes.
- The Browser is active while its window is key, or while one of Chromium's windows over it is key, such as a bubble or a dialog. Menu commands, extensions and `chrome.windows` all go by that. `browser.isActive` reads it, and `.active` changes report it.
- When Chromium wants the Browser in front, Wren Runtime makes its window key, and activates your app first if it isn't active, as Chrome does. That happens when a page focuses its window, an extension calls `chrome.windows.update`, someone switches to one of its tabs from another window, or Settings opens from an incognito window.

Draw your toolbar as active or inactive by `browser.isActive`, as Chrome draws its frame. A window drawn by its key state alone looks inactive whenever page info or another bubble is open.

Attach every Browser you show, after its view is in a window. Attaching again moves the Browser to the new view, and `attach(to: nil)` detaches it.

## Tab views

Each tab has a view, `tab.view`, an ordinary `NSView` that shows its page. It's the same view for the tab's whole life, even when Chromium replaces the page inside it or the tab moves to another Browser.

Chromium draws a page only while its view is in a window and neither it nor any view around it is hidden. Take background tabs' views out of the window, or hide them, and their pages stop drawing. The page is the size of its view, at the scale of the window's screen.

Three things are easy to get wrong:

- **Round the frames.** Chromium sizes a page in whole points and drops any fraction, which leaves a one-pixel strip of whatever is behind the view along its edge. Give tab views and the side panel view whole-point frames.
- **Add the view, then size it.** Put a tab's view in the window before you give it a frame, as the code above does. A background tab sized before it's added can keep drawing at its old size.
- **Keep pages out of translucent views.** A page inside an `NSVisualEffectView` that blends with what's behind the window gets blended too, and looks washed out. Put the effect view behind the pages, as a sibling.

### Switching tabs

When `changed` reports `.tabs` for a Browser, its tabs or its active tab changed. Read `browser.tabs` and `browser.activeTab` again, swap in the new active tab's view, and give its page keyboard focus:

```swift
// Your tab strip's click.
try tab.activate()

// Later, in changed, for .tabs.
showTab(browser.activeTab)
try browser.activeTab?.focus()
```

`focus()` puts focus back on the element that had it, as Chrome does when you switch tabs. On the New Tab page, Chromium wants your address field to have focus instead, and calls `focusLocationBar`.

Chrome's Search the Web command calls `focusLocationBar` too, with `browser.locationFocus` set to `.search`. Focus the address field in the default search engine's keyword mode. `browser.locationText(.searchKeyword)` gives the keyword to search with, and `.searchKeywordName` the name to show on its chip.

### Drawing a tab strip

Everything a tab strip shows comes from the tab and its state. Each piece has a field in `change.tabFields` that tells you when to read it again.

| To draw | Read | Changes as |
| --- | --- | --- |
| Title and address | `tab.title`, `tab.url` | `.title`, `.url` |
| Favicon | `tab.faviconPNG`, a PNG at 2x to draw at 16 points | `.favicon` |
| Spinner, back and forward | `tab.state()`'s `loading`, `canGoBack` and `canGoForward` | `.loading`, `.history` |
| Pinned, grouped, discarded | `tab.state()`'s `pinned`, `group` and `discarded` | `.pinned`, `.group`, `.discarded` |
| The link under the pointer | `tab.state().status`, `tab.statusText` | `.status` |

### Moving and dragging tabs

Your tab strip's drag and drop is your own code, and Wren Runtime has a call for every kind of drop. Chromium's rules then decide where the tabs end up, so a tab may land somewhere other than where you dropped it. Read `tab.index` afterwards.

| The drop                        | The call                                |
| ------------------------------- | --------------------------------------- |
| Within the strip                | `tab.move(to:)`                         |
| A group, by its label           | `group.move(to:)`                       |
| A split, as one                 | `split.move(to:)`                       |
| Onto another tab, to split them | `tab.split(with:)`                      |
| Out of a split                  | `split.unsplit()`, then `tab.move(to:)` |
| Into a group                    | `group.add([tab])`                      |
| Out of its group                | `BrowserTab.ungroup([tab])`             |
| Into a new group                | `BrowserTab.group(tabs)`                |
| Onto another window's strip     | `browser.moveTabs(_:at:activate:)`      |
| Outside every window            | `BrowserTab.moveToNewBrowser(_:_:)`     |

These are Chromium's rules:

- Pinned tabs come first, and `tab.setPinned` moves a tab to match.
- A group's tabs stay together. A group never lands inside another group or between a split's tabs, but at the nearer end.
- Pinning, grouping or moving one side of a split on its own ends the split. Move a split with `split.move(to:)`, and a group with `group.move(to:)`, and each keeps its ID and look.
- A split keeps its pinning, and joins a group it lands inside.

The web sample, in a release's `samples` folder, drags tabs, splits and groups in its tab strip.

### Moving tabs to another window

Tabs can move to another Browser of the same Profile, or out into a new one, as Chrome's Move Tab to New Window and its drags between windows do. They keep their pages, history, IDs and views, and any dialog over a page or docked DevTools comes with them. Opening the same address in the other window and closing the tab would lose all of that.

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
// A drop on another window's strip, at the slot the drop shows.
other_browser.move_tabs(&[tab], Some(slot), true)?;

// Move Tab to New Window.
BrowserTab::move_to_new_browser(mt, &[tab], move |result| {
    if let Ok(browser) = result {
        open_window(browser);
    }
});
```

Chromium's rules decide where the tabs land, as for a move within a strip:

- Pinned tabs stay pinned, among the other window's pinned tabs.
- A group whose tabs all move goes with them and keeps its ID. Tabs that leave others of their group behind leave the group.
- Moving either side of a split moves both, and the split keeps its ID.
- A tab that lands between two tabs of a group joins the group.
- With `activate`, which is the default, the moved tabs' active one, or else the first, becomes the other window's active tab.

Wren Runtime doesn't bring the other window forward, so do it yourself, as Chrome does. A new Browser arrives in the completion rather than in `browserOpened`, so show it as you would one you created. A window whose last tab moves out closes after the call returns, and `browserClosed` reports it.

Tabs move only between normal Browsers of one Profile, and an incognito Profile counts as a different one. A move between Profiles throws `.notAllowed`, and nothing moves.

### Pictures of tabs

A hover card, a tab switcher or an overview needs a picture of a tab's page. `tab.capture` takes one, of a tab that's showing or in the background, and hands it to its completion.

```swift
// A hover card's picture, at twice the card's width in points.
tab.capture(maxWidth: 2 * 240, format: .jpeg, ignorePolicy: true) { result in
    guard case .success(let picture) = result else { return }
    card.image = NSImage(data: picture.data)
}
```

```rust
// A hover card's picture, at twice the card's width in points.
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

- **What's in it.** The page alone, as it draws after the call, without docked DevTools, your UI, or Chromium's windows over it. A background page draws for the picture, but stays hidden, and no `visibilitychange` fires.
- **Its size.** `maxWidth` and `maxHeight` are in pixels. The picture keeps the page's shape, fits inside both, and is never larger than the page itself, which is its view's size at its screen's scale. A background page has its view's size, and a tab added in the background has the active tab's size. A page that has left a `<wren-view>` element keeps the element's size until its view gets one of its own.
- **Its format.** A JPEG is smaller than a PNG, which suits a `data:` URL in an HTML app.
- **When it fails.** A discarded or crashed tab fails with `.noImage`, and so does a page that draws nothing within 5 seconds.
- **Unloaded tabs.** A tab a restore left in the background, or a discarded tab, waits to load until it's shown, and `tab.state().unloaded` is true. Capturing it loads it, as Chrome's hover cards load a restored tab, and the first capture may fail. Check `unloaded` first, and show no picture for such a tab. After a restore, Chromium loads some background tabs by itself, as Chrome does: at least 4, and up to 20 while memory allows, of those used in the last 30 days.
- **Policy.** The DisableScreenshots policy refuses pictures with `.notAllowed`, as Chrome refuses screenshots to extensions. `ignorePolicy` takes the picture anyway, as Chrome's own hover cards do. Set it only for a picture that stays in your own UI, never for one you share or save.

A tab switcher shows every tab's picture at once by keeping one for each tab, as Chrome does. When the active tab changes, capture the tab that was active. Its page draws once for the picture while staying hidden.

```swift
// In changed, once the active tab has changed.
if let leaving = previousActive, leaving != browser.activeTab {
    leaving.capture(maxWidth: 480, format: .jpeg, ignorePolicy: true) { result in
        if case .success(let picture) = result { pictures[leaving] = picture }
    }
}
```

Keep the pictures by `tab.persistentID` if they should last beyond this launch. Take a new one when a card opens on a tab, keep a discarded tab's last picture, and drop a closed tab's.

Chrome on a Mac shows no pictures on its hover cards. On other systems it shows one only for a tab that isn't active.

### Showing a popup's tab in a normal window

A page's popup, or an extension's popup window, has one tab and no tab strip. Chrome's Show as Tab moves that tab into a normal window, and Wren Runtime has the same in two calls. `browser.movePopupTab` moves it into a Browser you choose, and `BrowserTab.movePopupTabToNewBrowser` into a new one. The page doesn't load again. It keeps its history, its sessionStorage and `window.opener`, and any dialog over it and its DevTools come with it.

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
// Show as Tab In, with the Browser the person picked.
let moved = browser.move_popup_tab(tab, None, true)?;

// Show as Tab.
BrowserTab::move_popup_tab_to_new_browser(mt, tab, move |result| {
    if let Ok((browser, moved)) = result {
        open_window_showing(browser, moved);
    }
});
```

The tab can't keep its ID when it moves between kinds of Browser, so it becomes a new tab, with a new ID and a new view, which both calls give you. Its `persistentID` stays the same, so anything you keep by it still finds the tab. The old ID throws `.tabUnavailable` from then on.

The tab lands unpinned, and joins a group it lands inside. A page in full screen leaves it. The popup, left with no tabs, closes after the call, and Wren Runtime doesn't bring the other window forward.

Only the tab of a popup or an app popup can move this way, and any other throws `.notAllowed`. Offer the command only while the key window is a popup, as Chrome does. Chrome also turns it off in full screen.

### Covering a tab with your own UI

When your own UI covers a tab, such as a sheet over it, `tab.setIgnoresInput(true)` stops its page taking the mouse, the scroll wheel, keys, drags and VoiceOver's actions, as Chromium's own dialogs over a tab do. VoiceOver can still read the page. `tab.setIgnoresInput(false)` lets it take input again.

Move the keyboard to your own UI when you set it. The page still takes text from an input method once it has the keyboard, and although `tab.focus()` throws `.notAllowed` for the tab, the Tab key and AppKit's key view loop can still reach it.

The setting stays with the tab when it moves to another window, and when Chromium discards its page, but isn't saved in the session. `tab.state().ignoresInput` reads it, and `.ignoresInput` changes report it. It's separate from Chromium's own blocks, such as DevTools' dialogs, so lifting yours leaves theirs in place.

### Keeping a tab on its site

A tab can ask you before it leaves its page. An app's home tab can then keep to its own site, and send other links to new tabs. `tab.setWatchesNavigations(true)` turns this on, and `navigation` answers each request:

```swift
delegate.navigation = { request in
    let here = URL(string: request.tab.url)?.host
    let there = URL(string: request.url)?.host
    return here == nil || here == there ? .allow : .openInNewTab
}
```

Chromium asks when the tab's page starts going to another document, and again at each redirect, whether a page, an extension or Chromium itself started it. The request says how it started:

- `byPage` is true for a page's link, form or script, and for an extension's `chrome.tabs.update`, and `initiatorOrigin` names who started it.
- `userGesture` is true when a click or a key started it.
- `post` is true when it sends a form.
- `redirect` is true at a redirect.

Chromium doesn't ask about your own calls, such as `tab.navigate(to:)` or opening a suggestion or a bookmark. Nor does it ask about a reload, going back or forward, a discarded or restored page loading again, a frame inside the page, a jump to a fragment, or `about:blank`. A watched tab never prerenders, so no page appears from a prerender without asking.

There are three answers:

- **`.allow`** lets it go ahead.
- **`.cancel`** stops it. The tab stays on its page, with no error page.
- **`.openInNewTab`** stops it, and opens its URL in a new tab in front, next to the tab, as a `target=_blank` link would. A form's POST is sent once, from the new tab. At a redirect, the new tab loads the redirect's target.

The new tab's state says the watched tab opened it, as a foreground tab, and its `openedForNavigation` is the request's `id`. Both are set by the time `changed` first reports the tab, so you know which tab your answer opened.

`navigation` runs inside Chromium's call, so decide and return. Calls that navigate, such as `tab.navigate(to:)`, throw `.notAllowed` until it returns. If you send a watched tab's very first navigation away, the tab stays blank. Close it afterwards with `DispatchQueue.main.async` if you don't want it.

The setting stays with the tab when it moves to another window and when Chromium discards its page, but isn't saved in the session. `tab.state().watchesNavigations` reads it, and `.watchesNavigations` changes report it.

### Media playing in a tab

When a tab plays sound or video, you can show what's playing and control it, as Chrome's media controls in its toolbar do. The controls are yours to draw, as a button, a bar or a row for each tab.

- **What's playing.** `tab.state().media` says whether it's `.playing`, `.paused` or `.none`, and which actions the page offers now, in `actions`. It also gives the position: `positionMs` when you read it, `durationMs`, `rate`, and `live` for a stream with no end. Chromium reports the position only when it jumps or the rate changes, so move your bar by `rate` in between. `.media` changes report all of it.
- **Its title and artwork.** `tab.text(.mediaTitle)`, `.mediaArtist`, `.mediaAlbum` and `.mediaSource` give its text. Without a title from the page, the title is the page's own and the source is the site, such as `example.com`. `tab.mediaArtworkPNG` is the artwork, at Chrome's size, and nil until it arrives. `.mediaMetadata` changes report both.
- **Controlling it.** `tab.controlMedia(.pause)` asks the page, and the result comes back as a `.media` change. Play, pause and stop work for anything that can play, and stop is what Chrome's close button does. The seek actions jump 5 seconds, and `tab.seekMedia(to:)` goes to a position. Previous and next track, skipping an ad, and Picture in Picture work when the page offers them. An action the page doesn't offer now throws `.notAllowed`. You can make these calls anywhere, even inside a callback.
- **Calls.** A video call can offer `.toggleMicrophone`, `.toggleCamera` and `.hangUp`, and `media.microphone` and `media.camera` say whether each is on, as the page reports it. They're reported even when there's nothing to play or pause.

macOS's Now Playing, the keyboard's media keys and the Picture in Picture window's buttons are Chromium's, and they control the same media, so their effects reach you as changes too. Chromium also floats a call or a video in Picture in Picture by itself when someone switches away from its tab, if the page asks for it.

### When a tab's page crashes

When a page's process dies, the tab's view goes blank. Chrome shows its "Aw, Snap!" page there, but that page is part of Chrome's window, so in your app the view stays blank until you show something of your own over it. `tab.state().crash` says what happened: the page crashed, was killed, or ran out of memory. `.crashed` changes report it, and `tab.reload()` loads the page again.

### Which tab opened a tab

`tab.state().openedBy` names the tab whose page opened this one, and `openedAs` says how it opened:

| `openedAs` | Opened as |
| --- | --- |
| `.foregroundTab` | A tab in front, as a link with `target=_blank` opens |
| `.backgroundTab` | A tab behind, as a ⌘-click, a middle click or Open Link in New Tab opens |
| `.window` | A new window, as a Shift-click or Open Link in New Window opens |
| `.popup` | A popup, as `window.open` with a size opens |

Both are set by the time `changed` or `browserOpened` first reports the tab, and never change. A `noopener` link hides the opener from the page, but not from you. The record stays when the tab moves, when Chromium discards its page, and after the opener closes, when `openedBy` names a tab no call can find. Tabs you add, tabs extensions add and restored tabs have no opener, because the session doesn't keep it.

Use it as Chrome does: show "Opened from" on a hover card, move focus back to the opener when a tab closes, or group a page with the tabs it opened.

## Windows Chromium opens

Chromium opens Browsers of its own. A page calls `window.open`, an extension opens a popup window, someone undocks DevTools, or a session is restored. Each one arrives in `browserOpened`, which is why every app has to handle it. Show it in a window, or close it:

```swift
delegate.browserOpened = { browser, type, opener in
    let features = browser.windowFeatures
    let size = browser.requestedBounds?.size ?? CGSize(width: 800, height: 600)
    let window = makeWindow(
        size: size,
        tabStrip: features.contains(.tabStrip),
        address: features.contains(.locationBar))
    // Then attach the Browser and show its active tab, as for one you create.
    present(window, for: browser)
}
```

- **Which parts to draw.** `browser.windowFeatures` says which parts Chrome gives this kind of window. A page's popup has a title bar and a read-only address, and no tab strip. An extension's popup window has only a title bar. A normal Browser has everything. Go by the features rather than the type, because two popups can differ.
- **Where and how big.** `browser.requestedBounds` is the size the page asked for, and sometimes the position. For a restored window, it's the frame the window had.
- **Whether it takes focus.** If `browser.isActive` is true, show the window and make it key. If it's false, Chromium opened it without activating it, as for a window an extension opens unfocused. Order it in behind the key window, so it doesn't take the keyboard, with `window.order(.below, relativeTo: NSApp.keyWindow?.windowNumber ?? 0)`.
- **Minimised.** `browser.requestedWindowState` is `.minimized` for a restored window that was minimised, or one an extension created minimised. Show the window, then minimise it.
- **Where it came from.** `opener` is the Browser of the tab that opened this one's first tab, if any. Place a page's popup or a Shift-click's window near it, as Chrome does.

A Browser you ignore stays open with nowhere to show, so if you don't want popups, close them.

## When Chromium asks to move or resize a window

Sometimes Chromium asks for a Browser's window to move, resize, minimise, zoom or restore. An extension calls `chrome.windows.update`, a debugging tool sets the window's bounds, or a popup's page calls `resizeTo` or `moveTo`. The window is yours, so Chromium asks through `windowRequest`, and you do it, adapt it or ignore it. Whatever you do, Chromium then reads the window as it is, and so do extensions and DevTools.

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

In Rust, implement `App::window_request`:

```rust
fn window_request(&self, _: MainThread, browser: Browser, request: WindowRequest) {
    match request {
        WindowRequest::Frame { x, y, width, height } => { /* set the frame */ }
        WindowRequest::State(WindowState::Minimized) => { /* minimise */ }
        _ => {}
    }
}
```

- The frame is in AppKit's screen coordinates, as `requestedBounds` is.
- Chromium only asks for a change, never for what the window already is, and only for a Browser attached to a view.
- A frame or `.normal` for a window in full screen means leaving full screen first, as Chrome does.
- A Browser in the background shares its window with the one in front, so ignore its requests, as above, or bring it forward first.
- The request runs from the event loop, after Chromium's call has returned. So `chrome.windows.update` replies with the window as it was, and a later `chrome.windows.get` sees the change.
- Without the callback, Wren Runtime ignores every request.

When someone minimises or zooms a window themselves, Chromium sees it without any help from you. The session keeps a minimised window minimised, and a window in full screen at the frame it had before.

## DevTools

DevTools stays with the page it inspects.

```swift
try tab.openDevTools()            // where it was last; on the right at first
try tab.openDevTools(.bottom)     // .right, .left, .bottom, .undocked
try tab.closeDevTools()
```

Docked DevTools is inside the tab's view, and Wren Runtime lays out the page and DevTools together, with nothing for you to do. DevTools' own menu can move it to another side, or out into a window of its own. That window is a `.devtools` Browser, which arrives in `browserOpened`. `tab.state().devtools` says where DevTools is.

Only tabs of a normal Browser can dock DevTools. A popup's DevTools always opens in its own window. For a web view's DevTools, see [Web views](web-views.md).

## Split tabs

A split shows two tabs of a Browser side by side, as Chrome's split view does. Each side is still an ordinary tab.

```swift
let split = try first.split(with: second)       // or browser.newSplitTab()
let state = try split.state()                   // first, second, layout, ratio
try split.setRatio(0.4, dragging: true)         // while your divider is dragged
```

While either side is the active tab, show both views, divided by `state.ratio`, with a divider of your own between them. `.split` changes report changes to the split. Chromium keeps the two tabs together, and closing one side ends the split, as does pinning or moving one side on its own.

To close a split as one, close both tabs together. Each page may ask in `beforeunload`, and if one keeps its tab, both stay open and the split with them:

```swift
BrowserTab.close([state.first, state.second].compactMap { $0 }) { result in
    if case .failure(.cancelled) = result { /* a page kept its tab */ }
}
```

The completion gets `.success` once both tabs have gone. Chrome's Close Tab closes only the active side, as `tab.close()` does, so which one ⌘W runs is your choice.

## Side panels

Chromium has side panels for the reading list, bookmarks, history, reading mode, customising the New Tab page, and extensions' own panels. A Browser shows one at a time, in one view, and you decide where that view goes.

```swift
let panel = browser.sidePanelView            // the same view for every panel
let state = try browser.sidePanelState()     // open, width, rightAligned
try browser.showSidePanel("ReadingList")
try browser.closeSidePanel()
```

Put `sidePanelView` beside the pages, and show it at `state.width` while `state.open` is true. Panels also open from Chromium itself, such as from the New Tab page's Customize button, or when an extension calls `chrome.sidePanel.open`. So follow `.sidePanel` changes, not only your own calls. If you draw a handle to resize the panel, report the new width with `browser.setSidePanelWidth` as it's dragged. Chromium remembers the width for each kind of panel.

## Full screen

When a page calls `requestFullscreen()`, it wants the whole window, and Chromium asks you:

```swift
delegate.fullscreen = { browser, entering in
    window(for: browser).toggleFullScreen(nil)     // start the change
    return true                                    // "I will"
}

// When AppKit finishes (windowDidEnterFullScreen or windowDidExitFullScreen):
try browser.setFullscreen(true)
```

While `browser.fullscreenTab` names a tab, hide your tab strip and toolbar, and give that tab's view the whole window. Chromium shows its "Press Esc" notice, and asks you to leave full screen when someone presses Escape. If you return `false`, or have no handler, the page leaves full screen at once.

Call `setFullscreen` when someone takes the window into full screen themselves too, with the green button or your menu.

## Dialogs that block a window

Some of Chromium's dialogs block a whole window, such as an extension's install prompt or the confirmation before removing one. They appear as sheets on the window you attached the Browser to, as in Chrome, and so do the file panels for Open File, Save Page and a page's file field. While one is open, Wren Runtime already stops clicks below the title bar and turns off Chromium's commands.

Blocking your own UI is up to you. While `browser.hasModalDialog` is true, turn off New Tab, Close Tab, the address field and switching tabs, and leave New Window and Quit working. `.modalDialog` changes report it.

Dialogs that belong to one page, such as an alert or a sign-in for HTTP authentication, block only that page, and Chromium handles them.

## Several Browsers in one window

One window can switch between several Browsers, such as workspaces each on its own Profile. [Workspaces](workspaces.md) shows how to set one up, switch, quit and restore it.

## Restoring the last session

Chromium records every Browser's tabs as the app runs, and restores nothing until you ask:

```swift
profile.restoreLastSession { result in
    if case .success(let tabs) = result, tabs > 0 { return }   // windows arrived
    openFirstWindow()                                         // nothing to restore
}
```

Each restored window arrives in `browserOpened` with its tabs, their order and their history. Its `requestedBounds` is the frame the window had, and its `requestedWindowState` is `.minimized` if it was minimised. `profile.lastSession` tells you what there is to restore without restoring it, for an app that asks first, as Chrome does after a crash.

Each launch restores a Profile's last session once, as Chrome does. A second `restoreLastSession` fails with `.notAllowed`, so no two windows end up with the same tabs.

`browser.reopenClosedTab()` is Chrome's Reopen Closed Tab, for a tab or a whole window. Turn its menu item on and off by `browser.canReopenClosedTab`.

### Tabs your app makes itself

If your app makes some tabs again at each launch from its own records, such as pinned sites, keep them out of the session. Otherwise a restore brings them back a second time.

```swift
let tab = try browser.addTab(url: url)
try tab.setSessionExcluded(true)   // right after adding it
```

A restore leaves such a tab out, and keeps the rest in order, with the nearest kept tab active. Reopen Closed Tab still brings it back within the same launch, but it's never written to disk. `tab.state()` says whether a tab is left out, and `.sessionExcluded` changes report it. Keep such tabs out of groups, because Chrome saves a copy of each group, and restoring the group would bring them back.

### A window of several Browsers

A window that switches between several Browsers comes back as one window only if you tag each Browser. [Workspaces](workspaces.md#bringing-the-window-back) shows how.

### Finding a restored tab's records

A tab's `id` lasts one launch. Its `persistentID` lasts as long as the tab itself, so a restored tab has the one it had before the quit. Key what you keep for each tab by it, such as a name someone gave the tab, and look it up when the tab comes back:

```swift
names[tab.persistentID] = "Reading list"          // as someone names it
// After a restore, in browserOpened:
for tab in browser.tabs {
    if let name = names[tab.persistentID] { showName(name, for: tab) }
}
```

The ID survives navigation, moves within a Browser and between Browsers, pinning, grouping, and Chromium discarding the tab. A tab Reopen Closed Tab brings back within the same launch keeps it. A new tab, a duplicate, and a tab reopened from an earlier launch get a new one. Compare IDs as whole strings, because their form can change.

## The rest of the browser's UI

The address field and its suggestions, the site icon, extensions' buttons and Chromium's popovers each have their own data and calls. [The browser's UI, piece by piece](ui-checklist.md) lists every piece with the API for it.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `browser.attach(to:)` | `Browser::attach_to_view` | `Wren_AttachBrowserToView` |
| `browser.isActive` | `Browser::is_active` | `Wren_IsBrowserActive` |
| `browser.tabs` | `Browser::tabs` | `Wren_GetTabCount`, `Wren_GetTabAt` |
| `browser.activeTab` | `Browser::active_tab` | `Wren_GetActiveTab` |
| `tab.view` | `BrowserTab::view` | `Wren_GetTabView` |
| `tab.activate()` | `BrowserTab::activate` | `Wren_ActivateTab` |
| `tab.focus()` | `BrowserTab::focus` | `Wren_FocusTab` |
| `BrowserTab.close(_:completion:)` | `BrowserTab::close_all` | `Wren_CloseTabs` |
| `browser.locationFocus` | `Browser::location_focus` | `Wren_GetLocationFocus` |
| `tab.state()` | `BrowserTab::state` | `Wren_GetTabState` |
| `tab.faviconPNG` | `BrowserTab::favicon_png` | `Wren_CopyTabFaviconPNG` |
| `tab.statusText` | `BrowserTab::status_text` | `Wren_CopyTabText`, `WREN_TAB_TEXT_STATUS` |
| `tab.move(to:)` | `BrowserTab::move_to` | `Wren_MoveTab` |
| `tab.index` | `BrowserTab::index` | `Wren_GetTabIndex` |
| `tab.setPinned` | `BrowserTab::set_pinned` | `Wren_SetTabPinned` |
| `group.move(to:)` | `TabGroup::move_to` | `Wren_MoveTabGroup` |
| `split.move(to:)` | `TabSplit::move_to` | `Wren_MoveTabSplit` |
| `browser.moveTabs` | `Browser::move_tabs` | `Wren_MoveTabsToBrowser` |
| `BrowserTab.moveToNewBrowser` | `BrowserTab::move_to_new_browser` | `Wren_MoveTabsToNewBrowser` |
| `tab.capture` | `BrowserTab::capture` | `Wren_CaptureTab` |
| `browser.movePopupTab` | `Browser::move_popup_tab` | `Wren_MovePopupTabToBrowser` |
| `BrowserTab.movePopupTabToNewBrowser` | `BrowserTab::move_popup_tab_to_new_browser` | `Wren_MovePopupTabToNewBrowser` |
| `tab.setIgnoresInput` | `BrowserTab::set_ignores_input` | `Wren_SetTabIgnoresInput` |
| `tab.setWatchesNavigations` | `BrowserTab::set_watches_navigations` | `Wren_SetTabWatchesNavigations` |
| `AppDelegate.navigation`, `NavigationRequest` | `App::navigation`, `NavigationRequest` | `on_navigation`, `WrenNavigationRequest` |
| `tab.state().media` | `TabState::media` | `WrenTabState.media` |
| `tab.text(.mediaTitle)` | `BrowserTab::text(TabText::MediaTitle)` | `Wren_CopyTabText`, `WREN_TAB_TEXT_MEDIA_TITLE` |
| `tab.mediaArtworkPNG` | `BrowserTab::media_artwork_png` | `Wren_CopyTabMediaArtworkPNG` |
| `tab.controlMedia` | `BrowserTab::control_media` | `Wren_ControlTabMedia` |
| `tab.seekMedia(to:)` | `BrowserTab::seek_media` | `Wren_SeekTabMedia` |
| `tab.reload()` | `BrowserTab::reload` | `Wren_Reload` |
| `browser.windowFeatures` | `Browser::window_features` | `Wren_GetWindowFeatures` |
| `browser.requestedBounds` | `Browser::requested_bounds` | `Wren_GetRequestedBounds` |
| `browser.requestedWindowState` | `Browser::requested_window_state` | `Wren_GetRequestedWindowState` |
| `AppDelegate.windowRequest` | `App::window_request` | `on_window_request` |
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
| `profile.restoreLastSession` | `Profile::restore_last_session` | `Wren_RestoreLastSession` |
| `profile.lastSession` | `Profile::last_session` | `Wren_GetLastSession` |
| `browser.reopenClosedTab()` | `Browser::reopen_closed_tab` | `Wren_ReopenClosedTab` |
| `browser.canReopenClosedTab` | `Browser::can_reopen_closed_tab` | `Wren_CanReopenClosedTab` |
| `tab.setSessionExcluded` | `BrowserTab::set_session_excluded` | `Wren_SetTabSessionExcluded` |
| `tab.persistentID` | `BrowserTab::persistent_id` | `Wren_CopyTabText`, `WREN_TAB_TEXT_PERSISTENT_ID` |

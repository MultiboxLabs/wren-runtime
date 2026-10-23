# Windows and tabs

This page is about how a browser on Wren shows its pages. It covers the views
Chromium gives you, where they go, and the windows Chromium asks you to make.
Read [How a Wren app works](app-model.md) first. The code is in Swift, and the
[table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and
in C.

Chromium decides what exists: which tabs there are, which is active, whether
DevTools is docked, and whether a split is on. You decide where it goes on
screen.

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

- Chromium's dialogs, permission prompts and passkey prompts appear over the
  view, and follow it as it moves or resizes.
- The Browser is active while the window is key, or while one of Chromium's
  windows over it is key, such as a bubble or a dialog. Menu commands,
  extensions and `chrome.windows` go by that. Read it as `browser.isActive`, and
  follow `.active` in `changed`.
- Draw your bars as in use by `browser.isActive`, as Chrome draws its frame. A
  window drawn by its key state alone looks inactive whenever page info opens.
- When Chromium wants the Browser in front, Wren makes the window key.

Attach every Browser you show. The view must already be in a window. Attaching
again moves the Browser to the new view, and `attach(to: nil)` detaches it.

## Tab views

`tab.view` is an `NSView` that holds the tab's page. It is the same view for the
whole life of the tab, even when Chromium replaces the page inside it, or the
tab moves to another Browser.

Chromium draws the page only while the view is in a window and neither it nor an
ancestor is hidden. A page that is not shown stops drawing. So take the views of
background tabs out of the window, or hide them. The page is the view's size, so
set the view's frame or its autoresizing mask. Its scale follows the window's
screen.

Three things to get right:

- **Whole points.** Chromium sizes a page in whole points and drops any
  fraction. That leaves a one-pixel strip of whatever is behind the view along
  its edge. Round the frames you give tab views and the side panel view.
- **Add, then size.** Put a tab's view in the window before giving it its frame,
  as the snippet above does. A background tab sized first can keep drawing at
  its old size.
- **Not inside a translucent view.** A page inside an `NSVisualEffectView` that
  blends with what is behind the window is blended too, and looks washed out.
  Put the effect view behind the pages, as a sibling.

### Switching tabs

When `changed` reports `.tabs` for a Browser, its list of tabs or its active tab
changed. Read `browser.tabs` and `browser.activeTab` again, swap the views, and
give the new page keyboard focus:

```swift
try tab.activate()          // your tab strip's click
// … later, in changed, for .tabs:
show(browser.activeTab)     // swap the views
try browser.activeTab?.focus()
```

`focus()` gives focus back to the element that had it, as Chrome does on a tab
switch. On the New Tab page, Chromium wants the address field to have focus
instead, and calls `focusLocationBar`.

### What a tab strip draws

Everything comes from the tab and its state. Each has a field in
`change.tabFields` that says when to read it again.

| To draw                    | Read                                                  | Changes as                        |
| -------------------------- | ----------------------------------------------------- | --------------------------------- |
| Title, address             | `tab.title`, `tab.url`                                | `.title`, `.url`                  |
| Favicon                    | `tab.faviconPNG`, a PNG at 2x to draw at 16 points    | `.favicon`                        |
| Spinner, back and forward  | `tab.state()`: `loading`, `canGoBack`, `canGoForward` | `.loading`, `.history`            |
| Pinned, grouped, discarded | `tab.state()`: `pinned`, `group`, `discarded`         | `.pinned`, `.group`, `.discarded` |
| The link under the pointer | `tab.state().status`, `tab.statusText`                | `.status`                         |

Moving and pinning, with `tab.move(to:)` and `tab.setPinned`, follow Chromium's
ordering rules. Pinned tabs come first, and a group's tabs stay together. So a
tab may land somewhere other than where you asked. Read `tab.index` afterwards.

### Dragging tabs

A tab strip's drag and drop is the app's own code. Wren has the calls a drop
needs, and Chromium's rules decide where the tabs end up.

| A drop                     | Call                                    |
| -------------------------- | --------------------------------------- |
| Reorder within the strip   | `tab.move(to:)`, then read `tab.index`  |
| Onto another tab, to split | `tab.split(with:)`                      |
| Out of a split             | `split.unsplit()`, then `tab.move(to:)` |
| Into a group               | `group.add([tab])`                      |
| Out of its group           | `BrowserTab.ungroup([tab])`             |
| Into a new group           | `BrowserTab.group(tabs)`                |

Pinning, grouping or moving one side of a split alone ends the split. Wren has
no call that moves a tab into another Browser. Wren's web sample, in a release's
`samples` folder, drags tabs, splits and groups in its page's tab strip.

### A crashed tab

When a page's process dies, the tab's view shows nothing. Chromium's "Aw, snap!"
page is part of Chrome's window, which a Wren app does not have. So you draw
your own. `tab.state().crash` says why: crashed, killed, or out of memory.
`.crashed` in `tabFields` says when. `tab.reload()` loads the page again.

## Windows Chromium opens

Chromium opens Browsers of its own. A page calls `window.open`, an extension
opens a popup window, the user undocks DevTools, or a session is restored. Each
arrives in `browserOpened`, which is why that callback is required. Show it in a
window, or close it:

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

- `browser.windowFeatures` says which parts Chrome gives this kind of window. A
  page's popup has a title bar and a read-only address, and no tab strip. An
  extension's popup window has only a title bar. A normal Browser has
  everything. Go by the features, not the type, because two popups can differ.
- `browser.requestedBounds` is the size the page asked for, and perhaps the
  position. For a restored window, it is the frame the window had.
- A Browser you ignore stays open with nowhere to show. If you do not want
  popups, close them.

## DevTools

DevTools stays with the page it inspects.

```swift
try tab.openDevTools()            // where it was last; the right side at first
try tab.openDevTools(.bottom)     // .right, .left, .bottom, .undocked
try tab.closeDevTools()
```

Docked, DevTools is inside the tab's view. Wren lays the page and DevTools out
together, and you do nothing. DevTools' own dock menu moves it between sides, or
out into a window of its own. That window is a `.devtools` Browser, which
arrives in `browserOpened`. `tab.state().devtools` says where DevTools is.

Only tabs of a normal Browser dock DevTools. A popup's DevTools always opens in
its own window. For a web view's DevTools, see [Web views](web-views.md).

## Split tabs

A split shows two tabs of a Browser side by side, as one. It is Chromium's split
view. Each side is still an ordinary tab.

```swift
let split = try first.split(with: second)       // or browser.newSplitTab()
let state = try split.state()                   // first, second, layout, ratio
try split.setRatio(0.4, dragging: true)         // while your divider is dragged
```

While either side is the active tab, show both views, divided by `state.ratio`,
and draw your own divider. `.split` in `changed` reports changes. Chromium keeps
the two tabs together. Closing one side ends the split, and so does moving or
pinning one side alone.

## Side panels

Chromium has side panels: the reading list, bookmarks, history, reading mode,
Customize Chromium, and extensions' own panels. A Browser shows one at a time,
in one view, and you decide where that view goes.

```swift
let panel = browser.sidePanelView            // the same view for every panel
let state = try browser.sidePanelState()     // open, width, rightAligned
try browser.showSidePanel("ReadingList")
try browser.closeSidePanel()
```

Place `sidePanelView` beside the pages, and show it at `state.width` while
`state.open` is true. Panels also open from Chromium itself, for example from
the New Tab page's Customize button, or when an extension calls
`chrome.sidePanel.open`. So follow `.sidePanel` in `changed`, not only your own
calls. If you draw a resize handle, report the width with
`browser.setSidePanelWidth` as the user drags it. Chromium remembers the width
for each kind of panel.

## Full screen

A page that calls `requestFullscreen()` wants the whole window. Chromium asks
you:

```swift
delegate.fullscreen = { browser, entering in
    window(for: browser).toggleFullScreen(nil)     // start the change
    return true                                    // "I will"
}

// When AppKit finishes (windowDidEnterFullScreen / windowDidExitFullScreen):
try browser.setFullscreen(true)
```

While `browser.fullscreenTab` names a tab, hide your tab strip and toolbar, and
give that tab's view the whole window. Chromium shows its "Press Esc" notice,
and asks you to leave when the user presses Escape. If you return `false`, or
set no handler, the page leaves full screen at once.

Also call `setFullscreen` when the user takes the window into full screen
themselves, with the green button or your menu.

## Dialogs that block the window

In Chrome, some of Chromium's dialogs block the whole window, such as an
extension's install prompt or an uninstall confirmation. In a Wren app they are
sheets of the attached view's window, as in Chrome, and so are the file panels
of Open File, Save Page and a page's file field. While a dialog is open, Wren
already refuses clicks below the title bar and disables Chromium's commands.

Your own UI is yours to block. While `browser.hasModalDialog` is true, disable
New Tab, Close Tab, the address field and tab switching. Leave New Window and
Quit working. `.modalDialog` in `changed` reports each change.

Dialogs that belong to one page, such as an alert or HTTP authentication, block
only that page. Chromium handles them.

## Several Browsers in one window

A window can show several Browsers in turn, such as workspaces each on its own
Profile, drawn by one web view's page. Attach every one of them to the same
view, and keep all but one in the background:

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

A Browser in the background still belongs to the window. Until you bring it
forward:

- It is never active.
- The menu's Chromium commands go to the Browser in front.
- Its sheets, its pages' dialogs and its bubbles are not shown, and block
  nothing. They wait, and show once it is in front.
- Its tabs stay open and loaded.

What your page shows in its tab elements is yours to change. Wren only decides
which Browser the window answers for.

Chromium sometimes asks for a Browser to come forward. A site's notification is
clicked, an extension calls `chrome.windows.update`, or a page focuses a window
it opened. A Browser in the background stays there, and Wren tells you, so you
decide whether to switch:

```swift
app.changed = { changes in
    for change in changes where change.kind == .foregroundRequest {
        show(change.browser!)          // or badge the workspace's button
    }
}
```

`browser.wantsForeground` stays true until the Browser is brought forward.

Keep exactly one Browser of a window in front. Wren does not put the others in
the background for you. Two in front share the window as two attached Browsers
always have. Both are active, the menu reaches the one made first, and one's
sheet blocks both.

## Restoring the last session

Chromium records every Browser's tabs as the app runs, and restores nothing
until you ask.

```swift
profile.restoreLastSession { result in
    if case .success(let tabs) = result, tabs > 0 { return }   // windows arrived
    openFirstWindow()                                         // nothing to restore
}
```

Each restored window arrives in `browserOpened`, with its tabs, their order and
history. Its `requestedBounds` is the frame the window had.
`profile.lastSession` reads what there is to restore without restoring it. It is
for an app that wants to ask first, as Chrome does after a crash.

A launch restores a Profile's last session once, as Chrome does. A second
`restoreLastSession` fails with `.notAllowed`, so no two windows hold the same
tabs.

`browser.reopenClosedTab()` is Chrome's Reopen Closed Tab, for a tab or a whole
window. Enable its menu item by `browser.canReopenClosedTab`.

### A window of several Browsers

Chromium records each Browser as a window of its own. A window that shows
several Browsers in turn would come back as one window per Browser. Tag each
Browser with its window and its place in it, and Chromium keeps the tag in the
session:

```swift
try personal.setSessionTag("\(windowID) 0")
try work.setSessionTag("\(windowID) 1")
```

A restored Browser has its tag already in `browserOpened`. Read it there, and
attach the Browser to the window it names, making that window if it is the first
of its Browsers to arrive:

```swift
let parts = browser.sessionTag.split(separator: " ")
if parts.count == 2, let window = windows[String(parts[0])] ?? makeWindow(String(parts[0])) {
    window.add(browser, at: Int(parts[1]) ?? 0)
}
```

Change a tag when its Browser moves, for example to record which one is in
front. A tag is at most 128 bytes of UTF-8. A Browser that Reopen Closed Tab
brings back has its tag too, but it comes back alone. Chromium reopens one
Browser at a time, from the list of that Browser's Profile. A Browser Chromium
opens from a tagged one, such as a new window, has no tag.

### Finding a restored tab's records

A tab's `id` lasts one launch. Its `persistentID` lasts as long as the tab: a
restored tab has the ID it had before the quit. Key what the app keeps for each
tab by it, such as a name the user gave the tab, and look the record up when the
tab comes back:

```swift
names[tab.persistentID] = "Reading list"          // as the user names it
// After a restore, in browserOpened:
for tab in browser.tabs {
    if let name = names[tab.persistentID] { show(name, for: tab) }
}
```

The ID survives navigation, moves, also to another Browser, pinning, grouping
and Chromium discarding the tab to save memory. Reopen Closed Tab brings it back
in the same launch. A new tab and a duplicate get a new one, and so does a tab
reopened from an earlier launch's closed tabs. Compare the ID whole; its form
may change.

## The rest of the browser's UI

The address field with its suggestions, the site icon, extensions' buttons, and
Chromium's popovers each have their own data and calls.
[The UI a browser provides](ui-checklist.md) lists every piece with the API for
it.

## The names in Swift, Rust and C

| Swift                        | Rust                                           | C                                   |
| ---------------------------- | ---------------------------------------------- | ----------------------------------- |
| `browser.attach(to:)`        | `Browser::attach_to_view`                      | `Wren_AttachBrowserToView`          |
| `browser.isActive`           | `Browser::is_active`                           | `Wren_IsBrowserActive`              |
| `browser.tabs`               | `Browser::tabs`                                | `Wren_GetTabCount`, `Wren_GetTabAt` |
| `browser.activeTab`          | `Browser::active_tab`                          | `Wren_GetActiveTab`                 |
| `tab.view`                   | `BrowserTab::view`                             | `Wren_GetTabView`                   |
| `tab.activate()`             | `BrowserTab::activate`                         | `Wren_ActivateTab`                  |
| `tab.focus()`                | `BrowserTab::focus`                            | `Wren_FocusTab`                     |
| `tab.state()`                | `BrowserTab::state`                            | `Wren_GetTabState`                  |
| `tab.faviconPNG`             | `BrowserTab::favicon_png`                      | `Wren_CopyTabFaviconPNG`            |
| `tab.statusText`             | `BrowserTab::status_text`                      | `Wren_CopyTabStatusText`            |
| `tab.move(to:)`              | `BrowserTab::move_to`                          | `Wren_MoveTab`                      |
| `tab.index`                  | `BrowserTab::index`                            | `Wren_GetTabIndex`                  |
| `tab.setPinned`              | `BrowserTab::set_pinned`                       | `Wren_SetTabPinned`                 |
| `tab.reload()`               | `BrowserTab::reload`                           | `Wren_Reload`                       |
| `browser.windowFeatures`     | `Browser::window_features`                     | `Wren_GetWindowFeatures`            |
| `browser.requestedBounds`    | `Browser::requested_bounds`                    | `Wren_GetRequestedBounds`           |
| `tab.openDevTools`           | `BrowserTab::open_dev_tools`                   | `Wren_OpenDevTools`                 |
| `tab.closeDevTools()`        | `BrowserTab::close_dev_tools`                  | `Wren_CloseDevTools`                |
| `tab.split(with:)`           | `BrowserTab::split_with`                       | `Wren_SplitTabs`                    |
| `browser.newSplitTab()`      | `Browser::new_split_tab`                       | `Wren_NewSplitTab`                  |
| `split.setRatio`             | `TabSplit::set_ratio`                          | `Wren_SetSplitRatio`                |
| `browser.sidePanelView`      | `Browser::side_panel_view`                     | `Wren_GetSidePanelView`             |
| `browser.sidePanelState()`   | `Browser::side_panel_state`                    | `Wren_GetSidePanelState`            |
| `browser.showSidePanel`      | `Browser::show_side_panel`                     | `Wren_ShowSidePanel`                |
| `browser.setSidePanelWidth`  | `Browser::set_side_panel_width`                | `Wren_SetSidePanelWidth`            |
| `AppDelegate.fullscreen`     | `App::fullscreen`                              | `on_fullscreen`                     |
| `browser.setFullscreen`      | `Browser::set_fullscreen`                      | `Wren_SetFullscreen`                |
| `browser.fullscreenTab`      | `Browser::fullscreen_tab`                      | `Wren_GetFullscreenTab`             |
| `browser.hasModalDialog`     | `Browser::has_modal_dialog`                    | `Wren_HasModalDialog`               |
| `browser.isBackgrounded`     | `Browser::set_backgrounded`, `is_backgrounded` | `Wren_SetBrowserBackgrounded`       |
| `browser.wantsForeground`    | `Browser::wants_foreground`                    | `Wren_HasForegroundRequest`         |
| `profile.restoreLastSession` | `Profile::restore_last_session`                | `Wren_RestoreLastSession`           |
| `profile.lastSession`        | `Profile::last_session`                        | `Wren_GetLastSession`               |
| `browser.reopenClosedTab()`  | `Browser::reopen_closed_tab`                   | `Wren_ReopenClosedTab`              |
| `browser.canReopenClosedTab` | `Browser::can_reopen_closed_tab`               | `Wren_CanReopenClosedTab`           |
| `browser.sessionTag`         | `Browser::session_tag`                         | `Wren_CopyBrowserSessionTag`        |
| `browser.setSessionTag(_:)`  | `Browser::set_session_tag`                     | `Wren_SetBrowserSessionTag`         |
| `tab.persistentID`           | `BrowserTab::persistent_id`                    | `Wren_CopyTabPersistentID`          |

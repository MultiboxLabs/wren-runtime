# Windows and tabs

How a browser on Wren shows its pages: the views Chromium gives you, where they
go, and the windows Chromium asks you to make. Read
[How a Wren app works](app-model.md) first.

Chromium decides what exists (which tabs, which is active, whether DevTools is
docked, whether a split is on). You decide where it goes on screen.

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

`attach` matters more than it looks. It makes that window the Browser's window:

- Chromium's dialogs, permission prompts and passkey prompts appear over the
  view, and follow it as it moves or resizes;
- the Browser is active while the window, or a window of Chromium's over it (a
  bubble, a dialog), is key, which is what menu commands, extensions and
  `chrome.windows` go by. Read it as `browser.isActive` (`.active` in `changed`)
  and draw your bars as in use by it, as Chrome draws its frame: a window drawn
  by its key state alone looks left whenever page info opens;
- when Chromium wants the Browser in front, the window becomes key.

Attach every Browser you show. The view must already be in a window. Attaching
again moves it; `attach(to: nil)` detaches.

## Tab views

`tab.view` is an `NSView` that holds the tab's page. It stays the same view for
as long as the tab lives, even when Chromium replaces the page inside it or the
tab moves to another Browser. Chromium follows the view:

- **Visible** while the view is in a window and neither it nor an ancestor is
  hidden. A page that is not visible stops rendering, so take the views of
  background tabs out of the window, or hide them.
- **Sized** to the view. Set its frame or its autoresizing mask.
- **Scaled** to the window's screen.

Three things to get right:

- **Whole points.** Chromium sizes a page in whole points and drops any
  fraction, which leaves a one-pixel strip of whatever is behind the view along
  its edge. Round the frames you give tab views and the side panel view.
- **Add, then size.** Put a tab's view in the window before giving it its frame,
  as the snippet above does. A background tab sized first can keep drawing at
  its old size.
- **Not inside a translucent view.** A page inside an `NSVisualEffectView` that
  blends with what is behind the window is blended too, and comes out washed.
  Put the effect view behind the pages as a sibling.

### Switching tabs

When `changed` reports `.tabs` for a Browser, its list or its active tab
changed. Read `browser.tabs` and `browser.activeTab` again, swap the views, and
give the new page keyboard focus:

```swift
try tab.activate()          // your tab strip's click
// … later, in changed, for .tabs:
show(browser.activeTab)     // swap the views
try browser.activeTab?.focus()
```

`focus()` restores focus to the element that had it, as Chrome does on a tab
switch. On the New Tab page Chromium wants the address field instead, and calls
`focusLocationBar`.

### What a tab strip draws

Everything comes from the tab and its state, and each has a field in
`change.tabFields` that says when to read it again:

| To draw                    | Read                                                  | Changes as                        |
| -------------------------- | ----------------------------------------------------- | --------------------------------- |
| Title, address             | `tab.title`, `tab.url`                                | `.title`, `.url`                  |
| Favicon                    | `tab.faviconPNG` (a PNG at 2x; draw it at 16 points)  | `.favicon`                        |
| Spinner, back and forward  | `tab.state()`: `loading`, `canGoBack`, `canGoForward` | `.loading`, `.history`            |
| Pinned, grouped, discarded | `tab.state()`: `pinned`, `group`, `discarded`         | `.pinned`, `.group`, `.discarded` |
| The link under the pointer | `tab.state().status`, `tab.statusText`                | `.status`                         |

Moving and pinning (`tab.move(to:)`, `tab.setPinned`) follow Chromium's ordering
rules: pinned tabs first, a group's tabs together. The place a tab lands may not
be the one asked for, so read `tab.index` afterwards.

### A crashed tab

When a page's process dies, the tab's view shows nothing. Chromium's "Aw, snap!"
page is part of Chrome's window, which a Wren app does not have, so you draw
your own: `tab.state().crash` says why (crashed, killed, out of memory),
`.crashed` in `tabFields` says when, and `tab.reload()` loads the page again.

## Windows Chromium opens

Chromium opens Browsers of its own: a page calls `window.open`, an extension
opens a popup window, the user undocks DevTools, a session is restored. Each
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
  page's popup has a title bar and a read-only address and no tab strip; an
  extension's popup window has only a title bar; a normal Browser has
  everything. Go by the features, not the type: two popups can differ.
- `browser.requestedBounds` is the size, and perhaps the position, the page
  asked for, or the frame a restored window had.
- A Browser you ignore stays open with nowhere to show. If you do not want
  popups, close them.

## DevTools

DevTools stays with the page it inspects.

```swift
try tab.openDevTools()            // where it was last; the right side at first
try tab.openDevTools(.bottom)     // .right, .left, .bottom, .undocked
try tab.closeDevTools()
```

Docked, DevTools is inside the tab's view: Wren lays the page and DevTools out
together, and you do nothing. DevTools' own dock menu moves it between sides, or
out into a window of its own, which is a `.devtools` Browser that arrives in
`browserOpened`. `tab.state().devtools` says where it is.

Only tabs of a normal Browser dock; a popup's DevTools always opens undocked.

## Split tabs

A split shows two tabs of a Browser side by side as one, which is Chromium's
split view. Each side is still an ordinary tab.

```swift
let split = try first.split(with: second)       // or browser.newSplitTab()
let state = try split.state()                   // first, second, layout, ratio
try split.setRatio(0.4, dragging: true)         // while your divider is dragged
```

While either side is the active tab, show both views, divided by `state.ratio`,
and draw your own divider. `.split` in `changed` reports changes. Chromium keeps
the two tabs together: closing one side, or moving or pinning one alone, ends
the split.

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

Place `sidePanelView` beside the pages and show it while `state.open`, at
`state.width`. Panels also open from Chromium itself (the New Tab page's
Customize button, an extension calling `chrome.sidePanel.open`), so follow
`.sidePanel` in `changed` and not only your own calls. If you draw a resize
handle, report the width as it is dragged with `browser.setSidePanelWidth`;
Chromium remembers it per kind of panel.

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

While `browser.fullscreenTab` names a tab, hide your tab strip and toolbar and
give that tab's view the window. Chromium shows its "Press Esc" notice and asks
you to leave when the user presses Escape. Return `false`, or set no handler,
and the page leaves full screen at once.

Also call `setFullscreen` when the user takes the window into full screen
themselves, with the green button or your menu.

## Dialogs that block the window

Some of Chromium's dialogs block the whole window in Chrome: an extension's
install prompt, an uninstall confirmation. In a Wren app they are sheets over
the attached view. Wren already refuses clicks below the title bar and disables
Chromium's commands while one is open. Your own UI is yours to block: while
`browser.hasModalDialog`, disable New Tab, Close Tab, the address field and tab
switching, and leave New Window and Quit available. `.modalDialog` in `changed`
reports each change.

Dialogs that belong to one page (an alert, HTTP authentication) block only that
page, and Chromium handles them.

## Several Browsers in one window

A window can show several Browsers in turn: workspaces, each on its own Profile,
drawn by one web view's page. Attach every one of them to the same view and keep
all but one **in the background**:

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

In C: `Wren_SetBrowserBackgrounded`, `Wren_IsBrowserBackgrounded`.

A Browser in the background still belongs to the window. Until you bring it
forward it is never active, the menu's Chromium commands go to the Browser in
front, and its sheets, its pages' dialogs and its bubbles are not shown and
block nothing: they wait, and show once it is in front. Its tabs stay open and
loaded. What your page shows in its tab elements is yours to change; Wren only
decides which Browser the window answers for.

Chromium sometimes asks for a Browser to come forward: a site's notification is
clicked, an extension calls `chrome.windows.update`, a page focuses a window it
opened. One in the background stays there, and you are told, so that you decide
whether to switch:

```swift
app.changed = { changes in
    for change in changes where change.kind == .foregroundRequest {
        show(change.browser!)          // or badge the workspace's button
    }
}
```

`browser.wantsForeground` (`Wren_HasForegroundRequest`) stays true until the
Browser is brought forward.

Keep exactly one in front. Wren does not put the others in the background for
you, and two in front share the window as two attached Browsers always have:
both are active, the menu reaches the one made first, and one's sheet blocks
both.

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
history, and in `requestedBounds` the frame the window had.
`profile.lastSession` reads what there is to restore without restoring it, for
an app that wants to ask first, as Chrome does after a crash.

`browser.reopenClosedTab()` is Chrome's Reopen Closed Tab, for a tab or a whole
window; enable its menu item by `browser.canReopenClosedTab`.

## The rest of the browser's UI

The address field with its suggestions, the site icon, extensions' buttons, and
Chromium's popovers each have their own data and calls.
[The UI a browser provides](ui-checklist.md) lists every piece with the API for
it.

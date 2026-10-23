# Web views

A web view is a page your app makes and places in its own window, and it is not
a tab. No Browser has it, and you need no Browser to make one. It can show your
own UI from [one of your schemes](schemes.md), or any website. The API is in
[`WrenWebView.h`](../include/WrenWebView.h).
What works in this release is in [This release](this-release.md). The
[table at the end](#the-names-in-swift-rust-and-c) gives each name in Swift,
Rust and C.

[Adding web pages to an existing app](embedding.md) says when to use a web view
and when a tab, and shows one serving pages from an app's bundle. The page can
show tabs inside itself, in [views in a web view](views-in-web-views.md), and
call your app, with [messages](messages.md).

```swift
guard let webView = WebView(profile: profile, url: "app://ui/index.html") else { return }
let view = webView.view
view.frame = window.contentView!.bounds
view.autoresizingMask = [.width, .height]
window.contentView!.addSubview(view)
```

```rust
let web_view = WebView::new(&profile, "app://ui/index.html", &WebViewOptions::default())
    .expect("a web view");
// web_view.view() is the NSView to add to the window, with objc2.
```

## Rules

- **You own it.** The page lives until you destroy the view, which also takes it
  out of its window. In Swift that is when the `WebView` is released, and in
  Rust when it is dropped. While it lives, Chromium keeps its Profile loaded.
- **Place it like a tab's view.** The page shows while the view is in a window
  and not hidden. Its size is the view's, and its scale is that of the window's
  screen. Give the view a whole number of points each way.
- **It is not a tab.** It has no tab ID. Wren reports none of a tab's or a
  Browser's changes for it, only its own, as
  [Loading and crashing](#loading-and-crashing) describes. It is not in history,
  or in a restored session. A request to your scheme from its page names the
  view (`SchemeRequest.webView`) and no tab.
- **A pinch does not zoom it,** as none zooms Chrome's own UI. Neither does a
  two-finger double tap. The page still gets the Ctrl-wheel event a pinch starts
  with, so a page that zooms a map of its own still can. A tab or side panel in
  [a `<wren-view>`](views-in-web-views.md) is a page of its own. A pinch over a
  tab zooms that tab, as in Chrome. A scroll over it scrolls the tab, and never
  your page, even at the tab's end.
- **A two-finger swipe does not take it back,** as none takes Chrome's toolbar
  back. A swipe over a tab or side panel in a `<wren-view>` takes that page back
  or forward, with macOS's arrow at its edge, as in Chrome. It needs "Swipe
  between pages" on in the Mac's trackpad settings.
- **Your page stretches at its end like any page,** when a scroll on it goes
  past the end and it can scroll. Chromium's own UI pages do the same. For bars
  that should stay still, as Chrome's toolbar does, give your page's root
  `overscroll-behavior: none`. A tab in a `<wren-view>` still stretches at its
  own ends.
- **Your menu's shortcuts work in it.** A key the page does not handle goes to
  the app's main menu.
- **You choose what debugging tools are told it is,** when you make it. With
  `kind: .page`, the default, Puppeteer, chromedriver and `chrome://inspect`
  list it among the tabs, as they list an Electron window. With
  `kind: .browserUi`, it is marked as the browser's own UI. Those tools then do
  not offer it as a tab, and reach it only by asking for it. DevTools works in
  full for both.
- **It can be see-through.** Made with `transparent: true`, the page is
  transparent wherever it draws nothing. So what you put behind the view in its
  window shows there, such as an `NSVisualEffectView` for a translucent window
  behind an HTML UI. Whatever the page draws still covers, CSS backgrounds
  included. Wren changes nothing about the window. It holds for every page the
  view loads.

## DevTools

A web view's DevTools opens in its own window. `openDevTools()` makes a DevTools
Browser, which arrives in `browserOpened` like any other. It shows only once you
put its tab's view in a window. If you close it there, DevTools is gone as it
opens.

`closeDevTools()` closes it, and its Browser closes. `state.devtools` says
whether it is open. So a menu item can toggle it:

```swift
if webView.state.devtools { try? webView.closeDevTools() } else { try? webView.openDevTools() }
```

```rust
let open = web_view.state().map(|state| state.devtools).unwrap_or(false);
let _ = if open { web_view.close_dev_tools() } else { web_view.open_dev_tools() };
```

It is not a tab's DevTools. The Chromium command `developerTools` in your menu
reaches a tab, never a web view.

Make the view with `disallowUserDevToolsActivation: true`, or
`disallow_user_devtools_activation` in Rust and C, so that only your app opens
DevTools. Its text fields' menu then has no Inspect, and `openDevTools()` still
works. A shipped build that should have no DevTools leaves out the item that
opens it.

## Trusted origins

What your page gets from your app, it gets by an option you set when you make
the view. It gets it only while it is one of your own pages.

| The page has                                            | Option in Swift | In Rust and C  |
| ------------------------------------------------------- | --------------- | -------------- |
| [`wren`](messages.md)                                   | `messages`      | `messages`     |
| [Marks for Chromium's bubbles](anchors-in-web-views.md) | `anchors`       | `anchors`      |
| [`<wren-view>`](views-in-web-views.md)                  | `embedsViews`   | `embeds_views` |
| [Areas that move the window](#dragging-the-window)      | `dragsWindow`   | `drags_window` |

By default, your own pages are the pages on [your schemes](schemes.md). One
setting widens that, for one view, and stays fixed for the view's life:

|       | Where                                                                          |
| ----- | ------------------------------------------------------------------------------ |
| Swift | `WebView(profile:url:…, trustedOrigins: ["http://localhost:5173"])`            |
| Rust  | `WebViewOptions { trusted_origins: vec!["http://localhost:5173".into()], .. }` |
| C     | `WrenWebViewOptions.trusted_origins` and `trusted_origin_count`                |

Each entry is an origin: a scheme, a host and a port, with no path. To that
view, a page on a trusted origin is your own page, exactly as a page on your
scheme is. The list turns nothing on by itself. Each thing still needs its
option. A list given with none of those options is refused, and the view is not
made.

It is for a development server that serves your UI while you build it. Then the
same page works from `http://localhost:5173` as from `app://ui/`. List nothing
in a build you ship. Whoever serves a trusted origin reaches every handler you
set for the view, places your Browsers' anchors, and can show any tab whose ID
it writes.

What it does not do:

- **It does not open your schemes.** They stay closed to a trusted origin, as to
  any website. So a page served by your development server cannot load
  `app://ui/icon.png` or `fetch("app://…")`. Serve the page's own files from the
  same server, by relative URLs. If the page must load from your scheme while
  you develop, declare that scheme [web accessible](schemes.md) in your
  development build.
- **It is one view's.** The same origin has nothing in a tab, in a frame of your
  page, or in another view that did not list it.
- **It is fixed.** There is no way to add an origin later, to trust one for
  every view at once, or to take your own schemes off the list.

## Dragging the window

A window whose title bar is HTML is moved by the page. Make the view with
`dragsWindow`, and say in CSS which areas move the window:

```css
.bar {
  app-region: drag;
}
.bar button {
  app-region: no-drag;
}
```

```swift
let window = NSWindow(
    contentRect: frame,
    styleMask: [.titled, .closable, .resizable, .miniaturizable, .fullSizeContentView],
    backing: .buffered, defer: false)
window.titlebarAppearsTransparent = true
window.titleVisibility = .hidden
let webView = WebView(profile: profile, url: "app://ui/window.html", dragsWindow: true)!
```

This is Chromium's own mechanism, the one Electron apps use, and the page writes
no script for it. Over a drag area, the view behaves as a title bar does.

- A left mouse-down moves the window, and does not reach the page.
- A double click does what the user chose in System Settings for a title bar:
  zoom, minimise or nothing.
- Right clicks, control-clicks, scrolling and pointer movement reach the page as
  anywhere else. So a drag area keeps its `contextmenu` event, its hover styles
  and a tab strip's scrolling.

The rules:

- **You make the window.** Wren changes nothing about its style. Give it a
  full-size content view and a hidden, transparent title bar, and place the web
  view under it. The window's own close, minimise and zoom buttons stay the
  window's, above the page. Leave them room, about 78 points at the left.
- **Mark what must take clicks.** A drag area is the box of the element with the
  property. Chromium makes the property inherited, so each element inside a drag
  area drags too. Give each button, tab and field inside one
  `app-region: no-drag`.
- **Only the page moves the window.** In a title bar's band, macOS moves a
  window from a press by itself, without asking your app, wherever it sees
  nothing solid. A page drawn in another process looks empty to it. A view made
  with `dragsWindow` stops that over the whole view, as Chromium's own windows
  do. So a press on your page's tabs never moves the window, and its drag areas
  do. A view made without `dragsWindow` is left to macOS.
- **A `<wren-view>` is `no-drag` by default.** So a page whose whole body drags
  still has tabs that take their clicks. Your own CSS can say otherwise.
- **Only your own page.** The areas count while the view's page is on one of
  your schemes or a [trusted origin](#trusted-origins). A frame inside the page,
  and a tab the page shows, declare nothing. A view that has gone to any other
  site drags nothing.
- **They belong to the page shown now.** They go when the page reloads, goes
  elsewhere or crashes, and come back with the next page's first layout. While
  the page is gone, nothing of it moves the window.

Not offered: resizing the window from the page, and anything like this on a
platform other than macOS yet.

## Loading and crashing

A web view says what its page is doing, and tells you when that changes:

```swift
webView.changed = { state in
    if state.crash != .none {
        try? webView.reload()                       // the page's process died
    } else if state.loadError != 0 {
        try? webView.load("app://ui/offline.html")  // could not load
    } else if state.drawn {
        window.makeKeyAndOrderFront(nil)            // show the window now
    }
}
let now = webView.state                             // readable at any time
```

In Rust, `web_view.set_changed(Some(Box::new(|state| …)))` takes the same
closure, and `web_view.state()` reads the state.

| `state`     | Means                                                                                                                                                      |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `loading`   | The page is loading, as a tab's `loading` says.                                                                                                            |
| `loadError` | Chromium's network error for the last load, a negative number, or 0. See below. It is 0 again from when the next load starts.                              |
| `crash`     | Why the page is gone, if it is: its process crashed, was killed or ran out of memory. The view shows nothing.                                              |
| `drawn`     | The page shown now has drawn its first frame with something in it, as Electron's `ready-to-show`. False from when another page commits, and while crashed. |
| `devtools`  | DevTools for the page is open. That is from when its Browser arrives in `browserOpened` until it has closed, however it was opened or closed.              |
| `isLoaded`  | Not loading, no `loadError`, and no `crash`.                                                                                                               |

The common errors are `-105`, the name was not found; `-102`, the connection was
refused; `-106`, there is no network; and `-6`, your scheme's handler found
nothing.

In C the state is `Wren_GetWebViewState`. Each change is a
`WREN_CHANGE_WEB_VIEW` record in `on_changed` that names the view, in
`WrenChange.web_view`. In Swift the same record also reaches
`AppDelegate.changed`, as `.webView`.

- **A failed load shows Chromium's error page** in the view, or leaves the page
  that was there. Load something of your own if your UI should not show it. A
  page your server answered, with any status, did not fail.
- **`reload()` loads the page again at its address now,** which may not be the
  one you gave it. Use it after a crash, after a failed load, or to pick up
  changed files. `load(_:)` does the same for an address you give.
- **Show a new window when `drawn` turns true,** not before, so it is never
  blank. The page draws while its view is in a window, shown or not, so add the
  view first. Show the window on a load error or a crash too. Chromium's error
  page draws, but a crashed page does not, so the window might never appear.
- **While the page is crashed,** its [drag areas](#dragging-the-window) are gone
  with it. So a window whose title bar it drew cannot be moved by it.

## A bare page

A web view has little of what Chromium's UI gives a tab's page.

- Only a text field has a context menu, a short one of Wren's. It has spelling
  suggestions, undo, cut, copy, paste and select all, and Inspect unless only
  the app opens DevTools.
- Elsewhere there is no menu. The page sends its own items from its
  `contextmenu` event, and your app shows them. See
  [Menus](menus.md#context-menus).
- `alert()`, permission prompts, file choosers and `window.open` do nothing.

That suits your own UI. A website that needs them belongs in a tab.

## Not offered yet

The page's address and title.

## The names in Swift, Rust and C

| Swift                       | Rust                             | C                                                |
| --------------------------- | -------------------------------- | ------------------------------------------------ |
| `WebView(profile:url:…)`    | `WebView::new`, `WebViewOptions` | `Wren_CreateWebView`, `WrenWebViewOptions`       |
| releasing the `WebView`     | dropping the `WebView`           | `Wren_DestroyWebView`                            |
| `webView.view`              | `WebView::view`                  | `Wren_GetWebViewNativeView`                      |
| `webView.load(_:)`          | `WebView::load`                  | `Wren_LoadWebView`                               |
| `webView.reload()`          | `WebView::reload`                | `Wren_ReloadWebView`                             |
| `webView.state`             | `WebView::state`                 | `Wren_GetWebViewState`                           |
| `webView.changed`           | `WebView::set_changed`           | `WREN_CHANGE_WEB_VIEW`                           |
| `webView.focus()`           | `WebView::focus`                 | `Wren_FocusWebView`                              |
| `webView.openDevTools()`    | `WebView::open_dev_tools`        | `Wren_OpenWebViewDevTools`                       |
| `webView.closeDevTools()`   | `WebView::close_dev_tools`       | `Wren_CloseWebViewDevTools`                      |
| `kind: .page`, `.browserUi` | `WebViewKind::Page`, `BrowserUi` | `WREN_WEB_VIEW_PAGE`, `WREN_WEB_VIEW_BROWSER_UI` |

# Web views

A web view is a page your app makes and places in its own window, which is not a
tab: no Browser has it, and you need no Browser to have one. It can show your
own UI from [one of your schemes](schemes.md), or any website. The API is in
[`WrenWebView.h`](../include/WrenWebView.h);
what works in this release is in [This release](this-release.md).

[Adding web pages to an existing app](embedding.md) says when to use a web view
and when a tab, and shows one serving pages from an app's bundle. The page can
show tabs inside itself ([views in a web view](views-in-web-views.md)) and call
your app ([messages](messages.md)).

```swift
guard let webView = WebView(profile: profile, url: "app://ui/index.html") else { return }
let view = webView.view
view.frame = window.contentView!.bounds
view.autoresizingMask = [.width, .height]
window.contentView!.addSubview(view)
```

In C: `Wren_CreateWebView`, `Wren_GetWebViewNativeView`, `Wren_LoadWebView`,
`Wren_ReloadWebView`, `Wren_GetWebViewState`, `Wren_FocusWebView`,
`Wren_OpenWebViewDevTools`, `Wren_DestroyWebView`.

## Rules

- **You own it.** The page lives until you destroy the view (in Swift, until the
  `WebView` is released), which also takes it out of its window. While it lives,
  Chromium keeps its Profile loaded.
- **Place it like a tab's view.** The page is visible while the view is in a
  window and not hidden, its size is the view's, and its scale is the window's
  screen's. Give the view a whole number of points each way.
- **It is not a tab.** It has no tab ID, and none of a tab's or a Browser's
  changes are reported for it (its own are: see
  [Loading and crashing](#loading-and-crashing)); it is not in history or in a
  restored session. A request to your scheme from its page names the view
  (`SchemeRequest.webView`) and no tab.
- **Your menu's shortcuts work in it.** A key the page does not handle goes to
  the app's main menu.
- **DevTools opens in its own window**: `openDevTools()` makes a DevTools
  Browser, which arrives in `browserOpened` like any other. Make the view with
  `devTools: false` to leave it out of a shipped build: its text fields' menu
  then has no Inspect and `openDevTools()` throws.
- **You choose what debugging tools are told it is**, when you make it:
  `kind: .page` (the default) lists it among the tabs for Puppeteer,
  chromedriver and `chrome://inspect`, as an Electron window is listed;
  `kind: .browserUi` marks it as the browser's own UI, which those tools are not
  offered as a tab and reach only by asking for it. DevTools works in full for
  both.
- **It can be see-through.** Made with `transparent: true`, the page is
  transparent where it draws nothing, so what you put behind the view in its
  window shows there: an `NSVisualEffectView`, for a translucent window behind
  an HTML UI. What the page draws, its CSS backgrounds among it, still covers.
  Wren changes nothing of the window's. It holds for every page the view loads.

## Trusted origins

What your page gets from your app, it gets by an option you set when you make
the view, and only while it is one of your own pages:

| The page has                                            | Option (Swift, C)                                |
| ------------------------------------------------------- | ------------------------------------------------ |
| [`wren`](messages.md)                                   | `messages`, `WrenWebViewOptions.messages`        |
| [Marks for Chromium's bubbles](anchors-in-web-views.md) | `anchors`, `WrenWebViewOptions.anchors`          |
| [`<wren-view>`](views-in-web-views.md)                  | `embedsViews`, `WrenWebViewOptions.embeds_views` |
| [Areas that move the window](#dragging-the-window)      | `dragsWindow`, `WrenWebViewOptions.drags_window` |

By default "your own pages" are the pages on [your schemes](schemes.md). One
setting widens that, for one view, fixed for the view's life:

|       | Where                                                                          |
| ----- | ------------------------------------------------------------------------------ |
| Swift | `WebView(profile:url:…, trustedOrigins: ["http://localhost:5173"])`            |
| Rust  | `WebViewOptions { trusted_origins: vec!["http://localhost:5173".into()], .. }` |
| C     | `WrenWebViewOptions.trusted_origins` and `trusted_origin_count`                |

Each entry is an origin: a scheme, a host and a port, with no path. A page on a
trusted origin is your own page to that view, exactly as a page on your scheme
is. The list turns nothing on by itself: each thing still needs its option, and
a list given with none of them is refused (the view is not made).

It exists for a development server that serves your UI while you build it, so
that the same page works from `http://localhost:5173` as from `app://ui/`.
**List nothing in a build you ship**: whoever serves a trusted origin reaches
every handler you set for the view, places your Browsers' anchors and shows any
tab whose ID it writes.

What it does not do:

- **It does not open your schemes.** They stay closed to a trusted origin as to
  any website, so a page served by your development server cannot load
  `app://ui/icon.png` or `fetch("app://…")`. Serve the page's own files from the
  same server, by relative URLs. If the page must load from your scheme while
  you develop, declare that scheme [web accessible](schemes.md) in your
  development build.
- **It is one view's.** The same origin in a tab, in a frame of your page, or in
  another view that did not list it has nothing.
- There is no way to add an origin later, to trust one for every view at once,
  or to take your own schemes off the list.

## Dragging the window

A window whose title bar is HTML is moved by the page. Make the view with
`dragsWindow` (C: `WrenWebViewOptions.drags_window`), and say in CSS which areas
move the window:

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
no script for it. Over a drag area the view behaves as a title bar:

- A left mouse-down moves the window and does not reach the page. A double click
  does what the user chose in System Settings for a title bar (zoom, minimise,
  nothing).
- Right clicks, control-clicks, scrolling and the pointer's movement reach the
  page as anywhere else, so a drag area keeps its `contextmenu` event, its hover
  styles and a tab strip's scrolling.

Rules:

- **You make the window.** Wren changes nothing of its style. Give it a
  full-size content view and a hidden, transparent title bar, and place the web
  view under it. The window's own buttons (close, minimise, zoom) stay the
  window's and are above the page: leave them room, about 78 points at the left.
- **Mark what must take clicks.** A drag area is the box of the element with the
  property, and so is the box of each element inside it: in Chromium 155 the
  property is inherited. Each button, tab and field inside needs
  `app-region: no-drag`.
- **Only the page moves the window.** In a title bar's band macOS moves a window
  from a press by itself, without asking your app, wherever it sees nothing
  solid, and a page drawn in another process looks empty to it. A view made with
  `dragsWindow` keeps all of itself from that, as Chromium's own windows do, so
  a press on your page's tabs never moves the window and its drag areas do. A
  view made without it is left to macOS.
- **A `<wren-view>` is `no-drag` by default**, so a page whose whole body drags
  still has tabs that take their clicks. Your own CSS can say otherwise.
- **Only your own page.** The areas count while the view's page is on one of
  your schemes or a [trusted origin](#trusted-origins). A frame inside the page
  and a tab the page shows declare nothing, and a view that has gone to any
  other site drags nothing.
- **They are the page's that is there.** They go when the page reloads, goes
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

| `state`     | Means                                                                                                                                                                                                                                                      |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `loading`   | The page is loading, as a tab's `loading` says.                                                                                                                                                                                                            |
| `loadError` | The last load failed: Chromium's network error for it, a negative number (`-105` the name was not found, `-102` the connection was refused, `-106` no network, `-6` your scheme's handler found nothing). 0 otherwise, and from when the next load starts. |
| `crash`     | Why the page is gone, if it is: its process crashed, was killed or ran out of memory. The view shows nothing.                                                                                                                                              |
| `drawn`     | The page shown now has drawn once: its first frame with something in it has reached the view, as Electron's `ready-to-show`. False from when another page commits, and while crashed.                                                                      |
| `isLoaded`  | Not loading, no `loadError`, no `crash`.                                                                                                                                                                                                                   |

In C the state is `Wren_GetWebViewState`, and each change is a
`WREN_CHANGE_WEB_VIEW` record in `on_changed` that names the view
(`WrenChange.web_view`). In Swift the same record also reaches
`AppDelegate.changed`, as `.webView`.

- **A failed load shows Chromium's error page** in the view, or leaves the page
  that was there. Load something of your own if your UI should not show it. A
  page your server answered, with any status, did not fail.
- **`reload()`** (`Wren_ReloadWebView`) loads the page again at the address it
  has now, which may not be the one you gave it. Use it after a crash, after a
  failed load, or to pick up changed files. `load(_:)` does the same for an
  address you give.
- **Show a new window when `drawn` turns true**, not before, and it is never
  blank. The page draws while its view is in a window, shown or not, so add the
  view first. Show it on a load error or a crash too (Chromium's error page
  draws; a crashed page does not), or the window may never appear.
- **While the page is crashed** its [drag areas](#dragging-the-window) are gone
  with it, so a window whose title bar it drew cannot be moved by it.

## A bare page

A web view has little of what Chromium's UI gives a tab's page. Only a text
field has a context menu, a short one of Wren's (spelling suggestions, undo,
cut, copy, paste, select all, Inspect); elsewhere there is none: the page sends
its own items from its `contextmenu` event, and your app shows them
([Menus](menus.md#context-menus)). `alert()`, permission prompts, file choosers
and `window.open` do nothing. That suits your own UI. A website that needs them
belongs in a tab.

## Not offered yet

The page's address and title.

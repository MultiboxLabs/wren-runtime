# Web views

A web view puts a page anywhere in your own window: a panel of your app's UI, a whole window drawn in HTML, or a website with no browser around it. It isn't a tab, so it needs no Browser, and it comes without a tab's dialogs and history. Your own pages in a web view can also do things no website can, such as call your app, drag the window, and draw Liquid Glass.

[Web pages in an existing app](embedding.md) says when to use a web view and when to use a tab. The code here is in Swift, with Rust where it differs. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## Making a web view

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

The web view lives until you let go of it. In Swift that's when the `WebView` is released, and in Rust when it's dropped. Its view then leaves its window, and the page ends. While it lives, Chromium keeps its Profile loaded.

Place its view as you would a tab's. The page shows while the view is in a window and isn't hidden, at the view's size and the screen's scale. Give the view whole-point sizes, because Chromium drops any fraction of a point.

The page can come from [one of your URL schemes](schemes.md), from a [development server](#trusting-a-development-server) while you build it, or from any website.

## How a web view differs from a tab

A web view is a page on its own, with none of the browser around it. Your own UI doesn't want most of what a browser adds, so the differences mostly help.

- **It isn't part of a Browser.** It has no tab ID, and Wren Runtime reports only [its own changes](#knowing-when-it-loads-fails-or-crashes). Its pages don't go in history or in the session. A request to your scheme from it names the web view, in `SchemeRequest.webView`, and no tab.
- **It has none of Chromium's dialogs.** `alert()`, permission prompts, file choosers and `window.open` do nothing. A website that needs them belongs in a tab.
- **Context menus are yours.** Text fields have a short editing menu, with spelling suggestions, Undo, Cut, Copy, Paste, Select All, and Inspect unless [only your app opens DevTools](#devtools). Elsewhere, the page sends its own items from its `contextmenu` event, and your app shows them. See [Menus](menus.md#context-menus).
- **It behaves like an app's UI, not a web page.** A pinch or a two-finger double tap doesn't zoom it, and a two-finger swipe doesn't go back, as in Chrome's own toolbar. The page still gets the Control-wheel event a pinch starts with, so a page that zooms its own map still can.
- **It still stretches at its ends.** Like any page that can scroll, it stretches when a scroll goes past its end. For bars that should stay still, as Chrome's toolbar does, give the page's root `overscroll-behavior: none`.
- **Your shortcuts work in it.** Keys the page doesn't use go on to your main menu.

Tabs and side panels that a web view's page [shows inside itself](views-in-web-views.md) are pages of their own, and behave as they do in Chrome. A pinch over a tab zooms that tab, a scroll over it scrolls the tab and never your page, and a swipe over it goes back or forward, with macOS's arrow at its edge, when "Swipe between pages" is on in the trackpad settings.

## Knowing when it loads, fails or crashes

A web view tells you what its page is doing, and when that changes:

```swift
webView.changed = { state in
    if state.crash != .none {
        try? webView.reload()                       // the page's process died
    } else if state.loadError != 0 {
        try? webView.load("app://ui/offline.html")  // couldn't load
    } else if state.drawn {
        window.makeKeyAndOrderFront(nil)            // show the window now
    }
}
let now = webView.state                             // readable at any time
```

In Rust, `web_view.set_changed` takes the same kind of closure, and `web_view.state()` reads the state.

| `state` | What it says |
| --- | --- |
| `loading` | The page is loading. |
| `loadError` | Chromium's network error for the last load, a negative number, or 0 once the next load starts. |
| `crash` | Why the page is gone, if it is: its process crashed, was killed, or ran out of memory. The view then shows nothing. |
| `drawn` | The page has drawn its first frame with something in it, like Electron's `ready-to-show`. It's false again once another page starts, and while the page is crashed. |
| `devtools` | DevTools is open for the page. |
| `isLoaded` | Not loading, with no load error and no crash. |

The usual errors are `-105`, when the name wasn't found, `-102`, when the connection was refused, `-106`, when there's no network, and `-6`, when your scheme's handler found nothing.

- **Show a new window when `drawn` turns true,** so it's never blank. The page draws while its view is in a window, even a hidden one, so add the view first. Show the window on a load error or a crash too, or it might never appear.
- **A failed load shows Chromium's error page,** or leaves the page that was there. Load a page of your own if your UI shouldn't show it. A page your server answered, with any status, didn't fail.
- **`reload()` loads the page that's there now,** which may not be the address you gave. Use it after a crash, after a failed load, or to pick up changed files. `load(_:)` loads an address you give.
- **While the page is crashed,** its [drag areas](#a-title-bar-drawn-in-html) go with it, so it can't move its window.

The same change also reaches `AppDelegate.changed`, as a `.webView` change that names the view.

## DevTools

A web view's DevTools opens in a window of its own. `openDevTools()` opens a DevTools Browser, which arrives in `browserOpened` like any other, and shows once you put its tab's view in a window. If your `browserOpened` closes everything, DevTools closes as soon as it opens.

`closeDevTools()` closes it, and `state.devtools` says whether it's open, so a menu item can toggle it:

```swift
if webView.state.devtools { try? webView.closeDevTools() } else { try? webView.openDevTools() }
```

```rust
let open = web_view.state().map(|state| state.devtools).unwrap_or(false);
let _ = if open { web_view.close_dev_tools() } else { web_view.open_dev_tools() };
```

The `developerTools` command in your menu opens a tab's DevTools, never a web view's.

To let only your app open DevTools, make the view with `disallowUserDevToolsActivation: true`. Text fields' menus then have no Inspect, and `openDevTools()` still works. A shipped build that should have no DevTools at all leaves out whatever calls it.

## What your own pages can do

Your own pages can do things websites can't, each turned on by an option when you make the web view:

| The page gets | Option |
| --- | --- |
| [`wren`, to call your app and hear from it](messages.md) | `messages` |
| [Buttons that Chromium's bubbles open from](anchors-in-web-views.md) | `anchors` |
| [Tabs and side panels in `<wren-view>`](views-in-web-views.md) | `embedsViews` |
| [Areas that drag the window](#a-title-bar-drawn-in-html) | `dragsWindow` |
| [Where a trackpad scroll is](#following-a-trackpad-scroll) | `wheelPhases` |

Your own pages are the pages on [your URL schemes](schemes.md). A website the web view goes to gets none of these, so a link out of your UI can't reach your app.

### Trusting a development server

While you build your UI, you may want to serve it from a development server, such as Vite at `http://localhost:5173`, with live reloading. `trustedOrigins` makes that server's pages count as your own, in one web view:

```swift
let webView = WebView(
    profile: profile, url: "http://localhost:5173/", messages: true,
    trustedOrigins: ["http://localhost:5173"])
```

```rust
let options = WebViewOptions {
    messages: true,
    trusted_origins: vec!["http://localhost:5173".into()],
    ..WebViewOptions::default()
};
let web_view = WebView::new(&profile, "http://localhost:5173/", &options);
```

Each entry is an origin, a scheme, a host and a port, without a path. The list turns nothing on by itself, and each ability still needs its option. A list with none of those options is refused, and the web view isn't made. The list is fixed for the web view's life, and applies to no other web view, tab or frame.

List nothing in a build you ship. Whoever serves a trusted origin can call every handler you set for the web view, place your Browsers' bubbles, and show any tab whose ID it writes.

A trusted origin still can't load from your schemes, so a page from your development server can't load `app://ui/icon.png`. Serve the page's files from the same server, with relative URLs. If the page has to load from your scheme while you develop, make that scheme [web accessible](schemes.md) in development builds.

## A title bar drawn in HTML

If your page draws the window's title bar, it can also move the window, as Electron apps do. Make the web view with `dragsWindow`, and mark in CSS which parts drag the window:

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

The page needs no script for it. Over a drag area, the web view behaves like a title bar:

- A press with the left button moves the window, and the page doesn't see it.
- A double click does what the person chose for title bars in System Settings, such as zoom or minimise.
- Right clicks, scrolling and pointer movement reach the page as anywhere else, so a drag area keeps its `contextmenu` event, its hover styles and a tab strip's scrolling.

To let clicks in drag areas reach the page too, add `dragAreaClicks: true`. A press then moves the window only once the pointer moves more than 3 points. A press released before that is a click, which the page gets all at once on release. A double click reaches the page as two clicks, then does the title bar's action.

A few things to set up:

- **The window is yours.** Wren Runtime doesn't change its style. Give it a full-size content view and a hidden, transparent title bar, as above. The close, minimise and zoom buttons stay above the page, so leave them about 78 points at the left.
- **Mark what takes clicks.** `app-region` is inherited, so everything inside a drag area drags too. Give each button, tab and field inside one `app-region: no-drag`. A `<wren-view>` is `no-drag` already.
- **Only the page moves the window.** macOS normally moves a window from a press in its title bar band wherever it sees nothing solid, and a page drawn by another process looks empty to it. A web view made with `dragsWindow` stops that, as Chromium's own windows do, so a press on your page's tabs never moves the window by mistake.
- **Only your own page.** Drag areas count only on your own pages, not in a frame inside them or a tab they show. They go when the page reloads, navigates or crashes, and come back with the next page's first layout.

The page can't resize the window.

## A see-through page

Make the web view with `transparent: true`, and the page is transparent wherever it draws nothing. What you put behind the view then shows through, such as an `NSVisualEffectView` for a translucent window behind an HTML UI. Whatever the page draws still covers it, CSS backgrounds included. This holds for every page the web view loads, and Wren Runtime changes nothing about the window.

## Liquid Glass

Pages in a web view made with `glass: true` can draw elements as Liquid Glass, the material of macOS 26's controls. The glass shows what's behind the element, blurred and bent, as AppKit's `NSGlassEffectView` does.

Liquid Glass is experimental. Any release can change or remove it, minor ones included, so give each element with glass a fallback ([Compatibility](compatibility.md#what-each-release-keeps)).

`WebView.canDrawGlass`, `WebView::can_draw_glass()` in Rust, says whether this Mac can draw glass. It's true on macOS 26 or later, and doesn't change while your app runs. Where it's false, a web view made with `glass` keeps the properties, and your page draws its fallback, so make the web view with glass only where it draws:

```swift
let webView = WebView(profile: profile, url: "app://ui/index.html", glass: WebView.canDrawGlass)
```

One thing still differs: a web view made without `glass` draws a `backdrop-filter` as Chrome does, from your page's own pixels, while one made with `glass` draws a blur with Core Animation even where glass can't draw. So in a [see-through](#a-see-through-page) page, `glass: true` keeps a blur of the window behind it on those Macs too.

```css
.toolbar {
  -wren-glass: regular; /* or clear */
  border-radius: 18px;
  background: white; /* drawn only where glass can't be */
}
```

The glass fills the element's border box, rounded by the largest of its corner radii, in place of its background, border and shadow. It only appears on an element that paints one of those, so give the element a background even though it won't show. The element's content and focus ring draw on top of the glass. Behind the glass is the page, and, where a [see-through](#a-see-through-page) page draws nothing, the window behind it.

- **Light or dark.** The glass follows the element's `color-scheme`, so one element can have dark glass in a light page.
- **Tint.** `-wren-glass-tint` tints the glass toward a colour, as AppKit's tinted glass does. Its default, `auto`, leaves it untinted. A colour the web view hasn't drawn before may show untinted for a frame or two.
- **The window and the system.** While the window isn't active, the glass is subdued and untinted, as AppKit's is. It follows Reduce Transparency and Increase Contrast too.
- **A fallback.** Sometimes glass can't draw, such as while the window is captured for screen sharing. Then the element draws as its CSS says, with its background, or a `backdrop-filter`, which on an element with glass draws only where the glass doesn't. Chromium also stops drawing glass for the whole window while your page has very many layers, or a `backdrop-filter` that Core Animation can't draw. Tabs and side panels in a `<wren-view>` draw apart from your page, so a website there never stops your glass.
- **Blurs beside glass.** A `backdrop-filter` with `blur()` and colour filters such as `saturate()` keeps your glass drawing, and blurs whatever is behind it, glass included. A `backdrop-filter` with `url()` or `drop-shadow()`, or on an element with a mask, a `clip-path`, a `filter` or corners of different radii, stops glass while it shows.
- **Every page in the web view can use it,** whatever its origin, because it only changes how the page's own pixels draw.
- **Websites can't detect it.** In tabs, and in web views made without `glass`, the properties don't exist, and `CSS.supports("-wren-glass: regular")` is false. So no website can tell your browser from Chrome by them.

Glass comes only as a rounded rectangle, separate glass shapes don't merge, and it's only on macOS.

## Following a trackpad scroll

Pages in a web view made with `wheelPhases: true` know where a trackpad scroll is: whether the fingers have landed, are moving, or have lifted, and whether the scroll glides on after them. A page can then follow two fingers as a native control does, and settle when they lift.

```swift
let webView = WebView(profile: profile, url: "app://ui/index.html", wheelPhases: true)
```

```js
addEventListener("wheel", (event) => {
  if (event.wrenPhase === "changed") follow(event.deltaX);
  if (event.wrenPhase === "ended") settle();
});
```

Each `wheel` event has `wrenPhase`, for the fingers, and `wrenMomentumPhase`, for the glide after they lift. A release's `page/wren.d.ts` has their types.

| Value | What it means |
| --- | --- |
| `"none"` | Not in this part of the scroll. A mouse wheel's events have `"none"` for both. |
| `"may-begin"` | The fingers landed and haven't moved. |
| `"began"` | The fingers started to move. |
| `"changed"` | The fingers moved, or are resting on the trackpad. |
| `"ended"` | The fingers lifted, or the glide stopped. |
| `"cancelled"` | The fingers lifted without moving. |

- **Some events have no delta.** The page also hears the fingers land and lift, and the glide end, as `wheel` events with no delta that can't be cancelled. Websites never get these, so a handler that counts events or divides by a delta needs to skip them.
- **The lift comes late.** It arrives up to 100 ms after the fingers leave, while Chromium waits to see whether a glide follows. If one does, the lift comes just before the glide's first event.
- **Speed is yours to work out.** Divide a delta by the time since the last event. Chromium may merge events while the page is busy, adding up their deltas.
- **Scrolling is unchanged.** The page scrolls as before, and `preventDefault()` works as in Chrome.
- **Websites can't detect it.** Only your own pages have the phases. Elsewhere `"wrenPhase" in WheelEvent.prototype` is false. Only a Mac's trackpad gives phases.

## How debugging tools see it

By default, Puppeteer, chromedriver and `chrome://inspect` list a web view among the tabs, as they list an Electron window. Make it with `kind: .browserUi` to mark it as the browser's own UI instead. Those tools then don't offer it as a tab, and reach it only when asked for it by name. DevTools works fully either way.

## Not available yet

A web view can't tell you its page's address or title yet.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `WebView(profile:url:…)` | `WebView::new`, `WebViewOptions` | `Wren_CreateWebView`, `WrenWebViewOptions` |
| releasing the `WebView` | dropping the `WebView` | `Wren_DestroyWebView` |
| `webView.view` | `WebView::view` | `Wren_GetWebViewNativeView` |
| `webView.load(_:)` | `WebView::load` | `Wren_LoadWebView` |
| `webView.reload()` | `WebView::reload` | `Wren_ReloadWebView` |
| `webView.state` | `WebView::state` | `Wren_GetWebViewState` |
| `webView.changed` | `WebView::set_changed` | `WREN_CHANGE_WEB_VIEW` |
| `webView.focus()` | `WebView::focus` | `Wren_FocusWebView` |
| `webView.openDevTools()` | `WebView::open_dev_tools` | `Wren_OpenWebViewDevTools` |
| `webView.closeDevTools()` | `WebView::close_dev_tools` | `Wren_CloseWebViewDevTools` |
| `kind: .page`, `.browserUi` | `WebViewKind::Page`, `WebViewKind::BrowserUi` | `WREN_WEB_VIEW_PAGE`, `WREN_WEB_VIEW_BROWSER_UI` |

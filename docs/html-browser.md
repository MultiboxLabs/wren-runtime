# A browser drawn in HTML

Your browser's whole window can be one web page. The tab strip, the toolbar and the address field are HTML, the tabs' pages show inside that page, and Chromium's own bubbles open from its buttons. The page is also the part of your browser you can take to another platform, because only a small native host is written for each one.

This page shows how the pieces fit together, the way the web sample has them. The sample comes in a release's `samples` folder, with a Rust host in `host` and a React page in `ui`. Each piece has a guide of its own, linked below.

## Two sides

Like an Electron app, with its renderer and main process, your browser has two sides:

|  | What it is | Written in |
| --- | --- | --- |
| The page | Your UI, in a [web view](web-views.md) | HTML, CSS and your own script |
| The host | The window, and the answers to the page's calls | Swift, Rust or C, on Wren Runtime |

The page has none of Wren Runtime's API. The two sides talk through [messages](messages.md), with names you choose.

## The window

One web view fills the window, made with every option the page needs:

```swift
let webView = WebView(
    profile: uiProfile, url: "app://ui/index.html", kind: .browserUi,
    embedsViews: true,      // <wren-view>: tabs and side panels in the page
    messages: true,         // wren.invoke and wren.on
    anchors: true,          // buttons that Chromium's bubbles open from
    dragsWindow: true)!     // app-region: drag moves the window
window.contentView!.addSubview(webView.view)
browser.attach(to: webView.view)
```

```rust
let web_view = WebView::new(
    &ui_profile,
    "app://ui/index.html",
    &WebViewOptions {
        kind: WebViewKind::BrowserUi,
        embeds_views: true,
        messages: true,
        anchors: true,
        drags_window: true,
        ..WebViewOptions::default()
    },
)
.expect("a web view");
// Add web_view.view() to the window's content view with objc2, then:
browser.attach_to_view(Some(web_view.view()));
```

- **Attach the Browser to the web view's own view.** The window then counts as the Browser's. The Browser is active while the window is key, and Chromium centres its dialogs over the tab's element.
- **Give the UI a Profile of its own.** The sample makes one ephemeral Profile at launch, with `WrenApp.createEphemeralProfile`, for every window's page. The UI's storage then stays out of the person's own sites, and a window that shows several Profiles belongs to none of them.
- **Serve the page from [your scheme](schemes.md),** from files in your app bundle.
- **Let the page draw the title bar.** Make the window with a hidden title bar, and mark what moves it in CSS, as [A title bar drawn in HTML](web-views.md#a-title-bar-drawn-in-html) shows. The page can't tell where the window's own buttons are, so tell it how much room to leave.

## The page shows, the host acts

Chromium owns the tabs. The host reads them when the page asks or a change arrives, and keeps no copy. The page keeps the last copy it was sent, and draws from it. A click in the page changes nothing by itself. It calls the host, and the page hears the result as a change, like every other change.

```tsx
// The page
<div className="tab" onPointerDown={() => wren.invoke("tabs.activate", tab.id)}>
  {tab.title}
</div>
<wren-view data-id={`tab-${active}`} />
```

```swift
// The host, in Swift
try webView.handle("tabs.activate") { (id: Int32) -> Void in
    guard let tab = BrowserTab(id), tab.browser == browser else { throw WrenError.tabUnavailable }
    try tab.activate()
}
delegate.changed = { changes in
    if changes.contains(where: { $0.kind == .tabs && $0.browser == browser }) {
        try? webView.send("tabs.changed", tabList())
    }
}
```

```rust
// The host, in Rust
let window_browser = browser.clone();
web_view.handle("tabs.activate", move |id: i32| {
    let tab = BrowserTab::from_id(mt, id)
        .filter(|tab| tab.browser().as_ref() == Some(&window_browser))
        .ok_or(Error::TabUnavailable)?;
    tab.activate()
})?;
// In App::changed, for each .tabs change to this window's Browser:
web_view.send("tabs.changed", &tab_list(&browser))?;
```

- **The page asks first.** Nothing is kept for a page that isn't there yet, so the page's first act is to ask for everything, and the host sends changes from then on.
- **Check every ID a call names** against the window's own Browser, as above.
- **Send icons in messages,** as `data:` URLs. A favicon is a few kilobytes, so send a tab whole once, and after that only what changed.
- **Answer Chromium's questions in the host.** What Chromium asks inside its own call, such as `popover` and `fullscreen`, the host answers there, then tells the page.

Keep every message name in one file that types the page's code, as the sample's `contract.ts` does. Keep the host's side of it in code that doesn't touch the platform, as the sample's Rust crate `bridge` does. Both then go with the page to the next platform.

## Each piece of the browser

[The browser's UI, piece by piece](ui-checklist.md) lists every piece. In a page, they work like this:

| Piece | The page | The host |
| --- | --- | --- |
| Tabs' pages and side panels | A [`<wren-view>`](views-in-web-views.md) for each, by ID. A split is two elements with a divider of your own. | Sends the IDs |
| Chromium's bubbles, such as page info, permission prompts and the star | A [mark](anchors-in-web-views.md) over each button | Names each mark once as the Browser's anchor, and opens a bubble when the page asks |
| Extensions' buttons | A mark over each, drawn while it's pinned or popped out | Names `extension:<id>` for each, and sends the extensions' state |
| Your own popover in place of Chromium's, such as an extensions panel | Draws it from what the host sends, and says when it closes | Answers `popover` with its own, sends the data, and closes the request |
| The address field | An `<input>` and a list, from Chromium's location state and suggestions | Passes on the typed text and the suggestions, from [`WrenOmnibox.h`](../include/WrenOmnibox.h) |
| A crashed tab, the status bubble | HTML of its own over the tab's element | Sends the tab's state |
| Full screen | Hides its bars while a tab is in full screen | Takes the window in and out of full screen, and says which tab |
| Context menus | Sends the items, and waits for the choice | Shows them with [`showContextMenu`](menus.md#context-menus), and answers with the choice |
| The main menu | Nothing | Sets it [as data](menus.md#the-main-menu), with `setMainMenu`. Keys the page doesn't use go to it. |
| DevTools for the page itself | A menu item or a key | `openDevTools()` and `closeDevTools()` on the web view. See [Web views](web-views.md#devtools). |

## While you build it

Serve the page from a development server, such as Vite, and have development builds of your app load it from there and [trust its origin](web-views.md#trusting-a-development-server):

```swift
let webView = WebView(
    profile: uiProfile, url: "http://localhost:5173/", kind: .browserUi,
    embedsViews: true, messages: true, anchors: true, dragsWindow: true,
    trustedOrigins: ["http://localhost:5173"])!
```

In Rust, put the origin in `WebViewOptions::trusted_origins`.

The page then has everything it has on your scheme, and your edits show without rebuilding the app. Trust nothing in a build you ship. The web sample reads its server's address, from `WREN_WEB_SAMPLE_UI`, only when it isn't signed by a team.

## What stays native

To bring the browser to another platform, you keep the page and write the host again, with the same Wren Runtime calls in that platform's language. The main menu and context menus are [data](menus.md), which Wren Runtime shows, and so is the clipboard, which the sample's bridge [reads and writes](menus.md#copying-and-pasting-from-your-own-commands) through Wren Runtime.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `WebView(profile:url:…)` | `WebView::new`, `WebViewOptions` | `Wren_CreateWebView`, `WrenWebViewOptions` |
| `kind: .browserUi` | `WebViewKind::BrowserUi` | `WREN_WEB_VIEW_BROWSER_UI` |
| `WrenApp.createEphemeralProfile` | `MainThread::create_ephemeral_profile` | `Wren_CreateEphemeralProfile` |
| `browser.attach(to:)` | `Browser::attach_to_view` | `Wren_AttachBrowserToView` |
| `webView.handle` | `WebView::handle` | `Wren_SetPageCallHandler` |
| `webView.send` | `WebView::send` | `Wren_SendPageMessage` |
| `BrowserTab(id)` | `BrowserTab::from_id` | `WrenTabID` |

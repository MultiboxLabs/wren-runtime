# Web pages in an existing app

Wren Runtime isn't only for browsers. If your Mac app needs a web page somewhere, such as a help window, a sign-in page, a dashboard, or a whole pane written in HTML, Wren Runtime can give it one, and your app stays native. Your windows, views and menus are still your own, and each page is one more view among them.

Compared with moving your app to Electron, you keep your native code and UI. Compared with `WKWebView`, you get Chromium's rendering, its DevTools and Chrome's extensions. This page shows what changes in your app, and which of Wren Runtime's two kinds of page to use.

The code here is in Swift. The Rust SDK has the same calls, with the same names in Rust's case.

## What changes in your app

Four things change. The first is the one that matters most.

### Chromium runs the event loop

Chromium has to start before `NSApplication` does, and then it runs the event loop itself. So your app's entry point becomes `WrenApp.run`, in place of `NSApplicationMain` or a SwiftUI `@main` `App`.

```swift
// main.swift
import AppKit
import Wren

var delegate = AppDelegate(browserOpened: { browser, _, _ in browser.close { _ in } })
delegate.ready = ready              // create your windows here

var startup = Startup(userDataDirectory: directory, keychain: .mock)
startup.schemes = [Scheme("app", privileges: [.standard, .secure, .fetch])]
exit(WrenApp.run(startup, delegate) == .normal ? 0 : 1)
```

What you did in `applicationDidFinishLaunching` moves to `ready`. The other events an app delegate usually handles arrive as Wren Runtime's callbacks: `reopen` for a click on the Dock icon, `openURLs` for links and files, and `willQuit` when the app quits. Windows, view controllers, and SwiftUI views in `NSHostingView` all work as before.

A SwiftUI app built on `App` and `Scene` needs an AppKit entry point and AppKit windows first, and its views carry over.

If your app runs another event loop, such as an embedded language runtime's, keep that loop on a thread of its own. When it has work for the main thread, hand it over with `WrenApp.postToMainThread`. The work runs from Chromium's event loop, even while a menu is open. Work still waiting when the app quits doesn't run.

### `wren bundle` makes the app

Chromium needs its framework and helper apps inside your app bundle, signed together with it. [`wren bundle`](wren-cli.md) makes that bundle from your executable and a short manifest, which also names the Wren Runtime release your app uses. So you build an executable, with SwiftPM, Cargo or any build system that makes one, and bundle it afterwards. Bundling from inside an Xcode app target hasn't been tried.

Chromium adds a few hundred megabytes to your app, as Electron does.

### A data folder

Chromium keeps a Profile on disk even if you never show a browser, with its cookies, caches and local storage. Give it a folder in Application Support, as `userDataDirectory`. Only one process at a time can use the folder. See [Startup](startup.md).

### Popups need an answer

Any page can call `window.open`, so every app handles `browserOpened`. An app that isn't a browser usually closes what arrives, as above, or opens the address in the person's own browser.

DevTools arrives there too. `openDevTools()` on a web view opens a `.devtools` Browser, so an app that closes everything also closes DevTools as it opens. To inspect your pages, show that Browser in a window instead, as [Windows and tabs](windows-and-tabs.md#windows-chromium-opens) shows.

```swift
var delegate = AppDelegate(browserOpened: { browser, type, _ in
    if type == .devtools { showInWindow(browser) } else { browser.close { _ in } }
})
```

## Two kinds of page

|  | Web view | Tab in a Browser |
| --- | --- | --- |
| What it is | A plain page you own | A real browser tab |
| Use it for | Your own UI and content | Websites |
| Needs a Browser | No | Yes |
| Dialogs, permission prompts, file choosers, `window.open` | No | Yes, Chromium's |
| History, sessions, extensions | No | Yes |
| Tells you when it loads, fails or crashes | Yes | Yes |
| Tells you its title and address | No | Yes |
| Messages between the page and your app | Yes | No |
| DevTools | In a window of its own | Docked, or in a window |

Use a web view for pages you wrote. It's lighter, and needs nothing else. What it lacks is what your own UI doesn't want anyway.

Use a tab for the open web. A site may ask for a permission, open a file chooser, show an alert, or sign in through a popup, and only a tab has Chromium's UI for those.

## Your own pages in a web view

Serve your pages from your app with a [URL scheme](schemes.md), and show them in a [web view](web-views.md):

```swift
let pages = Bundle.main.resourceURL!.appendingPathComponent("Pages")

func ready() {
    // Your pages, served from the app's bundle.
    try? WrenApp.handleScheme("app") { request in
        guard let url = URL(string: request.url) else { return .failure(.notFound) }
        let file = pages.appendingPathComponent(url.path).standardizedFileURL
        guard file.path.hasPrefix(pages.path + "/") else { return .failure(.accessDenied) }
        return .file(file.path)
    }
    WrenApp.openProfile { result in
        guard case .success(let opened) = result else { return }
        profile = opened
        startMyApp()
    }
}

func showHelp() {
    guard let profile, let view = WebView(profile: profile, url: "app://help/index.html") else {
        return
    }
    help = view                      // the page lives as long as this object
    let window = makeWindow()
    window.contentView!.addSubview(view.view)
    view.view.frame = window.contentView!.bounds
    view.view.autoresizingMask = [.width, .height]
    window.makeKeyAndOrderFront(nil)
}
```

List the `Pages` folder in the manifest's [`resources`](wren-cli.md#the-manifest), so `wren bundle` copies it into the app.

Your page and your app can talk to each other in two ways:

- **Messages.** Make the web view with `messages: true`. The page calls your app with `wren.invoke`, and your app sends to the page with `send`. See [Messages between a page and your app](messages.md).
- **Your scheme.** The page calls `fetch("app://help/api/status")`, and your scheme handler answers with JSON or anything else. A handler can answer later, so it can do real work first, but it can't see a request's body.

## A website in a tab

For a website, create a Browser and show its one tab. This is a browser's whole page area, without a tab strip:

```swift
func showSite(_ address: String, in container: NSView) {
    profile?.createBrowser(url: address) { result in
        guard case .success(let browser) = result, let page = browser.activeTab?.view else {
            return
        }
        site = browser
        browser.attach(to: container)     // Chromium's prompts appear over it
        container.addSubview(page)
        page.frame = container.bounds
        page.autoresizingMask = [.width, .height]
    }
}
```

`container` must already be in a window. `changed` tells you the page's title, address and whether it's loading. If the site can open links in new tabs, follow `.tabs` changes too. See [Windows and tabs](windows-and-tabs.md).

If two uses shouldn't share cookies and storage, give each its own Profile. A sign-in page in an ephemeral Profile, from `WrenApp.createEphemeralProfile`, leaves nothing behind when it closes.

## Before you decide

- **macOS on Apple silicon only,** for now.
- **Chromium starts with your app.** There's no way yet to start it later, when a page is first needed.
- **A crash in Chromium's own code takes your app with it.** Pages run in processes of their own, so a page that crashes doesn't.
- **Wren Runtime isn't at 1.0 yet.** The API still changes between releases, and your app is built and bundled with one release.

[What Wren Runtime can't do yet](limitations.md) lists what isn't available.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `WrenApp.run` | `wren::run` | `Wren_Main` |
| `WrenApp.postToMainThread` | `wren::post_to_main_thread` | `Wren_PostToMainThread` |
| `WrenApp.handleScheme` | `MainThread::set_scheme_handler` | `Wren_SetSchemeHandler` |
| `WebView` | `WebView` | `Wren_CreateWebView` |
| `WrenApp.createEphemeralProfile` | `MainThread::create_ephemeral_profile` | `Wren_CreateEphemeralProfile` |
| `AppDelegate.reopen`, `openURLs`, `willQuit` | `App::reopen`, `open_urls`, `will_quit` | `on_reopen`, `on_open_urls`, `on_will_quit` |

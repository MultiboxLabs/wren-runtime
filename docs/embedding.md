# Adding web pages to an existing app

Wren is not only for browsers. An app that wants a web page somewhere in its UI can use Wren for that and nothing more. The page might be a help window, a sign-in page, a dashboard, or a whole pane written in HTML. Your app stays a native app. Its windows, its views and its menus are still yours, and a page is one more view among them.

Wren is the alternative to moving the app into Electron. It is also the alternative to `WKWebView` when you need Chromium's rendering, DevTools or extensions. The code here is in Swift. The same calls in Rust have the same names in Rust's case, and each item's comment in the crate names its C function.

## What changes in your app

Four things change, and the first is the one that matters.

### 1. Chromium runs the event loop

Chromium has to start before `NSApplication` does, and it runs the event loop itself. So the app's entry point becomes `WrenApp.run`, in place of `NSApplicationMain` or SwiftUI's `@main` `App`. In Rust it is `wren::run`.

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

What you did in `applicationDidFinishLaunching` moves to `ready`. The events an app delegate usually handles arrive as Wren's callbacks: `reopen` for a click on the Dock icon, `openURLs` for URLs and files, and `willQuit` for termination. Windows, view controllers, and SwiftUI views in `NSHostingView` work as before.

A SwiftUI app built on `App` and `Scene` needs an AppKit entry point and windows first. Its views carry over.

An app that runs another event loop, such as an embedded runtime's, keeps it on a thread of its own. That thread waits for the loop's work, then hands the work to the main thread with `WrenApp.postToMainThread`. In Rust it is `wren::post_to_main_thread`, and in C `Wren_PostToMainThread`. The work runs from Chromium's event loop, also while a context menu is open. Work still waiting when the app quits is dropped unrun.

### 2. The app is bundled by `wren bundle`

Chromium needs its framework and helper apps inside your bundle, signed together with it. [`wren bundle`](wren-cli.md) makes that bundle from your compiled executable and a short manifest. The manifest also names the Wren release the app uses, which `wren install` links into the project. So the app is built as an executable, with SwiftPM, Cargo or any build system that makes one, and bundled afterwards. Bundling from inside an Xcode app target has not been tried.

The framework is large, a few hundred megabytes in every app, as with Electron.

### 3. A data directory

Chromium keeps a Profile on disk even if you never show a browser, with its cookies, caches and local storage. Give it a folder under Application Support, in `Startup.userDataDirectory`. Only one process at a time may use a folder. See [Startup](startup.md).

### 4. Popups need an answer

`browserOpened` is required because a page can call `window.open`. An app that is not a browser usually closes what arrives, as above, or opens the address in the user's own browser.

DevTools arrives there too. `openDevTools()` on a web view makes a `.devtools` Browser, and an app that closes everything closes DevTools as it opens. To inspect your pages, show that Browser in a window instead. See [Windows and tabs](windows-and-tabs.md#windows-chromium-opens).

```swift
var delegate = AppDelegate(browserOpened: { browser, type, _ in
    if type == .devtools { showInWindow(browser) } else { browser.close { _ in } }
})
```

## Two kinds of page

|  | Web view | Tab in a Browser |
| --- | --- | --- |
| What it is | A bare page you own | A real browser tab |
| For | Your own UI and content | Websites |
| Needs a Browser | No | Yes |
| Dialogs, permission prompts, file choosers, `window.open` | No | Yes, Chromium's |
| History, sessions, extensions on the page | No | Yes |
| Told when it loads, fails or crashes | Yes (`webView.changed`) | Yes (`changed`) |
| Told its title and address | No | Yes (`changed`) |
| Messages between the page and your app | Yes (`wren`) | No |
| DevTools | In a window of its own | Docked or in a window |

Use a web view for pages you wrote. It is the lighter thing, and it needs nothing else. What it lacks is what your own UI does not want.

Use a tab for the open web. A site may ask for a permission, open a file chooser, show an alert, or sign in through a popup. Only a tab has Chromium's UI for those.

## Your own pages in a web view

Serve the pages from your app with a [URL scheme](schemes.md), and show them in a [web view](web-views.md). In Rust the handler is `MainThread::set_scheme_handler`.

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

Put the `Pages` folder in the bundle with the manifest's [`resources`](wren-cli.md#the-manifest).

The page and your app can talk in two ways.

- **Messages.** Make the web view with `messages: true`. The page calls your app with `wren.invoke`, and your app sends to the page with `send`. See [Messages between a page and your app](messages.md).
- **Your scheme.** The page calls `fetch("app://help/api/…")`, and your scheme handler answers with JSON or anything else. A handler may answer later, so it can do real work first. It does not see a request's body.

## A website in a tab

For a site, make a Browser and show its one tab. This is the whole of a browser's page area, with no tab strip:

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

`container` must already be in a window. Follow `changed` for the page's title, address and loading state. Follow `.tabs` too if the site may open links in new tabs. See [Windows and tabs](windows-and-tabs.md).

If the data of two uses should not mix, give each its own Profile. A sign-in page in an ephemeral Profile, from `WrenApp.createEphemeralProfile`, leaves nothing behind.

## What to know before you commit

- **macOS on Apple silicon only,** for now.
- **Your app carries Chromium,** and starts it at launch. There is no way yet to start Chromium later, on first use.
- **A crash in Chromium's browser code takes the app with it.** Pages run in their own processes, and a crashed page does not.
- **Wren is before 1.0.** The API still changes between releases, and an app must be built and bundled with the same one.

The current gaps, for web views in particular, are listed at the end of [Web views](web-views.md) and in [This release](this-release.md).

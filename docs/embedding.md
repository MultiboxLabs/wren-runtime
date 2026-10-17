# Adding web pages to an existing app

Wren is not only for browsers. An app that wants a web page somewhere in its UI
(a help window, a sign-in page, a dashboard, a whole pane written in HTML) can
use it for that and nothing more. Your app stays a native app: its windows, its
views and its menus are still yours, and a page is one more view among them.

This is the alternative to moving the app into Electron, or to `WKWebView` when
you need Chromium: its rendering, its DevTools, its extensions.

## What changes in your app

Four things, and the first is the one that matters.

### 1. Chromium runs the event loop

Chromium has to start before `NSApplication` does, and it runs the event loop
itself. So the app's entry point becomes `WrenApp.run`, in place of
`NSApplicationMain` or SwiftUI's `@main` `App`:

```swift
// main.swift
import AppKit
import Wren

var delegate = AppDelegate(browserOpened: { browser, _, _ in try? browser.close() })
delegate.ready = ready              // create your windows here

var startup = Startup(userDataDirectory: directory, keychain: .mock)
startup.schemes = [Scheme("app", privileges: [.standard, .secure, .fetch])]
exit(WrenApp.run(startup, delegate) == .normal ? 0 : 1)
```

What you did in `applicationDidFinishLaunching` moves to `ready`. The events an
app delegate usually handles arrive as Wren's callbacks: `reopen` for a Dock
click, `openURLs` for URLs and files, `willQuit` for termination. Windows, view
controllers and SwiftUI views (in `NSHostingView`) work as before.

A SwiftUI app built on `App` and `Scene` needs an AppKit entry point and windows
first; its views carry over.

### 2. The app is bundled by `wren bundle`

Chromium needs its framework and helper apps inside your bundle, signed together
with it. [`wren bundle`](wren-cli.md) makes that bundle from your compiled
executable and a short manifest, which also names the Wren release the app uses
(`wren install` links it into the project), so the app is built as an executable
(with SwiftPM, or any build system that makes one) and bundled afterwards.
Bundling from inside an Xcode app target has not been tried.

The framework is large: a few hundred megabytes in every app, as with Electron.

### 3. A data directory

Chromium keeps a Profile on disk even if you never show a browser: cookies,
caches, local storage. Give it a folder under Application Support
(`Startup.userDataDirectory`), and note that only one running process may use a
folder. See [Startup](startup.md).

### 4. Popups need an answer

`browserOpened` is required because a page can call `window.open`. An app that
is not a browser usually closes what arrives, as above, or opens the address in
the user's own browser.

## Two kinds of page

|                                                           | Web view                | Tab in a Browser      |
| --------------------------------------------------------- | ----------------------- | --------------------- |
| What it is                                                | A bare page you own     | A real browser tab    |
| For                                                       | Your own UI and content | Websites              |
| Needs a Browser                                           | No                      | Yes                   |
| Dialogs, permission prompts, file choosers, `window.open` | No                      | Yes, Chromium's       |
| History, sessions, extensions on the page                 | No                      | Yes                   |
| Told when it loads, its title, its address                | No                      | Yes (`changed`)       |
| DevTools                                                  | In a window of its own  | Docked or in a window |

**Use a web view for pages you wrote.** It is the lighter thing, it needs
nothing else, and what it lacks is what your own UI does not want.

**Use a tab for the open web.** A site may ask for a permission, open a file
chooser, show an alert or sign in through a popup, and only a tab has Chromium's
UI for those.

## Your own pages in a web view

Serve the pages from your app with a [URL scheme](schemes.md), and show them in
a [web view](web-views.md):

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

Put the `Pages` folder in the bundle with the manifest's
[`resources`](wren-cli.md#the-manifest).

The page and your app can talk in one direction today: the page calls
`fetch("app://help/api/…")` and your scheme handler answers, with JSON or
anything else. A handler may answer later, so it can do real work first. There
is no call from your app into the page yet, and the handler does not see a
request's body.

## A website in a tab

For a site, make a Browser and show its one tab. This is the whole of a
browser's page area, with no tab strip:

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

`container` must already be in a window. Follow `changed` for the page's title,
address and loading state, and for `.tabs` if the site may open links in new
tabs ([Windows and tabs](windows-and-tabs.md)).

Give each kind of use its own Profile if their data should not mix: a sign-in
page in an ephemeral Profile leaves nothing behind
(`WrenApp.createEphemeralProfile`).

## What to know before you commit

- **macOS on Apple silicon only**, for now.
- **Your app carries Chromium**, and starts it at launch. There is no way yet to
  start Chromium later, on first use.
- **A crash in Chromium's browser code takes the app with it.** Pages run in
  their own processes and do not.
- **Pre-1.0.** The API still changes between releases, and an app must be built
  and bundled with the same one.

The current gaps, for web views in particular, are listed at the end of
[Web views](web-views.md) and in [This release](this-release.md).

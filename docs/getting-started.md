# Getting started

By the end of this page you'll have a Mac app that opens a window on a web page. The page runs in a real Chromium tab, so links, typing, context menus and permission prompts all work as they do in Chrome. In Swift it takes three files and four commands, and the same app in Rust is [further down](#the-same-app-in-rust). You won't compile Chromium at any point, because each Wren Runtime release comes with it already built.

## What you need

- A Mac with Apple silicon.
- Xcode or the Command Line Tools.
- Rust 1.85 or later, if you're writing the app in Rust.
- The `wren` command. Install it once:

  ```sh
  curl -fsSL https://wrenbrowser.com/runtime/cli/install.sh | sh
  ```

  [The `wren` command](wren-cli.md#installing-wren) has the details, including how to install another release or into another folder.

## 1. Describe the app

Make a folder for the project, and put a `wren-app.json` in it. It gives your app's name and bundle ID, and the Wren Runtime release it's built with:

```json
{
  "name": "MyBrowser",
  "displayName": "My Browser",
  "bundleId": "com.example.mybrowser",
  "wren": "0.5.0"
}
```

Then fetch that release:

```sh
wren install
```

The first time, `wren install` downloads the release. After that, every project on this Mac that names it shares one copy. The project gets a link to it at `.wren/runtime`, and your build files point there, so the project builds the same way on any Mac. The `.wren` folder keeps itself out of git.

## 2. Add the Swift SDK

Put this in `Package.swift`. It depends on the Swift SDK that came with the release:

```swift
// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "MyBrowser",
    platforms: [.macOS(.v13)],
    dependencies: [.package(path: ".wren/runtime/wren-swift")],
    targets: [
        .executableTarget(
            name: "MyBrowser",
            dependencies: [.product(name: "Wren", package: "wren-swift")])
    ],
    swiftLanguageVersions: [.v5]
)
```

## 3. Write the program

Put this in `Sources/MyBrowser/main.swift`:

```swift
import AppKit
import Wren

var profile: Profile?
var browser: Browser?
var window: NSWindow?

// Closing the window asks the Browser to close; the app quits once it has.
final class WindowDelegate: NSObject, NSWindowDelegate {
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        browser?.close { _ in }
        return false
    }
}
let windowDelegate = WindowDelegate()

// Puts the active tab's view in the window, in place of the one before.
func showActiveTab() {
    guard let content = window?.contentView, let view = browser?.activeTab?.view else { return }
    if view.superview === content { return }
    content.subviews.forEach { $0.removeFromSuperview() }
    content.addSubview(view)
    view.frame = content.bounds
    view.autoresizingMask = [.width, .height]
    try? browser?.activeTab?.focus()
}

func show(_ created: Browser) {
    let made = NSWindow(
        contentRect: NSRect(x: 0, y: 0, width: 1100, height: 760),
        styleMask: [.titled, .closable, .resizable, .miniaturizable],
        backing: .buffered, defer: false)
    made.isReleasedWhenClosed = false
    made.title = "My Browser"
    made.delegate = windowDelegate
    made.center()
    browser = created
    window = made
    // Chromium's dialogs and prompts appear over this view.
    created.attach(to: made.contentView)
    showActiveTab()
    made.makeKeyAndOrderFront(nil)
    NSApp.activate(ignoringOtherApps: true)
}

// Required: Chromium opened a Browser of its own (a popup, DevTools). This
// app shows one window, so it closes them, and undocked DevTools with them.
var delegate = AppDelegate(browserOpened: { opened, _, _ in opened.close { _ in } })
delegate.ready = {
    WrenApp.openProfile { result in
        guard case .success(let opened) = result else { return WrenApp.quit() }
        profile = opened
        opened.createBrowser(url: "https://example.com") { result in
            guard case .success(let created) = result else { return WrenApp.quit() }
            show(created)
        }
    }
}
delegate.changed = { changes in
    for change in changes where change.browser == browser {
        switch change.kind {
        case .tabs: showActiveTab()
        case .tab where change.tabFields.contains(.title):
            if change.tab == browser?.activeTab { window?.title = change.tab?.title ?? "" }
        default: break
        }
    }
}
delegate.browserClosed = { closed in
    if closed == browser { WrenApp.quit() }
}

let data = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
    .appendingPathComponent("MyBrowser").path
let status = WrenApp.run(Startup(userDataDirectory: data, keychain: .mock), delegate)
browser = nil
profile = nil
exit(status == .normal ? 0 : Int32(status.rawValue))
```

Here's what happens when it runs.

`WrenApp.run` starts Chromium inside your app's process, and runs the event loop until the app quits. It takes the place of `NSApplicationMain`. `Startup` tells Chromium where to keep its data. With `.mock`, the key that encrypts cookies and saved passwords stays in memory, which is what you want while you develop. [Startup](startup.md) covers what a shipped app sets instead.

Once Chromium is running, it calls `ready`. The app opens a Profile there, which holds one person's history, cookies and extensions, and creates a Browser in it. A Browser is a set of tabs, which Chrome would show as one window.

Wren Runtime doesn't give the Browser a window, so the program makes one. `attach` tells Chromium which view holds the Browser's tabs, so its dialogs and prompts appear over that view. Each tab has an ordinary `NSView`, and its page shows while the view is in a window.

When something changes, Chromium calls `changed`. This app listens for two kinds of change: a new active tab, such as when a link opens in a new tab, and a new title.

Chromium can also open Browsers of its own, for a popup or for DevTools in a window of its own. Every app handles `browserOpened`, and this one closes them, because it only shows one window. [Windows and tabs](windows-and-tabs.md#windows-chromium-opens) shows how to give them windows instead.

## 4. Build and run

```sh
swift build -c release
wren bundle --incremental --executable .build/release/MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
open MyBrowser.app
```

`swift build` makes an executable, but that executable can't start on its own. It loads Chromium from the app bundle around it, and `wren bundle` makes that bundle. The bundle holds Chromium, its helper apps under your app's name, and an ad hoc signature that this Mac accepts.

Run `wren bundle` after every build. With `--incremental`, it updates only the executable and the signature when nothing else changed, which takes under half a second. [While you develop](wren-cli.md#while-you-develop) has more.

A window opens on example.com. ⌘Q quits, after Chrome's "Hold ⌘Q to Quit", because Wren Runtime gives your app a standard main menu until you set your own.

## The same app in Rust

The Rust SDK is the crate `wren`, in the release's `wren-rust` folder. Its names are the Swift SDK's in Rust's case, such as `active_tab` for `activeTab`.

Use the same `wren-app.json`, and this `Cargo.toml`:

```toml
[package]
name = "my-browser"
version = "0.1.0"
edition = "2024"

[dependencies]
wren = { path = ".wren/runtime/wren-rust" }
objc2 = "0.6"
objc2-app-kit = "0.3"
objc2-foundation = "0.3"
```

Put this in `src/main.rs`:

```rust
use std::cell::RefCell;
use std::rc::Rc;

use objc2::rc::Retained;
use objc2::{MainThreadMarker, MainThreadOnly};
use objc2_app_kit::{
    NSApplication, NSAutoresizingMaskOptions, NSBackingStoreType, NSView, NSWindow,
    NSWindowStyleMask,
};
use objc2_foundation::{NSPoint, NSRect, NSSize, NSString};
use wren::{
    App, Browser, BrowserType, Change, ChangeKind, ExitStatus, Keychain, MainThread, NativeView,
    Startup, TabFields,
};

// What the app holds, shared with the closures it hands Wren Runtime.
#[derive(Default)]
struct State {
    browser: RefCell<Option<Browser>>,
    window: RefCell<Option<Retained<NSWindow>>>,
}

// Puts the active tab's view in the window, in place of the one before.
fn show_active_tab(window: &NSWindow, browser: &Browser) {
    let (Some(content), Some(tab)) = (window.contentView(), browser.active_tab()) else {
        return;
    };
    let Some(view) = tab.view() else { return };
    let view: Retained<NSView> = unsafe { Retained::retain(view.as_ptr().cast()) }.unwrap();
    if unsafe { view.superview() }.as_deref() == Some(&*content) {
        return;
    }
    for old in content.subviews() {
        old.removeFromSuperview();
    }
    view.setFrame(content.bounds());
    view.setAutoresizingMask(
        NSAutoresizingMaskOptions::ViewWidthSizable | NSAutoresizingMaskOptions::ViewHeightSizable,
    );
    content.addSubview(&view);
    let _ = tab.focus();
}

impl State {
    fn show(&self, browser: Browser) {
        let mtm = MainThreadMarker::new().unwrap();
        let window = unsafe {
            NSWindow::initWithContentRect_styleMask_backing_defer(
                NSWindow::alloc(mtm),
                NSRect::new(NSPoint::new(0.0, 0.0), NSSize::new(1100.0, 760.0)),
                NSWindowStyleMask::Titled
                    | NSWindowStyleMask::Closable
                    | NSWindowStyleMask::Resizable
                    | NSWindowStyleMask::Miniaturizable,
                NSBackingStoreType::Buffered,
                false,
            )
        };
        unsafe { window.setReleasedWhenClosed(false) };
        window.setTitle(&NSString::from_str("My Browser"));
        window.center();
        // Chromium's dialogs and prompts appear over this view.
        let content = window.contentView().unwrap();
        browser.attach_to_view(unsafe {
            NativeView::from_raw(Retained::as_ptr(&content).cast_mut().cast())
        });
        show_active_tab(&window, &browser);
        window.makeKeyAndOrderFront(None);
        NSApplication::sharedApplication(mtm).activate();
        *self.browser.borrow_mut() = Some(browser);
        *self.window.borrow_mut() = Some(window);
    }
}

struct MyBrowser(Rc<State>);

impl App for MyBrowser {
    fn ready(&self, mt: MainThread) {
        let state = self.0.clone();
        mt.open_profile(None, move |result| {
            let Ok(profile) = result else { return mt.quit() };
            profile.create_browser("https://example.com", move |result| match result {
                Ok(browser) => state.show(browser),
                Err(_) => mt.quit(),
            });
        });
    }

    // Required: Chromium opened a Browser of its own (a popup, DevTools).
    // This app shows one window, so it closes them, and undocked DevTools
    // with them.
    fn browser_opened(&self, _: MainThread, opened: Browser, _: BrowserType, _: Option<Browser>) {
        opened.close(|_| {});
    }

    fn changed(&self, _: MainThread, changes: &[Change]) {
        let browser = self.0.browser.borrow().clone();
        let window = self.0.window.borrow().clone();
        let (Some(browser), Some(window)) = (browser, window) else {
            return;
        };
        for change in changes.iter().filter(|change| change.browser.as_ref() == Some(&browser)) {
            match (change.kind, change.tab) {
                (ChangeKind::Tabs, _) => show_active_tab(&window, &browser),
                (ChangeKind::Tab, Some(tab))
                    if change.tab_fields.contains(TabFields::TITLE)
                        && Some(tab) == browser.active_tab() =>
                {
                    window.setTitle(&NSString::from_str(&tab.title()));
                }
                _ => {}
            }
        }
    }

    fn browser_closed(&self, mt: MainThread, closed: Browser) {
        if self.0.browser.borrow().as_ref() == Some(&closed) {
            mt.quit();
        }
    }
}

fn main() {
    let data = format!(
        "{}/Library/Application Support/MyBrowser",
        std::env::var("HOME").unwrap()
    );
    let state = Rc::new(State::default());
    let status = wren::run(Startup::new(data, Keychain::Mock), MyBrowser(state.clone()));
    // Drops the Browser and the window before the process ends.
    drop(state);
    std::process::exit(if status == ExitStatus::Normal { 0 } else { status as i32 });
}
```

The program follows the Swift one step by step. Where it differs, it's because Rust does.

- The callbacks are the methods of a type that implements `App`, which `wren::run` takes. They get `&self`, because one callback can run while another is still running. So the state lives in `RefCell`s, and each callback borrows it only briefly.
- Each callback gets a `MainThread`, which the calls that don't belong to a Profile or a Browser need, such as `open_profile` and `quit`. It also keeps Wren Runtime's values on the main thread, where they belong.
- A tab's view comes as a `NativeView`, which objc2 turns into an `NSView`.
- Closing this window only hides it, and ⌘Q quits. To make closing the window close the Browser, as the Swift program does, give the window a delegate with objc2's `define_class!`.

Build it with Cargo, and bundle it the same way:

```sh
wren install
cargo build --release
wren bundle --executable target/release/my-browser \
  --manifest wren-app.json --output MyBrowser.app
open MyBrowser.app
```

The crate finds Chromium through `.wren/runtime`. To build against another release, set `WREN_RELEASE_DIR` to its folder.

## If it doesn't start

- **The app quits at once.** Run its executable from a terminal to see why, as `MyBrowser.app/Contents/MacOS/MyBrowser`. If a `Startup` value is wrong, Wren Runtime prints which one.
- **`WrenApp.run` returns `.apiMismatch`.** The app was built against one release and bundled with another. Build and bundle with the same one.
- **It returns `.alreadyRunning`.** Another copy of the app is using the same data folder. Only one process at a time can use it.
- **The window is empty where the page should be.** The tab's view isn't in the window, it or a view around it is hidden, or it has no size.
- **DevTools or a popup flashes and disappears.** Each one is a Browser of its own, and the `browserOpened` above closes them all. [Windows and tabs](windows-and-tabs.md#windows-chromium-opens) shows how to give them windows.

## Tell your coding agent

Each release comes with an `AGENTS.md` for coding agents. It says what Wren Runtime is, where to look things up, and what agents most often get wrong. An agent won't find it by itself, because git ignores `.wren`, and so do search tools that follow `.gitignore`. Add this to your project's own `AGENTS.md` or `CLAUDE.md`:

```markdown
## Wren Runtime

This app is built with Wren Runtime, linked at `.wren/runtime`. Before working with Wren Runtime, read `.wren/runtime/AGENTS.md`. The headers in `.wren/runtime/include` are the reference. Search `.wren/runtime` by its path, because git ignores it.
```

## Fetching a release by hand

If you'd rather not use `wren install`, download a release, check it, unpack it wherever you keep releases, and link it into the project yourself:

```sh
curl -fLO https://github.com/MultiboxLabs/wren-runtime/releases/download/v0.5.0/wren-runtime-0.5.0-mac-arm64.zip
curl -fLO https://github.com/MultiboxLabs/wren-runtime/releases/download/v0.5.0/SHA256SUMS
shasum -a 256 --ignore-missing -c SHA256SUMS
ditto -x -k wren-runtime-0.5.0-mac-arm64.zip ~/wren-runtimes
# In the project's folder:
mkdir -p .wren && ln -s ~/wren-runtimes/wren-runtime-0.5.0-mac-arm64 .wren/runtime
echo '*' > .wren/.gitignore
```

Download it with `curl` or `gh`, not a web browser. macOS quarantines what a web browser downloads, and then won't run the release's `bin/wren`.

## Before you ship

This first app still shows Chromium's name and logo in Chromium's own pages, and keeps its key in memory. [Startup](startup.md) describes each of these settings.

- **Your name.** Set `productName`, so toasts, Settings, dialogs and prompts say your app's name instead of "Chromium".
- **Your logo.** Set `productIconPNG`, and the dark and SVG versions if you have them, so Settings and dialogs show your logo instead of Chromium's.
- **The keychain.** Use `.system` with a `keychainName` that never changes. Data written with `.mock` can't be read with the system key later.
- **About.** Set `copyright`, `companyName`, `helpURL` and `appVersion`, which Settings › About and `chrome://version` show.
- **The bundle.** Give `wren-app.json` an `icon`, a `version` and a `copyright`, then sign with `wren bundle` and notarise with `wren notarize`. [The `wren` command](wren-cli.md) shows how.

## Next

- [How a Wren Runtime app works](app-model.md) explains Profiles, Browsers, tabs and changes in full.
- [Windows and tabs](windows-and-tabs.md) turns this app into a browser, with a tab strip, popups in their own windows, and DevTools.
- [Web pages in an existing app](embedding.md) is for an app that isn't a browser.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `WrenApp.run` | `wren::run` | `Wren_Main` |
| `Startup` | `Startup::new` | `WrenStartup` |
| `AppDelegate.ready` | `App::ready` | `on_ready` |
| `WrenApp.openProfile` | `MainThread::open_profile` | `Wren_OpenProfile` |
| `profile.createBrowser` | `Profile::create_browser` | `Wren_CreateBrowser` |
| `browser.attach(to:)` | `Browser::attach_to_view` | `Wren_AttachBrowserToView` |
| `browser.activeTab` | `Browser::active_tab` | `Wren_GetActiveTab` |
| `tab.view` | `BrowserTab::view` | `Wren_GetTabView` |
| `tab.focus()` | `BrowserTab::focus` | `Wren_FocusTab` |
| `AppDelegate.changed` | `App::changed` | `on_changed` |
| `AppDelegate.browserOpened` | `App::browser_opened` | `on_browser_opened` |
| `AppDelegate.browserClosed` | `App::browser_closed` | `on_browser_closed` |
| `browser.close` | `Browser::close` | `Wren_CloseBrowsers` |
| `WrenApp.quit` | `MainThread::quit` | `Wren_Quit` |
| `.apiMismatch`, `.alreadyRunning` | `ExitStatus::ApiMismatch`, `ExitStatus::AlreadyRunning` | `WREN_EXIT_API_MISMATCH`, `WREN_EXIT_ALREADY_RUNNING` |

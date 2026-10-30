# Getting started

This page makes a first Wren app. It is one window that shows a web page in a real Chromium tab. It takes three files and four commands in Swift. The same app in Rust is [at the end](#the-same-app-in-rust).

## What you need

- A Mac with Apple silicon.
- Xcode or the Command Line Tools, for `swift build` and `codesign`.
- For Rust, also Rust 1.85 or later, from rustup or Homebrew.
- The `wren` command, installed once. Releases are published in the private repository `MultiboxLabs/wren-runtime`. So you need access to it, and the GitHub CLI `gh`, signed in. Put `wren` in a folder on your `PATH`, such as `~/.local/bin`:

  ```sh
  gh release download --repo MultiboxLabs/wren-runtime --pattern wren \
    --output ~/.local/bin/wren
  chmod +x ~/.local/bin/wren
  ```

A project names the Wren release it is built with. `wren install` fetches that release, once per Mac, and links it into the project as `.wren/runtime`. The project's build files point there. So they are the same on every Mac, and the app is built and bundled with the same release.

The release holds everything an app needs. You never build Chromium to write an app. The app runs on macOS 13 or later, on Apple silicon.

| In a release (`.wren/runtime`) | What it is |
| --- | --- |
| `Wren Framework.framework` | Chromium with Wren in it. `wren bundle` copies it into your app. |
| `wren-swift` | The Swift package: `Wren`, the Swift API, and `CWren`, the C API under it. |
| `wren-rust` | The Rust crate `wren`: the API for Rust, and `wren::sys`, the C API under it. |
| `include` | The C headers, with a module map. |
| `bin/wren` | The command that turns an executable into an app. |
| `docs` | These guides. |
| `app-template`, `LICENSES`, `release.json` | What `wren` starts each app from, the licences to ship, and which build this is. |

## 1. The package

`Package.swift`:

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

## 2. The program

`Sources/MyBrowser/main.swift`:

```swift
import AppKit
import Wren

var profile: Profile?
var browser: Browser?
var window: NSWindow?

// Closing the window asks the Browser to close; the app quits once it has.
final class WindowDelegate: NSObject, NSWindowDelegate {
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        try? browser?.close()
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
var delegate = AppDelegate(browserOpened: { opened, _, _ in try? opened.close() })
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

Here is what it does, in the order it runs.

1. `WrenApp.run` starts Chromium in this process and runs the app's event loop. It takes the place of `NSApplicationMain`, and returns only when the app quits. `Startup` says where Chromium keeps its data. `.mock` keeps the key that encrypts cookies and passwords in memory, which suits development. A shipped app uses the Keychain instead. See [Startup](startup.md).
2. `ready` runs once Chromium is up. Nearly every call must wait for it.
3. A Profile is one user's data, such as history, cookies and extensions. `openProfile` with no ID opens the Profile Chromium starts with.
4. A Browser is a set of tabs, which Chrome shows as one window. Wren gives it no window. You make one, and `attach` tells Chromium which of its views holds the tabs.
5. Each tab has a view, an ordinary `NSView`. A page shows while its view is in a window, at the view's size.
6. `changed` reports what Chromium did. Here that is two things. The tabs or the active tab changed, for example because a link opened a new tab. Or a tab has a new title.

The names this program uses, in Swift, in Rust and in C:

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
| `AppDelegate.browserClosed` | `App::browser_closed` | `on_browser_closed` |
| `browser.close()` | `Browser::close` | `Wren_CloseBrowser` |
| `WrenApp.quit` | `MainThread::quit` | `Wren_Quit` |

The C headers in `include` describe each one fully. Each Swift and Rust item's comment names its C side.

## 3. The manifest

`wren-app.json` says what the app is called, and which Wren release it is built with:

```json
{
  "name": "MyBrowser",
  "displayName": "My Browser",
  "bundleId": "com.example.mybrowser",
  "wren": "0.3.0"
}
```

## Build, bundle, run

```sh
wren install
swift build -c release
wren bundle --incremental --executable .build/release/MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
open MyBrowser.app
```

`wren install` puts release 0.3.0 on this Mac, fetching it only the first time, and links `.wren/runtime` to it. The `.wren` folder keeps itself out of git. Run `wren install` again after changing `"wren"`.

`swift build` makes only an executable, and that executable does not start on its own. It loads the framework from the app bundle it is in. `wren bundle` makes the bundle. The bundle has the framework, Chromium's helper apps under your app's name, and an ad hoc signature that this Mac accepts. Run `wren bundle` again after every build. With `--incremental`, it keeps the bundle when only the executable changed and signs only the app, in under half a second. The app also starts sooner, because macOS scans a bundle only the first time it starts. See [While you develop](wren-cli.md#while-you-develop).

A window opens on example.com. Links, typing, context menus, text selection and permission prompts all work, because the view is Chromium's own. ⌘Q quits, after Chrome's "Hold ⌘Q to Quit", because Wren installs a standard main menu unless you ask for another. See [Startup](startup.md).

## The same app in Rust

The `wren` crate in the release's `wren-rust` folder is the same API for Rust. Its names are the Swift names in Rust's case: `active_tab` for `activeTab`. Each item's comment in the crate names the C function it calls, so a search for a Rust name leads to the header.

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

`src/main.rs`:

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

// What the app holds, shared with the closures it hands Wren.
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
        let _ = opened.close();
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

It differs from the Swift program where Rust differs.

- The callbacks are the methods of an `App`, which `wren::run` takes. They get `&self`, because a callback can run inside another. Keep state in `RefCell`s and borrow it briefly.
- Each callback gets a `MainThread`. The calls that take no handle need it, such as `open_profile` and `quit`. Handles and IDs stay on the main thread.
- A tab's view is a `NativeView`, a pointer to an `NSView`. objc2 turns it into an `NSView`.
- Closing this window only hides it, and ⌘Q quits. To make closing ask the Browser to close, as the Swift program does, give the window a delegate with objc2's `define_class!`.

Build and bundle as before, with Cargo:

```sh
wren install
cargo build --release
wren bundle --executable target/release/my-browser \
  --manifest wren-app.json --output MyBrowser.app
open MyBrowser.app
```

The crate links the framework beside it, through `.wren/runtime`. To use another release, set `WREN_RELEASE_DIR` to its folder.

## A release downloaded by hand

You can also fetch a release with `gh`, check it, unpack it where you keep it, and link it into the project as `wren install` would:

```sh
gh release download v0.3.0 --repo MultiboxLabs/wren-runtime \
  --pattern wren-runtime-0.3.0-mac-arm64.zip --pattern SHA256SUMS
shasum -a 256 --ignore-missing -c SHA256SUMS
ditto -x -k wren-runtime-0.3.0-mac-arm64.zip ~/wren-runtimes
# In the project's folder:
mkdir -p .wren && ln -s ~/wren-runtimes/wren-runtime-0.3.0-mac-arm64 .wren/runtime
echo '*' > .wren/.gitignore
```

Use `gh` rather than a web browser. macOS marks what a web browser downloads as quarantined, and then refuses to run `bin/wren` from it.

## Tell your coding agent

The release has an `AGENTS.md` for coding agents. It says what Wren is, where to look things up, and the facts agents most often get wrong. An agent finds it only if the project points there. `.wren` is ignored by git, so search tools that respect `.gitignore` skip it.

Add this to the project's own `AGENTS.md`, or `CLAUDE.md`:

```markdown
## Wren

This app is built with Wren, linked at `.wren/runtime`. Before working with Wren, read `.wren/runtime/AGENTS.md`. The headers in `.wren/runtime/include` are the reference. Search `.wren/runtime` by its path, because git ignores it.
```

## When it does not start

- **The app exits at once.** Run its executable from a terminal to see why, as `MyBrowser.app/Contents/MacOS/MyBrowser`. An invalid `Startup` prints its reason.
- **`WrenApp.run` returns `.apiMismatch`.** In Rust it is `ExitStatus::ApiMismatch`. The app was compiled against one release and bundled with another. Build and bundle with the same one.
- **It returns `.alreadyRunning`.** Another copy of the app is using the same data directory. Only one process at a time can use a directory.
- **Nothing shows where the page should be.** The tab's view is not in a window, or it or an ancestor is hidden, or it has no size.
- **DevTools or a popup opens and is gone at once.** Each is a Browser of its own, and `browserOpened` above closes every one. Show it in a window instead. See [Windows and tabs](windows-and-tabs.md#windows-chromium-opens).

## Before you ship

The first app leaves Chromium's own name and settings in place. A shipped app sets these, all described in [Startup](startup.md).

- **The name.** Set `productName`, or Chromium's own UI says "Chromium" in its toasts, Settings, dialogs and prompts.
- **The logo.** Set `productIconPNG`, and the dark and SVG variants if you have them, or Chromium's logo shows in Settings and dialogs.
- **The keychain.** Use `.system` with a `keychainName` that never changes. `.mock` is for development only, and data written with it cannot be read with the system key later.
- **About.** Set `copyright`, `companyName`, `helpURL` and `appVersion` for Settings > About and chrome://version.
- **The bundle.** Give `wren-app.json` an `icon`, a `version` and a `copyright`, and sign and notarise with `wren bundle` and `wren notarize`. See [The `wren` command](wren-cli.md).

## Next

- [How a Wren app works](app-model.md) explains Profiles, Browsers, tabs and changes properly.
- [Windows and tabs](windows-and-tabs.md) turns this app into a browser, with a tab strip, popups in windows of their own, and DevTools.
- [Adding web pages to an existing app](embedding.md) is for an app that is not a browser.

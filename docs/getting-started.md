# Getting started

This makes a first Wren app: one window that shows a web page in a real Chromium
tab. It takes three files and four commands, in Swift; the same app in Rust is
[at the end](#the-same-app-in-rust).

## What you need

- A Mac with Apple silicon.
- Xcode or the Command Line Tools, for `swift build` and `codesign`.
- For Rust, Rust 1.85 or later (rustup or Homebrew) as well.
- The `wren` command, installed once. Releases are published in the private
  repository `MultiboxLabs/wren-runtime`, so you need access to it and the
  GitHub CLI (`gh`), signed in. Put `wren` in a folder on your `PATH`, such as
  `~/.local/bin`:

  ```sh
  gh release download --repo MultiboxLabs/wren-runtime --pattern wren \
    --output ~/.local/bin/wren
  chmod +x ~/.local/bin/wren
  ```

A project names the Wren release it is built with, and `wren install` fetches
that release (once per Mac) and links it into the project as `.wren/runtime`.
The project's build files point there, so they are the same on every Mac, and
the app is built and bundled with the same release. The release holds everything
an app needs: no Chromium build is needed to write an app. The app runs on macOS
13 or later on Apple silicon.

| In a release (`.wren/runtime`)             | What it is                                                                       |
| ------------------------------------------ | -------------------------------------------------------------------------------- |
| `Wren Framework.framework`                 | Chromium with Wren in it. It is copied into your app.                            |
| `wren-swift`                               | The Swift package: `Wren`, the Swift API, and `CWren`, the C API under it.       |
| `wren-rust`                                | The Rust crate `wren`: the API for Rust, and `wren::sys`, the C API under it.    |
| `include`                                  | The C headers, with a module map.                                                |
| `bin/wren`                                 | The command that turns an executable into an app.                                |
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
// app shows one window, so it closes them.
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

What it does, in the order it runs:

1. `WrenApp.run` starts Chromium in this process and runs the app's event loop.
   It takes the place of `NSApplicationMain` and returns only when the app
   quits. `Startup` says where Chromium keeps its data; `.mock` keeps the key
   that encrypts cookies and passwords in memory, which suits development
   ([Startup](startup.md)).
2. `ready` runs once Chromium is up. Nearly every call must wait for it.
3. A **Profile** is a user's data: history, cookies, extensions. `openProfile`
   with no ID opens the one Chromium starts with.
4. A **Browser** is a set of tabs, what Chrome shows as one window. Wren gives
   it no window: you make one and say which of its views shows the tabs
   (`attach`).
5. Each **tab** has a view, an ordinary `NSView`. A page is visible while its
   view is in a window, and is the view's size.
6. `changed` reports what Chromium did: here, that the tabs or the active tab
   changed (a link opened a new tab), and that a tab has a new title.

## 3. The manifest

`wren-app.json` says what the app is called, and which Wren release it is built
with:

```json
{
  "name": "MyBrowser",
  "displayName": "My Browser",
  "bundleId": "com.example.mybrowser",
  "wren": "0.1.0"
}
```

## Build, bundle, run

```sh
wren install
swift build -c release
wren bundle --executable .build/release/MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
open MyBrowser.app
```

`wren install` puts release 0.1.0 on this Mac, fetching it only the first time,
and links `.wren/runtime` to it; `.wren` keeps itself out of git. Run it again
after changing `"wren"`. `swift build` makes only an executable, and that
executable does not start on its own: it loads the framework from the bundle it
is in. `wren bundle` makes the bundle, with the framework, Chromium's helper
apps under your app's name, and an ad hoc signature that this Mac accepts. Run
it again after every build.

A window opens on example.com. Links, typing, context menus, text selection and
permission prompts all work, because the view is Chromium's own. ⌘Q quits: Wren
installs a standard main menu unless you ask for another
([Startup](startup.md)).

## The same app in Rust

The `wren` crate in the release's `wren-rust` folder is the same API for Rust,
with the same names. With the same `wren-app.json`, `Cargo.toml`:

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
    // This app shows one window, so it closes them.
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

The differences from Swift are Rust's:

- The callbacks are the methods of an `App`, which `wren::run` takes. They get
  `&self`, as a callback can run inside another: keep state in `RefCell`s and
  borrow it briefly.
- Each callback is given a `MainThread`, which the calls that take no handle
  (`open_profile`, `quit`) need. Handles and IDs stay on the main thread.
- A tab's view is a `NativeView`, a pointer to an `NSView`; objc2 makes it one.
- Closing this window only hides it; ⌘Q quits. A window delegate (objc2's
  `define_class!`) would ask the Browser to close, as the Swift program does.

Build and bundle as before, with Cargo:

```sh
wren install
cargo build --release
wren bundle --executable target/release/my-browser \
  --manifest wren-app.json --output MyBrowser.app
open MyBrowser.app
```

The crate links the framework beside it, through `.wren/runtime`; to use
another, set `WREN_RELEASE_DIR` to its folder.

## A release downloaded by hand

A release can also be downloaded from its page,
`wren-runtime-<version>-mac-arm64.zip`. A browser marks what it downloads as
quarantined, and macOS then refuses to run `bin/wren` from it; clear the mark
once it is unzipped, and link it into the project as `wren install` would:

```sh
xattr -dr com.apple.quarantine wren-runtime-0.1.0-mac-arm64
mkdir -p .wren && ln -s "$PWD/wren-runtime-0.1.0-mac-arm64" .wren/runtime
```

What `wren install` fetches is never quarantined.

## When it does not start

- **The app exits at once.** Run its executable from a terminal to see why:
  `MyBrowser.app/Contents/MacOS/MyBrowser`. An invalid `Startup` prints its
  reason.
- **`WrenApp.run` returns `.apiMismatch`** (`ExitStatus::ApiMismatch` in Rust).
  The app was compiled against one release and bundled with another. Build and
  bundle with the same one.
- **`.alreadyRunning`.** Another copy of the app is using the same data
  directory. One process per directory.
- **Nothing shows where the page should be.** The tab's view is not in a window,
  or it or an ancestor is hidden, or it has no size.

## Next

- [How a Wren app works](app-model.md) explains Profiles, Browsers, tabs and
  changes properly.
- [Windows and tabs](windows-and-tabs.md) turns this into a browser: a tab
  strip, popups in windows of their own, DevTools.
- [Adding web pages to an existing app](embedding.md), if a browser is not what
  you are making.

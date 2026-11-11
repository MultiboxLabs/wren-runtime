# Wren Runtime

Wren Runtime is a library for building browsers on top of Chromium. It lets you customise Chromium extensively without ever compiling it.

Building a browser on Chromium usually means forking it. That means keeping a Chromium checkout, waiting hours for each build, and moving your patches onto every new Chromium release. Wren Runtime does that work for you. Each release comes with Chromium already built, and your app links against it like any other library.

You get full control over the browser's UI, including its windows, tab strip, toolbar and address field. Chromium still handles everything behind them, so tabs, Profiles, history, restored sessions, extensions from the Chrome Web Store, saved passwords, downloads, permission prompts and DevTools all work as they do in Chrome, without you building any of it.

## SDKs

Wren Runtime comes with a Swift SDK and a Rust SDK, and both cover the whole API. Underneath them is a C API, which you can call from C or Objective-C, or wrap for another language.

| In a release | What it is |
| --- | --- |
| The Swift package `Wren` | The Swift SDK |
| The Rust crate `wren` | The Rust SDK |
| The C headers | The C API that both SDKs are built on |
| `wren.d.ts` | TypeScript types for a browser UI written in HTML |
| The `wren` command | Fetches releases, and bundles, signs and notarises your app |
| Two sample browsers | A native one in Swift, and one with an HTML UI over a Rust host |

## What you can build

- **A browser with a native UI.** Your tab strip and toolbar are AppKit or SwiftUI views, around Chromium's tabs.
- **A browser whose UI is a web page.** The whole window is one HTML page, built with React or anything else, over a small Rust or Swift host. Tabs go into the page's layout with a `<wren-view>` element.
- **Workspaces in one window.** One window can hold several complete browsers, each with its own tabs, cookies and extensions, and switch between them instantly. Chrome ties each window to one browser, and Wren Runtime doesn't.
- **Web pages inside an app that isn't a browser.** Add a page to an app you already have, or a full browser tab with its extensions and saved passwords.

## How it compares

Electron, CEF and Wren Runtime all put Chromium in your app. They differ in how much of Chrome you get, and in who draws the UI.

|  | Electron | CEF | Wren Runtime |
| --- | --- | --- | --- |
| What you get from Chromium | Its page engine, with Node.js beside it | Its page engine. Its Chrome style adds Chrome's browser features. | Chrome's browser, without Chrome's windows |
| Your UI | HTML | Native, in C or C++, or through a binding | Native, in AppKit or SwiftUI, or HTML |
| Your code | JavaScript on Node.js, in Chromium's main process | C or C++, in Chromium's main process | Swift, Rust or C, in Chromium's main process |
| Chrome extensions | Some of Chrome's extension APIs | In Chrome style, through Chrome's own UI | As in Chrome, with their buttons and popups in your UI |
| Profiles, saved passwords, history, sessions | You build them | In Chrome style, through Chrome's own UI | Built in, with their state and actions in the API |
| Tabs, with groups, splits, pinning and moving between windows | You build them | You build them | Chromium's tab model and its rules, under a tab strip of your own design |
| Your app's name and icon in Chromium's pages, menus and prompts | Electron has almost none of them | Not offered | Built in |
| Platforms | Mac, Windows, Linux | Mac, Windows, Linux | Mac on Apple silicon |

**Coming from Electron.** In Electron, your app's code is JavaScript on Node.js in Chromium's main process, and its UI is web pages in renderer processes that reach that code over IPC. With Wren Runtime, your code runs in the same main process, but it's native code that calls into Chromium directly. Your UI can be native views, or still be HTML. Pages and your code message each other much like `ipcRenderer` and `ipcMain`, and you can serve your own URL schemes as with Electron's `protocol`.

**Coming from CEF.** CEF gives you Chromium's page engine on many platforms, and you build the browser around it. Its Chrome style adds Chrome's features, but mostly with Chrome's own toolbar and dialogs. There's no tab model to reuse, and Chromium's pages and prompts keep Chromium's name. Wren Runtime gives you Chrome's features as an API, tab model included, under your app's name and icon. Your app gets the tabs, the address bar's suggestions, the extensions' buttons and the downloads as state, so your UI can show them any way you like.

**Coming from `WKWebView`.** A web view gives you a page. Wren Runtime gives you the browser around it, so every tab comes with Chrome's extensions, passwords, history and DevTools.

## A first look

Here's the start of a browser in Swift. It opens the user's Profile and creates a Browser with one tab:

```swift
import AppKit
import Wren

// Chromium opened a Browser of its own, such as a popup. This app closes it.
var delegate = AppDelegate(browserOpened: { opened, _, _ in opened.close { _ in } })
delegate.ready = {
    WrenApp.openProfile { result in
        guard case .success(let profile) = result else { return WrenApp.quit() }
        profile.createBrowser(url: "https://example.com") { result in
            guard case .success(let browser) = result else { return WrenApp.quit() }
            // Keep the Browser, and put browser.activeTab?.view in a window.
        }
    }
}
let data = NSHomeDirectory() + "/Library/Application Support/MyBrowser"
exit(WrenApp.run(Startup(userDataDirectory: data, keychain: .mock), delegate) == .normal ? 0 : 1)
```

The same in Rust:

```rust
use wren::{App, Browser, BrowserType, ExitStatus, Keychain, MainThread, Startup};

struct MyBrowser;

impl App for MyBrowser {
    fn ready(&self, mt: MainThread) {
        mt.open_profile(None, move |result| {
            let Ok(profile) = result else { return mt.quit() };
            profile.create_browser("https://example.com", move |result| {
                let Ok(browser) = result else { return mt.quit() };
                // Keep the Browser, and put browser.active_tab()'s view in a window.
            });
        });
    }

    // Chromium opened a Browser of its own, such as a popup. This app closes it.
    fn browser_opened(&self, _: MainThread, opened: Browser, _: BrowserType, _: Option<Browser>) {
        opened.close(|_| {});
    }
}

fn main() {
    let data = format!("{}/Library/Application Support/MyBrowser", std::env::var("HOME").unwrap());
    let status = wren::run(Startup::new(data, Keychain::Mock), MyBrowser);
    std::process::exit(if status == ExitStatus::Normal { 0 } else { 1 });
}
```

[Getting started](docs/getting-started.md) has the whole program in both languages.

## Getting started

Install the `wren` command:

```sh
curl -fsSL https://wrenbrowser.com/runtime/cli/install.sh | sh
```

Then follow [Getting started](docs/getting-started.md). It takes you from an empty folder to a window showing a page, in three files and four commands.

## Requirements

- A Mac with Apple silicon. Other platforms aren't supported yet.
- Xcode or the Command Line Tools, and Rust 1.85 or later for Rust apps.
- Your app runs on macOS 13 or later.

Wren Runtime isn't at 1.0 yet. Each app is built against one release, and moving to the next means building again, sometimes with code changes. [This release](docs/this-release.md) has the release notes, with its Chromium version and known issues, and [What Wren Runtime can't do yet](docs/limitations.md) lists what isn't available.

## Documentation

- [Getting started](docs/getting-started.md) walks you through a first app.
- [How a Wren Runtime app works](docs/app-model.md) explains the ideas the rest of the API is built on.
- [The guides](docs/README.md) are sorted by what you're building.
- [Compatibility](docs/compatibility.md) lists what changed between releases, and how to update your app.
- [`Wren.h`](include/Wren.h) and the headers it includes document every function.

## Licence

Wren Runtime is proprietary software of Avocado and Bear Limited, which trades as Multibox Labs. `LICENSE` has the full terms. In short:

- You can use Wren Runtime to develop and test your own apps inside your organisation.
- You can use it to build, run, change and contribute to [Wren Browser](https://github.com/MultiboxLabs/wren-browser), but not to distribute builds of it.
- To ship an app with Wren Runtime to anyone outside your organisation, you need a written licence from Multibox Labs for that app. Ask at licensing@wrenbrowser.com.

An app that ships Wren Runtime also ships Chromium's notices, which are in the release's `LICENSES` folder. The framework decodes H.264, AAC and HEVC, so if your app ships them, getting any patent licence it needs is up to you.

## Help

Report bugs and request features in this repository's issues. Report security vulnerabilities privately, as `SECURITY.md` describes.

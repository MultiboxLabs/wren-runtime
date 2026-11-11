# Startup

A few things have to be settled before Chromium starts: where your app keeps its data, how cookies and passwords are encrypted, which main menu it gets, and what name and logo Chromium's own pages show. They all go in one `Startup` value, which `WrenApp.run` reads once at launch.

The code here is in Swift. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## A shipped app's startup

```swift
let support = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
var startup = Startup(
    userDataDirectory: support.appendingPathComponent("My Browser").path,
    keychain: .system)
startup.keychainName = "My Browser"
startup.productName = "My Browser"
startup.productIconPNG = try Data(contentsOf: iconURL)
startup.copyright = "Copyright 2026 Example Ltd."
startup.companyName = "Example Ltd."
startup.appVersion = "1.2.0"

let status = WrenApp.run(startup, delegate)
```

Only the data folder and the keychain are required, and the rest have sensible defaults. If a value isn't valid, the app doesn't start. `run` returns `.invalidStartup`, and Wren Runtime prints which value was wrong to stderr.

## Where your data lives

`userDataDirectory` is the folder where Chromium keeps every Profile, with its history, cookies, extensions and caches. Give it an absolute path. On a Mac it belongs in `~/Library/Application Support`, in a folder named after your app.

Only one process can use the folder at a time. If someone launches your app again while it's running, the second launch tells the running app about it and returns `.alreadyRunning`. The running app opens nothing because of it.

At the first launch, Chromium creates a Profile in a folder called `Default`. `startupProfileName` names it, and without it Chromium picks a name. It never renames a Profile that already exists. Chromium also writes crash reports to `Crashpad`, inside the data folder.

## How cookies and passwords are encrypted

Chromium encrypts cookies and saved passwords with a key, and `keychain` says where the key is kept.

|  | `.system` | `.mock` |
| --- | --- | --- |
| The key | In the login Keychain, as "\<keychainName\> Safe Storage" | Fixed, in memory |
| Use it for | Apps signed with a Developer ID | Development and tests |

With `.system`, give a `keychainName`, and keep it for the life of your product. A new name means a new key, and Chromium can't read the cookies and passwords it saved with the old one.

Use `.mock` while you develop. An ad hoc signature changes with every build, so with `.system` macOS would ask for Keychain access after each one. Data saved with one kind of key can't be read with the other, so give development builds a data folder of their own. The native sample uses `WrenSample` for signed builds and `WrenSample Development` for the rest.

## The main menu

Your app has a working main menu from the first launch, with Quit, Copy, Paste and the rest. `menu` chooses which one.

- **`.default`** has App, File, Edit, View and Window menus, like Electron's default menu. The View menu's Reload, Developer Tools and zoom items act on the Browser in the key window.
- **`.chromium`** is Chrome's own main menu, without the lists Chrome fills in as it runs, such as bookmarks, history and Profiles. Its commands act on the Browser in the key window. Find opens Chromium's find bar, and Open File, Save Page and Print run Chromium's own commands. Commands that need UI Wren Runtime doesn't have, such as Name Window, are turned off.

In both, ⌘Q warns before quitting while Chrome's Warn Before Quitting setting is on, and pages and downloads in progress can stop the quit.

Most browsers replace the menu once they're running. From `ready` on, you can describe your own menu [as data](menus.md#the-main-menu) with `WrenApp.setMainMenu`, with Chrome's commands among your own items. You can also build an `NSMenu` yourself, and an item copied from Chrome's menu keeps working in it.

A Browser counts as the one in the key window when the view you attached it to is in that window. Keys a page doesn't use go on to the main menu, so your shortcuts work while a page has focus.

## Your name and logo in Chromium's pages

Settings, the New Tab page, `chrome://` pages, toasts, dialogs and prompts are still Chromium's. These fields put your product's name and logo there instead of Chromium's.

| Field | Where it shows |
| --- | --- |
| `productName` | In Chromium's own text, in every language, such as "About My Browser" |
| `productIconPNG` | Everywhere Chromium shows its logo. A square PNG, from 256 to 4096 pixels wide. |
| `productIconDarkPNG`, `productIconSVG`, `productIconDarkSVG` | Where Chromium shows a dark or vector logo. Each falls back to the PNG. |
| `copyright`, `companyName` | Settings › About and `chrome://version`, instead of "The Chromium Authors" |
| `helpURL` | What "Get help" opens in Settings › About. Without it, the row is hidden. |
| `appVersion` | The version line in Settings › About, such as "Version 1.2.0 (Official Build, Chromium 157.0.8103.39)" |

Each is optional, and Chromium's own stays for any you leave out. `Wren.h` gives the exact rules for each field, beside `WrenStartup`.

A few places keep Chromium's name or artwork, such as some error-page text, icons built into Chromium, and languages that change the form of a name. [This release](this-release.md) lists them.

Your app's own name, Dock icon and bundle ID aren't set here. They come from [`wren-app.json`](wren-cli.md#the-manifest) when you bundle the app.

## What Chromium doesn't do by itself

Chromium started as Chrome does a few things a browser built on it usually wouldn't want. Wren Runtime turns them off, and you can do each one yourself when you want it.

- **No window opens by itself.** Not at launch, not when someone clicks the Dock icon, and not when macOS asks your app to open a link. You get `ready`, `reopen` and `openURLs`, and open what suits your app.
- **The last session isn't restored by itself,** after a quit or after a crash. Chromium still records it, and `profile.restoreLastSession` brings it back. See [Windows and tabs](windows-and-tabs.md#restoring-the-last-session).
- **Chrome's field trials don't apply.** Your app runs on Chromium's built-in defaults, plus a fixed list of features that Chrome turns on.

Chromium's sandboxes and site isolation work exactly as they do in Chrome.

## Chrome's security features

Chrome turns on some security features because it's Google's build, and a build of Chromium leaves them off. Your app gets them as follows.

- **Certificate Transparency is enforced,** as in Chrome. A site whose certificate comes from a public certificate authority must show it was publicly logged, or the page fails with `ERR_CERTIFICATE_TRANSPARENCY_REQUIRED`. Chromium keeps its list of logs current through its component updater. To turn it off, pass `--disable-features=CertificateTransparencyAskBeforeEnabling` in `developmentSwitches`.
- **`channel`** says which of Chrome's channels your app is on: `.stable`, `.beta`, `.dev` or `.canary`. The default, `.unknown`, is what a build of Chromium reports, and Chromium treats it as a developer's build. Every extension then gets the APIs Chrome keeps to its dev and canary channels, such as the `processes` permission. A shipped app usually sets `.stable`, so extensions see what they would in Chrome.
- **`googleAPIKey`** is your app's own key for Google's services. Safe Browsing's lists of dangerous sites download only with one, so without a key Safe Browsing has nothing to check pages against. Wren Runtime ships no key, because Google's terms don't let one be shared. Get a key for the Safe Browsing API from Google. Its terms say which uses need an agreement with Google. Chromium also sends the key to its other Google services. Some of Chrome's Safe Browsing protections, such as real-time checks, stay off in a build of Chromium even with a key.

## Other settings

- **`schemes`** lists URL schemes your app serves itself, such as `app://`. They go here because Chromium fixes its list of schemes as it starts. See [Your own URL schemes](schemes.md).
- **`extensions`** lists extensions your app ships, as `.crx` files. If Wren Runtime can't read one, the app doesn't start. The settings for Manifest V2 extensions and native messaging hosts are here too. See [Extensions your app ships](extensions.md).
- **`developmentSwitches`** passes extra Chromium command-line switches while you develop, such as `--remote-debugging-pipe`. Wren Runtime refuses a switch that would override something `Startup` decides, such as the data folder.

## An app without a Dock icon

Your app can't set `LSUIElement` in its `Info.plist`, because Chromium would take it for one of its own helper processes and fail to start. Set the activation policy in `ready` instead. It runs before the event loop starts, so the Dock icon never appears.

```swift
delegate.ready = {
    NSApp.setActivationPolicy(.accessory)
    // Then open the first Profile and Browser, as usual.
}
```

## Being the default browser

`WrenApp.defaultBrowserState` asks macOS whether your app is the default browser. `WrenApp.setAsDefaultBrowser` asks macOS to make it so, which shows macOS's own prompt, as Chrome's Settings do. Both answer in a completion.

macOS offers your app as a browser only if its `Info.plist` claims `http`, `https` and `public.html`. List them in the manifest's `urlSchemes` and `documentTypes`, and `wren bundle` adds them. See [The `wren` command](wren-cli.md#the-manifest). The links macOS then sends your app arrive in `openURLs`.

After your first ask, Wren Runtime checks again each time your app becomes active, and reports a different answer as a `.defaultBrowser` change. So you hear about it when someone picks another browser in System Settings. `canSet` is false where policy turns the setting off.

## Checking versions

`WrenApp.versionInfo` reports the release's version, the Chromium version and Wren Runtime's patch revision. `WrenApp.apiVersion` is the API version your app was built against, and `WrenApp.frameworkAPIVersion` is the version of the framework that loaded. If they differ, `run` returns `.apiMismatch` without starting.

## The names in Swift, Rust and C

Rust's `Startup` has the same fields in snake case, such as `user_data_directory` and `product_icon_png`. `Startup::new` fills in the defaults.

| Swift | Rust | C |
| --- | --- | --- |
| `Startup` | `Startup`, `Startup::new` | `WrenStartup` |
| `.system`, `.mock` | `Keychain::System`, `Keychain::Mock` | `WREN_KEYCHAIN_SYSTEM`, `WREN_KEYCHAIN_MOCK` |
| `.default`, `.chromium` | `AppMenu::Default`, `AppMenu::Chromium` | `WREN_MENU_DEFAULT`, `WREN_MENU_CHROMIUM` |
| `Scheme` | `Scheme` | `WrenScheme` |
| `AppExtension` | `AppExtension` | `WrenAppExtension` |
| `.unknown`, `.stable`, `.beta`, `.dev`, `.canary` | `Channel::Unknown`, `Channel::Stable` and the others | `WREN_CHANNEL_UNKNOWN`, `WREN_CHANNEL_STABLE` and the others |
| `googleAPIKey` | `google_api_key` | `google_api_key` |
| `WrenApp.run` | `wren::run` | `Wren_Main` |
| `.invalidStartup`, `.alreadyRunning`, `.apiMismatch` | `ExitStatus::InvalidStartup`, `ExitStatus::AlreadyRunning`, `ExitStatus::ApiMismatch` | `WREN_EXIT_INVALID_STARTUP`, `WREN_EXIT_ALREADY_RUNNING`, `WREN_EXIT_API_MISMATCH` |
| `WrenApp.setMainMenu` | `MainThread::set_main_menu` | `Wren_SetMainMenu` |
| `WrenApp.defaultBrowserState` | `MainThread::default_browser_state` | `Wren_GetDefaultBrowserState` |
| `WrenApp.setAsDefaultBrowser` | `MainThread::set_as_default_browser` | `Wren_SetAsDefaultBrowser` |
| `WrenApp.versionInfo` | `wren::version_info` | `Wren_GetVersionInfo` |
| `WrenApp.apiVersion` | `wren::API_VERSION` | `WREN_API_VERSION` |
| `WrenApp.frameworkAPIVersion` | `wren::framework_api_version` | `Wren_GetAPIVersion` |

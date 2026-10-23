# Startup

`Startup` is everything Chromium needs to know before it runs. It says where its
data goes, where its encryption key is kept, which menu to install, and what
your app is called. `WrenApp.run` reads it once. A value Wren cannot accept
stops the launch. `run` then returns `.invalidStartup`, and Wren prints the
reason to stderr. The [table at the end](#the-names-in-swift-rust-and-c) gives
each name in Rust and in C.

```swift
var startup = Startup(userDataDirectory: directory, keychain: .system)
startup.keychainName = "My Browser"
startup.productName = "My Browser"
startup.productIconPNG = try Data(contentsOf: iconURL)
startup.schemes = [Scheme("app", privileges: [.standard, .secure, .fetch])]

let status = WrenApp.run(startup, delegate)
```

## The data directory

`userDataDirectory` is an absolute path. Chromium keeps every Profile in it,
with their history, cookies, extensions and caches. On a Mac it belongs under
`~/Library/Application Support/<your app>`.

- **One process per directory.** A second launch on the same directory returns
  `.alreadyRunning`. Wren tells the first process about it, and that process
  opens nothing by itself.
- **The startup Profile.** As in Chrome, Chromium starts on the Profile whose
  Browser was active last, and on a new Profile in `Default` at the first
  launch. `WrenApp.openProfile()` opens it. `startupProfileName` names that new
  Profile. It never renames a Profile. Without it, Chromium chooses the name.
- **Crash reports** go to `Crashpad` inside the directory.

## The keychain

Chromium encrypts cookies and saved passwords with a key. `keychain` says where
the key lives.

|         | `.system`                                                          | `.mock`               |
| ------- | ------------------------------------------------------------------ | --------------------- |
| The key | In the login Keychain, as the item "\<keychainName\> Safe Storage" | A fixed key in memory |
| For     | An app signed with a Developer ID                                  | Development and tests |

`.system` needs `keychainName`. Keep that name for the life of the product. A
new name means a new key, and Chromium can no longer read the cookies and
passwords already on disk.

Use `.mock` while developing. An ad hoc signature changes with every build, and
macOS asks for access to the Keychain again each time. Data written with one
kind of key cannot be read with the other. So give development builds a data
directory of their own. Wren's sample uses `WrenSample` and
`WrenSample Development`.

## The main menu

Wren installs a main menu before `ready`. `menu` chooses which one.

- **`.default`.** App, File, Edit, View and Window menus, like Electron's
  default menu. The View menu's Reload, Developer Tools and zoom items act on
  the Browser in the key window.
- **`.chromium`.** Chrome's own main menu, without the lists Chrome fills in as
  it runs, such as bookmarks, history and profiles. Its commands act on the
  Browser in the key window. Find… opens Chromium's find bar. Commands whose UI
  Wren does not have, such as Name Window, are disabled. Open File, Save Page
  and Print run Chromium's own commands.

In both, ⌘Q warns before quitting while Chrome's Warn Before Quitting is on, and
pages' `beforeunload` and the prompt for downloads in progress can stop the
quit. [Menus](menus.md#asking-before-quitting) shows how to show your own
warning.

Either menu is only a starting point. From `ready` on, a real browser usually
sets its own. It can describe it [as data](menus.md#the-main-menu) with
`WrenApp.setMainMenu`, with Chrome's commands among its own items. Or it can
build an `NSMenu` itself, in which an item copied from Chrome's menu keeps
working.

A Browser is in the key window when the view it is attached to, by
`browser.attach(to:)`, is in that window.

A key that a page does not use goes on to the main menu. So your shortcuts work
while a page has focus.

## Your name and icon in Chromium's UI

Some of what the user sees is still Chromium's: Settings, the New Tab page,
`chrome://` pages, toasts, dialogs and prompts. These fields put your product
there in place of "Chromium". Without them, that UI says "Chromium" and shows
Chromium's logo, so a shipped app sets them.

| Field                                                        | Where it shows                                                                                       |
| ------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------- |
| `productName`                                                | In Chromium's own text, in whatever language it runs in: "Customise My Browser", "About My Browser". |
| `productIconPNG`                                             | Every place Chromium shows its logo. A square PNG, 256 to 4096 pixels wide.                          |
| `productIconDarkPNG`, `productIconSVG`, `productIconDarkSVG` | Optional variants, where Chromium shows a dark or a vector logo. Each falls back to the PNG.         |
| `copyright`, `companyName`                                   | Settings › About and `chrome://version`, in place of "The Chromium Authors".                         |
| `helpURL`                                                    | What "Get help" in Settings › About opens. Without it, the row is hidden.                            |

All are optional. Leave them out and Chromium's stay. `Wren.h` gives each
field's exact rules beside `WrenStartup`.

These fields set what Chromium shows inside the app. The app's own name, its
Dock icon and its bundle ID are set when the app is bundled, in
[`wren-app.json`](wren-cli.md#the-manifest).

A few places keep Chromium's name or artwork: some error-page text, icons
compiled into Chromium, and languages that inflect the name.
[This release](this-release.md) lists them.

## URL schemes

`schemes` declares the URL schemes your app answers itself. They must be
declared here, because Chromium fixes its list of schemes as it starts. See
[Your own URL schemes](schemes.md).

## Extensions

`extensions` lists the extensions your app ships, as `.crx` files, for this
launch only. Wren reads each file's ID and version before Chromium starts. A
file it cannot read stops the launch. See
[Extensions your app ships](extensions.md).

## Development switches

`developmentSwitches` passes extra Chromium command-line switches, each starting
with `--`, such as `--remote-debugging-pipe`. They are for development. Wren
rejects a switch that would override what `Startup` decides, such as the data
directory, or restoring the last session.

## What Wren decides for you

Chromium started the usual way does several things a Wren app would not want.
Wren turns them off, and each has an API if you want it back.

- **No window opens by itself.** Not at launch, not when the user clicks the
  Dock icon, and not when macOS asks the app to open a URL. You get `ready`,
  `reopen` and `openURLs`, and open what you choose.
- **No session is restored by itself,** after a quit or after a crash. Chromium
  still records each session. Restore it with `profile.restoreLastSession` when
  you want it. See
  [Windows and tabs](windows-and-tabs.md#restoring-the-last-session), which also
  says how to bring back a window of several Browsers as one window.
- **Chrome's field trials are not applied.** A Wren app runs on Chromium's
  compiled defaults, plus a fixed list of features Chrome has on.

Chromium's sandboxes and site isolation are untouched.

## An app with no Dock icon

A Wren app cannot declare `LSUIElement` in its `Info.plist`. Chromium takes any
such process for one of its own helpers, and fails to start. Set the activation
policy in `ready` instead, which runs before the event loop starts:

```swift
delegate.ready = {
    NSApp.setActivationPolicy(.accessory)
    …
}
```

## Checking the version

`WrenApp.versionInfo` reports the release's version, the Chromium version and
Wren's patch revision. `WrenApp.apiVersion` is the API the app was compiled
against. `WrenApp.frameworkAPIVersion` is the API of the framework that loaded.
`run` refuses to start when they differ.

## The names in Swift, Rust and C

Rust's `Startup` has the same fields in snake case, such as
`user_data_directory` and `product_icon_png`. `Startup::new` fills in the
defaults.

| Swift                                | Rust                                                       | C                                                        |
| ------------------------------------ | ---------------------------------------------------------- | -------------------------------------------------------- |
| `Startup`                            | `Startup`, `Startup::new`                                  | `WrenStartup`                                            |
| `.system`, `.mock`                   | `Keychain::System`, `Keychain::Mock`                       | `WREN_KEYCHAIN_SYSTEM`, `WREN_KEYCHAIN_MOCK`             |
| `.default`, `.chromium`              | `AppMenu::Default`, `AppMenu::Chromium`                    | `WREN_MENU_DEFAULT`, `WREN_MENU_CHROMIUM`                |
| `Scheme`                             | `Scheme`                                                   | `WrenScheme`                                             |
| `AppExtension`                       | `AppExtension`                                             | `WrenAppExtension`                                       |
| `WrenApp.run`                        | `wren::run`                                                | `Wren_Main`                                              |
| `.invalidStartup`, `.alreadyRunning` | `ExitStatus::InvalidStartup`, `ExitStatus::AlreadyRunning` | `WREN_EXIT_INVALID_STARTUP`, `WREN_EXIT_ALREADY_RUNNING` |
| `WrenApp.setMainMenu`                | `MainThread::set_main_menu`                                | `Wren_SetMainMenu`                                       |
| `WrenApp.versionInfo`                | `wren::version_info`                                       | `Wren_GetVersionInfo`                                    |
| `WrenApp.apiVersion`                 | `wren::API_VERSION`                                        | `WREN_API_VERSION`                                       |
| `WrenApp.frameworkAPIVersion`        | `wren::framework_api_version`                              | `Wren_GetAPIVersion`                                     |

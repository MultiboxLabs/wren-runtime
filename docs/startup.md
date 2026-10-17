# Startup

`Startup` (in C, `WrenStartup`) is everything Chromium needs to know before it
runs: where its data goes, where its encryption key is kept, which menu to
install, and what your app is called. It is read once, by `WrenApp.run`. A value
Wren cannot accept stops the launch: `run` returns `.invalidStartup` and the
reason is printed to stderr.

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

- **One running process per directory.** A second launch on the same directory
  returns `.alreadyRunning`; the first process is told about it and opens
  nothing by itself.
- `profileDirectory` names the startup Profile's folder inside it. The default,
  `"Default"`, is what Chrome uses.
- Crash reports go to `Crashpad` inside the directory.

## The keychain

Chromium encrypts cookies and saved passwords with a key. `keychain` says where
that key lives.

|         | `.system`                                                          | `.mock`               |
| ------- | ------------------------------------------------------------------ | --------------------- |
| The key | In the login Keychain, as the item "\<keychainName\> Safe Storage" | A fixed key in memory |
| For     | An app signed with a Developer ID                                  | Development and tests |

`.system` needs `keychainName`. **Keep that name for the life of the product**:
a new name means a new key, and the cookies and passwords already on disk can no
longer be read.

Use `.mock` while developing. An ad hoc signature changes with every build, and
macOS asks for the Keychain again each time it does. Data written with one kind
of key cannot be read with the other, so give development builds a data
directory of their own; the sample uses `WrenSample` and
`WrenSample Development`.

## The main menu

Wren installs a main menu before `ready`. `menu` chooses which:

- **`.default`**: App, File, Edit, View and Window menus, as Electron's default
  menu has. The View menu's Reload, Developer Tools and zoom items act on the
  Browser in the key window.
- **`.chromium`**: Chrome's own main menu, without the lists Chrome fills in as
  it runs (bookmarks, history, profiles). Its commands act on the Browser in the
  key window; Find… opens Chromium's find bar. Commands whose UI Wren does not
  have, such as Print, are disabled.

Either is only a starting point. From `ready` onwards a real browser usually
sets its own: as [data](menus.md#the-main-menu) (`WrenApp.setMainMenu`), with
Chrome's commands among its own, or as an `NSMenu` it builds, in which an item
copied from Chrome's menu keeps working.

A Browser is "in the key window" when the view it is attached to
(`browser.attach(to:)`) is in that window.

A key that a page does not use goes on to the main menu, so your shortcuts work
while a page has focus.

## Your name and icon in Chromium's UI

Some of what the user sees is still Chromium's: Settings, the New Tab page,
`chrome://` pages, dialogs and prompts. These fields put your product there in
place of "Chromium".

| Field                                                        | Where it shows                                                                                       |
| ------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------- |
| `productName`                                                | In Chromium's own text, in whatever language it runs in: "Customise My Browser", "About My Browser". |
| `productIconPNG`                                             | Every place Chromium shows its logo. A square PNG, 256 to 4096 pixels wide.                          |
| `productIconDarkPNG`, `productIconSVG`, `productIconDarkSVG` | Optional variants, where Chromium shows a dark or a vector logo. Each falls back to the PNG.         |
| `copyright`, `companyName`                                   | Settings › About and `chrome://version`, in place of "The Chromium Authors".                         |
| `helpURL`                                                    | What "Get help" on Settings › About opens. Without it the row is hidden.                             |

All are optional; leave them out and Chromium's stay. `Wren.h` gives each
field's exact rules beside `WrenStartup`.

This is what Chromium shows _inside_ the app. The app's own name, its Dock icon
and its bundle ID are set when the app is bundled, in
[`wren-app.json`](wren-cli.md#the-manifest).

A few places keep Chromium's name or artwork: some error-page text, icons
compiled into Chromium, and languages that inflect the name.
[This release](this-release.md) lists them.

## URL schemes

`schemes` declares the URL schemes your app answers itself. They must be
declared here because Chromium fixes its list of schemes as it starts; see
[Your own URL schemes](schemes.md).

## Development switches

`developmentSwitches` passes extra Chromium command-line switches, each starting
with `--`, for example `--remote-debugging-pipe`. They are for development. A
switch that would override what `Startup` decides (the data directory, restoring
the last session) is rejected.

## What Wren decides for you

Chromium started the usual way does several things a Wren app would not want.
Wren turns them off, and each has an API if you want it back:

- **No window opens by itself.** Not at launch, not when the Dock icon is
  clicked, not when macOS asks the app to open a URL. You get `ready`, `reopen`
  and `openURLs` and open what you choose.
- **No session is restored by itself**, after a quit or after a crash. Chromium
  still records each session; restore it with `profile.restoreLastSession` when
  you want it
  ([Windows and tabs](windows-and-tabs.md#restoring-the-last-session)).
- **Chrome's field trials are not applied.** A Wren app runs on Chromium's
  compiled defaults plus a fixed list of features Chrome has on.

Chromium's sandboxes and site isolation are untouched.

## An app with no Dock icon

A Wren app cannot declare `LSUIElement` in its `Info.plist`: Chromium takes any
such process for one of its own helpers and fails to start. Set the activation
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
against and `WrenApp.frameworkAPIVersion` the one the loaded framework has;
`run` refuses to start when they differ.

# Wren 0.3.0

This release of Wren is for apps on macOS 13 or later on Apple silicon. Wren is before 1.0. An app is built and bundled with one release, and moves to the next by building again. The Compatibility guide says what may change, and what changed in this release.

| Part | Version |
| --- | --- |
| Wren | `0.3.0` |
| Chromium | `156.0.8078.80`, with Wren's patch revision `74` |
| `WREN_API_VERSION` | `64` |
| `WREN_UNSTABLE_UI_VERSION` | `4` |
| Rust, for the `wren` crate | `1.85` or later |

`release.json` in the release names the build exactly. `Wren_GetVersionInfo` reports it at runtime.

## Chromium, and security

`156.0.8078.80` is Chrome's Stable and Extended Stable release of 2026-10-29. Later Chrome releases will carry security fixes it lacks. An app that shows pages from the open web should move to the next Wren release when there is one.

The build has no Google API keys, so what needs them does not work. That includes Translate, and Safe Browsing's checks of downloads.

## Getting it

Install the `wren` command, which is published with each release. Name `0.3.0` in an app's `wren-app.json`, as `"wren": "0.3.0"`, and run `wren install`. It fetches the release, checks it against `SHA256SUMS`, and links it into the project. Getting started says how to fetch it with `gh` instead. The release is not signed. An app is signed when `wren bundle` makes it.

**From 0.2.0.** Install `wren` again: 0.3.0's bundles faster, reads more of `wren-app.json`, and adds `wren extension-id`. Then change `"wren"`, run `wren install`, make the changes the Compatibility guide lists under v0.3.0, and build again.

## Changes since 0.2.0

### New

- **Chromium `156.0.8078.80`.** From `156.0.8078.48`.
- **Windows.** Wren acts on Chromium's requests to show, activate and keep a window back as Chrome's windows do, and passes each request to move, resize, minimise or zoom a window to the app in `on_window_request`, which carries it out. `on_quit_committed` says once that a quit can no longer be cancelled.
- **Tabs.**
  - `Wren_MoveTabsToBrowser` and `Wren_MoveTabsToNewBrowser` move tabs to another Browser, keeping their pages. `Wren_MoveTabGroup` and `Wren_MoveTabSplit` move a group or a split whole.
  - `Wren_MovePopupTabToBrowser` and `Wren_MovePopupTabToNewBrowser` move a popup's tab into a normal Browser, as Chrome's Show as Tab does.
  - `Wren_CaptureTab` gives a PNG or JPEG of a tab's page, shown or not.
  - A tab's state says when it was last on screen, its alerts (audio, camera, sharing and more), whether it is muted, and whether it may ask before it closes. `Wren_SetTabMuted` mutes it.
  - `Wren_SetTabSessionExcluded` keeps a tab out of the session.
- **The clipboard.** `Wren_WriteClipboard` and `Wren_ReadClipboard` write and read text and a URL as Chrome's commands do, marked concealed or off the record.
- **Extensions.**
  - A Profile's extensions are read and changed through the Profile, with Allow in Incognito.
  - An extension written for Chrome talks to the app through native messaging, with no process: the app names its hosts in `WrenStartup.native_hosts` and answers each port.
  - `wren extension-id` prints the ID of a `.crx` or a folder. An internal extension's `key` may be a PEM block.
- **Downloads.** `Wren_GetDownloadHistory` gives a Profile's whole history, as chrome://downloads lists it, with Remove from List and Clear All.
- **Importing.** `Wren_AddHistory`, `Wren_AddCookies` and `Wren_InstallFromWebStore` bring history, cookies and store extensions in from another browser.
- **The password manager.** `Wren_SetPasswordManager` turns it off for a Profile.
- **Updates.** An app updates itself with Sparkle. Settings › About shows what the app's updater says, `Wren_Relaunch` relaunches as Chrome does, and `wren bundle` copies the app's frameworks and adds its `Info.plist` keys. `WrenStartup.app_version` names the app's version in About.
- **The default browser.** `Wren_GetDefaultBrowserState` and `Wren_SetAsDefaultBrowser`, with `WREN_CHANGE_DEFAULT_BROWSER`.
- **Web views.**
  - `glass` lets a view's pages draw an element as Liquid Glass, with `-wren-glass` and `-wren-glass-tint`.
  - `wheel_phases` gives a view's pages a trackpad scroll's phases on `WheelEvent`.
  - `drag_area_clicks` lets clicks in a drag area reach the page. A title bar's double click with Fill now fills.
- **Other calls.** `Wren_PostToMainThread` runs work from any thread on the main thread. `WrenToastOptions.name` names an app's toast. `Wren_ClearForegroundRequest` forgets a background Browser's request to come forward.
- **`wren bundle`.** URL types with names, document types with roles, imported and exported type declarations, entitlements with values, and a provisioning profile, all checked before anything is written. `--incremental` copies in only what changed during development.

### Changed

These break an app built on 0.2.0. The Compatibility guide says how to move, in C, Swift and Rust.

- Coming forward and staying back follow Chrome, and Chromium's requests to move or resize a window go to the app instead of being reported as done.
- A Profile's extension calls take the Profile, not a Browser, and `on_profile_extensions` takes a request.
- Rust's `Startup`, `SessionState`, `TabState`, `DownloadState`, `WebViewOptions` and `AppExtension` have new fields, so a struct literal must name them. Rust's `show_toast` takes `ToastOptions`.
- A download's progress is a change to it alone, with `WREN_CHANGE_DOWNLOAD`, and its actions include Remove.
- A tab with no favicon has none, in place of Chromium's globe.
- New change kinds, `WREN_CHANGE_DOWNLOAD` and `WREN_CHANGE_DEFAULT_BROWSER`, must be handled by a `switch` or `match` with no default.
- `wren bundle` refuses more manifests: `infoPlist` may not set the type declarations, and `usageDescriptions` is checked.

### Fixed

- A swipe's arrow over a tab in an element shows at the tab's edge.
- A `<permission>` element's prompt shows over the page, and no longer crashes the app when it closes.
- Chrome's Show As Tab no longer crashes on a zoomed page.

## Licence

Wren may be used only with the written permission of Multibox Labs, as `LICENSES/Wren.txt` says. The framework is Chromium, whose notices go with every app that ships it: `LICENSES/Chromium.txt`, `third-party.txt` and `credits.html`. It decodes H.264, AAC and HEVC. Any patent licensing an app that ships them needs is the app's.

## What it does not do yet

- **Platforms.** macOS on Apple silicon only.
- **Passkeys.** They are not saved in the system keychain.
- **What Chrome draws in its own window.** These are missing:
  - infobars, page info's offer to reload among them
  - the cookie controls, file-system access and memory saver icons in the address field
  - the panels drawn in Views: Lens results, About this site, and Merchant trust
  - reading mode's overlay, though its panel works
  - Lens's overlay. Image search opens Lens in a tab, and "Search This Tab" is not offered.
  - in-product help
  - moving the keyboard between a window's panes with F6
  - the Touch Bar's items for a card field
  - a page's `EyeDropper`, which is refused
- **Chrome's menu.** Name Window is disabled. The People menu and File's Share are left out.
- **App schemes.** They have no request bodies, streaming, redirects or CORS.
- **Bundling.** `wren` makes an app, not a disk image.

## Known issues

- A second launch in the instant before the first has started can take its place.
- A few places keep Chromium's name or artwork. They are three error-page strings, icons compiled into Chromium for the WebAuthn dialog, QR codes and the New Tab search box, and languages that inflect or transliterate the name.
- The crash handler outlives the app for over a minute.
- A bundle signed with a real identity and a provisioning profile has not been checked, nor Sparkle's updates of an app signed with a Developer ID. Both are checked only with bundles the tests make.
- A tab kept out of the session comes back with its tab group, which Chrome's saved groups keep apart from the session. Keep such tabs out of groups.
- While a web view's page has a `wheel` listener that is not passive, a swipe over a tab in a `<wren-view>` sometimes fails to go back.
- A bundled app makes an empty `~/Library/Application Support/<name>`, Chromium's default data folder, even when its data is elsewhere.

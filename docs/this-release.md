# Wren 0.2.0

This release of Wren is for apps on macOS 13 or later on Apple silicon. Wren is
before 1.0. An app is built and bundled with one release, and moves to the next
by building again. The Compatibility guide says what may change, and what
changed in this release.

| Part                       | Version                                          |
| -------------------------- | ------------------------------------------------ |
| Wren                       | `0.2.0`                                          |
| Chromium                   | `156.0.8078.48`, with Wren's patch revision `64` |
| `WREN_API_VERSION`         | `46`                                             |
| `WREN_UNSTABLE_UI_VERSION` | `4`                                              |
| Rust, for the `wren` crate | `1.85` or later                                  |

`release.json` in the release names the build exactly. `Wren_GetVersionInfo`
reports it at runtime.

## Chromium, and security

`156.0.8078.48` is Chrome's Stable release of 2026-10-20. Later Chrome releases
will carry security fixes it lacks. An app that shows pages from the open web
should move to the next Wren release when there is one.

The build has no Google API keys, so what needs them does not work. That
includes Translate, and Safe Browsing's checks of downloads.

## Getting it

Install the `wren` command, which is published with each release. Name `0.2.0`
in an app's `wren-app.json`, as `"wren": "0.2.0"`, and run `wren install`. It
fetches the release, checks it against `SHA256SUMS`, and links it into the
project. Getting started says how to fetch it with `gh` instead. The release is
not signed. An app is signed when `wren bundle` makes it.

**From 0.1.0.** Install `wren` again: 0.2.0's adds `wren --version` and
`wren pack-extension`. It keeps releases in `~/.wren/runtimes`, so
`~/Library/Application Support/Wren/runtimes`, where 0.1.0's kept them, can be
deleted. Then change `"wren"`, run `wren install`, make the changes the
Compatibility guide lists under v0.2.0, and build again.

## Changes since 0.1.0

### New

- **Chromium 156.** From `155.0.8059.12`.
- **Bookmarks.** A Profile's bookmarks are state an app reads, changes and
  hears, in `WrenBookmarks.h`. So an app draws its own bookmarks bar, and keeps
  folders of its own. Chromium's bookmark editor is offered too.
- **Extensions an app brings.** An app lists `.crx` files and internal
  extensions at startup, and chooses which Profiles get them. The user cannot
  remove them, and the extensions page says the app manages them. Manifest V2
  runs for the extensions the app names, or for all. `wren pack-extension` packs
  an extension's folder.
- **Managing Profiles.** An app renames and deletes a Profile, sets its avatar
  and theme colour, and clears its browsing data as Chrome's dialog does.
  `WrenStartup.startup_profile_name` names the Profile Chromium creates at the
  first launch.
- **Warn Before Quitting.** An app reads, changes and hears Chrome's setting,
  and answers ⌘Q's warning with its own prompt, none, or Chromium's panel, in
  `on_quit_prompt`. ⌘Q again asks again. `Wren_Quit` never warns.
- **A persistent ID for each tab.** `Wren_CopyTabPersistentID` gives each tab an
  ID that stays the same for its life, and that a restored tab has again. So an
  app finds its own record of a tab after a relaunch.
- **A tag for each Browser.** An app sets a short string on a Browser, which
  Chromium keeps in the session and with closed windows. A restored or reopened
  Browser has it before `on_browser_opened`.
- **Web views.**
  - `Wren_CloseWebViewDevTools` closes a web view's DevTools, and
    `WrenWebViewState.devtools` says whether they are open.
  - `disallow_user_devtools_activation` keeps a web view's DevTools for the app
    alone: the app opens them with `Wren_OpenWebViewDevTools`, and the text
    fields' menu has no Inspect.
  - A page calls `wren.pathForFile(file)` for the path of a file dropped on it,
    so an app opens a dropped file in a tab, as Chrome does.
  - A page ignores trackpad zoom and swipes, as Chrome's own UI does. A tab or
    side panel in a `<wren-view>` takes its own pinch, scroll and stretch, and
    goes back and forward with a two-finger swipe.
  - A `<wren-view>` whose `data-id` moves to another tab shows it at once, as
    Chrome switches tabs. A side panel's element still keeps the old panel until
    the new one has drawn.
- **Menus.** `WrenMenuItem.hidden` hides an item whose key still runs it. ⌘← and
  ⌘→ go back and forward in a tab when the page does not take them, as in
  Chrome, with no menu item.
- **Chrome's file commands.** Open File, Save Page As, Print and Print Using
  System Dialog work in Chrome's menu. Every sheet of a Browser, Chromium's
  dialogs and macOS's file panels alike, hangs off the app's window.
- **Documents.** A release has `AGENTS.md` for coding agents, and both samples'
  sources to read. Every public Swift and Rust item has a comment, and each
  wrapper names the C function it calls.
- **`wren --version`** prints the command's release.

### Changed

These break an app built on 0.1.0. The Compatibility guide says how to move, in
C, Swift and Rust.

- `WrenWebViewOptions.disable_devtools` is now
  `disallow_user_devtools_activation`.
- `WrenMenuItem.hidden` is new, which changes Rust's `MenuItem::Item`, a Swift
  pattern that lists `.item`'s values, and C that fills the struct by position.
- `WrenStartup.profile_directory` is gone. Wren starts on the Profile Chrome
  would.
- Pages no longer see ⌘Q.
- `Wren_RestoreLastSession` restores a Profile's last session once per launch.

These need nothing from an app:

- `wren install` keeps releases in `~/.wren/runtimes`.
- The Rust crate's build script links the app again when the framework changes
  in place.

### Fixed

- An app crashed when the renderer of a web view that shows a tab went away.
- Wren no longer writes its own diagnostic lines, with page URLs, to an app's
  output.
- The Swift package builds with no warning.
- The guides no longer ask for `xattr` after a download. They fetch a release
  with `gh`, which sets no quarantine flag.

## Licence

Wren may be used only with the written permission of Multibox Labs, as
`LICENSES/Wren.txt` says. The framework is Chromium, whose notices go with every
app that ships it: `LICENSES/Chromium.txt`, `third-party.txt` and
`credits.html`. It decodes H.264, AAC and HEVC. Any patent licensing an app that
ships them needs is the app's.

## What it does not do yet

- **Platforms.** macOS on Apple silicon only.
- **Passkeys.** They are not saved in the system keychain.
- **What Chrome draws in its own window.** These are missing:
  - infobars, page info's offer to reload among them
  - the cookie controls, file-system access and memory saver icons in the
    address field
  - the panels drawn in Views: Lens results, About this site, and Merchant trust
  - reading mode's overlay, though its panel works
  - Lens's overlay. Image search opens Lens in a tab, and "Search This Tab" is
    not offered.
  - in-product help
  - moving the keyboard between a window's panes with F6
  - the Touch Bar's items for a card field
  - a page's `EyeDropper`, which is refused
- **Chrome's menu.** Name Window is disabled. The People menu and File's Share
  are left out.
- **App schemes.** They have no request bodies, streaming, redirects or CORS.
- **Bundling.** `wren` makes an app, not a disk image.

## Known issues

- A second launch in the instant before the first has started can take its
  place.
- A few places keep Chromium's name or artwork. They are three error-page
  strings, icons compiled into Chromium for the WebAuthn dialog, QR codes and
  the New Tab search box, and languages that inflect or transliterate the name.
- The crash handler outlives the app for over a minute.
- A bundled app makes an empty `~/Library/Application Support/<name>`,
  Chromium's default data folder, even when its data is elsewhere.

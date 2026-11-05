# Wren 0.4.0

This release is for apps on macOS 13 or later, on Apple silicon. Wren is not at 1.0 yet. You build and bundle an app with one release, and move it to the next by building again. The Compatibility guide says what may change between releases, and what changed in this one.

| Part | Version |
| --- | --- |
| Wren | `0.4.0` |
| Chromium | `157.0.8103.39`, with Wren's patch revision `78` |
| `WREN_API_VERSION` | `71` |
| `WREN_UNSTABLE_UI_VERSION` | `4` |
| Rust, for the `wren` crate | `1.85` or later |

`release.json` names the exact build. `Wren_GetVersionInfo` reports the same at runtime.

## Chromium, and security

`157.0.8103.39` is Chrome's Stable release of 2026-11-03. Every later Chrome release carries security fixes this one lacks. If your app shows pages from the open web, move to the next Wren release when it comes out.

The build has no Google API keys, so features that need them do not work. Translate is one. Safe Browsing's download checks are another.

## Getting it

Install the `wren` command, which ships with each release. Put `"wren": "0.4.0"` in your app's `wren-app.json` and run `wren install`. It fetches the release, checks it against `SHA256SUMS`, and links it into the project. Getting started shows how to fetch it with `curl` instead. The release itself is unsigned. `wren bundle` signs your app when it makes it.

**From 0.3.0.** Install `wren` again before you bundle anything. Chromium 157 starts its renderers and other processes from the framework's Aperitif helpers. 0.3.0's `wren bundle` deletes those helpers, so an app it bundles with 0.4.0 cannot open a page. With the new `wren` installed, change `"wren"`, run `wren install`, make the changes the Compatibility guide lists under v0.4.0, and build again.

## Changes since 0.3.0

### New

- **Chromium `157.0.8103.39`.** The last release had `156.0.8078.80`.
- **Tabs.**
  - A tab's state says whether it is waiting to load, in `unloaded`. It says which tab opened it and how, in `opened_by` and `opened_as`.
  - `Wren_SetTabIgnoresInput` makes a tab's page ignore input, the way a tab-modal dialog does. The tab's state reports it in `ignores_input`.
  - A tab's state says what its media is doing. That covers playing or paused, the actions its page offers, its position, length and rate, and a call's microphone and camera. `Wren_CopyTabMediaText` copies texts such as the title, and `Wren_CopyTabMediaArtworkPNG` the artwork. `Wren_ControlTabMedia` and `Wren_SeekTabMedia` act on the media.
  - `Wren_SetTabWatchesNavigations` turns on `on_navigation` for a tab. Your app then allows each of the tab's navigations, cancels it, or opens it in a new tab. A tab opened that way names the request in `opened_for_navigation`.
  - `Wren_KillTabRenderer` ends a tab's renderer, as Chrome's task manager does.
- **Browsers.** `Wren_CreatePopupBrowser` makes a popup the way Chromium makes one for a page. `Wren_CloseBrowsers` closes several Browsers, all of them or none, and tells you how the close ended.
- **Profiles.** `WREN_CHANGE_PROFILE_OPENED` reports every Profile Chromium loads. Each extension in a Profile's request carries its ID. Pinning an extension, moving it and showing its requests now take the Profile, so they work with no window open.
- **The Dock menu.** `Wren_SetDockMenu` puts your app's items in it.
- **Delayed work.** `Wren_PostToMainThreadAfter` runs work on the main thread after a delay.
- **Web views.** In a view with `glass`, Core Animation now draws an element's `backdrop-filter` of `blur()` and colour filters. The blur sits beside the glass and sees it beneath.
- **Internal extensions.** Their content scripts may name your app's own schemes, such as `app://ui/*`.
- **`wren`.** It bundles the framework's Aperitif helpers under your app's name, which Chromium 157 needs.

### Changed

Each of these breaks an app built on 0.3.0. The Compatibility guide shows how to move, in C, Swift and Rust.

- `Wren_CloseBrowsers` replaces `Wren_CloseBrowser`, and answers through a callback.
- Pinning an extension, moving it and showing its requests take the Profile, not a Browser.
- While a tab is its Browser's active tab, its `last_active_ms` is the current time, whether or not anyone can see it.
- `on_browser_opened`'s `opener` now comes from Chromium's record of the open. A window a page opened with `noopener` names its opener too.
- Rust's `TabState`, `ExtensionState` and `ProfileExtension` have new fields. A struct literal, or a pattern without `..`, must name them. `ProfileExtension` is no longer `Copy`.

### Fixed

- An internal extension's content script on your app's scheme no longer crashes every renderer.
- The Web Store install, Profile deletion and the cookie import no longer hold on to your app's context after they call back.

## Licence

Wren is proprietary, under `LICENSES/Wren.txt`. You may develop and test your own apps with Wren, and build and contribute to Wren Browser. To ship an app with Wren to anyone outside your organisation, you need a written licence from Multibox Labs for that app. Ask at licensing@wrenbrowser.com.

The framework is Chromium, and every app that ships it carries Chromium's notices. They are `LICENSES/Chromium.txt`, `third-party.txt` and `credits.html`. The framework decodes H.264, AAC and HEVC. If your app ships them, any patent licence it needs is yours to get.

## What it does not do yet

- **Platforms.** Only macOS on Apple silicon.
- **Passkeys.** Wren does not save them in the system keychain.
- **What Chrome draws in its own window.** These are missing:
  - infobars, including page info's offer to reload
  - the cookie controls, file-system access and memory saver icons in the address field
  - the panels Chrome draws in Views, which are Lens results, About this site and Merchant trust
  - reading mode's overlay, though its panel works
  - Lens's overlay. Image search opens Lens in a tab, and Wren does not offer "Search This Tab".
  - in-product help
  - moving the keyboard between a window's panes with F6
  - the Touch Bar's items for a card field
  - a page's `EyeDropper`, which Wren refuses
- **Chrome's menu.** Name Window is disabled. The People menu and File's Share are left out.
- **App schemes.** They have no request bodies, streaming, redirects or CORS.
- **Bundling.** `wren` makes an app, not a disk image.

## Known issues

- If a second launch starts in the instant before the first has finished starting, it can take the first one's place.
- A few places keep Chromium's name or artwork. Three error-page strings say "Chromium". The WebAuthn dialog, QR codes and the New Tab search box use icons compiled into Chromium. Languages that inflect or transliterate the name keep Chromium's form.
- The crash handler outlives the app for over a minute.
- Nobody has yet checked a bundle signed with a real identity and a provisioning profile, or Sparkle updating an app signed with a Developer ID. The tests check both only with bundles they make themselves.
- A tab kept out of the session still comes back with its tab group, because Chrome's saved groups keep their tabs apart from the session. Keep such tabs out of groups.
- While a web view's page has a `wheel` listener that is not passive, a swipe back over a tab in a `<wren-view>` sometimes fails.
- A bundled app makes an empty `~/Library/Application Support/<name>`, Chromium's default data folder, even when its data lives elsewhere.

# Wren Runtime 0.5.0

Wren Runtime 0.5.0 moves to Chromium `157.0.8103.51`. It lets you close several tabs at once, all or none, turns on Chrome's security features, opens a page's eyedropper, and draws glass over an element's `backdrop-filter`.

## Updating from 0.4.0

1. Change `"wren"` in your app's `wren-app.json` to `"0.5.0"`, and run `wren install`.
2. Make the changes that `docs/compatibility.md` in the release lists under v0.5.0. The breaking changes below say what they are.
3. Build your app again.

## Highlights

**Closing several tabs, all or none.** You can now close a set of tabs only if every one of their pages agrees, such as both tabs of a split. `Wren_CloseTabs` asks each page's `beforeunload` in turn, closes the tabs once every page has agreed, and otherwise leaves them all open. Its callback gets `WREN_OK` once the tabs have gone, or `WREN_ERROR_CANCELLED` with every tab still open.

**Chrome's channel and Safe Browsing.** Your app can now say which of Chrome's channels it's on, so extensions get the APIs they would in Chrome, and give Safe Browsing your own Google API key, so it can download its lists of dangerous sites. `WrenStartup` has `channel` and `google_api_key` for them. `docs/startup.md` says how to get a key.

**A page's eyedropper.** A page's `EyeDropper` now opens macOS's color sampler, as it does in Chrome, in a tab and in a web view. Before, it was refused.

**Glass with a `backdrop-filter`.** An element with both `-wren-glass` and a `backdrop-filter` now draws as glass. Before, the filter left it drawing its fallback. `Wren_CanDrawGlass` tells you whether the Mac can draw glass at all, so you can make a web view with `glass` only where it shows.

## Breaking changes

Each of these breaks an app built on 0.4.0, or changes what it sees. `docs/compatibility.md` shows how to move, in C, Swift and Rust.

- Certificate Transparency is now enforced, as in Chrome. A page whose certificate from a public root wasn't logged fails with `ERR_CERTIFICATE_TRANSPARENCY_REQUIRED`.
- Liquid Glass is now experimental: a web view's `glass`, `-wren-glass`, `-wren-glass-tint` and `Wren_CanDrawGlass` can change or go in any release.
- One call now copies each of a subject's strings, with a kind, such as `Wren_CopyTabText`. 31 copy functions and `WrenMediaText` are removed.
- `Wren_OpenProfile(NULL)` now opens the Profile Chromium moves to after the startup Profile is deleted, where it used to fail.
- `WrenUpdates.h` is removed. Its declarations are in the new `WrenApp.h`, with quitting and the default browser. Chromium's prompts and toasts moved to the new `WrenPrompts.h`.
- A quit asked while `Wren_RestoreLastSession` is reading the session now waits for the restore. Before, the app kept running with no window.
- Rust's `Startup` has new fields, `channel` and `google_api_key`, so a struct literal of it must name them.

## Other changes

- Chromium is now `157.0.8103.51`, up from `157.0.8103.39`. It adds a fix for a use-after-free in libxslt, which a page's XSLT can reach.
- A site's Install button no longer installs a web app. An installed web app's launcher couldn't open a window in your app, so Wren Runtime turns installs off, as Chrome's `WebAppInstallByUserEnabled` policy does. Policy can turn them back on.
- `WrenTabs.h` now says that `Wren_AddTabsToGroup` and `Wren_UngroupTabs` keep a split given both its tabs, which chooses a split's group at a group's edge.
- The unstable calls that take a `WrenUIText` are now named `Wren_Copy…UIText`, and `WREN_UNSTABLE_UI_VERSION` is 5.

## Fixes

- A page now keeps its last frame while another of your app's windows covers it. Before, it went blank a second after it was covered, and stayed blank until the page drew again.
- A web view's page now gets your messages and answers in the order you sent them, while it's frozen or paused too.
- With a side panel's page focused, your main menu's shortcuts now work, as they do from a tab.
- A `backdrop-filter` blur near the window's edge no longer lets the page show through sharp.
- A tab's favicon now goes when the tab navigates to a page without one.
- Your app no longer ends with a crash report at quit while other work holds the Mac's efficiency cores. Chromium's shutdown waited for a worker it had started at background priority.
- `Wren_AddCookies` now writes a host-only cookie, one whose domain has no leading dot, as host-only. Before, it made every such cookie on a hostname a domain cookie, sent to the host's subdomains too, and refused each `__Host-` cookie, such as Google's on `accounts.google.com`.
- `Wren_AddCookies` now keeps a cookie whose creation date is more than 400 days before its expiry, as Chrome keeps one a site sets again with the same value. Before, Chromium capped its expiry at 400 days after its creation date, so a long-lived cookie, such as a Google sign-in's, could be dropped while its result said written. The expiry is kept up to 400 days from now, and such a creation date moves to 400 days before the expiry.

## Known issues

- A second launch that starts in the instant before the first has finished starting can take the first one's place.
- A few places keep Chromium's name or artwork. Three error-page strings say "Chromium". The passkey dialog, QR codes and the New Tab search box use icons built into Chromium. Languages that change the form of a name keep Chromium's.
- Chromium's crash handler keeps running for over a minute after the app quits.
- Nobody has checked a bundle signed with a real identity and a provisioning profile yet, or Sparkle updating an app signed with a Developer ID. The tests check both only with bundles they make themselves.
- A tab kept out of the session still comes back with its tab group, because Chrome saves groups' tabs apart from the session. Keep such tabs out of groups.
- While a web view's page has a `wheel` listener that isn't passive, swiping back over a tab in a `<wren-view>` sometimes fails.
- A bundled app makes an empty `~/Library/Application Support/<name>`, Chromium's default data folder, even when its data lives elsewhere.

`docs/limitations.md` in the release lists what Wren Runtime can't do yet.

## Versions

| Part | Version |
| --- | --- |
| Chromium | `157.0.8103.51`, with Wren Runtime's patch revision `84` |
| `WREN_API_VERSION` | `76` |
| `WREN_UNSTABLE_UI_VERSION` | `5` |
| macOS | 13 or later, on Apple silicon |
| Rust, for the Rust SDK | `1.85` or later |

`157.0.8103.51` is Chrome's Stable release of 2026-11-09. Every later Chrome release has security fixes that this one doesn't, so if your app shows pages from the open web, move to the next Wren Runtime release when it comes out.

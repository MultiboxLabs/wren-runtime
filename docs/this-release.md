# Wren Runtime 0.6.0

Wren Runtime 0.6.0 moves to Chromium `158.0.8127.37`. Your app can now take over a page's context menu, work with cookies, and choose the user agent its pages send. This release also includes everything from the 0.5.1 prerelease.

## Updating from 0.5.0

1. Set `"wren"` in `wren-app.json` to `"0.6.0"`, and run `wren install`.
2. Make the changes `docs/compatibility.md` lists under v0.6.0.
3. Build and bundle your app again.

## Highlights

**Your app decides the context menu.** Before Chromium shows a page's context menu, your app can now choose Chromium's menu, no menu, or its own, either native or drawn in your UI. See `on_context_menu` and `WrenContextMenus.h`.

**Full cookie APIs.** You can now read, delete and watch a Profile's cookies, and change its cookie settings. See `WrenCookies.h`.

**Your own user agent.** You can now set the user agent and client hints your pages send, for the whole app, for a site, or for one tab. See `WrenUserAgent.h`.

**Copying a tab, and closing one without asking.** `Wren_DuplicateTab` copies a tab with its history, and `Wren_CloseTabWithoutAsking` closes one without its page's "Leave site?" prompt.

**Session data across relaunches.** A Profile can now keep its session cookies and sessionStorage for the next restore, with `Wren_SetProfileKeepsSessionData`.

**Loading UI that matches Chrome's.** A tab's and a web view's state now include `shows_loading` and `load_progress`, so your spinner and progress bar can behave like Chrome's.

**Turning autofill off.** `Wren_SetAutofill` turns off a Profile's address and payment suggestions, and leaves the password manager alone.

**Knowing where links come from.** `on_open_urls` now tells you which app sent the links, or that they came from Handoff.

## Breaking changes

`docs/compatibility.md` explains how to update for each of these, in C, Swift and Rust.

- `Wren_AddCookies` is now `Wren_SetCookies`, and `WrenCookie` has a new `priority` field.
- `on_open_urls` now takes a `WrenOpenURLsRequest` instead of a list of URLs.
- `Wren_ShowContextMenu` now takes a menu ID from `Wren_CreateContextMenu` instead of a view and a point, and `WrenMenuCallback` is gone.
- Several enums and structs have new members, including new change kinds. A `switch` or `match` without a default case, or a Rust struct literal, may need updating.
- You can no longer pass `--user-agent` as a startup switch. Set `WrenStartup.user_agent` instead.
- Closing a page runs its `unload` handler only where the page allows it, as in Chrome 158.
- A hidden `<wren-view>` now keeps its tab's page, instead of sending it back to the tab's own view.
- Paste and Match Style now uses ⇧⌘V, as in Chrome, instead of ⌥⇧⌘V.
- A page whose renderer never started now reports as killed rather than crashed.
- A download blocked by policy now fails as blocked rather than as a server failure.

## Other changes

- Chromium is now `158.0.8127.37`, up from `157.0.8103.51`.
- Cookies and passwords your app saved with earlier releases still decrypt under Chromium 158.
- Settings' About page now says your browser is built on Wren Runtime.
- `wren pack-extension` now compresses extensions, and packs the same files Chrome's Pack extension does.
- Both samples use the new APIs.

## Fixes

- Answering a prompt that asks for both the camera and the microphone no longer crashes your app.
- With DevTools open, a page that already agreed to close is no longer asked a second time.
- New Isolated Window now stays available while a window shows a sheet, as in Chrome.
- `wren bundle` now signs Chromium's web app shortcut copier with your bundle ID, as Chrome does.

## Known issues

- If your app is opened twice at almost the same moment, the second copy can sometimes start instead of handing over to the first.
- A few places still show Chromium's name or artwork: three error messages, some icons, and some translations.
- Chromium's crash handler keeps running for over a minute after your app quits.
- Signing with a real identity and provisioning profile hasn't been tested yet, and neither has a Sparkle update of an app signed with a Developer ID.
- A tab kept out of the session still comes back if it was in a tab group.
- Swiping back over a tab in a `<wren-view>` sometimes fails while the page has a non-passive `wheel` listener.
- A bundled app creates an empty `~/Library/Application Support/<name>` folder.
- A page's `<geolocation>` and `<usermedia>` elements show Chromium's own prompt instead of asking your app.
- No test covers a download blocked by policy, New Isolated Window under a sheet, or a renderer that never starts. Those changes were checked only by reading Chromium's code.

`docs/limitations.md` lists what Wren Runtime can't do yet.

## Versions

| Part                       | Version                               |
| -------------------------- | ------------------------------------- |
| Chromium                   | `158.0.8127.37`, patch revision `102` |
| `WREN_API_VERSION`         | `90`                                  |
| `WREN_UNSTABLE_UI_VERSION` | `5`                                   |
| macOS                      | 13 or later, Apple silicon            |
| Rust SDK                   | Rust `1.85` or later                  |

Newer Chrome releases include security fixes that this one doesn't. If your app shows pages from the open web, update to the next Wren Runtime release when it's available.

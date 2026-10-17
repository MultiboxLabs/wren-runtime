# Compatibility

What may change between releases of Wren, and which framework an app runs with.

## Each app carries its framework

An app is bundled with the Wren framework of the release it was compiled against
(`wren bundle`), and runs only with that one: `Wren_Main` returns
`WREN_EXIT_API_MISMATCH` before Chromium starts when `WrenStartup.api_version`
is not the framework's `WREN_API_VERSION`, or `WrenStartup.unstable_ui_version`
not its `WREN_UNSTABLE_UI_VERSION`. The app's `wren-app.json` names its release
(`"wren"`), which `wren install` links and `wren bundle` takes the framework
from, so the two cannot differ. Moving an app to a newer release is changing
`"wren"`, running `wren install`, and building and bundling again; the Swift API
and the Rust crate set both versions for you.

## The core and the unstable header

The C API is two parts.

- **The core:** `Wren.h` and every header it includes but `WrenUnstableUI.h`.
  Profiles, Browsers, tabs and navigation, anchors, page actions, menus,
  schemes, web views, the popover requests, and the facts and actions behind
  each popover (a site's security and permissions, zoom, an extension's access,
  the bookmark's folders, a content setting's value, the passwords prompts, a
  prompt's answers): enough to draw every popover without the unstable header.
  From 0.1.0 on, a minor release does not remove or change any of it: an app
  that compiles against 0.1's headers compiles against 0.2's. A release may add
  to it, and `WREN_API_VERSION` then changes, so the app is compiled again.
- **The unstable header,** `WrenUnstableUI.h`: what follows Chrome's bubbles,
  which Chrome redesigns. Chrome's sentences (`WrenUIText` and every
  `Wren_Copy…Text` call, in Chromium's language) and the layout of one bubble
  (the extensions menu's two designs, the bookmark bubble's folder list, the
  content setting bubble). Any release may change or remove any of it, minor
  ones included; `WREN_UNSTABLE_UI_VERSION` changes with it. An app that draws
  its own popovers from it should expect to change that code when it moves to a
  new release. One that leaves the popovers to Chromium does not use it.

These promises hold from 0.1.0, the first release.

## Chromium

Each release pins one Chromium version (`Wren_GetVersionInfo`). A page or an
extension sees that Chromium; what Chromium itself changes between versions is
Chromium's.

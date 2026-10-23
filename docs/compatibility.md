# Compatibility

This page says what may change between releases of Wren, and which framework an
app runs with.

## Each app carries its framework

`wren bundle` bundles an app with the Wren framework of the release it was
compiled against, and the app runs only with that framework. `Wren_Main` returns
`WREN_EXIT_API_MISMATCH` before Chromium starts if `WrenStartup.api_version` is
not the framework's `WREN_API_VERSION`, or `WrenStartup.unstable_ui_version` is
not its `WREN_UNSTABLE_UI_VERSION`.

The app's `wren-app.json` names its release in `"wren"`. `wren install` links
that release, and `wren bundle` takes the framework from it, so the two cannot
differ. To move an app to a newer release, change `"wren"`, run `wren install`,
and build and bundle again. The Swift API and the Rust crate set both versions
for you.

## The core and the unstable header

The C API is two parts.

- **The core** is `Wren.h` and every header it includes except
  `WrenUnstableUI.h`. It has Profiles, Browsers, tabs and navigation, anchors,
  page actions, menus, schemes, web views and the popover requests. It also has
  the facts and actions behind each popover, such as a site's security and
  permissions, zoom, an extension's access, the bookmark's folders, a content
  setting's value, the passwords prompts and a prompt's answers. That is enough
  to draw every popover without the unstable header.

  From 0.1.0 on, a minor release does not remove or change any of the core. An
  app that compiles against 0.1's headers compiles against 0.2's. A release may
  add to the core, and `WREN_API_VERSION` then changes, so the app is compiled
  again.

  A new field may go anywhere in a struct, not only at its end. Set fields by
  name, as the guides do (`.on_ready = OnReady`). C that fills a struct by
  position can still compile and set the wrong field.

- **The unstable header** is `WrenUnstableUI.h`. It has what follows Chrome's
  bubbles, which Chrome redesigns. That is Chrome's sentences, in Chromium's
  language, through `WrenUIText` and every `Wren_Copy…Text` call. It is also the
  layout of particular bubbles: the extensions menu's two designs, the bookmark
  bubble's folder list, and the content setting bubble.

  Any release may change or remove any of it, minor releases included, and
  `WREN_UNSTABLE_UI_VERSION` changes with it. An app that draws its own popovers
  from it should expect to change that code when it moves to a new release. An
  app that leaves the popovers to Chromium does not use it. In the Swift API,
  each function that wraps it says "Unstable." In the Rust crate, its types are
  in `wren::unstable_ui`.

These promises hold from 0.1.0, the first release, except for the changes in
[Breaking changes](#breaking-changes).

## Chromium

Each release pins one Chromium version, which `Wren_GetVersionInfo` reports. A
page or an extension sees that Chromium. What Chromium itself changes between
versions is Chromium's.

## Breaking changes

Each release lists here what it removed or changed in the core, and what an app
does about it. A release not listed has none.

### v0.2.0

**DevTools in a web view.** `WrenWebViewOptions.disable_devtools` is now
`disallow_user_devtools_activation`.

| Before                | Now                                 |
| --------------------- | ----------------------------------- |
| C: `disable_devtools` | `disallow_user_devtools_activation` |
| Swift: `devTools:`    | `disallowUserDevToolsActivation:`   |
| Rust: `devtools`      | `disallow_user_devtools_activation` |

It still leaves out Inspect. But `Wren_OpenWebViewDevTools` no longer refuses,
since only the app can call it. To move: use the new name.

**Hidden menu items.** `WrenMenuItem` has a new field, `hidden`, after
`checked`.

| Language | What changes                                                                                                         |
| -------- | -------------------------------------------------------------------------------------------------------------------- |
| C        | Code that sets fields by name needs no change. Code that fills the struct by position adds `hidden` after `checked`. |
| Swift    | Building `.item(id:label:…)` is unchanged. A pattern that lists `.item`'s values adds `hidden` before `key`.         |
| Rust     | Code that writes `MenuItem::Item` out in full adds `hidden: false`. `MenuItem::item(id, label)` is unchanged.        |

**The startup Profile.** `WrenStartup.profile_directory` is gone, from Swift's
`profileDirectory` and Rust's `profile_directory` too. Wren now starts on the
Profile Chrome would: the one whose Browser was active last, and `Default` at
the first launch.

- To move: delete the field. In C, `startup_profile_name` now has its place, so
  code that fills `WrenStartup` by position must be checked field by field.
- `Wren_OpenProfile(NULL)`, `WrenApp.openProfile()` and Rust's `open_profile`
  with `None` can now give a Profile other than `Default`. An app that always
  wants `Default` opens it by ID: `openProfile(id: "Default")`.
- A deleted startup Profile no longer comes back new at the next launch. The
  launch starts on a Profile that is left.

**Pages no longer see ⌘Q.** The Quit item's key now goes to the main menu before
a tab's or a web view's page sees it, as in Chrome, where pages never get it. A
page's `keydown` listener no longer hears it, and cannot stop the quit with
`preventDefault()`. This is the same in C, Swift and Rust. To move: a page that
listened for ⌘Q asks the app instead, through a [message](messages.md); an app
that wants to ask before quitting answers `quitPrompt` (see
[Menus](menus.md#asking-before-quitting)).

**One restore a launch.** `Wren_RestoreLastSession` restored the same last
session each time it was called, opening its windows again. Now a launch
restores a Profile's last session once, as Chrome does, so no two open tabs
share a [persistent ID](windows-and-tabs.md#finding-a-restored-tabs-records).

| Language | A second restore in one launch now                          |
| -------- | ----------------------------------------------------------- |
| C        | `Wren_RestoreLastSession`: `WREN_ERROR_NOT_ALLOWED`, 0 tabs |
| Swift    | `restoreLastSession`: `.failure(.notAllowed)`               |
| Rust     | `restore_last_session`: `Err(Error::NotAllowed)`            |

A call that failed for another reason restored nothing, and can be made again.
To move: restore once per launch. To show a restored window's pages again, open
new tabs at the URLs of its tabs (`tab.url`, Rust's `BrowserTab::url`). They
have new IDs and no history.

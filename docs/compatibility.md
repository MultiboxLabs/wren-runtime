# Compatibility

This page says what may change between releases of Wren, and which framework an app runs with.

## Each app carries its framework

`wren bundle` bundles an app with the Wren framework of the release it was compiled against, and the app runs only with that framework. `Wren_Main` returns `WREN_EXIT_API_MISMATCH` before Chromium starts if `WrenStartup.api_version` is not the framework's `WREN_API_VERSION`, or `WrenStartup.unstable_ui_version` is not its `WREN_UNSTABLE_UI_VERSION`.

The app's `wren-app.json` names its release in `"wren"`. `wren install` links that release, and `wren bundle` takes the framework from it, so the two cannot differ. To move an app to a newer release, change `"wren"`, run `wren install`, and build and bundle again. The Swift API and the Rust crate set both versions for you.

## The core and the unstable header

The C API is two parts.

- **The core** is `Wren.h` and every header it includes except `WrenUnstableUI.h`. It has Profiles, Browsers, tabs and navigation, anchors, page actions, menus, schemes, web views and the popover requests. It also has the facts and actions behind each popover, such as a site's security and permissions, zoom, an extension's access, the bookmark's folders, a content setting's value, the passwords prompts and a prompt's answers. That is enough to draw every popover without the unstable header.

  From 0.1.0 on, a minor release does not remove or change any of the core. An app that compiles against 0.1's headers compiles against 0.2's. A release may add to the core, and `WREN_API_VERSION` then changes, so the app is compiled again.

  A new field may go anywhere in a struct, not only at its end. Set fields by name, as the guides do (`.on_ready = OnReady`). C that fills a struct by position can still compile and set the wrong field.

- **The unstable header** is `WrenUnstableUI.h`. It has what follows Chrome's bubbles, which Chrome redesigns. That is Chrome's sentences, in Chromium's language, through `WrenUIText` and every `Wren_Copy…Text` call. It is also the layout of particular bubbles: the extensions menu's two designs, the bookmark bubble's folder list, and the content setting bubble.

  Any release may change or remove any of it, minor releases included, and `WREN_UNSTABLE_UI_VERSION` changes with it. An app that draws its own popovers from it should expect to change that code when it moves to a new release. An app that leaves the popovers to Chromium does not use it. In the Swift API, each function that wraps it says "Unstable." In the Rust crate, its types are in `wren::unstable_ui`.

These promises hold from 0.1.0, the first release, except for the changes in [Breaking changes](#breaking-changes).

## Chromium

Each release pins one Chromium version, which `Wren_GetVersionInfo` reports. A page or an extension sees that Chromium. What Chromium itself changes between versions is Chromium's.

## Breaking changes

Each release lists here what it removed or changed in the core, and what an app does about it. A release not listed has none.

### v0.3.0

**`Startup` in Rust has a new field, `native_hosts`,** the native messaging hosts the app answers. It is empty in `Startup::new`. An app that builds a `Startup` as a struct literal must name it, or end the literal with `..Startup::new(…)`. C's `WrenStartup` and Swift's `Startup` gain the field too, with nothing to change: a C startup with the new fields zeroed, and a Swift one that leaves the field out, declare no host.

To move: add `native_hosts: Vec::new()` to a `Startup` literal.

**`Startup` in Rust has a new field, `app_version`,** which Settings > About shows in place of Chromium's version, with Chromium's after it. It is `None` in `Startup::new`. An app that builds a `Startup` as a struct literal must name it, or end the literal with `..Startup::new(…)`. C's `WrenStartup` and Swift's `Startup` gain the field too, with nothing to change: left out, About shows Chromium's version alone.

To move: add `app_version: None` to a `Startup` literal.

**`SessionState` in Rust has a new field, `relaunched`.** It is true when the last session ended in a relaunch. An app that builds a `SessionState` itself, or matches one with every field named, no longer compiles. C's `WrenSessionState` and Swift's `SessionState` gain the field too, which breaks nothing there.

To move: add `relaunched` where a `SessionState` is built, or `..` where one is matched.

**`MainThread::later` in Rust runs through `Wren_PostToMainThread`.** It used GCD's main queue. Its signature is the same. C and Swift only gain the new call, so nothing changes there. In Rust two things change:

- A closure still waiting when the event loop ends is dropped unrun, on the main thread, before `wren::run` returns. Before, it stayed in GCD's queue and was never run or dropped.
- Closures run where Chromium lets tasks run, such as while a context menu is open. GCD also ran them inside a modal loop of the app's own, such as `NSAlert`'s `runModal`, and they no longer run there.

To move: nothing, unless a closure must run inside such a modal loop. Then the app hands it to GCD's main queue itself.

**Coming forward and staying back, as Chrome's windows do.** Wren now acts on three of Chromium's requests as Chrome does. Nothing in the API changed, and this is the same in C, Swift and Rust.

- When Chromium shows a Browser that is already shown, such as for a switch to its tab from another Browser or for Settings opened from an incognito window, Wren makes its window key. A Browser in the background gets a foreground request instead.
- Before Wren makes a Browser's window key, it activates the app. A hidden app, or one behind another app, comes forward.
- `chrome.windows.update` with `focused: false` no longer marks a Browser inactive while its window is key.

To move: an app that brings a Browser forward on every foreground request now also does so for the requests above. One that wants otherwise decides on each request, as before.

**What Chromium asks of a window.** Wren no longer reports what Chromium asked for as the window's state. Nothing in the API was removed, and this is the same in C, Swift and Rust.

- `chrome.windows.update`, DevTools' `Browser.setWindowBounds` and a popup's `resizeTo` and `moveTo` used to change what extensions and DevTools read, `chrome.windows.get` among them, while the window stayed where it was. They now read the window's real frame and state.
- A window the user minimises or zooms now reads minimised or maximised, and the session saves it minimised. A window in full screen is saved at the frame it had before.
- Search the Web in Chrome's menu now runs `on_focus_location_bar`.

To move: nothing, unless the app wants Chromium's requests carried out. Then it handles the new callback and reads the new getters:

| C | Swift | Rust |
| --- | --- | --- |
| `WrenAppDelegate.on_window_request` | `AppDelegate.windowRequest` | `App::window_request` |
| `Wren_GetRequestedWindowState` | `Browser.requestedWindowState` | `Browser::requested_window_state` |
| `Wren_GetLocationFocus` | `Browser.locationFocus` | `Browser::location_focus` |

An app with Chrome's menu reads `Wren_GetLocationFocus` in `on_focus_location_bar`, to tell Search the Web from a plain focus.

**Installed extensions belong to the Profile.** The six calls for a Profile's installed extensions take the Profile instead of a Browser, under the same names, and work with no window. An off-the-record Profile gives its original's. The Browser's methods are gone, with nothing left in their place.

| C | Swift | Rust |
| --- | --- | --- |
| Pass `Wren_GetBrowserProfile(browser)` where you passed `browser` | `browser.extensionIDs` becomes `browser.profile?.extensionIDs`, and so on | `browser.extension_ids()` becomes `browser.profile()`, then `.extension_ids()`, and so on |

The calls are `Wren_GetExtensionCount`, `Wren_CopyExtensionID`, `Wren_GetExtensionState`, `Wren_CopyExtensionName`, `Wren_CopyExtensionIconPNG` and `Wren_SetExtensionEnabled`. A C compiler warns at the old argument's type, and C++ refuses it.

**Changes to them are reported for the Profile.** `WREN_CHANGE_EXTENSIONS` (Swift `.extensions`, Rust `ChangeKind::Extensions`) names the Profile, once the app has read its extensions, whether or not the Profile has a Browser. `WREN_CHANGE_EXTENSION_ACTIONS` now reports actions only, and no longer an extension installed, removed, turned on or off, or its icon loading.

To move: read the list again on the new kind, for the Profile it names, not on `EXTENSION_ACTIONS`.

**`on_profile_extensions` takes a request.** It gives the Profile's ID and its kind, persistent or ephemeral, and one entry for each extension, with whether the Profile gets it and how it starts there.

| C | Swift | Rust |
| --- | --- | --- |
| `(void* context, WrenProfileExtensionsRequest* request)`. Read `request->profile_id`, and set `request->extensions[i].gets` | `{ request in … }`, with `request.profileID` and `request.extensions[i].gets` | `fn profile_extensions(&self, request: &mut ProfileExtensionsRequest)`, with `request.profile_id` and `request.extensions[i].gets` |

To move: an app that set `gets[i] = false` sets the entry's `gets` instead. An app that told its own ephemeral Profile apart by a flag can read the request's `kind`.

**`AppExtension` has a new field,** `allowed_in_incognito_on_install`. It is false unless set, as Chrome leaves Allow in Incognito to the user. In C and Swift nothing changes: a zeroed struct and Swift's default argument leave it false. In Rust, a struct literal of `AppExtension` must name it. `AppExtension::bundled` and `AppExtension::internal` are unchanged.

**Fill fills.** A double click on a drag area, with the title bar's action set to Fill in System Settings, now fills the window as Chrome's windows do. It used to zoom. The same in C, Swift and Rust. To move: nothing.

**`WebViewOptions` has a new field,** `wheel_phases`, which gives a view's own pages a trackpad scroll's phases on `wheel` events. It is off unless set, so no page changes. In C and Swift nothing changes: a zeroed struct and Swift's default argument leave it off. In Rust, a struct literal of `WebViewOptions` must name it, or end with `..Default::default()`.

**`WebViewOptions` has a new field,** `glass`, which lets a view's pages draw elements as Liquid Glass with `-wren-glass`. It is off unless set. In C and Swift nothing changes: a zeroed struct and Swift's default argument leave it off. In Rust, a struct literal of `WebViewOptions` must name it, or end with `..Default::default()`.

**`WebViewOptions` has a new field,** `drag_area_clicks`, which lets clicks in drag areas reach the page. It is off unless set, so drag areas behave as before. In C and Swift nothing changes: a zeroed struct and Swift's default argument leave it off. In Rust, a struct literal of `WebViewOptions` must name it, or end with `..Default::default()`.

**Rust's `show_toast` takes `ToastOptions`.** `Browser::show_toast` took the text and an icon. It now takes `&ToastOptions`, which also has the toast's name. In C, `WrenToastOptions` gains `name`, which a zeroed struct leaves NULL, and Swift's `showToast` gains `name:` with a default, so nothing changes there.

To move, in Rust:

```rust
browser.show_toast(&ToastOptions {
    text: "Address copied".into(),
    ..Default::default()
})?;
```

**A download's state has its times,** `start_ms` and `end_ms`, and `off_the_record`. In C and Swift nothing changes: the fields are new at the end of `WrenDownloadState`, and Swift's `DownloadState` has no public initialiser. In Rust, `DownloadState` gains `start_ms`, `end_ms` and `off_the_record`, so a struct literal or a pattern of it without `..` must name them.

**A download's actions include Remove.** `actions` now holds `WREN_DOWNLOAD_REMOVE` (Swift `.remove`, Rust `DownloadAction::REMOVE`) where chrome://downloads offers Remove from List: for a done, cancelled or failed download, where policy allows. So a finished download no longer offers only the bubble's actions, and a cancelled one no longer offers none. The same in C, Swift and Rust. To move: an app that shows the bubble's actions alone leaves this bit out.

**A download's progress is a change to it alone.** `WREN_CHANGE_DOWNLOADS` (Swift `.downloads`, Rust `ChangeKind::Downloads`) now reports only a change to a Profile's lists: a download added or gone, one entering, leaving or moving in a list, or the history loading. A download's progress or state is the new `WREN_CHANGE_DOWNLOAD` (Swift `.download`, Rust `ChangeKind::Download`), whose `download` names it and whose `profile` is the download's own. A change that moves a download in a list is both.

| C | Swift | Rust |
| --- | --- | --- |
| On `WREN_CHANGE_DOWNLOAD`, read `Wren_GetDownloadState(change->download)` | On `.download`, read `change.download?.state()` | On `ChangeKind::Download`, read `change.download`'s state |

To move: an app that showed progress on `DOWNLOADS` alone reads the one download on the new kind. In Rust, `Change` gains `download`, so a struct literal or a pattern of it without `..` must name it. A `switch` or `match` over the change kinds with no default must handle the new kind.

**A tab's state says when it was last on screen, what it is doing, whether it is muted and whether closing it may ask:** `last_active_ms`, `alerts`, `muted` and `may_ask_before_close`. In C and Swift nothing changes: the fields are new at the end of `WrenTabState`, and Swift's `TabState` has no public initialiser. In Rust, `TabState` gains `last_active_ms`, `alerts`, `muted` and `may_ask_before_close`, so a struct literal or a pattern of it without `..` must name them. The new `tab_fields` bits, `ALERTS` and `MUTED`, break nothing.

**A tab with no favicon has none.** Until Chromium has a page's favicon, and for a page without one, `Wren_CopyTabFaviconPNG` returned Chromium's default globe. It now returns 0, as its comment said it would. Swift's `faviconPNG` and Rust's `favicon_png` are nil and `None` then. This is the same in C, Swift and Rust.

To move: nothing, unless the app drew the globe it was given. Then it draws an icon of its own when there is no favicon.

**A tab's state says whether it is kept out of the session:** `session_excluded`, set by the new `Wren_SetTabSessionExcluded`. In C and Swift nothing changes: the field is new at the end of `WrenTabState`, and Swift's `TabState` has no public initialiser. In Rust, `TabState` gains `session_excluded`, so a struct literal or a pattern of it without `..` must name it. The new `tab_fields` bit, `SESSION_EXCLUDED`, breaks nothing.

**A new change kind reports the default browser:** `WREN_CHANGE_DEFAULT_BROWSER` (Swift `.defaultBrowser`, Rust `ChangeKind::DefaultBrowser`), once the app has asked with `Wren_GetDefaultBrowserState` or `Wren_SetAsDefaultBrowser`. It has no subject. To move: a `switch` or `match` over the change kinds with no default must handle the new kind; an app that never asks never gets it.

**`wren bundle` checks more of `wren-app.json`.** It refuses a manifest whose `usageDescriptions` sets a key that does not end in `UsageDescription`, or gives a value that is not a string. Before, such a key went into `Info.plist` as it was, and could replace one `wren bundle` writes, such as the bundle ID. `infoPlist` may no longer set `UTImportedTypeDeclarations` or `UTExportedTypeDeclarations`, which the new fields `importedTypes` and `exportedTypes` write.

To move: put any other key in `infoPlist`, and type declarations in `importedTypes` and `exportedTypes`.

### v0.2.0

**DevTools in a web view.** `WrenWebViewOptions.disable_devtools` is now `disallow_user_devtools_activation`.

| Before                | Now                                 |
| --------------------- | ----------------------------------- |
| C: `disable_devtools` | `disallow_user_devtools_activation` |
| Swift: `devTools:`    | `disallowUserDevToolsActivation:`   |
| Rust: `devtools`      | `disallow_user_devtools_activation` |

It still leaves out Inspect. But `Wren_OpenWebViewDevTools` no longer refuses, since only the app can call it. To move: use the new name.

**Hidden menu items.** `WrenMenuItem` has a new field, `hidden`, after `checked`.

| Language | What changes |
| --- | --- |
| C | Code that sets fields by name needs no change. Code that fills the struct by position adds `hidden` after `checked`. |
| Swift | Building `.item(id:label:…)` is unchanged. A pattern that lists `.item`'s values adds `hidden` before `key`. |
| Rust | Code that writes `MenuItem::Item` out in full adds `hidden: false`. `MenuItem::item(id, label)` is unchanged. |

**The startup Profile.** `WrenStartup.profile_directory` is gone, from Swift's `profileDirectory` and Rust's `profile_directory` too. Wren now starts on the Profile Chrome would: the one whose Browser was active last, and `Default` at the first launch.

- To move: delete the field. In C, `startup_profile_name` now has its place, so code that fills `WrenStartup` by position must be checked field by field.
- `Wren_OpenProfile(NULL)`, `WrenApp.openProfile()` and Rust's `open_profile` with `None` can now give a Profile other than `Default`. An app that always wants `Default` opens it by ID: `openProfile(id: "Default")`.
- A deleted startup Profile no longer comes back new at the next launch. The launch starts on a Profile that is left.

**Pages no longer see ⌘Q.** The Quit item's key now goes to the main menu before a tab's or a web view's page sees it, as in Chrome, where pages never get it. A page's `keydown` listener no longer hears it, and cannot stop the quit with `preventDefault()`. This is the same in C, Swift and Rust. To move: a page that listened for ⌘Q asks the app instead, through a [message](messages.md); an app that wants to ask before quitting answers `quitPrompt` (see [Menus](menus.md#asking-before-quitting)).

**One restore a launch.** `Wren_RestoreLastSession` restored the same last session each time it was called, opening its windows again. Now a launch restores a Profile's last session once, as Chrome does, so no two open tabs share a [persistent ID](windows-and-tabs.md#finding-a-restored-tabs-records).

| Language | A second restore in one launch now                          |
| -------- | ----------------------------------------------------------- |
| C        | `Wren_RestoreLastSession`: `WREN_ERROR_NOT_ALLOWED`, 0 tabs |
| Swift    | `restoreLastSession`: `.failure(.notAllowed)`               |
| Rust     | `restore_last_session`: `Err(Error::NotAllowed)`            |

A call that failed for another reason restored nothing, and can be made again. To move: restore once per launch. To show a restored window's pages again, open new tabs at the URLs of its tabs (`tab.url`, Rust's `BrowserTab::url`). They have new IDs and no history.

# Compatibility

When you move your app to a newer Wren Runtime release, this page tells you what can change and what you need to do. It explains which framework your app runs with, what each release promises to keep, and every change so far that needs work in an app, with how to move across it in C, Swift and Rust.

## Your app carries its framework

`wren bundle` puts the framework of the release your app was built against into your app, and your app runs only with that framework. Before Chromium starts, `Wren_Main` checks that `WrenStartup.api_version` is the framework's `WREN_API_VERSION`, and `WrenStartup.unstable_ui_version` its `WREN_UNSTABLE_UI_VERSION`. If either differs, it returns `WREN_EXIT_API_MISMATCH`. The Swift SDK and the Rust SDK set both versions for you.

Your `wren-app.json` names the release in `"wren"`. `wren install` links that release, and `wren bundle` takes the framework from it, so the two can't differ. To move to a newer release, change `"wren"`, run `wren install`, and build and bundle again.

## What each release keeps

The C API has two parts, with different promises, and the core holds an experimental part that has none.

**The core** is `Wren.h` and every header it includes except `WrenUnstableUI.h`. It has Profiles, Browsers, tabs and navigation, anchors, page actions, menus, schemes, web views and popover requests. It also has the data and actions behind each popover, such as a site's security and permissions, zoom, an extension's access, the bookmark's folders, a site setting's value, the password prompts, and a prompt's answers. That's enough to draw every popover without the unstable header.

From 0.1.0 on, a minor release doesn't remove or change anything in the core, apart from the changes listed under [Breaking changes](#breaking-changes). An app that compiles against one release's headers compiles against the next. A release can add to the core, and `WREN_API_VERSION` then changes, so you build your app again.

A new field can go anywhere in a struct, not only at its end. Set fields by name in C, as the guides do, such as `.on_ready = OnReady`. C that fills a struct by position may still compile, and set the wrong field.

**The unstable header** is `WrenUnstableUI.h`. It has what follows the design of Chrome's bubbles, which Chrome redesigns from time to time. That's Chrome's own sentences, in Chromium's language, through `WrenUIText` and every `Wren_Copy…UIText` call that takes it, four lists' labels, and the layout of some bubbles: the extensions menu's two designs, the bookmark bubble's folder list, and the site setting bubble.

Any release can change or remove any of it, minor releases included, and `WREN_UNSTABLE_UI_VERSION` changes with it. If your app draws its own popovers from it, expect to change that code when you move to a new release. If you leave the popovers to Chromium, you don't use it. In the Swift SDK, each function that wraps it says "Unstable." In the Rust SDK, its types are in `wren::unstable_ui`.

**The experimental part** is Liquid Glass: a web view's `glass` option, `Wren_CanDrawGlass`, and the CSS properties `-wren-glass` and `-wren-glass-tint`. It's in the core's headers, but any release can change or remove it, minor releases included, with no forwarder. When one does, `WREN_API_VERSION` changes, as it does for any change to the core, and [Breaking changes](#breaking-changes) says how to move. Each of its comments starts "Experimental.", in the headers and in both SDKs. If your app uses glass, give each element with glass a fallback, so your UI still works the day it goes.

## Chromium

Each release uses one Chromium version, which `Wren_GetVersionInfo` reports. Pages and extensions see that Chromium, and what Chromium changes from one version to the next is Chromium's own.

## Breaking changes

Each release lists what it removed or changed in the core, and how to move your app. A release that isn't listed has none.

### v0.5.0

**Certificate Transparency is enforced.** Chromium now requires a certificate from a public root to be logged, as Chrome does, and a page whose certificate isn't fails with `ERR_CERTIFICATE_TRANSPARENCY_REQUIRED`. Apps in C, Swift and Rust compile as before. To move: nothing. If your app must turn it off, pass `--disable-features=CertificateTransparencyAskBeforeEnabling` in the startup's development switches.

**`Startup` has two new fields, `channel` and `google_api_key`.** They choose the channel Chromium reports and give Safe Browsing your own Google API key. Left as they are, the channel is unknown, as before, and no key is set. In C and Swift nothing changes, because a zeroed struct and Swift's defaults leave them so. In Rust, a struct literal of `Startup` must name both. `Startup::new` sets them to `Channel::Unknown` and `None`, so building it with `Startup::new` and setting fields after compiles as before.

**Liquid Glass is experimental.** A web view's `glass` option and the CSS properties `-wren-glass` and `-wren-glass-tint` no longer have the core's promise, and neither does the new `Wren_CanDrawGlass`: any release can change or remove them. Nothing about them changes in 0.5.0, so C, Swift and Rust apps compile and draw as before. To move: nothing now. To make a web view with glass only where it draws, pass `glass` the call's answer, `WebView.canDrawGlass` in Swift and `WebView::can_draw_glass()` in Rust.

**One call copies a subject's strings, with a kind.** When a tab, a download, a popover request or another subject has more than one string, one call now copies them all, and an enum says which. 31 copy functions are removed for 10 new ones, with no forwarders, and every string reads as before. A kind that doesn't apply, or a value outside the enum, copies "" and returns 1.

In C, call the new function with the kind, in place of the old function:

| Removed | Now |
| --- | --- |
| `Wren_CopyTabTitle`, `Wren_CopyTabURL`, `Wren_CopyTabStatusText`, `Wren_CopyTabPersistentID` | `Wren_CopyTabText(tab, kind, …)`, with `WREN_TAB_TEXT_TITLE`, `_URL`, `_STATUS` and `_PERSISTENT_ID` |
| `Wren_CopyTabMediaText` and `WrenMediaText` | `Wren_CopyTabText`, with `WREN_TAB_TEXT_MEDIA_TITLE`, `_MEDIA_ARTIST`, `_MEDIA_ALBUM` and `_MEDIA_SOURCE` |
| `Wren_CopyDownloadFileName`, `Wren_CopyDownloadPath`, `Wren_CopyDownloadURL` | `Wren_CopyDownloadText(download, kind, …)`, with `WREN_DOWNLOAD_TEXT_FILE_NAME`, `_PATH` and `_URL` |
| `Wren_CopyExtensionActionName`, `…Tooltip`, `…Badge` | `Wren_CopyExtensionActionText(browser, extension_id, kind, …)` |
| `Wren_CopyPageActionLabel`, `…Tooltip` | `Wren_CopyPageActionText(browser, action, kind, …)` |
| `Wren_CopySchemeRequestURL`, `…Method` | `Wren_CopySchemeRequestText(request, kind, …)` |
| `Wren_CopyPageCallName`, `…Value` | `Wren_CopyPageCallText(call, kind, …)` |
| `Wren_CopySidePanelID`, `…Title` | `Wren_CopySidePanelText(browser, kind, …)` |
| `Wren_CopyBookmarkItemTitle`, `…URL` | `Wren_CopyBookmarkItemText(profile, item, kind, …)` |
| `Wren_CopyProfileListID`, `…Name` | `Wren_CopyProfileListText(index, kind, …)` |
| `Wren_CopyPasswordsUsername`, `Wren_CopySavedUsername`, `Wren_CopyPassword`, `Wren_CopyCardLastDigits`, `Wren_CopyCardName`, `Wren_CopyPermissionID`, `Wren_CopyToastName` | `Wren_CopyPopoverText(popover, index, kind, …)`, with `WREN_POPOVER_TEXT_USERNAME`, `_SAVED_USERNAME`, `_PASSWORD`, `_CARD_LAST_DIGITS`, `_CARD_NAME`, `_PERMISSION_ID` and `_TOAST_NAME`. Pass 0 as the index for a kind without a list. |
| `Wren_CopyTranslateLanguageCode(popover, target, index, …)` | `Wren_CopyPopoverText(popover, index, kind, …)`, with `WREN_POPOVER_TEXT_TRANSLATE_SOURCE_CODE` or `_TRANSLATE_TARGET_CODE` |

In Swift, `MediaText` and `tab.mediaText(_:)` are removed. Use `tab.text(.mediaTitle)`, `.mediaArtist`, `.mediaAlbum` and `.mediaSource`. Every other property and method keeps its name, and each subject gains a method that takes the new enum, such as `tab.text(_:)` and `download.text(_:)`.

In Rust, `MediaText` and `BrowserTab::media_text` are removed. Use `tab.text(TabText::MediaTitle)` and the other media kinds. Every other method keeps its name, and each subject gains a method that takes the new enum, such as `BrowserTab::text` and `Download::text`.

**Opening the startup Profile after deleting it.** Once the startup Profile was deleted, `Wren_OpenProfile(NULL)` used to fail with `WREN_ERROR_PROFILE_UNAVAILABLE` until the app quit. It now opens the persistent Profile Chromium moves to, as Chrome does, which is the startup Profile from then on. Apps in C, Swift and Rust compile as before. If yours handled the failure, for example by choosing a Profile from the list itself, you can remove that code. In Swift the call is `WrenApp.openProfile()`, and in Rust `MainThread::open_profile` with `None`.

**`WrenUpdates.h` is now part of `WrenApp.h`.** The new `WrenApp.h` has quitting, relaunching, your updates and being the default browser. It has everything `WrenUpdates.h` had, and `WrenUpdates.h` is removed. `Wren_Quit`, Warn Before Quitting, the quit prompt and the default browser move there from `Wren.h`. Chromium's prompts and toasts move from `WrenUI.h` to the new `WrenPrompts.h`. Nothing is renamed, and `Wren.h` still includes every header. A C app that includes `WrenUpdates.h` by name includes `Wren.h` or `WrenApp.h` instead. Every other C app compiles as before, and so do apps in Swift and Rust.

**A quit while the last session is being restored.** A quit asked while `Wren_RestoreLastSession` was still reading the session used to leave the app running for good, with no window. The quit now waits for the restore. Its callback runs and its Browsers open, then the quit goes on and closes them. Apps in C, Swift and Rust compile as before. In Swift the call is `profile.restoreLastSession(_:)`, and in Rust `Profile::restore_last_session`.

The unstable calls that take a `WrenUIText` are now named `Wren_Copy…UIText`, such as `Wren_CopyPopoverUIText`, and `WREN_UNSTABLE_UI_VERSION` is 5. In Swift, the unstable `text(_:)` of a `Popover`, a `Download` and a `DownloadsPrompt` is now `uiText(_:)`, and in Rust, `ui_text`. As with anything in the unstable header, this isn't a breaking change under the promise above.

### v0.4.0

**The active tab counts as in use.** A tab's `last_active_ms` is now the current time while the tab is its Browser's active tab, whether or not it's on screen, such as in a minimised, covered or background window, or on another Space. Chrome never counts the active tab as idle. Any other tab reads when it was hidden or stopped being active, whichever came later. Before, the active tab of a window that wasn't on screen read the time it was hidden. The field is `last_active_ms` in C and Rust, and `lastActiveMs` in Swift.

To move: an app that closes or unloads idle tabs needs no change, and now leaves every window's active tab alone. An app that used the field to find tabs in windows that aren't on screen reads its own windows' state instead.

**A new window's opener comes from Chromium's own record.** `opener` in `on_browser_opened` is now the Browser of the tab that opened the new window's first tab, from Chromium's record of the open, not from the page's `window.opener`. So the window of a noopener link or popup, a Shift-click's window, and Open Link in New Window's window now name their opener, where before they gave none. The argument stays `opener` in C, `opener: Browser?` in Swift's `browserOpened`, and `opener: Option<Browser>` in Rust's `App::browser_opened`. Only its meaning changes.

To move: an app that took an opener to mean the page can script its new window reads the page's own state instead. An app that placed windows by their opener needs no change.

**A tab's state says whether it waits to load, whether it ignores input, and which tab opened it.** The new fields are `unloaded`, `ignores_input`, `opened_by` and `opened_as`, with the new enum `WrenTabOpenedAs`. In C and Swift nothing changes, because the fields are new at the end of `WrenTabState`, and Swift's `TabState` has no public initialiser. In Rust, `TabState` gains the four fields, so a struct literal or pattern of it without `..` must name them. The new `tab_fields` bit, `IGNORES_INPUT`, breaks nothing.

**A tab's state says whether it watches its navigations, and which request opened it.** The new fields are `watches_navigations` and `opened_for_navigation`, with `on_navigation` and `Wren_SetTabWatchesNavigations`. In C and Swift nothing changes. The fields are new at the end of `WrenTabState`, and the new delegate member is last in `WrenAppDelegate`, so an app that zero-fills the delegate, or uses Swift's `AppDelegate`, leaves it NULL. In Rust, `TabState` gains `watches_navigations: bool` and `opened_for_navigation: Option<NavigationID>`, so a struct literal or pattern of it without `..` must name them. `App::navigation` has a default that allows every navigation.

**A tab's state says what its media is doing.** The new field is `media`, a `WrenTabMedia`, with `Wren_CopyTabMediaText`, `Wren_CopyTabMediaArtworkPNG`, `Wren_ControlTabMedia` and `Wren_SeekTabMedia`. In C and Swift nothing changes, because the field is new at the end of `WrenTabState`, and Swift's `TabState` has no public initialiser. In Rust, `TabState` gains `media: TabMedia`, so a struct literal or pattern of it without `..` must name it. The new `tab_fields` bits, `MEDIA` and `MEDIA_METADATA`, break nothing.

**Pinning, moving and site access requests take the Profile.** They change the Profile's settings, so they now take the Profile, and work with no window. An incognito Profile now refuses to change site access requests with `WREN_ERROR_NOT_ALLOWED`, as it already refused pins. Pinning or moving an extension that's off gives `WREN_ERROR_NOT_ALLOWED`, where before it gave `WREN_ERROR_INVALID_ARGUMENT` for having no button, and an ID that isn't installed gives `WREN_ERROR_INVALID_ARGUMENT`. The Browser's calls are gone, with nothing in their place.

| C | Swift | Rust |
| --- | --- | --- |
| `Wren_SetExtensionActionPinned(browser, …)` becomes `Wren_SetExtensionPinned(Wren_GetBrowserProfile(browser), …)` | `browser.setExtensionActionPinned` becomes `browser.profile?.setExtensionPinned` | `Browser::set_extension_action_pinned` becomes `Profile::set_extension_pinned` on `browser.profile()` |
| `Wren_MoveExtensionAction(browser, …)` becomes `Wren_MoveExtension(Wren_GetBrowserProfile(browser), …)` | `browser.moveExtensionAction(_:to:)` becomes `browser.profile?.moveExtension(_:to:)` | `Browser::move_extension_action` becomes `Profile::move_extension` |
| `Wren_SetExtensionShowsRequests` takes `Wren_GetBrowserProfile(browser)` instead of `browser` | `browser.setExtensionShowsRequests` becomes `browser.profile?.setExtensionShowsRequests` | `Browser::set_extension_shows_requests` becomes `Profile::set_extension_shows_requests` |

`WrenExtensionState` gains `shows_requests` at its end, so C and Swift need no other change. In Rust, `ExtensionState` gains `shows_requests: bool`, so a struct literal or pattern of it without `..` must name it. A C compiler warns about the old argument type of `Wren_SetExtensionShowsRequests`, and C++ refuses it.

**Each extension in `on_profile_extensions`' request names its ID.** `WrenProfileExtension` gains `id`, as its first field. In C, an app that reads the entries needs no change, but one that builds a `WrenProfileExtension` with positional initialisers now gives its first value to `id`, which Wren Runtime ignores. Swift's `ProfileExtension` gains `id` and has no public initialiser, so nothing breaks. In Rust, `ProfileExtension` gains `id: String`, and is no longer `Copy`.

To move: in Rust, clone an entry where your code copied one, and name `id` in a struct literal or pattern without `..`. In C, use designated initialisers.

**`Wren_CloseBrowsers` replaces `Wren_CloseBrowser`, and says how the close ended.** It takes several Browsers and closes all of them or none, as quitting does. Each page's `beforeunload` asks first, then the prompt about downloads in an incognito Profile the close would leave without a Browser. Its required callback gets `WREN_OK` after `on_browser_closed` for each Browser, or `WREN_ERROR_CANCELLED` when a page or the downloads prompt kept them open, or a quit started meanwhile. A Browser already in a close request, one whose Profile is being deleted, and any during a quit are refused with `WREN_ERROR_NOT_ALLOWED`.

In Swift, `browser.close() throws` becomes `browser.close(_ completion:)`, beside `Browser.close(_:completion:)` for several. In Rust, `Browser::close(&self) -> Result<(), Error>` becomes `close(&self, completion)`, beside `Browser::close_all(&[Browser], completion)`. Neither throws or returns a result.

`Wren_DeleteProfile` now also refuses, with `WREN_ERROR_NOT_ALLOWED`, a Profile with a Browser in a close request, and any Profile during a quit. Before, a quit while a deletion was asking left the deletion unanswered.

To move: in C, call `Wren_CloseBrowsers(&browser, 1, callback, context)`, and close your window in the callback on `WREN_OK`, rather than before asking. In Swift and Rust, pass a completion, or `{ _ in }` and `|_| {}` to keep the old behaviour. An app that closed a window of several Browsers one at a time closes them in one call, so choosing to stay keeps them all.

### v0.3.0

**`Startup` in Rust has a new field, `native_hosts`,** the native messaging hosts your app answers. `Startup::new` leaves it empty. C's `WrenStartup` and Swift's `Startup` gain the field too, with nothing to change, because a zeroed field in C, or leaving it out in Swift, declares no host.

To move: add `native_hosts: Vec::new()` to a `Startup` struct literal, or end the literal with `..Startup::new(…)`.

**`Startup` in Rust has a new field, `app_version`,** which Settings › About shows instead of Chromium's version, with Chromium's after it. `Startup::new` leaves it `None`. C's `WrenStartup` and Swift's `Startup` gain it too, with nothing to change, because About shows Chromium's version alone when it's left out.

To move: add `app_version: None` to a `Startup` struct literal, or end the literal with `..Startup::new(…)`.

**`SessionState` in Rust has a new field, `relaunched`,** which is true when the last session ended in a relaunch. An app that builds a `SessionState` itself, or matches one with every field named, no longer compiles. C's `WrenSessionState` and Swift's `SessionState` gain the field too, which breaks nothing there.

To move: add `relaunched` where you build a `SessionState`, or `..` where you match one.

**`MainThread::later` in Rust runs through `Wren_PostToMainThread`,** instead of GCD's main queue. Its signature is the same, and C and Swift only gain the new call, so nothing changes there. In Rust, two things change:

- A closure still waiting when the event loop ends is dropped without running, on the main thread, before `wren::run` returns. Before, it stayed in GCD's queue and was never run or dropped.
- Closures run wherever Chromium lets tasks run, such as while a context menu is open. GCD also ran them inside a modal loop of your own, such as `NSAlert`'s `runModal`, and they no longer run there.

To move: nothing, unless a closure has to run inside such a modal loop. Then hand it to GCD's main queue yourself.

**Windows come forward and stay back as Chrome's do.** Wren Runtime now acts on three of Chromium's requests as Chrome does. Nothing in the API changed, and this is the same in C, Swift and Rust.

- When Chromium shows a Browser that's already shown, such as when someone switches to its tab from another Browser, or opens Settings from an incognito window, Wren Runtime makes its window key. A Browser in the background gets a foreground request instead.
- Before Wren Runtime makes a Browser's window key, it activates your app, so a hidden app, or one behind another app, comes forward.
- `chrome.windows.update` with `focused: false` no longer marks a Browser inactive while its window is key.

To move: an app that brings a Browser forward on every foreground request now does so for the requests above too. To do otherwise, decide on each request, as before.

**What Chromium asks of a window is no longer reported as the window's state.** Nothing in the API was removed, and this is the same in C, Swift and Rust.

- `chrome.windows.update`, DevTools' `Browser.setWindowBounds`, and a popup's `resizeTo` and `moveTo` used to change what extensions and DevTools read, including `chrome.windows.get`, while the window stayed where it was. They now read the window's real frame and state.
- A window someone minimises or zooms now reads as minimised or maximised, and the session saves it minimised. A window in full screen is saved at the frame it had before.
- Search the Web in Chrome's menu now calls `on_focus_location_bar`.

To move: nothing, unless you want Chromium's requests carried out. Then handle the new callback, and read the new values:

| C | Swift | Rust |
| --- | --- | --- |
| `WrenAppDelegate.on_window_request` | `AppDelegate.windowRequest` | `App::window_request` |
| `Wren_GetRequestedWindowState` | `Browser.requestedWindowState` | `Browser::requested_window_state` |
| `Wren_GetLocationFocus` | `Browser.locationFocus` | `Browser::location_focus` |

An app with Chrome's menu reads `Wren_GetLocationFocus` in `on_focus_location_bar`, to tell Search the Web from a plain focus.

**Installed extensions belong to the Profile.** The six calls for a Profile's installed extensions take the Profile instead of a Browser, with the same names, and work without a window. An incognito Profile gives its original's. The Browser's methods are gone, with nothing in their place.

| C | Swift | Rust |
| --- | --- | --- |
| Pass `Wren_GetBrowserProfile(browser)` where you passed `browser` | `browser.extensionIDs` becomes `browser.profile?.extensionIDs`, and so on | `browser.extension_ids()` becomes `.extension_ids()` on `browser.profile()`, and so on |

The calls are `Wren_GetExtensionCount`, `Wren_CopyExtensionID`, `Wren_GetExtensionState`, `Wren_CopyExtensionName`, `Wren_CopyExtensionIconPNG` and `Wren_SetExtensionEnabled`. A C compiler warns about the old argument type, and C++ refuses it.

**Changes to installed extensions are reported for the Profile.** `WREN_CHANGE_EXTENSIONS`, which is `.extensions` in Swift and `ChangeKind::Extensions` in Rust, names the Profile, once your app has read its extensions, whether or not it has a Browser. `WREN_CHANGE_EXTENSION_ACTIONS` now reports only extensions' buttons, and no longer an extension installed, removed, turned on or off, or its icon loading.

To move: read the list again on the new kind, for the Profile it names, not on `EXTENSION_ACTIONS`.

**`on_profile_extensions` takes a request.** It gives the Profile's ID and its kind, persistent or ephemeral, and an entry for each extension, with whether the Profile gets it and how it starts there.

| C | Swift | Rust |
| --- | --- | --- |
| `(void* context, WrenProfileExtensionsRequest* request)`. Read `request->profile_id`, and set `request->extensions[i].gets`. | `{ request in … }`, with `request.profileID` and `request.extensions[i].gets` | `fn profile_extensions(&self, request: &mut ProfileExtensionsRequest)`, with `request.profile_id` and `request.extensions[i].gets` |

To move: where you set `gets[i] = false`, set the entry's `gets` instead. An app that told its own ephemeral Profile apart by a flag can read the request's `kind`.

**`AppExtension` has a new field, `allowed_in_incognito_on_install`.** It's false unless set, as Chrome leaves Allow in Incognito to the person. In C and Swift nothing changes, because a zeroed struct and Swift's default argument leave it false. In Rust, a struct literal of `AppExtension` must name it. `AppExtension::bundled` and `AppExtension::internal` are unchanged.

**Fill fills.** With the title bar's double-click action set to Fill in System Settings, a double click on a drag area now fills the window, as in Chrome's windows. It used to zoom. This is the same in C, Swift and Rust, and needs no change.

**`WebViewOptions` has a new field, `wheel_phases`,** which gives a web view's own pages a trackpad scroll's phases on `wheel` events. It's off unless set, so no page changes. In C and Swift nothing changes, because a zeroed struct and Swift's default argument leave it off. In Rust, a struct literal of `WebViewOptions` must name it, or end with `..Default::default()`.

**`WebViewOptions` has a new field, `glass`,** which lets a web view's pages draw elements as Liquid Glass with `-wren-glass`. It's off unless set. In C and Swift nothing changes, for the same reason. In Rust, a struct literal of `WebViewOptions` must name it, or end with `..Default::default()`.

**`WebViewOptions` has a new field, `drag_area_clicks`,** which lets clicks in drag areas reach the page. It's off unless set, so drag areas behave as before. In C and Swift nothing changes. In Rust, a struct literal of `WebViewOptions` must name it, or end with `..Default::default()`.

**Rust's `show_toast` takes `ToastOptions`.** `Browser::show_toast` took the text and an icon, and now takes `&ToastOptions`, which also has the toast's name. In C, `WrenToastOptions` gains `name`, which a zeroed struct leaves NULL, and Swift's `showToast` gains `name:` with a default, so nothing changes there.

To move, in Rust:

```rust
browser.show_toast(&ToastOptions {
    text: "Address copied".into(),
    ..Default::default()
})?;
```

**A download's state has its times, and whether it's incognito.** The new fields are `start_ms`, `end_ms` and `off_the_record`. In C and Swift nothing changes, because the fields are new at the end of `WrenDownloadState`, and Swift's `DownloadState` has no public initialiser. In Rust, `DownloadState` gains the three fields, so a struct literal or pattern of it without `..` must name them.

**A download's actions include Remove.** `actions` now has `WREN_DOWNLOAD_REMOVE`, which is `.remove` in Swift and `DownloadAction::REMOVE` in Rust, wherever chrome://downloads offers Remove from List: for a download that's done, cancelled or failed, where policy allows it. So a finished download no longer offers only the download bubble's actions, and a cancelled one no longer offers none. This is the same in C, Swift and Rust.

To move: an app that shows only the bubble's actions leaves this one out.

**A download's progress is a change to that download.** `WREN_CHANGE_DOWNLOADS`, which is `.downloads` in Swift and `ChangeKind::Downloads` in Rust, now reports only changes to a Profile's lists: a download added or gone, one entering, leaving or moving in a list, or the history loading. A download's progress or state is the new `WREN_CHANGE_DOWNLOAD`, `.download` in Swift and `ChangeKind::Download` in Rust, whose `download` names the download and whose `profile` is its own. A change that moves a download in a list is both.

| C | Swift | Rust |
| --- | --- | --- |
| On `WREN_CHANGE_DOWNLOAD`, read `Wren_GetDownloadState(change->download)` | On `.download`, read `change.download?.state()` | On `ChangeKind::Download`, read `change.download`'s state |

To move: an app that showed progress on `DOWNLOADS` alone reads the one download on the new kind. In Rust, `Change` gains `download`, so a struct literal or pattern of it without `..` must name it. A `switch` or `match` over the change kinds without a default must handle the new kind.

**A tab's state says when it was last on screen, what it's doing, whether it's muted, and whether closing it may ask.** The new fields are `last_active_ms`, `alerts`, `muted` and `may_ask_before_close`. In C and Swift nothing changes, because the fields are new at the end of `WrenTabState`, and Swift's `TabState` has no public initialiser. In Rust, `TabState` gains the four fields, so a struct literal or pattern of it without `..` must name them. The new `tab_fields` bits, `ALERTS` and `MUTED`, break nothing.

**A tab without a favicon has none.** Until Chromium has a page's favicon, and for a page without one, `Wren_CopyTabFaviconPNG` used to return Chromium's default globe. It now returns 0, as its comment said it would, and Swift's `faviconPNG` and Rust's `favicon_png` are nil and `None`. This is the same in C, Swift and Rust.

To move: nothing, unless your app drew the globe it was given. Then draw an icon of your own when there's no favicon.

**A tab's state says whether it's kept out of the session.** The new field is `session_excluded`, set by the new `Wren_SetTabSessionExcluded`. In C and Swift nothing changes, because the field is new at the end of `WrenTabState`, and Swift's `TabState` has no public initialiser. In Rust, `TabState` gains `session_excluded`, so a struct literal or pattern of it without `..` must name it. The new `tab_fields` bit, `SESSION_EXCLUDED`, breaks nothing.

**A new change kind reports the default browser.** `WREN_CHANGE_DEFAULT_BROWSER`, which is `.defaultBrowser` in Swift and `ChangeKind::DefaultBrowser` in Rust, arrives once your app has asked with `Wren_GetDefaultBrowserState` or `Wren_SetAsDefaultBrowser`. It names nothing.

To move: a `switch` or `match` over the change kinds without a default must handle the new kind. An app that never asks never gets it.

**`wren bundle` checks more of `wren-app.json`.** It refuses a manifest whose `usageDescriptions` has a key that doesn't end in `UsageDescription`, or a value that isn't a string. Before, such a key went into `Info.plist` as it was, and could replace one `wren bundle` writes, such as the bundle ID. `infoPlist` can no longer set `UTImportedTypeDeclarations` or `UTExportedTypeDeclarations`, which the new fields `importedTypes` and `exportedTypes` write.

To move: put any other key in `infoPlist`, and type declarations in `importedTypes` and `exportedTypes`.

### v0.2.0

**DevTools in a web view.** `WrenWebViewOptions.disable_devtools` is now `disallow_user_devtools_activation`.

| Language | Before             | Now                                 |
| -------- | ------------------ | ----------------------------------- |
| C        | `disable_devtools` | `disallow_user_devtools_activation` |
| Swift    | `devTools:`        | `disallowUserDevToolsActivation:`   |
| Rust     | `devtools`         | `disallow_user_devtools_activation` |

It still leaves out Inspect, but `Wren_OpenWebViewDevTools` no longer refuses, because only your app can call it.

To move: use the new name.

**Hidden menu items.** `WrenMenuItem` has a new field, `hidden`, after `checked`.

| Language | What changes |
| --- | --- |
| C | Code that sets fields by name needs no change. Code that fills the struct by position adds `hidden` after `checked`. |
| Swift | Building `.item(id:label:…)` is unchanged. A pattern that lists `.item`'s values adds `hidden` before `key`. |
| Rust | Code that writes out `MenuItem::Item` in full adds `hidden: false`. `MenuItem::item(id, label)` is unchanged. |

**The startup Profile.** `WrenStartup.profile_directory` is gone, along with Swift's `profileDirectory` and Rust's `profile_directory`. Wren Runtime now starts on the Profile Chrome would: the one whose Browser was active last, or `Default` at the first launch.

- To move, delete the field. In C, `startup_profile_name` now has its place, so check code that fills `WrenStartup` by position field by field.
- `Wren_OpenProfile(NULL)`, `WrenApp.openProfile()` and Rust's `open_profile` with `None` can now give a Profile other than `Default`. An app that always wants `Default` opens it by ID, as `openProfile(id: "Default")`.
- A deleted startup Profile no longer comes back new at the next launch, which starts on one of the Profiles that are left.

**Pages no longer see ⌘Q.** The Quit item's key now goes to the main menu before a tab's or a web view's page sees it, as in Chrome, where pages never get it. A page's `keydown` listener no longer hears it, and can't stop the quit with `preventDefault()`. This is the same in C, Swift and Rust.

To move: a page that listened for ⌘Q asks your app instead, with a [message](messages.md). To ask before quitting, answer `quitPrompt`, as [Menus](menus.md#asking-before-quitting) shows.

**One restore for each launch.** `Wren_RestoreLastSession` used to restore the same last session each time it was called, opening its windows again. Now a launch restores a Profile's last session once, as Chrome does, so no two open tabs share a [persistent ID](windows-and-tabs.md#finding-a-restored-tabs-records).

| Language | A second restore in one launch |
| --- | --- |
| C | `Wren_RestoreLastSession` returns `WREN_ERROR_NOT_ALLOWED`, with 0 tabs |
| Swift | `restoreLastSession` gives `.failure(.notAllowed)` |
| Rust | `restore_last_session` gives `Err(Error::NotAllowed)` |

A call that failed for another reason restored nothing, and can be made again.

To move: restore once per launch. To show a restored window's pages again, open new tabs at its tabs' URLs, from `tab.url` or Rust's `BrowserTab::url`. They get new IDs, and no history.

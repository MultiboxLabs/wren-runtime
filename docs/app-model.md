# How a Wren app works

This page has the ideas the whole API rests on. Its code is in Swift. The
[table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and
in C.

## What Wren is

Wren is Chromium's browser layer inside your app's process. That is the part of
Chrome under its windows, Chromium's `//chrome`, not only its `//content`. So a
Wren app has what Chrome has behind its UI: Profiles, tabs, history, sessions,
extensions, saved passwords, downloads, permission prompts and DevTools.

What Wren leaves out is Chrome's own windows and toolbar. Your app owns its
windows and draws its own browser UI. Chromium still supplies its bubbles,
dialogs and prompts, unless you draw your own in their place.
[The UI a browser provides](ui-checklist.md) says which is which, piece by
piece.

## One process, and Chromium runs it

Chromium's browser process runs inside your app's process. There is no second
"main process" to talk to, as in Electron. A call to Wren is a function call
into Chromium.

That has three consequences.

- **Wren owns the event loop.** `WrenApp.run` creates `NSApplication`, runs its
  event loop, and returns when the app quits. It replaces `NSApplicationMain`
  and SwiftUI's `@main` `App`. Your UI is AppKit, with SwiftUI views inside
  `NSHostingView` where you want them.
- **Everything happens on the main thread.** Call Wren only from the main
  thread, and only after `ready`. The exceptions are `WrenApp.run` itself and
  the version calls. Every callback arrives on the main thread too.
- **A crash in Chromium's browser code is a crash of your app.** Pages are still
  isolated. Each renders in a sandboxed helper process, as in Chrome, and a
  crashed page takes down only its tab.

## The objects

```text
WrenApp
 └─ Profile            a user's data: history, cookies, extensions, passwords
     └─ Browser        a set of tabs; what Chrome shows as one window
         └─ tab        a page with its history; has a view you place
```

Chromium owns all of them. Wren keeps no second copy of their state, and you
should keep none beyond what you draw. When something changes, read it again.

Wren opens no window and no Browser by itself, not even at launch. Your `ready`
callback makes the first one.

### Profiles

A Profile is one user's data on disk, inside the app's data directory.

| Kind           | What it is                                                                                                          |
| -------------- | ------------------------------------------------------------------------------------------------------------------- |
| persistent     | Kept on disk, and listed in `WrenApp.profileList`. The startup Profile is one.                                      |
| off the record | A persistent Profile's incognito side, from `profile.offTheRecord()`. It ends with its last Browser.                |
| ephemeral      | A throwaway, from `WrenApp.createEphemeralProfile`. It is on disk but not listed, and deleted once nothing uses it. |

`WrenApp.openProfile()` opens the startup Profile, and `openProfile(id:)` opens
another. As in Chrome, the startup Profile is the one whose Browser was active
last, and stays the same until the app quits. `createProfile` makes a new
persistent one. While you hold a Profile, Chromium keeps it loaded.

A persistent Profile can be changed as Chrome's settings page changes it:

- **Its name.** `profile.rename(_:)`. Wren trims spaces at either end.
- **Its avatar.** `WrenApp.avatars` lists the avatars Chrome's settings page
  offers, and `WrenApp.avatarPNG` draws one. `profile.setAvatar(_:)` sets one.
  `WrenApp.profileAvatarPNG(id:)` draws any listed Profile's avatar, loaded or
  not. The first avatar is a silhouette in the Profile's colours. Chromium
  downloads an avatar's large image from `www.gstatic.com` once, and draws a
  small built-in one until then. An avatar from an image of your own is not
  possible. Keep your own icons by Profile ID.
- **Its colour.** `profile.setColor(_:)` sets the theme's colour, an opaque
  `0xFFRRGGBB`, and `profile.resetColor()` goes back to Chromium's.
  `profile.color` reads it.
- **Deleting it.** `profile.delete` first closes the Profile's Browsers and
  those of its off-the-record side, and runs `beforeunload` on each, as quitting
  does. A page that cancels fails it with `.cancelled`, and nothing is deleted.
  The last persistent Profile cannot be deleted. As in Chrome, downloads in
  progress are cancelled without asking, the data goes at once and the folder at
  quit. Its web views end, and every handle to it or its off-the-record side
  closes. That also happens when the user deletes it from
  `chrome://profile-picker`. After the startup Profile is deleted,
  `openProfile()` fails until the next launch, which starts on another one.

`WrenStartup`'s `startupProfileName` names the startup Profile when Chromium
creates it. See [Startup](startup.md#the-data-directory).

`profile.clearBrowsingData` clears what Chrome's Clear browsing data dialog
clears, for one of its periods, in any kind of Profile. The completion gets the
kinds Chromium could not clear. Where policy forbids deleting history, a request
for history or downloads fails with `.notAllowed`, and nothing is cleared. Pages
of your own schemes keep their storage, as `chrome://` pages do.

```swift
profile.clearBrowsingData([.history, .siteData, .cache], period: .lastHour) { result in
    if case .success(let failed) = result, !failed.isEmpty { showFailure(failed) }
}
```

A `.profiles` change follows any change to the list: a Profile added, renamed,
deleted, or given a new avatar or colour, by your app or by Chromium's pages.

### Browsers

A Browser is Chromium's `Browser`: a tab strip and what belongs to it, such as
its active tab, its extensions' buttons and its side panel. Wren gives it no
window and draws nothing. You show it yourself.

- **Get one.** Make it with `profile.createBrowser(url:)`. Or receive it in
  `browserOpened` when Chromium opens one itself, for a popup, a `window.open`,
  undocked DevTools or a restored window.
- **Attach it.** `browser.attach(to:)` tells Chromium which view of your window
  holds its tabs. Its dialogs and prompts then appear there, and the Browser
  counts as active while that window is key.
- **Close it.** `browser.close()` asks it to close. A page may refuse with
  `beforeunload`. `browserClosed` says when the Browser has closed.

A Browser also closes when its last tab closes, as Chrome closes the window.
Nothing keeps a Browser open with no tabs. To keep a window open, add a New Tab
page before the last tab closes, or make a new Browser in `browserClosed`.

`browser.type` says what kind of Browser it is, such as `.normal`, `.popup` or
`.devtools`. `browser.windowFeatures` says which parts Chrome would give its
window, such as a tab strip, a toolbar and an address.

### Tabs

A tab is a value, `BrowserTab`, that wraps an ID. The ID never changes and is
never reused while the app runs. It stays with the tab when the tab is moved,
pinned, grouped, discarded to save memory, or dragged to another Browser. It is
safe to keep one after the tab has closed. Calls on it then throw
`.tabUnavailable`.

```swift
let tab = try browser.addTab(url: "https://example.com")
try tab.navigate(to: "https://example.org")
print(tab.title, tab.url, try tab.state().loading)
try tab.close()
```

`tab.view` is the `NSView` that shows the page.
[Windows and tabs](windows-and-tabs.md) covers placing it.

Tab groups (`TabGroup`), splits (`TabSplit`) and popover requests (`Popover`)
are values in the same way.

## Changes

Wren reports what changed. It does not send the new state. Read what you need
when a change arrives.

```swift
delegate.changed = { changes in
    for change in changes {
        switch change.kind {
        case .tabs:                    // tabs added, removed, moved, or another is active
            rebuildTabStrip(for: change.browser!)
        case .tab:                     // one tab's fields
            if change.tabFields.contains(.title) { updateTitle(of: change.tab!) }
            if change.tabFields.contains(.loading) { updateSpinner(of: change.tab!) }
        default: break
        }
    }
}
```

- **Changes are coalesced.** Each call has at most one record per Profile,
  Browser, tab, group, split, popover and web view. A tab's changed fields are
  combined in `change.tabFields`.
- **They arrive from the event loop.** Chromium has finished what it was doing
  when they arrive. So a handler may call any function.
- **Your own calls are reported too.** `tab.navigate` takes effect before it
  returns, and the matching change follows.
- **Some kinds start only once you have read the state.** Chromium has to track
  site state, zoom, the bookmark, the address field's text, downloads, bookmarks
  and "can reopen a closed tab". So their changes start after your first read of
  each, and then report every change.

`ChangeKind` lists every kind, each with what to read again. In C it is
`WrenChangeKind` in `Wren.h`.

## The callbacks

`AppDelegate` holds them. Only `browserOpened` is required.

| Callback            | When                                                                                                            |
| ------------------- | --------------------------------------------------------------------------------------------------------------- |
| `ready`             | Chromium is running. Create your windows now.                                                                   |
| `browserOpened`     | Chromium opened a Browser you did not create. Show it in a window, or close it.                                 |
| `browserClosed`     | A Browser closed, for any reason, its last tab closing among them.                                              |
| `changed`           | Something changed. See above.                                                                                   |
| `willQuit`          | The app is quitting and it can no longer be cancelled. Runs once, after every `browserClosed`.                  |
| `reopen`            | The user clicked the Dock icon while the app runs.                                                              |
| `openURLs`          | macOS asked the app to open URLs or files. Wren opens nothing itself.                                           |
| `focusLocationBar`  | Chromium wants your address field focused, for example on the New Tab page.                                     |
| `popover`           | Chromium is about to open a popover. You say whose it is.                                                       |
| `fullscreen`        | Chromium wants a window in or out of full screen. You say whether you will.                                     |
| `menuCommand`       | The user chose one of your main menu's items. See [Menus](menus.md).                                            |
| `downloadsPrompt`   | Downloads in progress would be cancelled. You say whose prompt it is.                                           |
| `profileExtensions` | A Profile is loading. You say which of your extensions it gets. See [Extensions your app ships](extensions.md). |
| `quitPrompt`        | ⌘Q would warn before quitting. You say whose warning it is.                                                     |

`popover`, `fullscreen`, `downloadsPrompt` and `quitPrompt` differ from the
rest. They run inside Chromium's call, because Chromium needs the answer to
carry on. In them, decide and return. Wren refuses calls that change Browsers,
tabs or popovers with `.notAllowed` until the callback returns. Do that work
afterwards, for example with `DispatchQueue.main.async`. `profileExtensions`
also runs inside Chromium's call, and makes no Wren calls at all.

## Asynchronous calls

Opening a Profile, creating a Browser and reading the last session take time.
They take a completion closure, which always runs later from the event loop,
never before the call returns:

```swift
profile.createBrowser(url: nil) { result in      // nil: Chromium's New Tab page
    switch result {
    case .success(let browser): show(browser)
    case .failure(let error): report(error)
    }
}
```

Every other call is synchronous.

## Errors

A call that can fail throws a `WrenError`. In C it returns a `WrenResult`. The
cases say what was wrong, not where.

| Error                                                                            | Meaning                                                                                   |
| -------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| `.closed`                                                                        | The Profile, Browser or web view has closed.                                              |
| `.tabUnavailable`, `.groupUnavailable`, `.splitUnavailable`, `.panelUnavailable` | Nothing open has this ID.                                                                 |
| `.profileUnavailable`                                                            | No such Profile, or Chromium could not load it.                                           |
| `.notAllowed`                                                                    | Not allowed here, such as a tab strip call on a popup, or a change from inside `popover`. |
| `.cancelled`                                                                     | A page's `beforeunload` cancelled it.                                                     |
| `.invalidArgument`                                                               | A malformed URL, tabs of two Browsers in one call, and the like.                          |
| `.navigationUnavailable`                                                         | There is no page to go back or forward to, or no page yet.                                |
| `.popoverClosed`, `.stalePage`, `.staleResult`                                   | What you are acting on has been replaced since you read it. Read again.                   |
| `.downloadUnavailable`, `.bookmarkUnavailable`                                   | Chromium no longer has this download, or no bookmark has this ID.                         |

The stale errors exist because UI lags behind the browser. A button drawn for
one page may be clicked after the tab has navigated. So the state such an action
depends on comes with a token, a `PageToken` or an `OmniboxResult`. The action
takes the token back, and Wren refuses it if the page or the list has changed.

## Lifetimes

`Profile` and `Browser` are classes. Each holds a reference to Chromium's
object, and releases it when the last Swift reference goes. A handle stays safe
to use after what it names has closed. Calls then throw `.closed` or do nothing,
and `isOpen` is false.

Two rules follow.

- **Keep what you use.** A `WebView` lives exactly as long as its Swift object.
  A Profile you hold stays loaded.
- **After `WrenApp.run` returns, everything has closed.** Drop your Profiles and
  Browsers, then exit.

## Quitting

`WrenApp.quit()` asks the app to quit. A page may cancel the quit with
`beforeunload`. With downloads in progress, `downloadsPrompt` asks first.
Otherwise every Browser gets `browserClosed`, `willQuit` runs once, and
`WrenApp.run` returns `.normal`. The default menu's Quit item does the same.

`WrenApp.quit()` never warns. ⌘Q, the default menu's Quit key, warns first while
Chrome's Warn Before Quitting is on, with Chromium's panel or your own
(`quitPrompt`). See [Menus](menus.md#asking-before-quitting).

`WrenApp.run` returns an `ExitStatus`:

- `.normal`.
- `.invalidStartup`. Wren prints the reason to stderr.
- `.alreadyRunning`. Another process has the data directory. Wren told it about
  this launch.
- `.apiMismatch`. The framework in the bundle is not the release the app was
  built against.
- `.chromiumError`.

## Security

Your app's code is trusted, and pages are not. Chromium's sandboxes stay on.
Pages cannot call Wren's API, and Wren opens no network port for control.
Permission and credential decisions stay tied to the page that asked. A request
ends with its navigation or its tab.

One thing is yours to get right: [your own URL schemes](schemes.md). The handler
decides what a URL serves. So it must not read outside its own folder, or act on
a request only because it arrived.

## The names in Swift, Rust and C

Rust's names are Swift's in Rust's case. The callbacks are the methods of the
`App` trait, and the calls that take no handle are on `MainThread`.

| Swift                            | Rust                                   | C                              |
| -------------------------------- | -------------------------------------- | ------------------------------ |
| `WrenApp.run`                    | `wren::run`                            | `Wren_Main`                    |
| `WrenApp.quit()`                 | `MainThread::quit`                     | `Wren_Quit`                    |
| `WrenApp.openProfile`            | `MainThread::open_profile`             | `Wren_OpenProfile`             |
| `WrenApp.createProfile`          | `MainThread::create_profile`           | `Wren_CreateProfile`           |
| `WrenApp.createEphemeralProfile` | `MainThread::create_ephemeral_profile` | `Wren_CreateEphemeralProfile`  |
| `WrenApp.profileList`            | `MainThread::profile_list`             | `Wren_GetProfileListCount`     |
| `profile.rename`                 | `Profile::rename`                      | `Wren_RenameProfile`           |
| `profile.setAvatar`              | `Profile::set_avatar`                  | `Wren_SetProfileAvatar`        |
| `profile.setColor`               | `Profile::set_color`                   | `Wren_SetProfileColor`         |
| `profile.delete`                 | `Profile::delete`                      | `Wren_DeleteProfile`           |
| `profile.clearBrowsingData`      | `Profile::clear_browsing_data`         | `Wren_ClearBrowsingData`       |
| `WrenApp.avatars`                | `MainThread::avatars`                  | `Wren_GetAvatarCount`          |
| `profile.offTheRecord()`         | `Profile::off_the_record`              | `Wren_GetOffTheRecordProfile`  |
| `profile.createBrowser`          | `Profile::create_browser`              | `Wren_CreateBrowser`           |
| `browser.attach(to:)`            | `Browser::attach_to_view`              | `Wren_AttachBrowserToView`     |
| `browser.close()`                | `Browser::close`                       | `Wren_CloseBrowser`            |
| `browser.type`                   | `Browser::browser_type`                | `Wren_GetBrowserType`          |
| `browser.windowFeatures`         | `Browser::window_features`             | `Wren_GetWindowFeatures`       |
| `browser.addTab`                 | `Browser::add_tab`                     | `Wren_AddTab`                  |
| `tab.navigate(to:)`              | `BrowserTab::navigate`                 | `Wren_Navigate`                |
| `tab.view`                       | `BrowserTab::view`                     | `Wren_GetTabView`              |
| `AppDelegate`                    | the `App` trait                        | `WrenAppDelegate`              |
| `AppDelegate.changed`            | `App::changed`                         | `on_changed`                   |
| `Change`, `ChangeKind`           | `Change`, `ChangeKind`                 | `WrenChange`, `WrenChangeKind` |
| `WrenError`                      | `wren::Error`                          | `WrenResult`                   |
| `ExitStatus`                     | `ExitStatus`                           | `WrenExit`                     |
| `DispatchQueue.main.async`       | `MainThread::later`                    |                                |

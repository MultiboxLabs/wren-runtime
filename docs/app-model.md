# How a Wren Runtime app works

Every Wren Runtime app rests on a few ideas. Chromium runs inside your app, its objects are the ones you work with, it tells you when they change, and it asks you before it shows its own UI. Once these are clear, the rest of the API follows the same patterns.

The code here is in Swift. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## Chromium runs in your app

Chromium runs inside your app's process, as it does in an Electron app's main process, and your code runs there with it. In Electron that code is JavaScript on Node.js, and the UI is web pages in other processes that reach it over IPC. In Wren Runtime it's your native code, and a call to Wren Runtime is a plain function call into Chromium. That shapes three things.

- **Wren Runtime runs the event loop.** `WrenApp.run` creates `NSApplication`, runs its event loop, and returns when the app quits. It takes the place of `NSApplicationMain`. Your UI is AppKit, with SwiftUI views in `NSHostingView` wherever you like. A SwiftUI `@main` `App` can't be used, because it runs an event loop of its own.
- **Everything happens on the main thread.** Call Wren Runtime from the main thread, once `ready` has run. Every callback arrives there too. From another thread, `WrenApp.postToMainThread` runs your work on the main thread. Only that, `WrenApp.run` and the version calls work from any thread.
- **Pages stay isolated.** Each page renders in a sandboxed process of its own, as in Chrome, and a page that crashes takes down only its tab. A crash in Chromium's own browser code takes down your app, as it would take down Chrome.

## Profiles, Browsers and tabs

```text
WrenApp
 └─ Profile            one person's data: history, cookies, extensions, passwords
     └─ Browser        a set of tabs, which Chrome shows as one window
         └─ tab        a page with its history, and a view to show it in
```

These are Chromium's own objects. Wren Runtime keeps no copy of their state, and your app doesn't need one either. When something changes, read what you show again.

Nothing opens by itself, not even at launch. Your `ready` callback opens the first Profile and creates the first Browser.

### Profiles

A Profile holds one person's browsing data: their history, cookies, saved passwords and extensions. Most apps use one. `WrenApp.openProfile()` opens the Profile Chromium starts with, which is the one whose Browser was active last, as in Chrome.

| Kind | What it is |
| --- | --- |
| Persistent | Kept on disk, and listed in `WrenApp.profileList`. The Profile Chromium starts with is one. |
| Off the record | A persistent Profile's incognito side, from `profile.offTheRecord()`. It ends when its last Browser closes. |
| Ephemeral | A Profile for one session, from `WrenApp.createEphemeralProfile`. Nothing lists it, and Chromium deletes it once nothing uses it. |

While you hold a Profile, Chromium keeps it loaded. [Profiles and their data](profiles.md) covers making more of them, changing their names and colours, clearing their data, and bringing data over from another browser.

### Browsers

A Browser is a set of tabs, together with what belongs to them, such as the active tab, extensions' buttons and the side panel. Chrome shows each Browser as one window. Wren Runtime gives a Browser no window and draws nothing, so it appears wherever you put it.

- **Making one.** `profile.createBrowser(url:)` makes a Browser. Chromium also opens Browsers of its own, for a popup, a page's `window.open`, DevTools in its own window, or a restored window. It hands each one to `browserOpened`, where you show it in a window or close it.
- **Showing it.** `browser.attach(to:)` tells Chromium which of your views holds the Browser's tabs. Chromium's dialogs and prompts then appear over that view, and the Browser counts as active while its window is key.
- **Closing it.** `browser.close` asks the Browser's pages first, and closes it once they agree. A page can refuse with `beforeunload`, and then the completion gets `.cancelled`. `browserClosed` tells you when a Browser has closed, for whatever reason.

A Browser also closes when its last tab closes, as Chrome closes a window. To keep a window open, add a New Tab page before the last tab goes, or create another Browser in `browserClosed`.

`browser.type` says what kind of Browser it is, such as `.normal`, `.popup` or `.devtools`. `browser.windowFeatures` says which parts Chrome would give its window, such as a tab strip, a toolbar and an address field.

### Tabs

A tab is a `BrowserTab`, a small value that holds the tab's ID. The ID stays the same while the tab is moved, pinned, grouped, discarded to save memory, or dragged to another Browser, and Chromium never reuses it while the app runs. You can keep a tab after it closes. Its calls then throw `.tabUnavailable`.

```swift
let tab = try browser.addTab(url: "https://example.com")
try tab.navigate(to: "https://example.org")
print(tab.title, tab.url, try tab.state().loading)
try tab.close()
```

`tab.close()` asks the page and returns at once, as Chrome's Close Tab does. To close several tabs as one and learn how it went, use `BrowserTab.close(_:completion:)`. Every tab stays open if one page keeps its tab with `beforeunload`, and the completion then gets `.cancelled`.

`tab.view` is the `NSView` that shows the tab's page. [Windows and tabs](windows-and-tabs.md) covers where to put it.

Tab groups, splits and popover requests work the same way, as `TabGroup`, `TabSplit` and `Popover` values.

## Changes

Wren Runtime tells you what changed, and you read the new state yourself. Your UI then always shows what Chromium has, with no second copy to keep in step.

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

- **One change for each thing.** Each call to `changed` has at most one change for each Profile, Browser, tab, group, split, popover and web view. The fields that changed on a tab come together in `tabFields`.
- **Safe to act on.** Changes arrive from the event loop, once Chromium has finished what it was doing, so you can call anything while handling them.
- **Your own calls too.** `tab.navigate` takes effect before it returns, and its change arrives afterwards.
- **Some state only after a first read.** Chromium tracks site state, zoom, bookmarks, the address field's text, downloads, the installed extensions and "can reopen a closed tab" only once someone asks. Their changes start after your first read of each, and from then on every change is reported.

`ChangeKind` lists every kind of change, and what to read again for each.

## Callbacks

Your app's callbacks are fields of `AppDelegate`. Only `browserOpened` is required.

| Callback | When it runs |
| --- | --- |
| `ready` | Chromium is running. Open your first window now. |
| `browserOpened` | Chromium opened a Browser you didn't create. Show it in a window, or close it. |
| `browserClosed` | A Browser closed, for any reason, including its last tab closing. |
| `changed` | Something changed. See [Changes](#changes). |
| `quitCommitted` | A quit can no longer be cancelled. It runs once, before any Browser closes. |
| `willQuit` | The app is about to quit. It runs once, after every Browser has closed. |
| `reopen` | Someone clicked the Dock icon while the app was running. |
| `openURLs` | macOS asked the app to open URLs or files. Wren Runtime opens nothing itself. |
| `focusLocationBar` | Chromium wants your address field focused, such as on the New Tab page. |
| `windowRequest` | Chromium asks for a window to move, resize, minimise, zoom or restore. You can do it, adapt it or ignore it. |
| `popover` | Chromium is about to open a popover. You say whether it shows Chromium's, yours or none. |
| `fullscreen` | A page wants its window in or out of full screen. You say whether you'll do it. |
| `menuCommand` | Someone chose one of your items in the main menu or the Dock menu. See [Menus](menus.md). |
| `downloadsPrompt` | Quitting or closing would cancel downloads in progress. You say whose prompt shows. |
| `quitPrompt` | ⌘Q would warn before quitting. You say whose warning shows. See [Menus](menus.md#asking-before-quitting). |
| `profileExtensions` | A Profile is loading. You say which of your extensions it gets. See [Extensions your app ships](extensions.md). |
| `navigation` | A tab that asks before leaving its page is about to. You say whether it goes. See [Windows and tabs](windows-and-tabs.md#keeping-a-tab-on-its-site). |
| `updateCheckRequested` | Chromium's About page wants to check for updates. See [Updating your app with Sparkle](updates.md). |

### Callbacks that run inside Chromium's call

Most callbacks run from the event loop. A few run while Chromium is in the middle of something, and it waits for them to return.

- `popover`, `fullscreen`, `downloadsPrompt`, `quitPrompt` and `navigation` ask a question that Chromium needs answered before it carries on.
- `quitCommitted` runs before Chromium closes anything for the quit.
- `profileExtensions` runs while a Profile loads.

In these, decide and return. Until they return, Wren Runtime refuses calls that change Browsers, tabs, navigation or popovers with `.notAllowed`. Do that work afterwards, for example with `DispatchQueue.main.async`. `profileExtensions` can't make any Wren Runtime calls.

## Calls that take time

Opening a Profile, creating a Browser and reading the last session take time, so they take a completion. It always runs later, from the event loop, never before the call returns.

```swift
profile.createBrowser(url: nil) { result in      // nil: Chromium's New Tab page
    switch result {
    case .success(let browser): show(browser)
    case .failure(let error): report(error)
    }
}
```

Every other call returns its answer straight away.

## Errors

A call that can fail throws a `WrenError`, whose cases say what was wrong.

| Error | What it means |
| --- | --- |
| `.closed` | The Profile, Browser or web view has closed. |
| `.tabUnavailable`, `.groupUnavailable`, `.splitUnavailable`, `.panelUnavailable` | Nothing open has this ID. |
| `.profileUnavailable` | There's no such Profile, or Chromium couldn't load it. |
| `.notAllowed` | The call isn't allowed here, such as a tab strip call on a popup, or a change from inside `popover`. |
| `.cancelled` | A page's `beforeunload` cancelled it. |
| `.invalidArgument` | A malformed URL, tabs of two Browsers in one call, or something like it. |
| `.navigationUnavailable` | There's no page to go back or forward to, or no page yet. |
| `.popoverClosed`, `.stalePage`, `.staleResult` | What you're acting on has changed since you read it. Read it again. |
| `.downloadUnavailable`, `.bookmarkUnavailable` | Chromium no longer has this download, or no bookmark has this ID. |

The last few errors exist because your UI can lag behind Chromium. Someone can click a button drawn for one page just after the tab has moved on to another. So the state an action depends on comes with a token, such as a `PageToken` or an `OmniboxResult`, and the action takes it back. If the page or the list has changed since, Wren Runtime refuses the action instead of applying it to the wrong page.

## Lifetimes

`Profile`, `Browser` and `WebView` are classes that keep Chromium's object alive while you hold them. A Profile you hold stays loaded, and a web view lasts exactly as long as its Swift object.

A handle stays safe to use after what it names has closed. Its calls then throw `.closed` or do nothing, and `isOpen` is false. When `WrenApp.run` returns, everything has closed. Drop your Profiles and Browsers, then exit.

## Quitting

`WrenApp.quit()` asks the app to quit, as ⌘Q, the Dock's Quit and logging out do. Every page gets its `beforeunload` first, and any page can cancel the quit, even a logout. With downloads in progress, `downloadsPrompt` asks first. Once nothing has cancelled:

1. `quitCommitted` runs, and the quit can no longer be cancelled.
2. Every Browser closes, and `browserClosed` reports each one.
3. `willQuit` runs.
4. `WrenApp.run` returns `.normal`.

`quitCommitted` is the place to act on the quit as a whole, because the `browserClosed` calls after it come from the quit, not from the person using the app. If one of your windows shows several Browsers, as in [Workspaces](workspaces.md), hide it there, so it doesn't empty one Browser at a time on screen. Save what you keep yourself there too. With no Browser open, `quitCommitted` runs just before `willQuit`.

When the system ends the app with SIGTERM, as launchd can at logout or shutdown, `quitCommitted` runs and the process ends, with no `browserClosed` or `willQuit`.

`WrenApp.quit()` never warns first. ⌘Q in the default main menu shows Chrome's "Hold ⌘Q to Quit" while Warn Before Quitting is on, and `quitPrompt` lets you show a warning of your own instead.

`WrenApp.run` returns an `ExitStatus`:

| Status | What it means |
| --- | --- |
| `.normal` | The app quit. |
| `.invalidStartup` | A `Startup` value was wrong. Wren Runtime prints which one to stderr. |
| `.alreadyRunning` | Another process is using the data folder. Wren Runtime told it about this launch. |
| `.apiMismatch` | The framework in the bundle isn't the release the app was built against. |
| `.chromiumError` | Chromium exited with an error. |

## Security

Your app's code is trusted, and pages aren't. Chromium's sandboxes stay on, pages can't call Wren Runtime, and Wren Runtime opens no network port for anything to control it through. Permission and credential prompts stay tied to the page that asked, and end with its navigation or its tab.

The one place that needs care is [your own URL schemes](schemes.md). Your handler decides what each URL serves, so it shouldn't read outside its own folder, or act on a request just because the request arrived.

## The names in Swift, Rust and C

In Rust, the callbacks are the methods of the `App` trait, and the calls that don't belong to a Profile or a Browser are on `MainThread`.

| Swift | Rust | C |
| --- | --- | --- |
| `WrenApp.run` | `wren::run` | `Wren_Main` |
| `WrenApp.quit()` | `MainThread::quit` | `Wren_Quit` |
| `WrenApp.postToMainThread` | `wren::post_to_main_thread` | `Wren_PostToMainThread` |
| `WrenApp.openProfile` | `MainThread::open_profile` | `Wren_OpenProfile` |
| `WrenApp.createEphemeralProfile` | `MainThread::create_ephemeral_profile` | `Wren_CreateEphemeralProfile` |
| `WrenApp.profileList` | `MainThread::profile_list` | `Wren_GetProfileListCount` |
| `profile.offTheRecord()` | `Profile::off_the_record` | `Wren_GetOffTheRecordProfile` |
| `profile.createBrowser` | `Profile::create_browser` | `Wren_CreateBrowser` |
| `browser.attach(to:)` | `Browser::attach_to_view` | `Wren_AttachBrowserToView` |
| `browser.close`, `Browser.close(_:completion:)` | `Browser::close`, `Browser::close_all` | `Wren_CloseBrowsers` |
| `BrowserTab.close(_:completion:)` | `BrowserTab::close_all` | `Wren_CloseTabs` |
| `browser.type` | `Browser::browser_type` | `Wren_GetBrowserType` |
| `browser.windowFeatures` | `Browser::window_features` | `Wren_GetWindowFeatures` |
| `browser.addTab` | `Browser::add_tab` | `Wren_AddTab` |
| `tab.navigate(to:)` | `BrowserTab::navigate` | `Wren_Navigate` |
| `tab.view` | `BrowserTab::view` | `Wren_GetTabView` |
| `AppDelegate` | the `App` trait | `WrenAppDelegate` |
| `AppDelegate.changed` | `App::changed` | `on_changed` |
| `AppDelegate.quitCommitted` | `App::quit_committed` | `on_quit_committed` |
| `Change`, `ChangeKind` | `Change`, `ChangeKind` | `WrenChange`, `WrenChangeKind` |
| `WrenError` | `wren::Error` | `WrenResult` |
| `ExitStatus` | `ExitStatus` | `WrenExit` |
| `DispatchQueue.main.async` | `MainThread::later` | none |

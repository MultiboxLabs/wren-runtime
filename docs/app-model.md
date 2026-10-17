# How a Wren app works

The ideas the whole API rests on. Names are the Swift API's, with the C name
beside each where it differs in more than spelling.

## One process, and Chromium runs it

Chromium's browser process runs inside your app's process. There is no second
"main process" to talk to, as Electron has: a call to Wren is a function call
into Chromium.

That has three consequences.

- **Wren owns the event loop.** `WrenApp.run` (`Wren_Main`) creates
  `NSApplication`, runs its event loop, and returns when the app quits. It
  replaces `NSApplicationMain` and SwiftUI's `@main` `App`. Your UI is AppKit,
  with SwiftUI views inside `NSHostingView` where you want them.
- **Everything happens on the main thread.** Call Wren only from the main
  thread, and only after `ready` (the exceptions are `WrenApp.run` itself and
  the version calls). Every callback arrives on the main thread too.
- **A crash in Chromium's browser code is a crash of your app.** Pages are still
  isolated: each renders in a sandboxed helper process, as in Chrome, and a
  crashed page takes down only its tab.

## The objects

```text
WrenApp
 └─ Profile            a user's data: history, cookies, extensions, passwords
     └─ Browser        a set of tabs; what Chrome shows as one window
         └─ tab        a page with its history; has a view you place
```

Chromium owns all of them. Wren keeps no second copy of their state, and neither
should you beyond what you draw: when something changes, read it again.

### Profiles

A Profile is one user's data on disk, inside the app's data directory.

| Kind           | What it is                                                                                              |
| -------------- | ------------------------------------------------------------------------------------------------------- |
| persistent     | Kept on disk and listed (`WrenApp.profileList`). The startup Profile is one.                            |
| off the record | A persistent Profile's incognito side (`profile.offTheRecord()`). It ends when its last Browser closes. |
| ephemeral      | A throwaway: on disk but not listed, deleted once nothing uses it (`WrenApp.createEphemeralProfile`).   |

`WrenApp.openProfile()` opens the startup Profile, `openProfile(id:)` another,
and `createProfile` makes a new persistent one. While you hold a Profile,
Chromium keeps it loaded.

### Browsers

A Browser is Chromium's `Browser`: a tab strip and what belongs to it (its
active tab, its extensions' buttons, its side panel). Wren gives it no window
and draws nothing. You show it:

- make it with `profile.createBrowser(url:)`, or receive it in `browserOpened`
  when Chromium opens one itself (a popup, `window.open`, undocked DevTools, a
  restored window);
- tell Chromium which view of your window shows its tabs with
  `browser.attach(to:)`, so that its dialogs and prompts appear there and the
  Browser counts as active while that window is key;
- close it with `browser.close()`. Closing is a request: a page may refuse it
  with `beforeunload`. `browserClosed` says when it has happened.

`browser.type` says what kind it is (`.normal`, `.popup`, `.devtools` and so
on), and `browser.windowFeatures` which parts Chrome would give its window: a
tab strip, a toolbar, an address.

### Tabs

A tab is a value, `BrowserTab`, wrapping an ID (`WrenTabID`). The ID never
changes and is never reused while the app runs: it follows the tab when it is
moved, pinned, grouped, discarded to save memory, or dragged to another Browser.
Holding one after the tab has closed is safe; calls on it throw
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

Wren reports what changed; it does not send the new state. Read what you need
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

- **Changes are coalesced.** Each call carries at most one record per Profile,
  Browser, tab, group, split and popover, with a tab's changed fields combined
  (`change.tabFields`).
- **They arrive from the event loop**, after Chromium has finished what it was
  doing, never in the middle of it. So a handler may call anything.
- **Your own calls are reported too.** `tab.navigate` takes effect before it
  returns, and the matching change follows.
- **Some are reported only once you have read the state.** Site state, zoom, the
  bookmark, the address field's text and "can reopen a closed tab" cost Chromium
  something to track, so their changes start after your first read of each.

The kinds are listed in `Wren.h` as `WrenChangeKind`, each with what to read
again.

## The callbacks

`AppDelegate` holds them; only `browserOpened` is required.

| Callback           | When                                                                                           |
| ------------------ | ---------------------------------------------------------------------------------------------- |
| `ready`            | Chromium is running. Create your windows now.                                                  |
| `browserOpened`    | Chromium opened a Browser you did not create. Show it in a window, or close it.                |
| `browserClosed`    | A Browser closed, for any reason.                                                              |
| `changed`          | Something changed (above).                                                                     |
| `willQuit`         | The app is quitting and it can no longer be cancelled. Runs once, after every `browserClosed`. |
| `reopen`           | The Dock icon was clicked while the app runs.                                                  |
| `openURLs`         | macOS asked the app to open URLs or files. Wren opens nothing itself.                          |
| `focusLocationBar` | Chromium wants your address field focused, for example on the New Tab page.                    |
| `popover`          | Chromium is about to open a popover; you say whose it is.                                      |
| `fullscreen`       | Chromium wants a window in or out of full screen; you say whether you will.                    |

`popover` and `fullscreen` are different from the rest: they run _inside_
Chromium's call, because Chromium needs the answer to carry on. In them, decide
and return. Calls that change Browsers, tabs or popovers are refused with
`.notAllowed` until the callback returns; do that work afterwards, for example
with `DispatchQueue.main.async`.

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

Everything else is synchronous.

## Errors

A call that can fail throws a `WrenError` (in C, returns a `WrenResult`). The
cases say what was wrong, not where:

| Error                                                                            | Meaning                                                                                       |
| -------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| `.closed`                                                                        | The Profile, Browser or web view has closed.                                                  |
| `.tabUnavailable`, `.groupUnavailable`, `.splitUnavailable`, `.panelUnavailable` | Nothing open has this ID.                                                                     |
| `.profileUnavailable`                                                            | No such Profile, or Chromium could not load it.                                               |
| `.notAllowed`                                                                    | Chromium's rules forbid it here: a tab strip call on a popup, a change from inside `popover`. |
| `.invalidArgument`                                                               | A malformed URL, tabs of two Browsers in one call, and the like.                              |
| `.navigationUnavailable`                                                         | There is no page to go back or forward to.                                                    |
| `.popoverClosed`, `.stalePage`, `.staleResult`                                   | What you are acting on has been replaced since you read it. Read again.                       |

The stale errors exist because UI lags behind the browser. A button drawn for
one page may be clicked after the tab has navigated. State that such actions
depend on comes with a token (`PageToken`, `OmniboxResult`), the action takes it
back, and Wren refuses it if the page or the list has changed.

## Lifetimes

`Profile` and `Browser` are classes that hold a reference to Chromium's object
and release it when the last Swift reference goes. A handle stays safe to use
after what it names has closed: calls throw `.closed` or do nothing, and
`isOpen` is false.

Two rules follow:

- **Keep what you use.** A `WebView` lives exactly as long as its Swift object;
  a held Profile stays loaded.
- **After `WrenApp.run` returns, everything has closed.** Drop your Profiles and
  Browsers, then exit.

## Quitting

`WrenApp.quit()` asks to quit. Pages may cancel it with `beforeunload`.
Otherwise every Browser gets `browserClosed`, `willQuit` runs once, and
`WrenApp.run` returns `.normal`. The default menu's Quit item does the same.

`WrenApp.run` returns an `ExitStatus`: `.normal`; `.invalidStartup` (the reason
is printed to stderr); `.alreadyRunning` (another process has the data
directory, and was told about this launch); `.apiMismatch` (the framework in the
bundle is not the release the app was built against); `.chromiumError`.

## Security

Your app's code is trusted; pages are not. Chromium's sandboxes stay on, pages
cannot call Wren's API, and Wren opens no network port for control. Permission
and credential decisions stay tied to the page that asked: a request ends with
its navigation or its tab.

One thing is yours to get right: [your own URL schemes](schemes.md). The handler
decides what a URL serves, so it must not read outside its own folder or act on
a request merely because it arrived.

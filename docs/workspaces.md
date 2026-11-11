# Workspaces: several Browsers in one window

One window of your browser can hold several complete Chrome browsers, and switch between them in an instant. Each one can be a workspace, such as one for work and one for home, with its own tabs, its own Profile, and so its own cookies, extensions and saved passwords. Chrome can't do this, because each of its windows is exactly one Browser.

Wren Runtime can, because of how it's built. A Browser in Wren Runtime is Chromium's own Browser, with no window of its own, and the window is yours. So nothing stops one window from holding several of them. You attach each Browser to the same view, and keep one in front. The others stay open and loaded in the background, and Chromium sends menu commands, dialogs and bubbles to the one in front, as if the window were its own. The session can even bring the whole window back as one window after a restart.

This page builds a workspace window step by step: setting it up, switching, handling Chromium's requests to bring a Browser forward, quitting, and restoring. It builds on [Windows and tabs](windows-and-tabs.md). The code is in Swift and Rust, and the [table at the end](#the-names-in-swift-rust-and-c) gives each name in C.

## Setting up the window

In a normal window, you attach one Browser with `attach(to:)`, which tells Chromium which of your views holds its tabs. That makes the view's window the Browser's window. Chromium's dialogs and prompts appear over that view, and the Browser counts as active while the window is key.

For a workspace window, attach each workspace's Browser to the same view, the one where your tabs appear. Every one of them then treats the window as its own. To say which one the window answers for right now, keep all but one in the background with `isBackgrounded`.

Here the window's UI is one [web view](web-views.md), whose page shows the tabs in [`<wren-view>` elements](views-in-web-views.md), so every Browser is attached to the web view's view. A native window works the same way, with the view that holds your tabs.

```swift
personal.attach(to: webView.view)
work.attach(to: webView.view)
work.isBackgrounded = true

func bringForward(_ next: Browser) {
    current.isBackgrounded = true      // first, so two are never in front
    next.isBackgrounded = false
    current = next
    // Then tell your page which tab to show in its <wren-view>.
}
```

```rust
personal.attach_to_view(Some(web_view.view()));
work.attach_to_view(Some(web_view.view()));
work.set_backgrounded(true)?;

fn bring_forward(current: &RefCell<Browser>, next: &Browser) -> Result<(), Error> {
    current.borrow().set_backgrounded(true)?; // first, so two are never in front
    next.set_backgrounded(false)?;
    *current.borrow_mut() = next.clone();
    // Then tell your page which tab to show in its <wren-view>.
    Ok(())
}
```

Keep exactly one Browser of a window in front, because Wren Runtime doesn't put the others in the background for you. Two in front would share the window. Both would be active, the menu would reach the one made first, and a sheet from one would block both.

## What a background Browser does

A Browser in the background still belongs to the window, and its tabs stay open and loaded. Until it comes forward:

- It's never active, and Chromium's menu commands go to the Browser in front.
- Its sheets, its pages' dialogs and its bubbles wait until it comes forward, and block nothing meanwhile.

What your window shows is up to you. Wren Runtime only decides which Browser the window answers for.

## When Chromium wants a workspace in front

Sometimes Chromium wants a background Browser to come forward. Someone clicks a site's notification, an extension calls `chrome.windows.update`, a page focuses a window it opened, someone switches to one of its tabs, or Settings opens from an incognito window. The Browser stays in the background, and Wren Runtime tells you with a `.foregroundRequest` change, so you decide whether to switch:

```swift
delegate.changed = { changes in
    for change in changes where change.kind == .foregroundRequest {
        bringForward(change.browser!)      // or badge the workspace's button
    }
}
```

```rust
struct MyBrowser {
    current: RefCell<Browser>,
}

impl App for MyBrowser {
    fn browser_opened(&self, _: MainThread, opened: Browser, _: BrowserType, _: Option<Browser>) {
        // Show it, as in Windows and tabs.
        let _ = opened;
    }

    fn changed(&self, _: MainThread, changes: &[Change]) {
        for change in changes {
            if change.kind == ChangeKind::ForegroundRequest {
                if let Some(browser) = &change.browser {
                    // Or badge the workspace's button.
                    let _ = bring_forward(&self.current, browser);
                }
            }
        }
    }
}
```

`browser.wantsForeground` stays true until the Browser comes forward, or until `browser.clearForegroundRequest()` drops the request and leaves the Browser where it is, such as once you've badged its button. Chromium's next request is then reported again.

## Quitting

When the app quits, its Browsers close one at a time, and `browserClosed` reports each one. A window that carries on when one of its Browsers closes would empty itself on screen, one workspace at a time. Hide it first, in `quitCommitted`, which runs once every page has agreed to the quit, before the first Browser closes:

```swift
delegate.quitCommitted = {
    for window in workspaceWindows {
        window.orderOut(nil)
    }
}
```

`quitCommitted` runs inside Chromium's call, so Wren Runtime refuses calls there that change Browsers. Save anything you keep about your workspaces there too.

## Bringing the window back

Chromium records each Browser as a window of its own, so a restore would bring a workspace window back as one window for each Browser. To bring it back as one window, tag each Browser with its window and its place in it. Chromium keeps the tag in the session:

```swift
try personal.setSessionTag("\(windowID) 0")
try work.setSessionTag("\(windowID) 1")
```

```rust
personal.set_session_tag(&format!("{window_id} 0"))?;
work.set_session_tag(&format!("{window_id} 1"))?;
```

When you call `profile.restoreLastSession`, as [Windows and tabs](windows-and-tabs.md#restoring-the-last-session) shows, each restored Browser arrives in `browserOpened` with its tag already set. Read the tag there, and attach the Browser to the window it names, making the window if this is the first of its Browsers to arrive:

```swift
let parts = browser.sessionTag.split(separator: " ")
if parts.count == 2, let workspace = workspaces[String(parts[0])] ?? makeWorkspace(String(parts[0])) {
    workspace.add(browser, at: Int(parts[1]) ?? 0)
}
```

- **Keep the tags current.** Change a tag when its Browser moves, such as to record which one is in front. A tag holds at most 128 bytes of UTF-8.
- **Background the restored Browsers.** A restore activates the last Browser it restores. If you put restored Browsers in the background as they arrive, you get a foreground request for that one too, which you can drop.
- **Reopen Closed Tab brings one Browser back.** A Browser it reopens has its tag, but comes back alone, because Chromium reopens one Browser at a time.
- **New windows aren't tagged.** A Browser that Chromium opens from a tagged one, such as a new window, has no tag until you give it one.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `browser.attach(to:)` | `Browser::attach_to_view` | `Wren_AttachBrowserToView` |
| `browser.isBackgrounded` | `Browser::set_backgrounded`, `Browser::is_backgrounded` | `Wren_SetBrowserBackgrounded` |
| `.foregroundRequest` | `ChangeKind::ForegroundRequest` | `WREN_CHANGE_FOREGROUND_REQUEST` |
| `browser.wantsForeground` | `Browser::wants_foreground` | `Wren_HasForegroundRequest` |
| `browser.clearForegroundRequest()` | `Browser::clear_foreground_request` | `Wren_ClearForegroundRequest` |
| `AppDelegate.quitCommitted` | `App::quit_committed` | `on_quit_committed` |
| `browser.sessionTag` | `Browser::session_tag` | `Wren_CopyBrowserSessionTag` |
| `browser.setSessionTag(_:)` | `Browser::set_session_tag` | `Wren_SetBrowserSessionTag` |

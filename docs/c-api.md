# The C API

Wren's API is a C API. The Swift API and the Rust crate are layers over it, with
the same functions under their own names. Each Swift and Rust item's comment
names the C function it calls. So everything in these guides can be done from C,
Objective-C or C++, or from any language that can call C.

The headers are the reference. This page explains how to read them.

| Header             | What is in it                                                                                                                                                                    |
| ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Wren.h`           | The shared types and results, startup, changes, the app's callbacks, `Wren_Main` and the version. It includes the others.                                                        |
| `WrenBrowser.h`    | Profiles, Browsers, several Browsers in one window, full screen, side panels.                                                                                                    |
| `WrenTabs.h`       | Tabs, navigation, tab groups, a tab's view, splits, DevTools.                                                                                                                    |
| `WrenExtensions.h` | Extensions' actions, and the installed extensions.                                                                                                                               |
| `WrenToolbar.h`    | Anchors, page actions, content setting icons.                                                                                                                                    |
| `WrenDownloads.h`  | A Profile's downloads, the download button, the prompts before closing or quitting with downloads in progress.                                                                   |
| `WrenUI.h`         | Popover requests, and the state behind Chromium's popovers: the site, extensions' access, zoom, the bookmark, content settings, passwords, addresses, cards, permission prompts. |
| `WrenOmnibox.h`    | What the address field shows, and Chromium's suggestions.                                                                                                                        |
| `WrenBookmarks.h`  | A Profile's bookmarks, read and changed: the bar, folders, opening one.                                                                                                          |
| `WrenSession.h`    | The last session, restoring it, reopening closed tabs.                                                                                                                           |
| `WrenSchemes.h`    | The app's own URL schemes.                                                                                                                                                       |
| `WrenWebView.h`    | Pages that are not tabs.                                                                                                                                                         |
| `WrenMenus.h`      | The main menu and context menus, from the app's items.                                                                                                                           |
| `WrenUnstableUI.h` | Chrome's sentences and the layout of three bubbles. It may change in any release. See [compatibility](compatibility.md).                                                         |

A release has them in `include`, with a module map that names the module
`CWren`.

## A minimal app

```objc
#import <AppKit/AppKit.h>
#include "Wren.h"

static WrenProfile* g_profile;
static WrenBrowser* g_browser;
static NSWindow* g_window;

static void OnBrowserCreated(void* context, WrenBrowser* browser, WrenResult result) {
  if (!browser) {
    Wren_Quit();
    return;
  }
  g_browser = browser;  // one reference, ours to release
  g_window = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 1100, 760)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskResizable
                  backing:NSBackingStoreBuffered
                    defer:NO];
  NSView* content = g_window.contentView;
  Wren_AttachBrowserToView(browser, (__bridge void*)content);
  NSView* page = (__bridge NSView*)Wren_GetTabView(Wren_GetActiveTab(browser));
  [content addSubview:page];
  page.frame = content.bounds;
  page.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [g_window makeKeyAndOrderFront:nil];
}

static void OnProfileOpened(void* context, WrenProfile* profile, WrenResult result) {
  if (!profile) {
    Wren_Quit();
    return;
  }
  g_profile = profile;  // one reference, ours to release
  Wren_CreateBrowser(profile, "https://example.com", OnBrowserCreated, NULL);
}

static void OnReady(void* context) {
  Wren_OpenProfile(NULL, OnProfileOpened, NULL);
}

// Required. This app shows one window, so it closes what Chromium opens.
static void OnBrowserOpened(void* context, WrenBrowser* browser, WrenBrowserType type,
                            WrenBrowser* opener) {
  Wren_CloseBrowser(browser);
  Wren_ReleaseBrowser(browser);
}

static void OnChanged(void* context, const WrenChange* changes, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (changes[i].kind != WREN_CHANGE_TAB ||
        !(changes[i].tab_fields & WREN_TAB_FIELD_TITLE)) {
      continue;
    }
    char title[256];
    Wren_CopyTabTitle(changes[i].tab, title, sizeof(title));
    g_window.title = @(title);
  }
}

static void OnBrowserClosed(void* context, WrenBrowser* browser) {
  if (browser == g_browser) {
    Wren_Quit();
  }
}

int main(void) {
  WrenStartup startup = {
      .api_version = WREN_API_VERSION,
      .unstable_ui_version = WREN_UNSTABLE_UI_VERSION,
      .user_data_dir = "/Users/me/Library/Application Support/MyBrowser",
      .keychain = WREN_KEYCHAIN_MOCK,
  };
  WrenAppDelegate delegate = {
      .on_ready = OnReady,
      .on_browser_opened = OnBrowserOpened,
      .on_changed = OnChanged,
      .on_browser_closed = OnBrowserClosed,
  };
  WrenExit status = Wren_Main(&startup, &delegate);
  if (g_browser) {
    Wren_ReleaseBrowser(g_browser);
  }
  if (g_profile) {
    Wren_ReleaseProfile(g_profile);
  }
  return status == WREN_EXIT_NORMAL ? 0 : (int)status;
}
```

Compile it against the release `wren install` linked, and bundle the result. The
project's `wren-app.json` names the release, as in
[Getting started](getting-started.md).

```sh
wren install
xcrun clang -fobjc-arc -I .wren/runtime/include -F .wren/runtime \
  -framework "Wren Framework" -framework AppKit main.m -o MyBrowser
wren bundle --executable MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
```

## Conventions

### Threads and timing

- Call everything on the main thread, after `on_ready`. Only `Wren_Main`,
  `Wren_GetAPIVersion` and `Wren_GetVersionInfo` may be called before. The four
  functions that format and parse group and split IDs may be called on any
  thread at any time.
- Completion callbacks, such as those of `Wren_OpenProfile` and
  `Wren_CreateBrowser`, run later from the event loop. They never run inside the
  call that started them.
- `on_changed` and most other delegate callbacks run from the event loop, not
  inside a Chromium notification. So they may call any function.
- `on_popover`, `on_fullscreen`, `on_downloads_prompt` and `on_quit_prompt` are
  the exceptions. They run inside Chromium's call. Until they return, calls that
  change Browsers, tabs or popovers return `WREN_ERROR_NOT_ALLOWED`.
- `on_profile_extensions` runs inside Chromium's call too, as a Profile loads.
  It makes no Wren calls.
- Closing a tab or a Browser, and `Wren_Quit`, are requests that take effect
  after the caller returns. Other calls take effect before they return, and the
  matching `on_changed` record follows.

### Handles

`WrenProfile*` and `WrenBrowser*` are reference-counted.

- A callback that creates or opens one gives you one reference. Those are
  `WrenProfileCallback`, `WrenBrowserCallback` and `on_browser_opened`. Release
  it with `Wren_ReleaseProfile` or `Wren_ReleaseBrowser`.
- A handle passed to any other callback, or returned by a `Get` function, is
  borrowed. Use it during the call, and retain it to keep it.
- A handle stays valid until its last reference is released, even after what it
  names has closed. Calls on it then return `WREN_ERROR_CLOSED` or do nothing.
- One Chromium Profile or Browser always has the same handle, so handles compare
  with `==`.

`WrenWebView*` is not counted. You own it from `Wren_CreateWebView` until
`Wren_DestroyWebView`.

Tabs, tab groups, splits and popover requests are plain values: `WrenTabID`,
`WrenTabGroupID`, `WrenSplitID` and `WrenPopoverID`. There is nothing to
release, and an ID is never reused while the process runs. A group's or a
split's ID has a text form, Chromium's, for a page or a file. See
`Wren_FormatTabGroupID` and `Wren_ParseTabGroupID`, and the same for a split.

### Views

A view is passed as `void*`, and is an `NSView*`. Views Wren returns are
borrowed, as from `Wren_GetTabView`, `Wren_GetSidePanelView` and
`Wren_GetWebViewNativeView`. Wren holds the views you pass weakly, as to
`Wren_AttachBrowserToView` and `Wren_SetAnchor`.

### Strings

Strings are UTF-8. A string you pass in need only be valid during the call.

Strings come out through `Copy` functions. Each fills your buffer, and returns
the size needed, including the terminating NUL. To read one of any length, ask
first:

```c
size_t needed = Wren_CopyTabURL(tab, NULL, 0);
char* url = malloc(needed);
Wren_CopyTabURL(tab, url, needed);
```

Images come out the same way, as PNG bytes. The function returns the PNG's size,
and copies it only if the buffer is large enough. 0 means there is no image.

### Results

Functions that can fail return a `WrenResult`. `WREN_OK` is 0. `Wren.h` explains
the other values beside the enum.

### State structs and changes

State is read into a struct you supply, as with `Wren_GetTabState` and
`Wren_GetSidePanelState`. `on_changed` says what to read again. Each
`WrenChange` has a kind, and for a tab, the bits of `tab_fields` that changed.
Records are valid only during the call.

### The API version

Set `WrenStartup.api_version` to `WREN_API_VERSION`, and
`WrenStartup.unstable_ui_version` to `WREN_UNSTABLE_UI_VERSION`. `Wren_Main`
returns `WREN_EXIT_API_MISMATCH` if either differs from the framework's. That
happens when an app is bundled with a release other than the one it was compiled
against. What each version promises is in [compatibility](compatibility.md).

## Binding another language

The API is meant to be bound. It has plain C types, no macros that matter beyond
constants, callbacks with a `void* context`, and no ownership rules beyond the
two above.

A release has two bindings, which show the patterns. They are the Swift API, in
`wren-swift` under `Sources/Wren`, and the Rust crate, in `wren-rust`. The
crate's `wren::sys` is the C API as bindgen made it. Both bindings use the same
patterns:

- a class or an owner for each counted handle
- a value for each ID
- a closure boxed as a callback's context
- `Copy` functions wrapped as strings and data

C, Swift and Rust are supported.

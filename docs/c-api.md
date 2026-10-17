# The C API

Wren's API is a C API. The Swift API and the Rust crate are layers over it with
the same functions under their own names, so everything in these guides can be
done from C, Objective-C or C++, or from any language that can call C.

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
| `WrenSession.h`    | The last session, restoring it, reopening closed tabs.                                                                                                                           |
| `WrenSchemes.h`    | The app's own URL schemes.                                                                                                                                                       |
| `WrenWebView.h`    | Pages that are not tabs.                                                                                                                                                         |
| `WrenMenus.h`      | The main menu and context menus, from the app's items.                                                                                                                           |
| `WrenUnstableUI.h` | Chrome's sentences and the layout of three bubbles; it may change in any release ([compatibility](compatibility.md)).                                                            |

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
      .profile_directory = "Default",
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

Compile it against the release `wren install` linked (its `wren-app.json` names
the release, as in [Getting started](getting-started.md)) and bundle the result:

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
  `Wren_GetAPIVersion` and `Wren_GetVersionInfo` may be called before.
- Completion callbacks (`Wren_OpenProfile`, `Wren_CreateBrowser`, …) run later
  from the event loop, never inside the call that started them.
- `on_changed` and the other delegate callbacks run from the event loop, not
  inside a Chromium notification, so they may call any function. `on_popover`
  and `on_fullscreen` are the exceptions: they run inside Chromium's call, and
  calls that change Browsers, tabs or popovers return `WREN_ERROR_NOT_ALLOWED`
  until they return.
- Closing a tab or a Browser, and `Wren_Quit`, are requests that take effect
  after the caller returns. Other calls take effect before they return, and the
  matching `on_changed` record follows.

### Handles

`WrenProfile*` and `WrenBrowser*` are reference-counted.

- A callback that **creates or opens** one (`WrenProfileCallback`,
  `WrenBrowserCallback`, `on_browser_opened`) hands you one reference. Release
  it with `Wren_ReleaseProfile` or `Wren_ReleaseBrowser`.
- A handle **passed to any other callback or returned by a `Get` function** is
  borrowed: use it during the call, and retain it to keep it.
- A handle stays valid until its last reference is released, even after what it
  names has closed. Calls on it then return `WREN_ERROR_CLOSED` or do nothing.
- One Chromium Profile or Browser always has the same handle, so handles compare
  with `==`.

`WrenWebView*` is not counted: you own it from `Wren_CreateWebView` to
`Wren_DestroyWebView`.

Tabs, tab groups, splits and popover requests are plain values (`WrenTabID`,
`WrenTabGroupID`, `WrenSplitID`, `WrenPopoverID`). There is nothing to release,
and an ID is never reused while the process runs. A group's or a split's ID has
a text form, Chromium's, for a page or a file (`Wren_FormatTabGroupID`,
`Wren_ParseTabGroupID`, and the same for a split).

### Views

A view is passed as `void*` and is an `NSView*`. Views Wren returns
(`Wren_GetTabView`, `Wren_GetSidePanelView`, `Wren_GetWebViewNativeView`) are
borrowed. Views you pass (`Wren_AttachBrowserToView`, `Wren_SetAnchor`) are held
weakly.

### Strings

Strings are UTF-8. A string you pass in need only be valid during the call.

Strings come out through `Copy` functions, which fill your buffer and return the
size needed, including the terminating NUL. To read one of any length, ask
first:

```c
size_t needed = Wren_CopyTabURL(tab, NULL, 0);
char* url = malloc(needed);
Wren_CopyTabURL(tab, url, needed);
```

Images come out the same way, as PNG bytes: the function returns the PNG's size
and copies it only if the buffer is large enough. 0 means there is none.

### Results

Functions that can fail return a `WrenResult`; `WREN_OK` is 0. The other values
are explained beside the enum in `Wren.h`.

### State structs and changes

State is read into a struct you supply (`Wren_GetTabState`,
`Wren_GetSidePanelState`, …). `on_changed` says what to read again: each
`WrenChange` has a kind, and for a tab the bits of `tab_fields` that changed.
Records are valid only during the call.

### The API version

Set `WrenStartup.api_version` to `WREN_API_VERSION`, and
`WrenStartup.unstable_ui_version` to `WREN_UNSTABLE_UI_VERSION`. `Wren_Main`
returns `WREN_EXIT_API_MISMATCH` if either differs from the framework's, which
happens when an app is bundled with a release other than the one it was compiled
against. What each version promises is in [compatibility](compatibility.md).

## Binding another language

The API is meant to be bound: plain C types, no macros that matter beyond
constants, callbacks with a `void* context`, and no ownership rules beyond the
two above. A release has two bindings, which show the patterns: the Swift API
(`wren-swift`, under `Sources/Wren`) and the Rust crate (`wren-rust`, whose
`wren::sys` is the C API as bindgen made it): a class or an owner per counted
handle, a value per ID, a closure boxed as a callback's context, and `Copy`
functions wrapped as strings and data.

C, Swift and Rust are supported.

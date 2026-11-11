# The C API

Underneath the Swift SDK and the Rust SDK, Wren Runtime is a C API, so you can use it directly from C, Objective-C or C++, or wrap it in any language that can call C. Everything in these guides works from C, because the SDKs call the same functions. This page explains the rules every function follows, so the headers make sense.

The headers are the reference, and their comments are the full contract. A release has them in `include`, with a module map that names the module `CWren`. Each Swift and Rust item's comment names the C function it calls.

## A first app in Objective-C

This is the app from [Getting started](getting-started.md), in Objective-C:

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

static void OnClosed(void* context, WrenResult result) {}

// Required. This app shows one window, so it closes what Chromium opens.
static void OnBrowserOpened(void* context, WrenBrowser* browser, WrenBrowserType type,
                            WrenBrowser* opener) {
  Wren_CloseBrowsers(&browser, 1, OnClosed, NULL);
  Wren_ReleaseBrowser(browser);
}

static void OnChanged(void* context, const WrenChange* changes, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (changes[i].kind != WREN_CHANGE_TAB ||
        !(changes[i].tab_fields & WREN_TAB_FIELD_TITLE)) {
      continue;
    }
    char title[256];
    Wren_CopyTabText(changes[i].tab, WREN_TAB_TEXT_TITLE, title, sizeof(title));
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

Compile it against the release `wren install` linked, then bundle it, with a `wren-app.json` as in Getting started:

```sh
wren install
xcrun clang -fobjc-arc -I .wren/runtime/include -F .wren/runtime \
  -framework "Wren Framework" -framework AppKit main.m -o MyBrowser
wren bundle --executable MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
```

## The headers

`Wren.h` includes all the others, so it's the only one to include.

| Header | What's in it |
| --- | --- |
| `Wren.h` | Shared types and results, startup, changes, your app's callbacks, `Wren_Main`, the versions, and work from other threads |
| `WrenBrowser.h` | Profiles and their password manager, importing history and cookies, Browsers, several Browsers in one window, full screen and side panels |
| `WrenTabs.h` | Tabs and navigation, tab groups, a tab's view, splits, DevTools, and what a tab is doing |
| `WrenExtensions.h` | Extensions' buttons, a Profile's installed extensions, installing from the Chrome Web Store, and your app's own |
| `WrenToolbar.h` | Where bubbles open from, page actions and site setting icons |
| `WrenDownloads.h` | A Profile's downloads and download history, the download button, and the prompts before closing or quitting |
| `WrenUI.h` | Chromium's popovers as requests, and the state behind them |
| `WrenPrompts.h` | Chromium's prompts and toasts as data, and your app's own toasts |
| `WrenOmnibox.h` | What the address field shows, and Chromium's suggestions |
| `WrenBookmarks.h` | A Profile's bookmarks |
| `WrenSession.h` | The last session, restoring it, and reopening closed tabs |
| `WrenSchemes.h` | Your own URL schemes |
| `WrenWebView.h` | Web views, and messages with their pages |
| `WrenMenus.h` | The main menu and context menus |
| `WrenClipboard.h` | Text and URLs on the system clipboard |
| `WrenApp.h` | Quitting, Warn Before Quitting and your own warning, relaunching, your updates on Chromium's About page, and being the default browser |
| `WrenUnstableUI.h` | Chrome's own sentences and the layout of three bubbles, which can change in any release. See [Compatibility](compatibility.md). |

## The rules every function follows

### Threads and timing

- **Call everything on the main thread, after `on_ready`.** The exceptions are `Wren_Main`, `Wren_GetAPIVersion`, `Wren_GetVersionInfo`, the functions that format and parse group and split IDs, and `Wren_PostToMainThread`.
- **`Wren_PostToMainThread` works from any thread,** from just before `on_ready` until the event loop ends. It runs your function on the main thread, from the event loop, in the order each thread posted. Work still waiting when the event loop ends gets its `discard` function instead, on the main thread, before `Wren_Main` returns.
- **Completion callbacks run later,** from the event loop, never inside the call that started them. That goes for `Wren_OpenProfile`, `Wren_CreateBrowser` and the rest.
- **Most delegate callbacks run from the event loop,** including `on_changed`, `on_reopen` and `on_open_urls`, so they can call any function.
- **Some run inside Chromium's call.** `on_popover`, `on_fullscreen`, `on_downloads_prompt`, `on_quit_prompt`, `on_quit_committed` and `on_navigation` run while Chromium waits. Until they return, calls that change Browsers, tabs, navigation or popovers return `WREN_ERROR_NOT_ALLOWED`. `on_profile_extensions` also runs inside Chromium's call, and can make no Wren Runtime calls at all.
- **Closing is a request.** `Wren_CloseTab`, `Wren_CloseTabs`, `Wren_CloseBrowsers` and `Wren_Quit` take effect after you return, and a page can cancel them. Every other call takes effect before it returns, and its `on_changed` record follows later.

### Handles

`WrenProfile*` and `WrenBrowser*` are reference-counted.

- **A callback that creates or opens one gives you a reference.** Those are `WrenProfileCallback`, `WrenBrowserCallback` and `on_browser_opened`. Release it with `Wren_ReleaseProfile` or `Wren_ReleaseBrowser`.
- **Every other handle is borrowed,** whether a callback passed it or a `Get` function returned it. Use it during the call, and retain it to keep it.
- **A handle stays valid until you release it,** even after what it names has closed. Calls on it then return `WREN_ERROR_CLOSED` or do nothing.
- **One Profile or Browser always has the same handle,** so you can compare handles with `==`.

`WrenWebView*` isn't counted. You own it from `Wren_CreateWebView` until `Wren_DestroyWebView`.

Tabs, tab groups, splits and popover requests are plain values: `WrenTabID`, `WrenTabGroupID`, `WrenSplitID` and `WrenPopoverID`. There's nothing to release, and Chromium never reuses an ID while the process runs. Group and split IDs also have a text form, for a page or a file, from `Wren_FormatTabGroupID` and `Wren_ParseTabGroupID`, and their split versions.

### Views

A view is passed as a `void*`, and is an `NSView*`. Views you get back, such as from `Wren_GetTabView`, `Wren_GetSidePanelView` and `Wren_GetWebViewNativeView`, are borrowed. Wren Runtime holds the views you pass in weakly, such as to `Wren_AttachBrowserToView` and `Wren_SetAnchor`.

### Strings and images

Strings are UTF-8. A string you pass in only needs to be valid during the call.

Strings come out through `Copy` functions, which fill your buffer and return the size needed, including the final NUL. To read a string of any length, ask for its size first:

```c
size_t needed = Wren_CopyTabText(tab, WREN_TAB_TEXT_URL, NULL, 0);
char* url = malloc(needed);
Wren_CopyTabText(tab, WREN_TAB_TEXT_URL, url, needed);
```

When something has more than one string, one call copies them all, and an enum says which. A tab's title, URL and media texts come from `Wren_CopyTabText`, with `WREN_TAB_TEXT_TITLE` and the other kinds of `WrenTabText`. A download, a popover request and a Profile's bookmarks work the same way. A kind that doesn't apply, such as a card's name on a passwords prompt, copies "" and returns 1, and so does a value outside the enum.

Images come out the same way, as PNG bytes. The function returns the PNG's size, and copies it only if your buffer is big enough. 0 means there's no image.

### Results

Functions that can fail return a `WrenResult`, where `WREN_OK` is 0. `Wren.h` explains each other value beside the enum.

### State and changes

You read state into a struct you supply, as with `Wren_GetTabState` and `Wren_GetSidePanelState`. `on_changed` tells you what to read again. Each `WrenChange` has a kind, and for a tab, the bits of `tab_fields` that changed. The records are valid only during the call.

### Versions

Set `WrenStartup.api_version` to `WREN_API_VERSION`, and `WrenStartup.unstable_ui_version` to `WREN_UNSTABLE_UI_VERSION`. If either differs from the framework's, `Wren_Main` returns `WREN_EXIT_API_MISMATCH`, which happens when an app is bundled with a different release from the one it was compiled against. [Compatibility](compatibility.md) says what each version promises.

## Wrapping it in another language

The C API is designed to be wrapped. It uses plain C types, has no macros that matter beyond constants, gives every callback a `void* context`, and has no ownership rules beyond the two above.

The Swift SDK, in a release's `wren-swift` under `Sources/Wren`, and the Rust SDK, in `wren-rust`, show how. The Rust SDK's `wren::sys` is the C API as bindgen made it. Both follow the same patterns:

- a class or an owner for each counted handle
- a value type for each ID
- a closure boxed as a callback's context
- `Copy` functions wrapped to return strings and data

C, Swift and Rust are the supported languages.

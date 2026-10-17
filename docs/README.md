# Building an app on Wren

Wren puts Chromium's browser layer in your app's process. Your app owns its
windows and draws its UI; Chromium supplies the pages and everything a browser
keeps behind them. These guides are for writing such an app with a Wren release,
on macOS on Apple silicon, in Swift, Rust or C.

## Start here

1. [Getting started](getting-started.md): a first app, from an empty folder to a
   window showing a page.
2. [How a Wren app works](app-model.md): the few ideas everything else rests on.
   Read it before writing more than the first app.

## Then, by what you are building

**A browser**

- [Windows and tabs](windows-and-tabs.md): placing tab views, popups, DevTools,
  split tabs, side panels, full screen.
- [The UI a browser provides](ui-checklist.md): every piece of browser UI, and
  for each whether you reuse Chromium's or draw your own.
- [Startup](startup.md): the data directory, the keychain, the main menu, and
  your name and icon in Chromium's own pages.
- [Menus](menus.md): the main menu and context menus from a list of items, with
  Chromium's commands and the system's own among yours.

**Web pages in an app that is not a browser**

- [Adding web pages to an existing app](embedding.md): what changes in your app,
  and which of Wren's two kinds of page to use.
- [Web views](web-views.md): a page in your window that is not a tab.
- [Views in a web view](views-in-web-views.md): a web view's page showing tabs,
  with their DevTools, and side panels inside itself.
- [Messages between a page and your app](messages.md): a web view's page calling
  your code, and hearing from it.
- [Anchors in a web view](anchors-in-web-views.md): Chromium's bubbles hanging
  off the buttons of a toolbar drawn in a page.
- [Your own URL schemes](schemes.md): serving your pages from your app's code
  and resources.

**Shipping it**

- [The `wren` command](wren-cli.md): bundling an app, signing it, and having
  Apple notarise it.

## Reference

- [The C API](c-api.md): the conventions of the headers, for C, Objective-C, or
  a binding to another language.
- [Compatibility](compatibility.md): what may change between releases, and which
  framework an app runs with.
- The headers themselves say what each function does:
  [`Wren.h`](../include/Wren.h) (startup,
  changes, the app's callbacks),
  [`WrenBrowser.h`](../include/WrenBrowser.h)
  (Profiles, Browsers, full screen, side panels),
  [`WrenTabs.h`](../include/WrenTabs.h)
  (tabs, groups, splits, DevTools),
  [`WrenExtensions.h`](../include/WrenExtensions.h),
  [`WrenToolbar.h`](../include/WrenToolbar.h)
  (anchors, page actions, content setting icons),
  [`WrenDownloads.h`](../include/WrenDownloads.h)
  (downloads, the download button, the prompts before closing or quitting),
  [`WrenUI.h`](../include/WrenUI.h)
  (popovers and the facts behind them),
  [`WrenUnstableUI.h`](../include/WrenUnstableUI.h)
  (Chrome's sentences and the layout of its bubbles; unstable),
  [`WrenOmnibox.h`](../include/WrenOmnibox.h)
  (the address field and suggestions),
  [`WrenSession.h`](../include/WrenSession.h)
  (restoring sessions and closed tabs),
  [`WrenSchemes.h`](../include/WrenSchemes.h),
  [`WrenWebView.h`](../include/WrenWebView.h)
  and
  [`WrenMenus.h`](../include/WrenMenus.h)
  (the main menu and context menus). A release has them in its `include` folder.
- The Swift API has every function of the C API under a Swift name; its sources
  are in a release's `wren-swift` package, under `Sources/Wren`.
- The Rust crate `wren` has every function under the same names, in Rust's case;
  its sources are in a release's `wren-rust` folder.
- [This release](this-release.md): its version, its Chromium, what it does not
  do yet, and its known issues.

## What to expect

Wren is pre-1.0. An app must be bundled with the release it was compiled
against: the framework refuses any other API version, and nothing yet promises
that an app built for one release runs on the next.
[This release](this-release.md) lists what is missing (infobars among them) and
the known issues.

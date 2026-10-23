# Building an app on Wren

Wren puts Chromium's browser layer in your app's process. Your app owns its
windows and draws its UI. Chromium supplies the pages, and everything a browser
keeps behind them. These guides are for writing such an app with a Wren release,
on macOS on Apple silicon, in Swift, Rust or C.

## Start here

1. [Getting started](getting-started.md) makes a first app, from an empty folder
   to a window showing a page.
2. [How a Wren app works](app-model.md) has the few ideas everything else rests
   on, and says what Wren is and is not. Read it before writing more than the
   first app.

## Then, by what you are building

### A browser

- [Windows and tabs](windows-and-tabs.md) covers placing tab views, dragging
  tabs, popups, DevTools, split tabs, side panels and full screen.
- [The UI a browser provides](ui-checklist.md) lists every piece of browser UI,
  and says for each whether you reuse Chromium's or draw your own.
- [A browser drawn in HTML](html-browser.md) is for a browser whose whole window
  is one web page, over a native host in Rust or Swift.
- [Startup](startup.md) covers the data directory, the keychain, the main menu,
  and your name and icon in Chromium's own pages.
- [Menus](menus.md) makes the main menu and context menus from a list of items,
  with Chromium's commands and the system's own among yours. It also shows how
  to ask before quitting.
- [Extensions your app ships](extensions.md) installs extensions of your own in
  the Profiles you choose, on or off, where the user cannot remove them.

### Web pages in an app that is not a browser

- [Adding web pages to an existing app](embedding.md) says what changes in your
  app, and which of Wren's two kinds of page to use.
- [Web views](web-views.md) covers a page in your window that is not a tab, and
  its DevTools.
- [Views in a web view](views-in-web-views.md) covers a web view's page that
  shows tabs, with their DevTools, and side panels inside itself.
- [Messages between a page and your app](messages.md) covers a web view's page
  calling your code, and hearing from it.
- [Anchors in a web view](anchors-in-web-views.md) hangs Chromium's bubbles off
  the buttons of a toolbar drawn in a page, extensions' buttons among them.
- [Your own URL schemes](schemes.md) serves your pages from your app's code and
  resources.

### Shipping it

- [The `wren` command](wren-cli.md) bundles an app, signs it, and has Apple
  notarise it.
- [Getting started](getting-started.md#before-you-ship) lists what a shipped app
  sets so Chromium's UI shows its name and not Chromium's.

## Reference

- [The C API](c-api.md) explains the conventions of the headers, for C,
  Objective-C, or a binding to another language.
- [Compatibility](compatibility.md) says what may change between releases, and
  which framework an app runs with.
- [This release](this-release.md) gives its version and its Chromium, and says
  what it does not do yet and its known issues.

The headers say what each function does. A release has them in its `include`
folder.

| Header                                                                                   | What is in it                                                          |
| ---------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- |
| [`Wren.h`](../include/Wren.h)                     | What Wren is, startup, changes, the app's callbacks                    |
| [`WrenBrowser.h`](../include/WrenBrowser.h)       | Profiles, Browsers, full screen, side panels                           |
| [`WrenTabs.h`](../include/WrenTabs.h)             | Tabs, groups, splits, DevTools                                         |
| [`WrenExtensions.h`](../include/WrenExtensions.h) | Extensions' actions, the installed extensions, and the app's own       |
| [`WrenToolbar.h`](../include/WrenToolbar.h)       | Anchors, page actions, content setting icons                           |
| [`WrenDownloads.h`](../include/WrenDownloads.h)   | Downloads, the download button, the prompts before closing or quitting |
| [`WrenUI.h`](../include/WrenUI.h)                 | Popovers, and the facts behind them                                    |
| [`WrenUnstableUI.h`](../include/WrenUnstableUI.h) | Chrome's sentences and the layout of its bubbles, which are unstable   |
| [`WrenOmnibox.h`](../include/WrenOmnibox.h)       | The address field and its suggestions                                  |
| [`WrenBookmarks.h`](../include/WrenBookmarks.h)   | A Profile's bookmarks                                                  |
| [`WrenSession.h`](../include/WrenSession.h)       | Restoring sessions, and closed tabs                                    |
| [`WrenSchemes.h`](../include/WrenSchemes.h)       | The app's own URL schemes                                              |
| [`WrenWebView.h`](../include/WrenWebView.h)       | Web views, and messages with their pages                               |
| [`WrenMenus.h`](../include/WrenMenus.h)           | The main menu and context menus                                        |

The Swift API, in a release's `wren-swift` package under `Sources/Wren`, has
every function of the C API under a Swift name. The Rust crate `wren`, in a
release's `wren-rust` folder, has every function under the Swift name in Rust's
case. In both, each item's comment says what it does and names its C function.
So a search for a C name in either finds its wrapper, and a search for a Swift
or Rust name finds its C name.

## What to expect

Wren is before 1.0. An app must be bundled with the release it was compiled
against. The framework refuses any other API version, and nothing yet promises
that an app built for one release runs on the next.
[This release](this-release.md) lists what is missing, infobars among them, and
the known issues.

# Wren Runtime guides

These guides take you from a first window to a browser you can ship, built on Wren Runtime with the Swift SDK, the Rust SDK or the C API. They also cover adding web pages to an app that isn't a browser.

The examples are in Swift, and many show the same code in Rust. Each guide ends with a table of the names it uses in Swift, Rust and C.

## New to Wren Runtime

Start with [Getting started](getting-started.md). It builds an app with one window showing a web page in a real Chromium tab, from three short files.

Then read [How a Wren Runtime app works](app-model.md). It explains Profiles, Browsers, tabs and changes, which every other guide builds on. [Profiles and their data](profiles.md) goes further into Profiles: making them, clearing their data, and bringing data over from another browser.

## Building a browser

| Guide | What it shows |
| --- | --- |
| [Windows and tabs](windows-and-tabs.md) | Showing tabs in your own windows under your own tab strip, with popups, DevTools, split tabs, side panels and full screen |
| [Workspaces](workspaces.md) | One window that holds several complete browsers, such as workspaces for work and home, each with its own tabs and Profile |
| [The browser's UI, piece by piece](ui-checklist.md) | Every piece of Chrome's UI, and whether you can reuse Chromium's or draw your own |
| [A browser drawn in HTML](html-browser.md) | A browser whose whole window is one web page, over a small native host |
| [Startup](startup.md) | Where your data lives, which main menu you get, and your name and logo in Chromium's own pages |
| [Menus](menus.md) | The main menu and context menus, with Chrome's commands among yours, and a prompt before quitting |
| [Extensions your app ships](extensions.md) | Extensions that come with your app, in the Profiles you choose |

## Adding web pages to an app

| Guide | What it shows |
| --- | --- |
| [Web pages in an existing app](embedding.md) | What changes in your app, and which kind of page to use |
| [Web views](web-views.md) | A page in your window that isn't a tab, such as a panel or a whole window of UI |
| [Messages between a page and your app](messages.md) | A web view's page calling your code, and your code telling the page what changed |
| [Views in a web view](views-in-web-views.md) | Tabs and side panels shown inside a web view's page |
| [Anchors in a web view](anchors-in-web-views.md) | Chromium's bubbles opening from buttons your page draws |
| [Your own URL schemes](schemes.md) | Pages served from your app's code and resources, at addresses such as `app://settings` |

## Shipping

| Guide | What it shows |
| --- | --- |
| [The `wren` command](wren-cli.md) | Bundling your executable into an app, signing it, and notarising it |
| [Updating your app with Sparkle](updates.md) | Updates through Sparkle, with their progress on Chromium's About page |
| [Before you ship](getting-started.md#before-you-ship) | What to set so Chromium's pages show your app's name instead of Chromium's |

## Reference

- [The C API](c-api.md) explains how the headers work, for apps in C or Objective-C, and for wrapping Wren Runtime in another language.
- [Compatibility](compatibility.md) says what can change between releases, and how to move your app across each breaking change.
- [This release](this-release.md) has the release notes: what's new, what breaks, what's fixed, and the known issues.
- [What Wren Runtime can't do yet](limitations.md) lists the parts of Chrome that aren't available yet.

The headers document every function in full. A release has them in its `include` folder.

| Header | What's in it |
| --- | --- |
| [`Wren.h`](../include/Wren.h) | Starting Wren Runtime, your app's callbacks, changes, and work from other threads |
| [`WrenBrowser.h`](../include/WrenBrowser.h) | Profiles, Browsers, full screen and side panels |
| [`WrenTabs.h`](../include/WrenTabs.h) | Tabs, groups, splits and DevTools |
| [`WrenExtensions.h`](../include/WrenExtensions.h) | Extensions' buttons, the installed extensions, and your app's own |
| [`WrenToolbar.h`](../include/WrenToolbar.h) | Where bubbles open from, page actions and site setting icons |
| [`WrenDownloads.h`](../include/WrenDownloads.h) | Downloads, the download button, and the prompts before closing or quitting |
| [`WrenUI.h`](../include/WrenUI.h) | Chromium's popovers, and the state behind them |
| [`WrenUnstableUI.h`](../include/WrenUnstableUI.h) | Chrome's own sentences and bubble layouts, which can change in any release |
| [`WrenPrompts.h`](../include/WrenPrompts.h) | Chromium's prompts and toasts as data, and your app's own toasts |
| [`WrenOmnibox.h`](../include/WrenOmnibox.h) | The address field and its suggestions |
| [`WrenBookmarks.h`](../include/WrenBookmarks.h) | A Profile's bookmarks |
| [`WrenSession.h`](../include/WrenSession.h) | Restoring the last session, and closed tabs |
| [`WrenSchemes.h`](../include/WrenSchemes.h) | Your own URL schemes |
| [`WrenWebView.h`](../include/WrenWebView.h) | Web views, and messages with their pages |
| [`WrenMenus.h`](../include/WrenMenus.h) | The main menu and context menus |
| [`WrenClipboard.h`](../include/WrenClipboard.h) | Text and URLs on the system clipboard |
| [`WrenApp.h`](../include/WrenApp.h) | Quitting and its warning, relaunching, your app's updates on Chromium's About page, and being the default browser |

The Swift SDK and the Rust SDK cover every function in the headers. Each Swift and Rust item's comment names the C function it wraps, so a search for either name finds the other.

## Before you start

Wren Runtime isn't at 1.0 yet. Your app runs only with the release it was built against, and a new release can change the API. [Compatibility](compatibility.md) lists every change that needs work in your app.

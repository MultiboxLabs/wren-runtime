# A browser drawn in HTML

A browser's whole window can be one web page. The tab strip, the toolbar and the
address field are HTML. The tabs' pages are elements of that page, and
Chromium's own bubbles hang off its buttons. The page is the part you can carry
to another platform.

This page shows how the pieces fit, as Wren's web sample has them. The sample is
in a release's `samples` folder, to read: a Rust host in `host`, and a React
page in `ui`. Each piece has its own guide, linked below. What works in this
release is in [This release](this-release.md).

The app has two sides, as an Electron app has a renderer and a main process.

|          | Is                                              | Written in                    |
| -------- | ----------------------------------------------- | ----------------------------- |
| The page | Your UI, in a [web view](web-views.md)          | HTML, CSS and your own script |
| The app  | The window, and the answers to the page's calls | Native code, on Wren's API    |

Wren puts none of its API in the page. The two sides talk through
[`wren`](messages.md), on names you choose.

## The window

One web view fills the window. It is made with the options the page needs:

```swift
let webView = WebView(
    profile: uiProfile, url: "app://ui/index.html", kind: .browserUi,
    embedsViews: true,      // <wren-view>: tabs and side panels in the page
    messages: true,         // wren.invoke and wren.on
    anchors: true,          // marks that Chromium's bubbles hang off
    dragsWindow: true)!     // app-region: drag moves the window
window.contentView!.addSubview(webView.view)
browser.attach(to: webView.view)
```

In Rust:

```rust
let web_view = WebView::new(
    &ui_profile,
    "app://ui/index.html",
    &WebViewOptions {
        kind: WebViewKind::BrowserUi,
        embeds_views: true,
        messages: true,
        anchors: true,
        drags_window: true,
        ..Default::default()
    },
)
.expect("a web view");
// Add web_view.view() to the window's content view with objc2, then:
browser.attach_to_view(Some(web_view.view()));
```

- **Attach the Browser to the web view's own view.** That makes the window the
  Browser's. The Browser is active while the window is key, and its dialogs are
  centred over its tab's element.
- **Give the UI a Profile of its own.** The sample makes one ephemeral Profile
  at startup, with `WrenApp.createEphemeralProfile`, for every window's page. So
  the UI's storage is not among the user's sites, and a window that shows
  several Profiles belongs to none of them.
- **Serve the page from [your scheme](schemes.md),** from files in your bundle.
- **The title bar is the page's.** Make the window with a hidden title bar, and
  mark what moves it in CSS. See [dragging](web-views.md#dragging-the-window).
  The page cannot know where the window's own buttons are, so tell it how much
  room to leave.

## The page shows, the app does

Chromium owns the tabs. The app reads them when the page asks or a change
arrives, and keeps no copy. The page keeps the last copy it was sent, to draw
from. A click changes nothing in the page. It calls the app, and the page hears
the result as a change, the way it hears every change.

```tsx
// The page
<div className="tab" onPointerDown={() => wren.invoke("tabs.activate", tab.id)}>…</div>
<wren-view data-id={`tab-${active}`} />
```

```swift
// The app, in Swift
try webView.handle("tabs.activate") { (id: Int32) -> Void in try tab(id).activate() }
// …and in AppDelegate.changed, for this window's Browser:
case .tabs: try? webView.send("tabs.changed", tabList())
```

```rust
// The app, in Rust
web_view.handle("tabs.activate", move |id: i32| {
    BrowserTab::from_id(mt, id).ok_or(Error::TabUnavailable)?.activate()
})?;
// …and in App::changed, for this window's Browser:
ChangeKind::Tabs => { let _ = web_view.send("tabs.changed", &tab_list()); }
```

- **The page asks first.** Nothing is kept for a page that is not there. So the
  page's first act is a call for everything, and the app sends changes from then
  on.
- **Check every ID a call names** against the window's own Browser.
- **Send icons inside messages,** as `data:` URLs. A favicon is a few kilobytes.
  So send a tab whole once, and then only what changed.
- **Answer Chromium's questions first.** What Chromium asks inside its own call,
  such as `popover` and `fullscreen`, the app answers there. Then it tells the
  page.

Keep the whole contract, every name, in one file that types the page's code.
Wren's sample has `contract.ts`. Keep the app's side of it in code that uses
nothing of the platform's. The sample's is a Rust crate, `bridge`. Then both
carry to the next platform with the page.

## Each piece of the browser

[The UI a browser provides](ui-checklist.md) lists them all. In a page:

| Piece                                                                     | The page                                                                                     | The app                                                                                                               |
| ------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| Tabs' pages, side panels                                                  | A [`<wren-view>`](views-in-web-views.md) by ID. Two elements and its own divider for a split | Sends the IDs                                                                                                         |
| Chromium's bubbles, such as page info, permission prompts and the star    | A [mark](anchors-in-web-views.md) over each button                                           | Names each mark once as the Browser's anchor, and shows a bubble when the page calls                                  |
| Extensions' buttons                                                       | A mark over each, named for its action, drawn while pinned or popped out                     | Names `extension:<id>` for each, and sends the actions' state                                                         |
| A popover of your own in place of Chromium's, such as an extensions panel | Draws it from what the app sends, and tells the app when it closes                           | Answers `popover` with the app's for that kind, sends its data, and closes the request                                |
| The address field                                                         | An `<input>` and a list, from Chromium's location state and suggestions                      | Forwards the query and the list ([`WrenOmnibox.h`](../include/WrenOmnibox.h))  |
| A crashed tab, the status bubble                                          | Its own HTML over the tab's element                                                          | Sends the tab's state                                                                                                 |
| Full screen                                                               | Hides its bars for the tab in full screen                                                    | Takes the window in and out, and says which tab                                                                       |
| Context menus                                                             | Says what the items are, and waits for the choice                                            | Passes them to [`showContextMenu`](menus.md#context-menus) and answers with the choice                                |
| The main menu                                                             | Nothing                                                                                      | [Data](menus.md#the-main-menu), through `setMainMenu`. Keys a page does not use go to it                              |
| The page's own DevTools                                                   | A menu item, or a key                                                                        | `openDevTools()` and `closeDevTools()` on the web view, by `state().devtools`. See [Web views](web-views.md#devtools) |

## While you build it

Serve the page from a development server. Have a development build of the app
load it from there, and [trust its origin](web-views.md#trusted-origins):

```swift
let webView = WebView(profile: uiProfile, url: "http://localhost:5173/", …,
                      trustedOrigins: ["http://localhost:5173"])!
```

In Rust, put the origin in `WebViewOptions::trusted_origins`.

The page then has everything it has on your scheme, and an edit shows without a
rebuild. Trust nothing in a build you ship. The sample reads its server's
address only when it is ad hoc signed.

## What is still native

A host for another platform keeps the page, and writes the window and the
handlers again, with the same calls of Wren's in that platform's language. The
main menu and a right click's menu are [data](menus.md), shown by Wren.

# Messages between a page and your app

The page of a [web view](web-views.md) and your app's native code can call each
other, as a page and the main process do in Electron with `ipcRenderer` and
`ipcMain`. The page has one object, `wren`; your app has a handler and a way to
send. Wren carries the messages and defines nothing on the channel: every name
is yours, and nothing of Wren's own API is in the page. The API is in
[`WrenWebView.h`](../include/WrenWebView.h)
("Messages"); what works in this release is in [This release](this-release.md).

## For pages you trust, and no others

This channel is for **your own UI**: pages you wrote and serve yourself. A page
that has `wren` can call every handler you set, with any value, so it has
whatever power those handlers give. Wren checks where a call comes from; it
cannot check what the call means.

- Give `messages` only to a view that shows your own pages. A view that shows a
  website has no `wren` on that site, and should not be made with `messages` at
  all.
- Do not use it to give websites, extensions or other people's content a way to
  reach your app. Wren has no channel for untrusted pages yet (see "What is
  missing"); until it has, such content gets Chromium's own mechanisms and
  nothing of yours.

## Which pages have it

By default, only pages on [your own schemes](schemes.md), in a view made with
`messages`. The one setting that widens this is the view's list of
[trusted origins](web-views.md#trusted-origins), for a development server that
serves your UI while you build it:

|       | Where                                                                                          |
| ----- | ---------------------------------------------------------------------------------------------- |
| Swift | `WebView(profile:url:messages: true, trustedOrigins: ["http://localhost:5173"])`               |
| Rust  | `WebViewOptions { messages: true, trusted_origins: vec!["http://localhost:5173".into()], .. }` |
| C     | `WrenWebViewOptions.messages`, with `trusted_origins` and `trusted_origin_count`               |

Every page that origin serves, in that view, then has `wren`, so list nothing in
a build you ship.

## The app's side

Make the view with `messages`, then say what each call does:

```swift
struct TabInfo: Encodable { let id: Int32; let title: String }
struct Navigation: Decodable { let id: Int32; let url: String }

let view = WebView(profile: profile, url: "app://ui/window.html", messages: true)!

func tabList() -> [TabInfo] {
    browser.tabs.map { TabInfo(id: $0.id, title: $0.title) }
}

// A call from the page. What the closure returns is the page's answer; a
// throw fails the call. It may be async.
try view.handle("tabs.list") { () -> [TabInfo] in tabList() }
try view.handle("tabs.navigate") { (call: Navigation) -> Void in
    guard let tab = browser.tabs.first(where: { $0.id == call.id }) else { throw NoSuchTab() }
    try tab.navigate(to: call.url)
}
try view.handle("tabs.new") { try browser.newTab() }  // no value, no answer: null

// A message to the page, here from the app's own change callback.
delegate.changed = { changes in
    if changes.contains(where: { $0.kind == .tabs || $0.kind == .tab }) {
        try? view.send("tabs.changed", tabList())
    }
}
```

In C there is one handler for the view, which gets every call with its name:
`WrenWebViewOptions.messages`, `Wren_SetPageCallHandler`,
`Wren_CopyPageCallName`, `Wren_CopyPageCallValue`, then `Wren_AnswerPageCall` or
`Wren_FailPageCall`; and `Wren_SendPageMessage`. Values are JSON text there, for
your own parser.

## The page's side

`wren` is there before the page's first script runs. There is nothing to load or
import:

```ts
type Tab = { id: number; title: string };

let tabs = await wren.invoke<Tab[]>("tabs.list");
wren.on("tabs.changed", (next: Tab[]) => render((tabs = next)));

try {
  await wren.invoke("tabs.navigate", {
    id: tabs[0].id,
    url: "https://example.com",
  });
} catch (error) {
  // The handler threw, or the app has no handler for this name.
}
```

That is all of it:

|                             |                                                                                                                    |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `wren.invoke(name, value?)` | Calls the app. A promise of its answer; it rejects with an `Error` if the handler fails the call or there is none. |
| `wren.on(name, listener)`   | Hears the app's messages of that name. Returns a function that stops.                                              |

For TypeScript, a release has the types in `page/wren.d.ts`; add it to your
page's project, with the DOM in its `lib`. It also types the elements Wren gives
a page, `<wren-view>` (`HTMLWrenViewElement`) and a mark's `name`, for the DOM.
A JSX library keeps its own list of elements: declare `wren-view` there, typed
by `HTMLWrenViewElement`.

## Rules

- **Values are what JSON holds**: objects, arrays, strings, numbers, booleans
  and null. You pass and get ordinary values on both sides; a `Date` arrives as
  a string, and a `Map`, binary data or a value over 1 MB does not cross. Serve
  bytes from [your scheme](schemes.md) instead.
- **Only the view's own page has `wren`.** It belongs to the main frame of a
  view made with `messages`, while that page is on one of your schemes or a
  listed origin. A tab, even on your scheme, a frame inside the page, and a view
  that has gone anywhere else have none: `typeof wren === "undefined"`.
- **Treat a call's value as input.** It comes from a page. Check an ID before
  acting on it, as a scheme's handler checks a path: the page is yours, but a
  script injected into it calls with the same right.
- **Answer every call once**, in the handler or later. In Swift, returning or
  throwing is the answer.
- **A call ends with its page.** If the page reloads, goes elsewhere or the view
  is destroyed before you answer, the call is over: answering throws `.closed`,
  and the page's promise is never settled.
- **Nothing is kept for a page that is not there.** `send` throws `.closed`
  while the view's page is still loading or has no channel. A page that has
  called you is there, so have the page ask first and send from then on.
- **Handlers run from the event loop**, on the main thread, outside Chromium's
  own notifications, so they may call any of Wren's API.

## What is missing

- **A channel for pages you do not trust.** Websites, extensions and other
  content that should reach a browser's own features need a different, narrower
  mechanism, built on what Chromium already has for them (extension APIs and the
  like). It is not designed or planned yet.

- A tab on your scheme (a New Tab page, a settings page) has no `wren`; it can
  still `fetch()` your scheme.
- Calls are asynchronous only, and there is no binary value.
- A page cannot answer what Chromium asks your app inside its own call (a
  popover request, full screen): answer in your app, then tell the page.

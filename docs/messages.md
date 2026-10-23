# Messages between a page and your app

The page of a [web view](web-views.md) and your app's native code can call each
other, as a page and the main process do in Electron with `ipcRenderer` and
`ipcMain`. The page has one object, `wren`. Your app has a handler and a way to
send. Wren carries the messages and defines none of them. Every name is yours.
Besides the channel, the page has one function of Wren's own,
[`wren.pathForFile`](#files-the-user-drops).

The API is in the "Messages" section of
[`WrenWebView.h`](../include/WrenWebView.h).
What works in this release is in [This release](this-release.md). The
[table at the end](#the-names-in-swift-rust-and-c) gives each name in Swift,
Rust and C.

## For pages you trust, and no others

This channel is for your own UI, pages you wrote and serve yourself. A page that
has `wren` can call every handler you set, with any value. So it has whatever
power those handlers give. Wren checks where a call comes from, but it cannot
check what the call means.

- Give `messages` only to a view that shows your own pages. A view that shows a
  website has no `wren` on that site, and should not be made with `messages` at
  all.
- Do not use it to give websites, extensions or other people's content a way to
  reach your app. Wren has no channel for untrusted pages yet, as
  [What is missing](#what-is-missing) says. Until it has, such content gets
  Chromium's own mechanisms and nothing of yours.

## Which pages have it

By default, only pages on [your own schemes](schemes.md), in a view made with
`messages`, have `wren`. The one setting that widens this is the view's list of
[trusted origins](web-views.md#trusted-origins). It is for a development server
that serves your UI while you build it:

|       | Where                                                                                          |
| ----- | ---------------------------------------------------------------------------------------------- |
| Swift | `WebView(profile:url:messages: true, trustedOrigins: ["http://localhost:5173"])`               |
| Rust  | `WebViewOptions { messages: true, trusted_origins: vec!["http://localhost:5173".into()], .. }` |
| C     | `WrenWebViewOptions.messages`, with `trusted_origins` and `trusted_origin_count`               |

Every page that origin serves, in that view, then has `wren`. So list nothing in
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

The same app in Rust, with serde for the values:

```rust
#[derive(Serialize)]
struct TabInfo { id: i32, title: String }
#[derive(Deserialize)]
struct Navigation { id: i32, url: String }

let view = WebView::new(
    &profile,
    "app://ui/window.html",
    &WebViewOptions { messages: true, ..Default::default() },
)
.expect("a web view");

// What the closure returns is the page's answer; an Err fails the call.
view.handle("tabs.list", move |()| Ok::<_, Error>(tab_list(&browser)))?;
view.handle("tabs.navigate", move |call: Navigation| {
    BrowserTab::from_id(mt, call.id).ok_or(Error::TabUnavailable)?.navigate(&call.url)
})?;

// In App::changed:
let _ = view.send("tabs.changed", &tab_list(&browser));
```

`handle_call` takes a `PageCall` to answer later instead, and `set_call_handler`
gets every call with no handler of its own.

In C there is one handler for the view, which gets every call with its name. The
calls are `Wren_SetPageCallHandler`, `Wren_CopyPageCallName` and
`Wren_CopyPageCallValue`, then `Wren_AnswerPageCall` or `Wren_FailPageCall`, and
`Wren_SendPageMessage` to send. Values are JSON text there, for your own parser.

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

| The page calls              | It does                                                                                                                       |
| --------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `wren.invoke(name, value?)` | Calls the app. Returns a promise of its answer, which rejects with an `Error` if the handler fails the call or there is none. |
| `wren.on(name, listener)`   | Hears the app's messages of that name. Returns a function that stops listening.                                               |
| `wren.pathForFile(file)`    | Gives the path on disk of a file the user dropped on the page. See [below](#files-the-user-drops).                            |

For TypeScript, a release has the types in `page/wren.d.ts`. Add it to your
page's project, with the DOM in its `lib`. It also types the elements Wren gives
a page, `<wren-view>` (`HTMLWrenViewElement`) and a mark's `name`, for the DOM.
A JSX library keeps its own list of elements. Declare `wren-view` there, typed
by `HTMLWrenViewElement`.

## Files the user drops

A page gets a file dropped on it from Finder or another app as a DOM `File`,
which has a name and contents but no path. `wren.pathForFile` gives the path. So
your app can do with the file what Chrome does, and open it in a tab:

```ts
strip.addEventListener("dragover", (event) => event.preventDefault());
strip.addEventListener("drop", (event) => {
  event.preventDefault();
  const paths = [...event.dataTransfer!.files]
    .map((file) => wren.pathForFile(file))
    .filter((path) => path !== null);
  void wren.invoke("tabs.openFiles", { paths, index: dropIndex(event) });
});
```

- **It is synchronous,** and needs no handler in your app.
- **It returns `null` for a file that is not on disk,** such as a `File` or
  `Blob` the page made or fetched. Anything that is not a `Blob` throws a
  `TypeError`.
- **Wren opens nothing.** Your app makes the `file://` URL and decides what
  opens. In Swift that is `URL(fileURLWithPath:)`, and in Rust
  `url::Url::from_file_path`. Chrome opens only what it can show. A type
  Chromium cannot show becomes a download.
- **It works only where `wren` is.** A website, in a web view or a tab, never
  learns a path, as in Chrome.
- **A drop the page leaves opens nothing** in a web view. The page leaves a drop
  when it does not call `preventDefault`.

## Rules

- **Values are what JSON holds:** objects, arrays, strings, numbers, booleans
  and null. You pass and get ordinary values on both sides. A `Date` arrives as
  a string. A `Map`, binary data, or a value over 1 MB does not cross. Serve
  bytes from [your scheme](schemes.md) instead.
- **Only the view's own page has `wren`.** It belongs to the main frame of a
  view made with `messages`, while that page is on one of your schemes or a
  listed origin. A tab, even on your scheme, has none. Neither does a frame
  inside the page, or a view that has gone anywhere else. There,
  `typeof wren === "undefined"`.
- **Treat a call's value as input.** It comes from a page. Check an ID before
  acting on it, as a scheme's handler checks a path. The page is yours, but a
  script injected into it calls with the same right.
- **Answer every call once,** in the handler or later. In Swift and Rust,
  returning or throwing is the answer.
- **A call ends with its page.** If the page reloads or goes elsewhere, or the
  view is destroyed, before you answer, the call is over. Answering then throws
  `.closed`, and the page's promise never settles.
- **Nothing is kept for a page that is not there.** `send` throws `.closed`
  while the view's page is still loading or has no channel. A page that has
  called you is there. So have the page ask first, and send from then on.
- **Handlers run from the event loop,** on the main thread, outside Chromium's
  own notifications. So they may call any of Wren's API.

## What is missing

- **A channel for pages you do not trust.** Websites, extensions and other
  content that should reach a browser's own features need a different, narrower
  mechanism. It would be built on what Chromium already has for them, such as
  extension APIs. It is not designed or planned yet.
- **`wren` in a tab.** A tab on your scheme, such as a New Tab page or a
  settings page, has no `wren`. It can still `fetch()` your scheme.
- **Synchronous calls and binary values.** Calls are asynchronous only, and
  values are JSON.
- **Page answers to Chromium.** A page cannot answer what Chromium asks your app
  inside its own call, such as a popover request or full screen. Answer in your
  app, then tell the page.

## The names in Swift, Rust and C

| Swift                    | Rust                              | C                                                 |
| ------------------------ | --------------------------------- | ------------------------------------------------- |
| `messages: true`         | `WebViewOptions::messages`        | `WrenWebViewOptions.messages`                     |
| `view.handle(_:)`        | `WebView::handle`, `handle_call`  | `Wren_SetPageCallHandler`                         |
| `PageCall`               | `PageCall`                        | `WrenPageCall`                                    |
| `call.name`, `call.json` | `PageCall::name`, `json`          | `Wren_CopyPageCallName`, `Wren_CopyPageCallValue` |
| `call.answer`            | `PageCall::answer`, `answer_json` | `Wren_AnswerPageCall`                             |
| `call.fail`              | `PageCall::fail`                  | `Wren_FailPageCall`                               |
| `view.send`              | `WebView::send`, `send_json`      | `Wren_SendPageMessage`                            |

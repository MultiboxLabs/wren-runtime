# Messages between a page and your app

A [web view](web-views.md)'s page and your app's native code can call each other, much as `ipcRenderer` and `ipcMain` do in Electron. The page calls your app with `wren.invoke` and hears from it with `wren.on`. Your app answers with handlers, and talks to the page with `send`. Every message's name is yours, and Wren Runtime carries the messages without reading them.

The code here is in Swift and Rust for your app, and TypeScript for the page. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Swift, Rust and C.

## Only for your own pages

Messages are for your own UI, pages you wrote and serve yourself. A page with `wren` can call every handler you set, with any value, so it has whatever power those handlers give. Wren Runtime checks where a call comes from, but it can't check what the call means.

- **Give `messages` only to a web view that shows your own pages.** Pages on [your URL schemes](schemes.md) have `wren`, and websites the web view goes to don't.
- **Don't use messages to let websites, extensions or other people's content reach your app.** Wren Runtime has no channel for pages you don't trust yet.

While you develop, a web view's [trusted development server](web-views.md#trusting-a-development-server) gets `wren` too. List nothing there in a build you ship.

## Your app's side

Make the web view with `messages`, then say what each call does:

```swift
struct TabInfo: Encodable { let id: Int32; let title: String }
struct Navigation: Decodable { let id: Int32; let url: String }

let view = WebView(profile: profile, url: "app://ui/window.html", messages: true)!

func tabList() -> [TabInfo] {
    browser.tabs.map { TabInfo(id: $0.id, title: $0.title) }
}

// A call from the page. What the closure returns is the page's answer, and a
// throw fails the call. It may be async.
try view.handle("tabs.list") { () -> [TabInfo] in tabList() }
try view.handle("tabs.navigate") { (call: Navigation) -> Void in
    guard let tab = browser.tabs.first(where: { $0.id == call.id }) else {
        throw WrenError.tabUnavailable
    }
    try tab.navigate(to: call.url)
}
try view.handle("tabs.new") { try browser.newTab() }  // no value, and null for an answer

// A message to the page, here from the app's own change callback.
delegate.changed = { changes in
    if changes.contains(where: { $0.kind == .tabs || $0.kind == .tab }) {
        try? view.send("tabs.changed", tabList())
    }
}
```

The same in Rust, with serde for the values:

```rust
#[derive(Serialize)]
struct TabInfo {
    id: i32,
    title: String,
}
#[derive(Deserialize)]
struct Navigation {
    id: i32,
    url: String,
}

fn tab_list(browser: &Browser) -> Vec<TabInfo> {
    browser.tabs().into_iter().map(|tab| TabInfo { id: tab.id(), title: tab.title() }).collect()
}

let view = WebView::new(
    &profile,
    "app://ui/window.html",
    &WebViewOptions { messages: true, ..WebViewOptions::default() },
)
.expect("a web view");

// What the closure returns is the page's answer, and an Err fails the call.
let listed = browser.clone();
view.handle("tabs.list", move |()| Ok::<_, Error>(tab_list(&listed)))?;
view.handle("tabs.navigate", move |call: Navigation| {
    BrowserTab::from_id(mt, call.id).ok_or(Error::TabUnavailable)?.navigate(&call.url)
})?;

// A message to the page, such as from App::changed.
view.send("tabs.changed", &tab_list(&browser))?;
```

In Rust, `handle_call` gives you a `PageCall` to answer later instead, and `set_call_handler` gets every call that has no handler of its own.

## The page's side

`wren` is there before the page's first script runs, with nothing to load or import:

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

That's all of it:

| The page calls | What it does |
| --- | --- |
| `wren.invoke(name, value?)` | Calls your app, and returns a promise of its answer. The promise rejects with an `Error` if the handler fails, or if there's no handler for the name. |
| `wren.on(name, listener)` | Listens for your app's messages with that name, and returns a function that stops listening. |
| `wren.pathForFile(file)` | Gives the path on disk of a file someone dropped on the page. See [Files dropped on the page](#files-dropped-on-the-page). |

A release has TypeScript types for all of it in `page/wren.d.ts`. Add the file to your page's project, with the DOM in its `lib`. It also types `<wren-view>`, as `HTMLWrenViewElement`, and the `name` attribute of [anchors](anchors-in-web-views.md). A JSX library keeps its own list of elements, so declare `wren-view` there too, typed as `HTMLWrenViewElement`.

## Files dropped on the page

A file dropped on a page from Finder or another app arrives as a DOM `File`, which has a name and contents but no path. `wren.pathForFile` gives you the path, so your app can do what Chrome does with a dropped file and open it in a tab:

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

- **It answers at once,** and needs no handler in your app.
- **It returns `null` for a file that isn't on disk,** such as a `Blob` the page made or fetched. Anything that isn't a `Blob` throws a `TypeError`.
- **Your app decides what opens.** Make the `file://` URL yourself, such as with `URL(fileURLWithPath:)` in Swift or `url::Url::from_file_path` in Rust. Chrome opens only what it can show, and a type Chromium can't show becomes a download.
- **Only pages with `wren` get paths.** A website never learns a path, in a web view or a tab, as in Chrome.
- **A drop the page doesn't handle opens nothing.** A page handles a drop by calling `preventDefault`.

## Things to know

- **Values are what JSON can hold.** That's objects, arrays, strings, numbers, booleans and null. A `Date` arrives as a string. A `Map`, binary data or a value over 1 MB can't be sent, so serve bytes from [your scheme](schemes.md) instead.
- **Only the web view's own page has `wren`.** A tab doesn't, even on your scheme, and neither does a frame inside your page. There, `typeof wren === "undefined"`.
- **Check what a call sends.** It comes from a page. The page is yours, but a script injected into it would call with the same rights. Check an ID before you act on it, as a scheme handler checks a path.
- **Answer every call once,** now or later. In Swift and Rust, returning or throwing is the answer.
- **A call ends with its page.** If the page reloads or navigates, or the web view ends, before you answer, the call is over. Answering then throws `.closed`, and the page's promise never settles.
- **`send` needs a page to receive it.** While the page is still loading, `send` throws `.closed`, and nothing is kept for later. So have the page call your app first, and send from then on.
- **Messages and answers arrive in the order you send them.** A page that Chromium has frozen gets them once it runs again, still in that order, and none of them runs its script before then.
- **Handlers run from the event loop,** on the main thread, so they can call anything in Wren Runtime.

## Not available yet

- **A channel for pages you don't trust.** Websites, extensions and other content that should reach a browser's own features need a narrower mechanism. It would be built on what Chromium already has for them, such as extension APIs, but it isn't designed yet.
- **`wren` in a tab.** A tab on your scheme, such as a New Tab page or a settings page, has no `wren`. It can still `fetch()` from your scheme.
- **Synchronous calls and binary values.** Calls are only asynchronous, and values are only JSON.
- **Answering Chromium from a page.** A page can't answer what Chromium asks inside its own call, such as a popover request or full screen. Answer in your app, then tell the page.

## The names in Swift, Rust and C

In C, one handler gets every call to a web view, with its name, and values are JSON text for your own parser.

| Swift | Rust | C |
| --- | --- | --- |
| `messages: true` | `WebViewOptions::messages` | `WrenWebViewOptions.messages` |
| `view.handle` | `WebView::handle`, `WebView::handle_call` | `Wren_SetPageCallHandler` |
| `PageCall` | `PageCall` | `WrenPageCall` |
| `call.name`, `call.json` | `PageCall::name`, `PageCall::json` | `Wren_CopyPageCallText` |
| `call.answer` | `PageCall::answer`, `PageCall::answer_json` | `Wren_AnswerPageCall` |
| `call.fail` | `PageCall::fail` | `Wren_FailPageCall` |
| `view.send` | `WebView::send`, `WebView::send_json` | `Wren_SendPageMessage` |

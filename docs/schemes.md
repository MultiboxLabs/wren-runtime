# Your own URL schemes

Your app can serve pages from its own code and resources, at addresses on a scheme of its own, such as `app://settings/`. It's like Electron's `protocol` module. A page on your scheme loads in a tab or a web view like any other, with its scripts, styles, images and `fetch()` calls, and your code answers each request.

Use a scheme for your browser's own pages, such as a New Tab page or settings, or for the UI of an app in a [web view](web-views.md). The code here is in Swift. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## Declaring a scheme and answering it

Chromium fixes its list of schemes as it starts, so you declare yours in `Startup`, and can't add one later. Once Chromium is running, you set a handler, which you can replace at any time.

```swift
var startup = Startup(userDataDirectory: directory, keychain: .system)
startup.schemes = [Scheme("app", privileges: [.standard, .secure, .fetch])]

var delegate = AppDelegate(browserOpened: { _, _, _ in })
delegate.ready = {
    try! WrenApp.handleScheme("app") { request in
        guard let url = URL(string: request.url), url.host == "ui" else {
            return .failure(.notFound)
        }
        let file = resources.appendingPathComponent(url.path).standardizedFileURL
        guard file.path.hasPrefix(resources.path + "/") else {
            return .failure(.accessDenied)
        }
        return .file(file.path)
    }
}
```

The handler runs on the main thread, and takes an `async` closure, so it can wait for its own work before it answers. `WrenApp.setSchemeHandler` is the plain form instead. It hands you the request, and you call `request.respond` once, straight away or later.

## What a request says

- `url`, `method`, and `header(_:)` for its headers
- `profile`, the Profile of the page that asked
- `tab`, when a tab's page asked, or `webView`, when a web view's page did
- `isDocument`, which is true when the request loads a page rather than something inside one

A handler can't see a request's body yet.

## How to answer

| Answer | Use it for |
| --- | --- |
| `.file(path)` | A file on disk. Chromium reads it off the main thread, and answers range requests for video and audio. Its type comes from its extension, unless you give a `Content-Type` header. |
| `.data(data, status:, headers:)` | Bytes you made, such as JSON for a `fetch()` or a page you built. Without a `Content-Type`, it's `text/plain`. |
| `.failure(.notFound)`, `.accessDenied`, `.failed` | A request that fails, with no response at all. |

Answer large files with `.file`, so the main thread doesn't read them.

Any page on your scheme, including a tab's, can call your app this way. It calls `fetch("app://ui/api/status")`, and your handler answers with JSON. A [web view](web-views.md)'s page has a better way, which also lets your app talk back: [messages](messages.md).

## Privileges

| Privilege | What it gives |
| --- | --- |
| `.standard` | URLs with a host and a path, as `https` has. Relative URLs work, each host is its own origin, and pages have web storage. Nearly every scheme wants it. |
| `.secure` | Pages are secure contexts, and an `https` page can load them without a mixed-content warning. |
| `.fetch` | The scheme's own pages can `fetch()` its URLs. |
| `.webAccessible` | Other pages can link to the scheme, frame its pages, and load its scripts and images. |

## Keeping your scheme safe

A scheme is closed by default. Your handler gets requests only for what your app or the person using it opens, such as a tab's address or a typed URL, and for what the scheme's own pages ask for. A website can't link to `app://settings/`, frame it, or load its scripts and images, and neither can an extension or one of your other schemes. Those requests fail before your handler sees them.

- **Give each of your browser's own pages a host,** such as `app://settings/` and `app://newtab/`, on a closed scheme.
- **Keep paths inside their folder.** Wren Runtime serves whatever file your handler names, so a handler that maps URLs to files has to keep the path inside its own folder, as above. Wren Runtime refuses a path with a `..` in it.
- **Think twice about `.webAccessible`.** It opens your scheme to every page, as an `https` site is open. Other pages can show its images, scripts and frames, but can't read its responses. Your handler can't tell who asked, so it mustn't act on a request just because it arrived.
- **Chromium's own schemes stay Chromium's.** You can't declare `https`, `file`, `chrome`, `chrome-extension`, `about`, `data` and the like.

## Not available yet

A request's body, streamed responses, redirects, service workers and workers started from your scheme, and CORS for other origins.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `Startup.schemes`, `Scheme` | `Startup::schemes`, `Scheme` | `WrenStartup.schemes`, `WrenScheme` |
| `WrenApp.handleScheme` | `MainThread::set_scheme_handler` | `Wren_SetSchemeHandler` |
| `WrenApp.setSchemeHandler` | `MainThread::set_scheme_handler` | `Wren_SetSchemeHandler` |
| `SchemeRequest` | `SchemeRequest` | `WrenSchemeRequest` |
| `request.respond` | `SchemeRequest::respond` | `Wren_RespondWithData`, `Wren_RespondWithFile`, `Wren_FailSchemeRequest` |
| `.file`, `.data` | `SchemeResponse::File`, `SchemeResponse::data` | `Wren_RespondWithFile`, `Wren_RespondWithData` |
| `.standard`, `.secure`, `.fetch`, `.webAccessible` | `SchemePrivileges::STANDARD` and the rest | `WREN_SCHEME_STANDARD` and the rest |

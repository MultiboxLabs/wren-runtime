# Your own URL schemes

A Wren app can declare URL schemes and answer their requests itself, as
Electron's `protocol` module lets an app do. `app://ui/index.html` then loads in
a tab from your code, with its scripts, styles, images and `fetch()` calls. The
API is in
[`WrenSchemes.h`](../include/WrenSchemes.h);
what works in this release is in [This release](this-release.md).

Use a scheme for the browser's own pages (a New Tab page, settings), or for the
UI of an app that is not a browser, shown in a [web view](web-views.md).

## Declare, then handle

Chromium fixes its list of schemes as it starts, in every process, so schemes
are part of the startup policy and cannot be added later. A handler is set once
Chromium is running, and can be replaced at any time.

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

In C: `WrenStartup.schemes`, `Wren_SetSchemeHandler`, and one of
`Wren_RespondWithData`, `Wren_RespondWithFile` or `Wren_FailSchemeRequest` for
each request.

## Answering a request

A handler gets a `SchemeRequest` and answers it with a `SchemeResponse`:

| Answer                                            | For                                                                                                                                      |
| ------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `.file(path)`                                     | A file on disk. Chromium reads it off the main thread; its type comes from the file's extension unless you give a `Content-Type` header. |
| `.data(data, status:, headers:)`                  | Bytes you built: JSON for a `fetch()`, a generated page. Without a `Content-Type` it is `text/plain`.                                    |
| `.failure(.notFound)`, `.accessDenied`, `.failed` | A load that fails, with no response at all.                                                                                              |

The request says what is asked and who asks: `url`, `method`, `header(_:)`,
`profile`, `tab` (when a tab's page asks), `webView` (when a web view's does)
and `isDocument` (the request loads a page, not something inside one).

`handleScheme` takes an `async` closure that returns the response, so it can
await its own work first. `setSchemeHandler` is the plain form: it hands you the
request and you call `request.respond` once, then or later.

That gives any page on the scheme, a tab's too, a way to call the app: the page
does `fetch("app://ui/api/…")` and the handler answers with JSON. A
[web view](web-views.md)'s page has a better one, which the app can also push
through: [messages](messages.md).

## Privileges

| Privilege     | What it gives                                                                                                                                                 |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| standard      | URLs with a host and a path, as `https` has, so relative URLs resolve, each host is its own origin, and pages have web storage. Nearly every scheme wants it. |
| secure        | Pages are secure contexts, and an `https` page may load them without a mixed-content warning.                                                                 |
| fetch         | The scheme's own pages may `fetch()` its URLs.                                                                                                                |
| webAccessible | Other pages may link to the scheme, frame its pages and load its scripts and images. Without it the scheme is closed to them.                                 |

## Rules

- **One handler per scheme, for the whole app.** The request names its Profile
  and, when a tab's page asks, the tab.
- **Answer every request once**, inside the handler or later. The handler runs
  on the main thread, so answer large files with a file path: Chromium reads
  them off the main thread and answers Range requests for media.
- **You choose what is served.** Wren serves whatever file the handler names. A
  handler that maps URLs to files must keep the path inside its own directory;
  Wren refuses a path with a `..` component.
- **A scheme is closed to other pages.** Your handler is asked only for what the
  app or the user opens (a tab's URL, a typed address) and for what the scheme's
  own pages ask. A website cannot link to `app://settings/`, frame it, or load
  its scripts and images, and neither can an extension or another of your
  schemes: the request fails and the handler never sees it. Put the browser's
  own pages (its UI, its settings) on a scheme like this, with a host for each
  (`app://settings/`, `app://newtab/`), since a link from one scheme to another
  is refused too.
- **`webAccessible` opens a scheme** to every page, as an `https` site is open:
  they may link to it and show its images, scripts and frames. They still cannot
  read its responses; only pages of the same origin can, and there is no CORS
  for an app's scheme yet. A handler for an open scheme cannot tell who asked,
  so it must not act on a request just because it arrived.
- **Chromium's own schemes stay Chromium's**: `https`, `file`, `chrome`,
  `chrome-extension`, `about`, `data` and the like cannot be declared.

## Not offered yet

A request's body, responses that stream, redirects, service workers and workers
started from a scheme's URL, and CORS from other origins.

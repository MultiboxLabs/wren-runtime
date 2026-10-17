# Anchors in a web view

Chromium hangs some of its own UI off a browser's toolbar: permission prompts
and page info off the site icon, an extension's popup off its button, the
bookmark and zoom bubbles off the address field. An app with native UI tells
Wren which of its views play those parts
([the UI a browser provides](ui-checklist.md)). When your toolbar is a page in a
[web view](web-views.md), the parts are elements of that page: your page marks
them, and your app says what each mark is for. The API is in
[`WrenWebView.h`](../include/WrenWebView.h)
("Anchors in a web view") and `Wren_SetAnchorElement` in
[`Wren.h`](../include/Wren.h); what works
in this release is in [This release](this-release.md).

## The page's side

A mark is an `<embed>` of Wren's type, with a name you choose, laid over the
button by your CSS:

```html
<span class="anchored">
  <button id="site-icon">ⓘ</button>
  <embed type="application/x-wren-anchor" name="site-icon" />
</span>
```

```css
.anchored {
  position: relative;
  display: inline-block;
}
.anchored > embed {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
}
```

That is all the page does. The mark draws nothing and takes no clicks. There is
no script to write and nothing to send: Chromium's own layout tells Wren where
the element is whenever your page lays out or scrolls, and when the element
goes.

## The app's side

Make the view with `anchors`, attach the Browser to the view, and say which
Browser's anchor each name is:

```swift
let view = WebView(profile: profile, url: "app://ui/window.html", anchors: true)!
browser.attach(to: view.view)

try browser.setAnchor(.location, in: view, element: "site-icon")
try browser.setAnchor(.extensions, in: view, element: "extensions")
try browser.setAnchor(.extensionAction(id), in: view, element: "action-\(id)")
```

In C: `WrenWebViewOptions.anchors`, then
`Wren_SetAnchorElement(browser, "location", view, "site-icon")`. The anchors'
names are listed in
[`WrenToolbar.h`](../include/WrenToolbar.h).

Name an anchor once. The naming lasts across your page's reloads; the bubbles
follow whichever element has the name at the time. A nil name forgets the
anchor. An anchor is one thing at a time: naming an element replaces a view
named with `setAnchor(_:to:)`, and the other way round.

Opening a bubble from a click is your page [calling your app](messages.md), on a
name of your own:

```ts
siteIcon.addEventListener("click", () => wren.invoke("site.info"));
```

```swift
try view.handle("site.info") { () -> Void in
    try browser.activeTab?.showPageInfo()
}
```

A second click on the button closes the bubble, as it does for a native button:
the click itself closes it, and Wren leaves it closed when the call that click
made asks to show it again.

## Which pages may mark anchors

By default, only pages on [your own schemes](schemes.md), in a view made with
`anchors`. The one setting that widens this is the view's list of
[trusted origins](web-views.md#trusted-origins), for a development server that
serves your UI while you build it:

|       | Where                                                                                         |
| ----- | --------------------------------------------------------------------------------------------- |
| Swift | `WebView(profile:url:anchors: true, trustedOrigins: ["http://localhost:5173"])`               |
| Rust  | `WebViewOptions { anchors: true, trusted_origins: vec!["http://localhost:5173".into()], .. }` |
| C     | `WrenWebViewOptions.anchors`, with `trusted_origins` and `trusted_origin_count`               |

Whoever serves a trusted origin, in that view, places your Browser's anchors, so
list nothing in a build you ship.

These have no right, and a mark in them does nothing:

- a view made without `anchors`;
- a frame inside your page;
- a tab, even one on your own scheme;
- your view once it has gone to any other site.

And a mark does nothing until your app names it for a Browser.

## Rules

- **An anchor is no further than the web view.** What lies outside the view is
  cut off. A mark wholly outside it, hidden, removed, or in a view that is not
  in the window its Browser is attached to is no anchor, and bubbles then open
  where they do when you name none: inside the top corners of the active tab.
- **A name is read when the element is made.** To rename a mark, replace the
  element. Of two with one name, the one made last counts.
- **Chromium keeps a bubble on screen and inside the Browser's window.** Near
  the right edge it opens leftwards, and near the bottom of the screen, upwards.
- **Mind your `Content-Security-Policy`.** A mark is a plugin element: a policy
  that forbids `object-src` leaves your page with none.

## What is missing

- A mark moved by an animation your page does not lay out again for (a CSS
  transform) is followed only once it does.
- An open bubble trails a moving button by one message from the page's process.
  It has not been measured.
- Pinch zoom in the UI's page has not been tried.
- An `<embed>` placed inside the `<button>` itself, which HTML's rules do not
  allow, has not been tried; use a wrapper as above.

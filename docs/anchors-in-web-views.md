# Anchors in a web view

Chromium hangs some of its own UI off a browser's toolbar. Permission prompts
and page info hang off the site icon, an extension's popup off its button, and
the bookmark and zoom bubbles off the address field. An app with native UI tells
Wren which of its views play those parts, as
[the UI a browser provides](ui-checklist.md) describes.

When your toolbar is a page in a [web view](web-views.md), the parts are
elements of that page. Your page marks them, and your app says what each mark is
for. The API is in the "Anchors in a web view" section of
[`WrenWebView.h`](../include/WrenWebView.h),
and in `Wren_SetAnchorElement` in
[`WrenToolbar.h`](../include/WrenToolbar.h).
What works in this release is in [This release](this-release.md).

## The page's side

A mark is an `<embed>` of Wren's type, with a name you choose. Your CSS lays it
over the button:

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
no script to write and nothing to send. Chromium's own layout tells Wren where
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

```rust
let view = WebView::new(
    &profile,
    "app://ui/window.html",
    &WebViewOptions { anchors: true, ..Default::default() },
)
.expect("a web view");
browser.attach_to_view(Some(view.view()));

browser.set_anchor_element(&BubbleAnchor::LOCATION, &view, Some("site-icon"))?;
browser.set_anchor_element(&BubbleAnchor::EXTENSIONS, &view, Some("extensions"))?;
browser.set_anchor_element(&BubbleAnchor::extension_action(&id), &view, Some(&format!("action-{id}")))?;
```

In C, set `WrenWebViewOptions.anchors`, then call
`Wren_SetAnchorElement(browser, "location", view, "site-icon")`.
[`WrenToolbar.h`](../include/WrenToolbar.h)
lists the anchors' names.

Name an anchor once. The naming lasts across your page's reloads, and the
bubbles follow whichever element has the name at the time. A nil name forgets
the anchor. An anchor is one thing at a time. Naming an element replaces a view
named with `setAnchor(_:to:)`, and naming a view replaces an element.

Your page opens a bubble from a click by [calling your app](messages.md), on a
name of your own:

```ts
siteIcon.addEventListener("click", () => wren.invoke("site.info"));
```

```swift
try view.handle("site.info") { () -> Void in
    try browser.activeTab?.showPageInfo()
}
```

A second click on the button closes the bubble, as it does for a native button.
The click itself closes the bubble. When the call that click made asks to show
the bubble again, Wren leaves it closed.

## Extensions' buttons

An extension's popup and its dialogs hang off the anchor `extension:<id>`. In
Swift that is `.extensionAction(id)`, and in Rust
`BubbleAnchor::extension_action(&id)`. This is how Wren's web sample does it.

1. **Name every action's anchor,** whether its button shows or not. Name each
   one for an element after the action, such as `action-<id>`. The naming lasts,
   and the mark counts only while the page has the element.
2. **Draw a button, with its mark, for each pinned action.** Also draw one for
   an action while its state's `poppedOut` is true. Chromium sets `poppedOut`
   before it opens the popup, and `changed` reports it first. So the popup opens
   below the button you just drew, as in Chrome.
3. **Draw the button pressed** while `popupOpen` is true, or the anchor's
   `showing` is.

`poppedOut` is only for the popup. Chromium does not pop the button out for the
action's dialogs, such as the one that asks before removing the extension. A
dialog hangs off `extension:<id>` while that mark is placed. Otherwise it hangs
off `extensions`, your extensions button. If you want a dialog under the
action's own button, draw that button yourself while the dialog shows.

The actions' state is `browser.extensionActionState(id)`, and
`.extensionActions` changes report each change.

## Which pages may mark anchors

By default, only pages on [your own schemes](schemes.md), in a view made with
`anchors`, may mark anchors. The one setting that widens this is the view's list
of [trusted origins](web-views.md#trusted-origins). It is for a development
server that serves your UI while you build it:

|       | Where                                                                                         |
| ----- | --------------------------------------------------------------------------------------------- |
| Swift | `WebView(profile:url:anchors: true, trustedOrigins: ["http://localhost:5173"])`               |
| Rust  | `WebViewOptions { anchors: true, trusted_origins: vec!["http://localhost:5173".into()], .. }` |
| C     | `WrenWebViewOptions.anchors`, with `trusted_origins` and `trusted_origin_count`               |

Whoever serves a trusted origin places your Browser's anchors in that view. So
list nothing in a build you ship.

A mark does nothing in these places:

- a view made without `anchors`
- a frame inside your page
- a tab, even one on your own scheme
- your view, once it has gone to any other site

A mark also does nothing until your app names it for a Browser.

## Rules

- **An anchor reaches no further than the web view.** What lies outside the view
  is cut off. A mark is no anchor when it is wholly outside the view, hidden,
  removed, or in a view that is not in the window its Browser is attached to.
  Bubbles then open where they do when you name none, inside the top corners of
  the active tab.
- **Wren reads a name when the element is made.** To rename a mark, replace the
  element. Of two marks with one name, the one made last counts.
- **Chromium keeps a bubble on screen, and inside the Browser's window.** Near
  the right edge it opens leftwards, and near the bottom of the screen, upwards.
- **Mind your `Content-Security-Policy`.** A mark is a plugin element. A policy
  that forbids `object-src` leaves your page with no marks.

## What is missing

- If an animation moves a mark without your page laying out again, as a CSS
  transform does, Wren follows the mark only once the page lays out.
- An open bubble trails a moving button by one message from the page's process.
  It has not been measured.
- Pinch zoom in the UI's page has not been tried.
- An `<embed>` inside the `<button>` itself, which HTML's rules do not allow,
  has not been tried. Use a wrapper, as above.

## The names in Swift, Rust and C

| Swift                              | Rust                                   | C                              |
| ---------------------------------- | -------------------------------------- | ------------------------------ |
| `anchors: true`                    | `WebViewOptions::anchors`              | `WrenWebViewOptions.anchors`   |
| `browser.setAnchor(_:in:element:)` | `Browser::set_anchor_element`          | `Wren_SetAnchorElement`        |
| `browser.setAnchor(_:to:)`         | `Browser::set_anchor`                  | `Wren_SetAnchor`               |
| `browser.anchorState(_:)`          | `Browser::anchor_state`                | `Wren_GetAnchorState`          |
| `.location`, `.extensions`         | `BubbleAnchor::LOCATION`, `EXTENSIONS` | `"location"`, `"extensions"`   |
| `.extensionAction(id)`             | `BubbleAnchor::extension_action`       | `"extension:<id>"`             |
| `browser.extensionActionState(_:)` | `Browser::extension_action_state`      | `Wren_GetExtensionActionState` |
| `tab.showPageInfo()`               | `BrowserTab::show_page_info`           | `Wren_ShowPageInfo`            |

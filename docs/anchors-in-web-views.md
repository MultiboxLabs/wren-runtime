# Anchors in a web view

Chromium opens some of its bubbles from buttons in a browser's toolbar. Page info and permission prompts open from the site icon, an extension's popup from its button, and the bookmark and zoom bubbles from the address field. When your toolbar is a page in a [web view](web-views.md), those buttons are elements of your page, and Chromium's bubbles can open from them just as they would from native buttons. Your page marks each button, and your app says what each mark is for.

The code here is in Swift and Rust for your app, and HTML and CSS for the page. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Swift, Rust and C.

## Marking a button in your page

A mark is an `<embed>` of Wren Runtime's type, with a name you choose, which your CSS lays over the button:

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

That's all your page does. The mark draws nothing, takes no clicks, and needs no script. Chromium's own layout tells Wren Runtime where it is whenever your page lays out or scrolls, and when it goes.

## Naming the marks in your app

Make the web view with `anchors`, attach the Browser to the web view's view, and say which of the Browser's anchors each mark is:

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
    &WebViewOptions { anchors: true, ..WebViewOptions::default() },
)
.expect("a web view");
browser.attach_to_view(Some(view.view()));

browser.set_anchor_element(&BubbleAnchor::LOCATION, &view, Some("site-icon"))?;
browser.set_anchor_element(&BubbleAnchor::EXTENSIONS, &view, Some("extensions"))?;
let action = format!("action-{id}");
browser.set_anchor_element(&BubbleAnchor::extension_action(&id), &view, Some(&action))?;
```

[`WrenToolbar.h`](../include/WrenToolbar.h) lists every anchor.

Name each anchor once. The name lasts across your page's reloads, and bubbles open from whichever element has the name at the time. Naming nil forgets the anchor. An anchor is one thing at a time, so naming an element replaces a native view you named with `setAnchor(_:to:)`, and the other way round.

## Opening a bubble from a click

Your page opens a bubble by [calling your app](messages.md), with a message name of your own:

```ts
siteIcon.addEventListener("click", () => wren.invoke("site.info"));
```

```swift
try view.handle("site.info") { () -> Void in
    try browser.activeTab?.showPageInfo()
}
```

A second click on the button closes the bubble, as with a native button. The click itself closes the bubble, and when the call it makes asks to show the bubble again, Wren Runtime leaves it closed.

## Extensions' buttons

An extension's popup and its dialogs open from the anchor `extension:<id>`, which is `.extensionAction(id)` in Swift. The web sample, in a release's `samples` folder, does it this way:

1. **Name every extension's anchor,** whether its button shows or not, for an element such as `action-<id>`. The name lasts, and counts only while the page has the element.
2. **Draw a button with its mark for each pinned extension,** and for any extension whose `poppedOut` is true. Chromium sets `poppedOut` before it opens the popup, and `changed` reports it first, so the popup opens below the button you just drew, as in Chrome.
3. **Draw the button pressed** while its `popupOpen`, or its anchor's `showing`, is true.

`poppedOut` is only for the popup. For an extension's dialogs, such as the one that asks before removing it, Chromium doesn't pop the button out. A dialog opens from `extension:<id>` while that mark is placed, and otherwise from `extensions`, your extensions button. To open a dialog under the extension's own button, draw that button while the dialog shows.

`browser.extensionActionState(id)` gives each extension's button state, and `.extensionActions` changes report it.

## Things to know

- **Only your own page can mark anchors.** That's the main page of a web view made with `anchors`, while it's on one of [your schemes](schemes.md) or a [trusted development server](web-views.md#trusting-a-development-server). A mark does nothing in a frame inside your page, in a tab, or once the web view has gone to another site. It also does nothing until your app names it.
- **Bubbles stay inside the web view.** A mark that's wholly outside the web view, hidden or removed doesn't count, and neither does one in a web view outside the Browser's window. Bubbles then open where they do with no anchor named, inside the top corners of the active tab.
- **Chromium keeps bubbles on screen,** and inside the Browser's window. Near the right edge, a bubble opens to the left, and near the bottom of the screen, upwards.
- **A mark's name is read once,** when the element is made. To rename a mark, replace the element. If two marks share a name, the newer one counts.
- **Your `Content-Security-Policy` must allow it.** A mark is a plugin element, so a policy that forbids `object-src` leaves your page with no marks.

## Not available yet

- When an animation moves a mark without the page laying out again, such as a CSS transform, Wren Runtime follows the mark only at the next layout.
- An open bubble trails a moving button slightly, by one message from the page's process. The delay hasn't been measured.
- Pinch zoom in your page, and an `<embed>` inside the `<button>` itself, haven't been tried. HTML doesn't allow the second anyway, so use a wrapper, as above.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `anchors: true` | `WebViewOptions::anchors` | `WrenWebViewOptions.anchors` |
| `browser.setAnchor(_:in:element:)` | `Browser::set_anchor_element` | `Wren_SetAnchorElement` |
| `browser.setAnchor(_:to:)` | `Browser::set_anchor` | `Wren_SetAnchor` |
| `browser.anchorState(_:)` | `Browser::anchor_state` | `Wren_GetAnchorState` |
| `.location`, `.extensions` | `BubbleAnchor::LOCATION`, `BubbleAnchor::EXTENSIONS` | `"location"`, `"extensions"` |
| `.extensionAction(id)` | `BubbleAnchor::extension_action` | `"extension:<id>"` |
| `browser.extensionActionState(_:)` | `Browser::extension_action_state` | `Wren_GetExtensionActionState` |
| `tab.showPageInfo()` | `BrowserTab::show_page_info` | `Wren_ShowPageInfo` |

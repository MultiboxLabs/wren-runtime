# Views in a web view

A [web view](web-views.md)'s page can show tabs and side panels inside itself, so your browser's whole window can be one HTML page. Where a native app would place a tab's view, your page writes an element, and Wren Runtime puts the tab's page in it:

```html
<wren-view data-id="tab-5"></wren-view>
<wren-view data-id="sidepanel-2"></wren-view>
```

The tab's page draws in the element, at the element's size, and takes clicks, keys and focus there. Otherwise the element is like any other in your page. Your CSS styles it, and your own HTML can draw over it. It's a stacking context of its own, so HTML after it, or with a higher `z-index`, draws over the tab.

## Showing a tab in your page

Make the web view with `embedsViews`, and tell its page which IDs to show, here in its address:

```swift
let url = "app://ui/window.html?tab=\(tab.id)&browser=\(browser.id)"
let webView = WebView(profile: profile, url: url, embedsViews: true)
browser.attach(to: webView?.view)
```

```rust
let url = format!("app://ui/window.html?tab={}&browser={}", tab.id(), browser.id());
let options = WebViewOptions { embeds_views: true, ..WebViewOptions::default() };
let web_view = WebView::new(&profile, &url, &options).expect("a web view");
browser.attach_to_view(Some(web_view.view()));
```

```css
wren-view {
  display: block;
  border-radius: 12px;
  overflow: hidden;
}
```

Attach the Browser to the web view's own view, as above. The web view's window then counts as the Browser's window, and Chromium centres a tab's dialogs, such as an alert or an HTTP sign-in, over the tab's element, inside the web view.

Give the element a size, such as with `display: block`. Until Wren Runtime defines the element, a moment after your page starts, it's an unknown inline element.

Your page can learn the IDs from its address, as here, or by [asking your app](messages.md), and your app can tell it when they change.

## What an element can show

| `data-id` | What it shows | The native view it stands in for |
| --- | --- | --- |
| `tab-` and a tab's ID | The tab's page, with docked DevTools and its divider, and a dimmed cover while a dialog blocks the tab | `tab.view` |
| `sidepanel-` and a Browser's ID | The Browser's open side panel, or nothing | `browser.sidePanelView` |

Wren Runtime keeps the element filled. Your page has nothing to do when DevTools docks or moves, when Chromium replaces a tab's page, or when another side panel opens.

## Switching tabs

To show another tab, change the element's `data-id`, or keep an element for each tab and show one at a time. A tab's element switches at once, and stays empty until the new page has drawn. A side panel's element keeps the old panel showing until the new one has drawn.

A split needs no element of its own. Show both tabs' elements, with a divider of your own between them. A click or a key in the side that isn't active makes it the active tab, as in a native window, and you hear about it as a change.

`tab.focus()` gives the keyboard to the tab inside the web view, once the element has the tab's page. The page arrives a moment after your page asks for it. Then the tab's `state().inElement` turns true, and `changed` reports `.inElement`, so give a newly active tab the keyboard at that change.

## The pointer over a view

Your page hears the pointer move onto and off a view, as it would for an iframe. The element gets `pointerenter` when the pointer moves onto the tab's page, and `pointerleave` when it moves off, onto your own HTML or onto another element. HTML you draw over the tab counts as your page's, so a card over the tab takes the pointer from the element, and the element gets it back when the pointer returns to the tab. Hover cards, tooltips and a sidebar that hides itself need no native code:

```js
const tabView = document.querySelector('wren-view[data-id^="tab-"]');
tabView.addEventListener("pointerenter", () =>
  sidebar.classList.remove("open"),
);
```

While the pointer is over the tab, the tab's page gets its moves, and your page gets no `pointermove`.

Your page misses one case. When the pointer leaves the web view straight from the tab, such as out of the window, only the tab's page is told. The element keeps the pointer, with no `pointerleave`, until the pointer comes back. Chromium treats an iframe at the edge of Chrome's window the same way. If your UI needs to know, have your app watch for the pointer leaving the window and [tell your page](messages.md).

## Things to know

- **A page is in one place at a time.** While an element shows a tab, `tab.view` is empty, and the same goes for a side panel. When the element goes, or the web view ends, the pages go back to their native views.
- **One element for each ID.** A second element with the same `data-id` takes the tab from the first.
- **An element with nothing to show stays empty,** such as for a closed tab, a closed panel, or an ID that names nothing.
- **The tab is still a tab.** Its ID, history, session and changes stay as they were, and so does everything you do to it through its Browser. Your page and the tab's page can't script each other.
- **Only your own page can show views.** That's the main page of a web view made with `embedsViews`, while it's on one of [your schemes](schemes.md) or a [trusted development server](web-views.md#trusting-a-development-server). Anywhere else, including in a tab or in a frame inside your page, the element shows nothing.
- **Group and split IDs are text in your page.** `String(group)` turns a `TabGroup` into text, and `TabGroup(text)` turns it back, and splits work the same way.
- **Your `Content-Security-Policy` must allow it.** Wren Runtime defines the element with a script it adds to your page, and fills it with plugin elements. A policy that forbids inline scripts or `object-src` leaves the element empty.

## Not available yet

- An open dialog doesn't follow an element that moves without changing size. It catches up at the next resize, or when the window moves.
- Your page isn't told when a side panel opens, or how wide it wants to be. Your app is told, and can [tell the page](messages.md).
- An extension's side panel hasn't been tried in an element.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `embedsViews: true` | `WebViewOptions::embeds_views` | `WrenWebViewOptions.embeds_views` |
| `tab.id` | `BrowserTab::id` | `WrenTabID` |
| `browser.id` | `Browser::id` | `Wren_GetBrowserID` |
| `tab.state().inElement` | `TabState::in_element` | `WrenTabState.in_element` |
| `String(group)`, `TabGroup(text)` | `TabGroup::to_string`, `TabGroup::parse` | `Wren_FormatTabGroupID`, `Wren_ParseTabGroupID` |

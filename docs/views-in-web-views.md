# Views in a web view

The page of a [web view](web-views.md) can show tabs and side panels inside
itself. So a browser's whole window can be one page of HTML. Where an app with
native UI places a view, your page writes an element:

```html
<wren-view data-id="tab-5"></wren-view>
<wren-view data-id="sidepanel-2"></wren-view>
```

Wren fills the element and keeps it filled. The pages are drawn in the element,
at its size, and the user uses them there. Clicks, keys and focus reach them.
The element sits in your page like any other, so your own HTML can be drawn over
it and your CSS styles it. It is a stacking context of its own, with
`isolation: isolate`. HTML after it, or with a higher `z-index`, is drawn over
the page.

The API is in the "Views in a web view" section of
[`WrenWebView.h`](../include/WrenWebView.h).
What works in this release is in [This release](this-release.md).

Make the view with `embedsViews`, and give its page the IDs:

```swift
let url = "app://ui/window.html?tab=\(tab.id)&browser=\(browser.id)"
let webView = WebView(profile: profile, url: url, embedsViews: true)
```

```css
wren-view {
  display: block;
  border-radius: 12px;
  overflow: hidden;
}
```

In Rust, set `WebViewOptions::embeds_views`, and use `BrowserTab::id` and
`Browser::id`. In C, set `WrenWebViewOptions.embeds_views`, and use a tab's
`WrenTabID` and `Wren_GetBrowserID`.

## What each element holds

| `data-id`                       | Holds                                                                                                 | The native view it stands for |
| ------------------------------- | ----------------------------------------------------------------------------------------------------- | ----------------------------- |
| `tab-` and a tab's ID           | The tab's page, docked DevTools beside it with its divider, and a scrim while a dialog blocks the tab | `tab.view`                    |
| `sidepanel-` and a Browser's ID | The Browser's open side panel, or nothing                                                             | `browser.sidePanelView`       |

Your page does nothing when DevTools docks or moves, when a tab's page is
replaced, or when the panel changes. Split tabs need no element of their own.
Show both tabs' elements with your own divider between them.

## Rules

- **Only one page may show views.** That is the main frame of a view made with
  `embedsViews`, while its page is on one of [your schemes](schemes.md) or on
  one of the view's [trusted origins](web-views.md#trusted-origins), which are
  for a development server. To a tab on either, a frame inside the view's page,
  or a view that has gone anywhere else, the element is nothing.
- **A page is in an element or in its native view, never both.** While an
  element shows a tab, `tab.view` is empty, and the same is true of a side
  panel. When the element goes, or the web view ends, the pages go back to their
  native views.
- **One element at a time.** A second element with the same `data-id` takes the
  tab from the first.
- **A split is two elements.** A press or a key in the side that is not the
  active tab makes it active, as in a native window. You hear it as a change,
  and do nothing to make it happen.
- **Switching tabs is yours.** Change the element's `data-id`, or keep one
  element per tab and show one. A tab's element shows a new page at once, and is
  empty until that page has drawn. A side panel's element keeps the old page
  showing until the new one has drawn.
- **An element with nothing to show is empty.** That is so for a closed tab, a
  closed panel, or an ID that names nothing.
- **The tab is still a tab.** Its ID, history, session and changes are as
  before, and so is everything you do to it through its Browser. The pages
  cannot script each other.
- **Focus waits for the page.** `tab.focus()` moves keyboard focus to the tab
  inside the web view once the element has the tab's page. That page arrives a
  moment after your page asks for it. The tab's `state().inElement` turns true
  then, and `changed` reports `.inElement`. Give a newly active tab the keyboard
  at that change.
- **A group's or a split's ID is text for your page.** Use `String(group)` and
  `TabGroup(text)`, and the same for a split. In C these are
  `Wren_FormatTabGroupID` and `Wren_ParseTabGroupID`.
- **Your page learns the IDs from you,** in its address, or by
  [asking your app](messages.md). Your app can also tell it when they change.
- **Give the element a size.** Until Wren has defined the element, a moment
  after your page starts, it is an unknown inline element.
- **Attach the Browser to the web view's own view,** with
  `browser.attach(to: webView.view)`. That makes the window the Browser's, and
  Chromium places its own UI for the tab from that view. A tab's dialogs, such
  as an alert or HTTP sign-in, are then centred over the tab's element and kept
  inside the web view. A Browser attached to another window puts them in that
  window instead.
- **Mind your `Content-Security-Policy`.** Wren defines the element with a
  script it adds to your page, and fills it with plugin elements. A policy that
  forbids inline script or `object-src` leaves the element empty.

## What is missing

- An open dialog does not follow an element that moves without changing size. It
  catches up at the next resize, or when the window moves.
- Wren does not tell your page that a side panel opened or how wide it wants to
  be. Your app is told, and can [tell the page](messages.md).
- An extension's side panel has not been tried in an element.

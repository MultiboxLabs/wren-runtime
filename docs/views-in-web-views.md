# Views in a web view

The page of a [web view](web-views.md) can show tabs and side panels inside
itself, so a browser's whole window can be one page of HTML. Where an app with
native UI places a view, your page writes an element:

```html
<wren-view data-id="tab-5"></wren-view>
<wren-view data-id="sidepanel-2"></wren-view>
```

Wren fills it and keeps it filled. The pages are drawn in the element, at its
size, and used there: clicks, keys and focus reach them. It lies in your page
like any element, so your own HTML can be over it and your CSS styles it. It is
a stacking context of its own (`isolation: isolate`): HTML after it, or with a
higher `z-index`, is drawn over the page. The API is in
[`WrenWebView.h`](../include/WrenWebView.h)
("Views in a web view"); what works in this release is in
[This release](this-release.md).

Make the view with the right to do it, and give its page the IDs:

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

In C: `WrenWebViewOptions.embeds_views`, a tab's `WrenTabID` and
`Wren_GetBrowserID`.

## What each element holds

| `data-id`                       | Holds                                                                                              | The native view it stands for |
| ------------------------------- | -------------------------------------------------------------------------------------------------- | ----------------------------- |
| `tab-` and a tab's ID           | The tab's page; docked DevTools beside it, with its divider; a scrim while a dialog blocks the tab | `tab.view`                    |
| `sidepanel-` and a Browser's ID | The Browser's open side panel, or nothing                                                          | `browser.sidePanelView`       |

Your page does nothing when DevTools docks or moves, a tab's page is replaced,
or the panel changes. Split tabs need no element of their own: show both tabs'
elements with your own divider between them.

## Rules

- **The right is one page's.** It belongs to the main frame of a view made with
  `embedsViews`, and only while that page is on one of
  [your schemes](schemes.md) or on one of the view's
  [trusted origins](web-views.md#trusted-origins), which are for a development
  server. A tab on either, a frame inside the view's page, and a view that has
  gone anywhere else have none: the element is nothing to them.
- **A page is in an element or in its native view, never both.** While an
  element shows a tab, `tab.view` is empty, and the same for a side panel. When
  the element goes or the web view ends, the pages are back in their native
  views.
- **One element at a time.** A second element with the same `data-id` takes the
  tab from the first.
- **A split is two elements.** A press or a key in the side that is not the
  active tab makes it active, as in a native window: you hear it as a change,
  and need do nothing to make it happen.
- **Switching tabs is yours.** Change the element's `data-id`, or keep one
  element per tab and show one.
- **An element with nothing to show is empty**: a closed tab, a closed panel, an
  ID that names nothing.
- **The tab is still a tab.** Its ID, history, session and changes are as
  before, and so is everything you do to it through its Browser. `tab.focus()`
  moves keyboard focus to it inside the web view, once the element has it: an
  element's page arrives after your page asks for it, and the tab's
  `state().inElement` turns true then, with `.inElement` in `changed`. Give a
  newly active tab the keyboard at that change. The pages cannot script each
  other.
- **A group's or a split's ID is text for your page:** `String(group)` and
  `TabGroup(text)` (`Wren_FormatTabGroupID`, `Wren_ParseTabGroupID`), the same
  for a split.
- **Your page learns the IDs from you:** in its address, or by
  [asking your app](messages.md), which can also tell it when they change.
- **Give it a size.** Until Wren has defined the element, which is a moment
  after your page starts, it is an unknown inline element.
- **Attach the Browser to the web view's own view**
  (`browser.attach(to: webView.view)`): that makes the window the Browser's, and
  Chromium's own UI for the tab is placed from it. A tab's dialogs (an alert,
  HTTP sign-in) are then centred over the tab's element and kept inside the web
  view. A Browser attached to another window puts them in that window instead.
- **Mind your `Content-Security-Policy`.** Wren defines the element with a
  script it adds to your page, and fills it with plugin elements: a policy that
  forbids inline script or `object-src` leaves it empty.

## What is missing

- An open dialog does not follow an element that moves without changing size; it
  does at the next resize, or when the window moves.
- Wren does not tell your page that a side panel opened or how wide it wants to
  be. Your app is told, and can [tell the page](messages.md).
- An extension's side panel has not been tried in an element.

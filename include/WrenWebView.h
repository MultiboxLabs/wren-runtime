// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: web views (C API). Wren.h includes this header. Wren.h also explains
// threads and handles.
//
// A web view is a page that is not a tab. The app makes it and places it in
// its own window, as it places a tab's view. No Browser has it, and the app
// needs no Browser to make one. It shows any URL: the app's own UI on one of
// its schemes (WrenSchemes.h), or a website.
//
// - It has no tab ID. Wren reports no tab or Browser changes for it. It is not
//   in history, in a session, or in Chromium's lists of tabs.
// - It is a bare page, without what Chromium's UI gives a tab's page.
//   alert() and other dialogs do nothing. Permission requests get no answer,
//   or are denied. File choosers and window.open do nothing. A site that needs
//   these belongs in a tab.
// - Only a text field has a context menu, and it is Wren's own. It has
//   spelling suggestions, undo, cut, copy, paste and select all. It also has
//   Inspect, unless the view was made with disallow_user_devtools_activation.
//   Elsewhere, the page sends its own items from its contextmenu event to the
//   app, and the app shows them (Wren_ShowContextMenu, WrenMenus.h).
// - A key the page does not handle goes to the app's main menu, as a tab's
//   does. So the menu's shortcuts work while the page has focus.
// - The app reads whether the page is loading, failed to load, lost its
//   process or has drawn, and hears each change. See "Loading and crashing".

#ifndef WREN_PUBLIC_WREN_WEB_VIEW_H_
#define WREN_PUBLIC_WREN_WEB_VIEW_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// A web view's handle, WrenWebView, is declared in Wren.h. The app owns it
// from Wren_CreateWebView until Wren_DestroyWebView. Nothing else ends the
// handle. The page can end earlier, if its Profile closes. Every call then
// returns WREN_ERROR_CLOSED.

// What DevTools and remote debugging tools are told a web view's page is.
// Remote debugging tools include Puppeteer, chromedriver and chrome://inspect.
// Tools can inspect and drive either kind in full. The kind decides only
// where a tool lists the page.
typedef enum WrenWebViewKind {
  // A page, as a tab's page is ("page"). Tools that list the browser's pages
  // list it among the tabs. Electron's windows are this kind.
  WREN_WEB_VIEW_PAGE = 0,
  // The browser's own UI ("browser_ui"), as Chromium's WebUI browser calls
  // its own pages. It is not offered as a tab. So a tool that drives the
  // user's tabs does not find it there. A tool that wants it attaches to it by
  // ID.
  WREN_WEB_VIEW_BROWSER_UI = 1,
} WrenWebViewKind;

typedef struct WrenWebViewOptions {
  WrenWebViewKind kind;
  // If true, only the app opens the view's DevTools, with
  // Wren_OpenWebViewDevTools. The text fields' menu has no Inspect. Use it
  // for a UI whose DevTools the app opens from an item of its own, or not at
  // all once shipped. Remote debugging, if the app turns it on, still reaches
  // the page.
  bool disallow_user_devtools_activation;
  // If true, the view's page may show tabs and side panels inside itself.
  // See "Views in a web view".
  bool embeds_views;
  // If true, the view's page and the app exchange messages. See "Messages".
  bool messages;
  // Origins, besides the app's schemes, whose pages this view trusts as the
  // app's own UI. Each is written like "http://localhost:5173". May be NULL.
  // See "Trusted origins".
  const char* const* trusted_origins;
  size_t trusted_origin_count;
  // If true, the view's page may mark elements for Chromium's bubbles to hang
  // off. See "Anchors in a web view".
  bool anchors;
  // If true, the areas the page marks with the CSS property `app-region` move
  // the app's window. See "Dragging the window".
  bool drags_window;
  // If true, the page is see-through wherever it draws nothing. What lies
  // behind the view in its window shows there, as it does behind Chromium's
  // own transparent WebUI.
  //
  // Whatever the page draws still covers, CSS backgrounds included. Wren
  // changes nothing about the window. The app puts what should show behind
  // the view, such as a material or the window's own background. Every page
  // in the view is see-through, not only the app's own.
  bool transparent;
  // If true, a click in a drag area reaches the page, and only a press that
  // moves moves the window. Needs drags_window. See "Dragging the window".
  bool drag_area_clicks;
  // If true, the view's pages may draw elements as Liquid Glass, with the CSS
  // property -wren-glass. See "Liquid Glass".
  bool glass;
  // If true, the view's own pages read a trackpad scroll's phases, and hear
  // its fingers land and lift and its glide end. See "Scroll phases".
  bool wheel_phases;
} WrenWebViewOptions;

// Makes a web view in `profile` and starts loading `url`. `options` may be
// NULL, which makes a WREN_WEB_VIEW_PAGE. While the view lives, Chromium keeps
// a persistent or ephemeral Profile loaded, as a reference to the Profile
// does.
//
// Returns NULL for any of these:
//
// - a URL that is not valid, or a kind that does not exist
// - a trusted origin that is not an origin
// - trusted origins with none of the options they would give anything to,
//   which are messages, anchors, embeds_views and drags_window
// - drag_area_clicks without drags_window
// - a Profile that has closed
// - a call inside a callback that asks the app a question
WREN_API WrenWebView* Wren_CreateWebView(WrenProfile* profile,
                                         const char* url,
                                         const WrenWebViewOptions* options);

// Ends the page and the handle. The view leaves its window.
WREN_API void Wren_DestroyWebView(WrenWebView* view);

// Returns the view's NSView. Borrowed, and valid until Wren_DestroyWebView.
//
// The app places it like any view. Chromium treats it as it treats a tab's
// view (see "Tab views" in WrenTabs.h). The page is visible while the view is
// in a window and not hidden. The page is the view's size, and its scale is
// that of the window's screen.
WREN_API void* Wren_GetWebViewNativeView(WrenWebView* view);

// Loads `url` in the view, as Wren_Navigate does in a tab. Returns
// WREN_ERROR_INVALID_ARGUMENT for a URL that is not valid.
WREN_API WrenResult Wren_LoadWebView(WrenWebView* view, const char* url);

// --- Loading and crashing ---------------------------------------------------

// What a web view's page is doing. The page has loaded when it is not
// loading, has no error and has not crashed.
typedef struct WrenWebViewState {
  // True while the page is loading, as WrenTabState.loading is for a tab.
  bool loading;
  // Chromium's network error for the page's last load, or 0 if it did not
  // fail. An error is a negative number:
  //
  // - -105: the name was not found.
  // - -102: the connection was refused.
  // - -106: there is no network.
  // - -6: the app's scheme handler found nothing.
  //
  // After a failure the view shows Chromium's error page, or the page it
  // showed before. A page a server answered did not fail, whatever its
  // status. The value is 0 again from when the next load starts.
  int load_error;
  // Why the page is gone, if it is: its process crashed or was killed. The
  // view shows nothing until the page loads again.
  WrenTabCrash crash;
  // True once the page shown now has drawn. That is when its first frame
  // with something in it has reached the view: enough text or images, or the
  // whole document parsed. It is Electron's ready-to-show.
  //
  // Show a new window when this turns true, so the window is never blank. A
  // page draws while its view is in a window and not hidden, whether or not
  // the window is shown yet. The value is false from when another page
  // commits, Chromium's error page included, and while the page is crashed.
  bool drawn;
  // True while DevTools for the page is open. That is from when DevTools'
  // Browser is shown (on_browser_opened) until it has closed. It does not
  // matter how DevTools was opened or closed: Wren_OpenWebViewDevTools,
  // Inspect or chrome://inspect.
  bool devtools;
} WrenWebViewState;

// Reads the view's state. WREN_CHANGE_WEB_VIEW reports each change, with one
// record per view in an on_changed call. Returns WREN_ERROR_CLOSED once the
// view's page has ended.
WREN_API WrenResult Wren_GetWebViewState(WrenWebView* view,
                                         WrenWebViewState* state);

// Loads the page again at the address it has now, as Wren_Reload does for a
// tab. Use it after a crash, a failed load, or a change to the page's files.
// Wren_LoadWebView loads an address the app gives instead.
WREN_API WrenResult Wren_ReloadWebView(WrenWebView* view);

// Gives the page keyboard focus within its view's window. It restores focus
// to the element that had it. It does not make the window key.
WREN_API WrenResult Wren_FocusWebView(WrenWebView* view);

// Opens DevTools for the view's page, of either kind. If DevTools is already
// open, it is shown again.
//
// DevTools opens in a WREN_BROWSER_DEVTOOLS Browser of its own, which reaches
// on_browser_opened. The app shows it as it shows a tab's undocked DevTools.
// It cannot dock, because a web view has no Browser.
//
// Returns WREN_ERROR_NOT_ALLOWED if Chromium's policy forbids DevTools.
WREN_API WrenResult Wren_OpenWebViewDevTools(WrenWebView* view);
// Closes the view's DevTools, as Wren_CloseDevTools does a tab's undocked
// DevTools. DevTools' Browser closes, and on_browser_closed reports it.
// Returns WREN_OK if DevTools is not open.
//
// With Wren_OpenWebViewDevTools and WrenWebViewState.devtools, this gives an
// app a menu item that toggles the DevTools of its own UI.
WREN_API WrenResult Wren_CloseWebViewDevTools(WrenWebView* view);

// --- Trusted origins --------------------------------------------------------
//
// A web view's page gets five things from the app, each turned on by its own
// option:
//
// - `wren`, by `messages`
// - marks for bubbles, by `anchors`
// - <wren-view>, by `embeds_views`
// - drag areas, by `drags_window`
// - a scroll's phases, by `wheel_phases`
//
// The page has them only while it is one of the app's own pages. By default
// those are the pages on the app's schemes (WrenSchemes.h).
//
// `trusted_origins` adds origins, for this view alone and fixed for its life.
// To this view, a page on a trusted origin is the app's own, exactly as a page
// on an app scheme is. A trusted origin turns nothing on. Each thing still
// needs its option.
//
// It is for a development server that serves the UI while the app is built,
// such as "http://localhost:5173". Whoever serves a trusted origin reaches
// every handler the app set for the view. They place its Browsers' anchors,
// and they can show any tab whose ID they write. So a shipped app lists no
// trusted origins.
//
// - A trusted origin does not open the app's schemes. They stay closed to it,
//   as to any website (WrenSchemes.h). So such a page loads its own files
//   from its own server.
// - The trust is this view's alone. The same origin has nothing in a tab, in a
//   frame of the view's page, or in another view that did not list it.

// --- Views in a web view ----------------------------------------------------
//
// The page of a view made with `embeds_views` can show tabs and side panels in
// elements of its own. They are the things an app with native UI gets a view
// for:
//
//   <wren-view data-id="tab-5"></wren-view>
//   <wren-view data-id="sidepanel-2"></wren-view>
//
// - "tab-" and a tab's ID shows what the tab's own view (Wren_GetTabView)
//   holds. That is the tab's page, and DevTools with its divider while
//   DevTools is docked to the tab, laid out on the side its dock menu chose.
//   While a dialog of the page's blocks it, a scrim covers the page.
// - "sidepanel-" and a Browser's ID (Wren_GetBrowserID) shows what the
//   Browser's side panel view (Wren_GetSidePanelView) holds. That is the open
//   panel's page, or nothing.
//
// The pages are drawn in the element, at its size, and the user uses them
// there. Clicks, keys and focus reach them. The element sits in the page like
// any other element, styled by the page's CSS. Give it `display: block` and a
// size. The page's own HTML can be drawn over it.
//
// Wren fills the element and keeps it filled. The page does nothing when
// DevTools docks, when a tab's page is replaced, or when the panel changes. A
// tab stays a tab, with its ID, its history and its changes. The pages cannot
// script each other.
//
// - A tab's page arrives in its element a moment after the page asks for it,
//   when Chromium attaches it. WrenTabState.in_element says when, and
//   Wren_FocusTab reaches the page from then on.
// - Only the view's own page may show views, and only while it is on one of
//   the app's schemes (WrenSchemes.h) or a trusted origin. To a tab on such a
//   page, a frame inside the view's page, or a view that has gone anywhere
//   else, the element is nothing.
// - A page is either in an element or in its native view, never both. While
//   an element shows a tab, the tab's own view is empty, and the same is true
//   of a side panel. When the element goes, or the web view's page ends, the
//   pages go back to their native views.
// - One element shows a thing at a time. A second element with the same
//   data-id takes it from the first.
// - An element whose data-id changes shows the other thing. An element for a
//   tab that has closed, or with an ID that names nothing, is empty.
// - The page learns the IDs from the app, in its address or by asking (see
//   "Messages").
// - Wren defines the element in the page with a script of its own. A page
//   whose Content-Security-Policy forbids inline script or plugins
//   (script-src, object-src) has no <wren-view>.
// - What Chromium shows for a tab outside its page is not in the element.
//   Context menus, <select> lists and dialogs are windows of their own.
// - A tab's dialog is centred over its element when the Browser is attached
//   to the web view's own view, through Wren_AttachBrowserToView with
//   Wren_GetWebViewNativeView. It is kept inside that view. If the Browser is
//   attached to another window, it places the dialog in that window.

// --- Anchors in a web view --------------------------------------------------
//
// The page of a view made with `anchors` marks the elements Chromium's
// bubbles hang off. An app with native UI names a view for each instead
// (Wren_SetAnchor in WrenToolbar.h). Examples are the site icon, the
// extensions button, the button for one extension's action, and the address
// field.
//
// A mark is an element of a type of Wren's, with a name the page chooses. The
// page's CSS lays it over the button:
//
//   <span class="anchored">
//     <button>...</button>
//     <embed type="application/x-wren-anchor" name="site-icon">
//   </span>
//
//   .anchored { position: relative; display: inline-block; }
//   .anchored > embed { position: absolute; inset: 0; width: 100%;
//                       height: 100%; pointer-events: none; }
//
// The app says which Browser's anchor a name is, with Wren_SetAnchorElement.
// Wren then follows the element. Chromium's own layout reports where it is
// whenever the page lays out or scrolls, and when it goes. Neither the page
// nor the app runs any script for it. The mark draws nothing and takes no
// input.
//
// - Only the view's own page may mark anchors, and only while it is on one of
//   the app's schemes (WrenSchemes.h) or a trusted origin. To a tab on such a
//   scheme, a frame inside the view's page, or a view that has gone anywhere
//   else, the element is nothing.
// - A mark does nothing until the app names it for a Browser.
// - An anchor reaches no further than the web view. What lies outside the
//   view is cut off. A mark wholly outside the view is no anchor. Neither is a
//   hidden mark, nor a mark in a view that is not in the window its Browser is
//   attached to. Bubbles then open where they would with no anchor named.
// - Wren reads a mark's name when the element is made. To rename a mark,
//   replace the element. Of two marks with one name, the one made last counts.
// - If an animation moves a mark without the page laying out again, as a
//   transform does, Wren follows the mark only once the page lays out.
// - A page whose Content-Security-Policy forbids plugins (object-src) has no
//   marks.

// --- Dragging the window ----------------------------------------------------
//
// The page of a view made with `drags_window` says in CSS which of its areas
// move the app's window, as a title bar does. It is for a window whose title
// bar is drawn in HTML:
//
//   .bar        { app-region: drag; }
//   .bar button { app-region: no-drag; }
//
// This is Chromium's own mechanism, which its own windows and Electron's use.
// The property is also spelt -webkit-app-region. Blink finds the areas after
// each layout. Over one, the web view behaves as a window's title bar does:
//
// - A left mouse-down starts moving the window, and does not reach the page.
// - A double click does what the user chose in System Settings for a title
//   bar's double click: zoom, fill, minimise or nothing.
// - Right clicks, control-clicks, scrolling and pointer movement reach the
//   page as anywhere else. So a drag area still gets its contextmenu event and
//   its hover styles.
//
// That is how Chromium's own windows and Electron's treat a drag area, on
// every platform: no left click there reaches the page. With
// `drag_area_clicks` the view tells a click from a drag instead:
//
// - A left press moves the window once the pointer moves more than 3 points,
//   and the page sees none of it. Released before then, it is a click, and
//   the page gets it then: its mousedown, mouseup and click come at the
//   release. In a window that is not key, the page gets it only where a
//   page's first click would reach it anywhere else.
// - A double click gives the page both clicks, then does the title bar's
//   action, as above. The page cannot stop it.
//
// An area is the box of the element that has the property. The property is
// inherited, so each element inside a drag area drags too. Give `no-drag` to
// each button, tab or field inside one. A <wren-view> is
// `no-drag` unless the page's CSS says otherwise, so a page whose whole body
// drags still has tabs that get their clicks.
//
// - The app makes the window. It needs a hidden title bar and a content view
//   that reaches the top: NSWindowStyleMaskFullSizeContentView, a transparent
//   title bar and a hidden title. Wren changes nothing about the window. The
//   window's own buttons stay above the page, so leave them room.
// - Only the page's drag areas move the window. In the title bar's band,
//   macOS moves a window from a press wherever it sees nothing opaque, and a
//   page drawn out of process looks empty to it. The view stops macOS from
//   doing that anywhere over it, as Chromium's own windows do. A view made
//   without `drags_window` is left to macOS.
// - Only the view's own page declares drag areas, and only while it is on
//   one of the app's schemes (WrenSchemes.h) or a trusted origin. A frame
//   inside the page declares none, and neither does a tab the page shows. A
//   view that has gone anywhere else drags nothing, and every click reaches
//   its page.
// - The areas belong to the page shown now. They go when it reloads, goes
//   elsewhere or loses its process. They come back with the next page's first
//   layout.

// --- Liquid Glass -----------------------------------------------------------
//
// The pages of a view made with `glass` may draw an element as Liquid Glass,
// the material of macOS 26's controls, which shows what lies behind it, blurred
// and bent:
//
//   .toolbar { -wren-glass: regular; border-radius: 18px; }
//
// -wren-glass is `none`, the default, `regular` or `clear`, the two styles of
// NSGlassEffectView. Glass shows only on an element that paints a background,
// border or shadow, which the glass replaces; give an element with none of them
// a background. Glass fills the element's border box, rounded by the
// largest of its four corner radii. The element's background, border and
// box-shadow are not drawn. Its content and its focus outline draw on the
// glass. What shows through is what lies behind the element: the page under
// it, and where the page is see-through (`transparent`), the window behind.
//
// - The glass is light or dark as the element's used `color-scheme` is, so
//   one element can have dark glass in a light page. Its `light-dark()`
//   colours take the same side.
// - -wren-glass-tint tints the glass toward a colour, as NSGlassEffectView's
//   tintColor does. It is `auto`, untinted, or any CSS colour. A colour the
//   view has not drawn before may draw untinted for a frame or two.
// - While its window is not active, the glass is subdued and untinted, as
//   AppKit's is. A window is active while it is key, or main while the app is
//   active, or a child window of one of those.
// - The glass follows the user's Reduce Transparency and Increase Contrast
//   settings, as AppKit's does.
//
// Where Wren cannot draw glass, the element draws as its CSS says, so give it
// a background to fall back on, or a backdrop-filter: on an element with glass
// it draws only where the glass does not. Glass cannot draw while the window
// is captured, as in screen sharing, or in a frame that Chromium draws without
// Core Animation layers. It does that for the whole window while the view's
// page has too many layers to give each one its own, or a backdrop-filter that
// Core Animation cannot draw: one with url() or drop-shadow(), on an element
// with a mask, a clip-path or a filter, or on corners of different radii. A
// backdrop-filter of blur() and colour filters on any other element draws with
// Core Animation, beside the glass, and sees glass beneath it. It blurs what
// is behind it in the window, so in a transparent page it blurs the window
// behind the view too, as glass does. A tab or side panel the page shows
// draws apart from it, so a website there never stops the page's glass.
//
// Every page in the view may use glass, in every frame and of any origin: it
// changes only how the page's own pixels draw. The properties do not exist in
// a view made without `glass`, or in a tab, so a website cannot tell Wren from
// Chrome by them there. Only the Mac draws glass.

// --- Scroll phases ----------------------------------------------------------
//
// The app's own pages in a view made with `wheel_phases`, in any frame, read
// the phases of a trackpad's scroll on each `wheel` event, as AppKit gives
// them, to follow the fingers as a native control does:
//
//   addEventListener('wheel', e => {
//     if (e.wrenPhase === 'ended') settle();
//   });
//
// `wrenPhase` is the fingers' phase, and `wrenMomentumPhase` the glide's
// after they lift. Each is one of:
//
// - "none": not in this part of the scroll. A mouse's wheel is "none" in both.
// - "may-begin": the fingers landed and have not moved.
// - "began": the fingers began to move.
// - "changed": they moved, or rest on the trackpad.
// - "ended": they lifted, or the glide stopped.
// - "cancelled": they lifted without moving.
//
// Such a page also hears what no other page does, each as a `wheel` event of
// no delta that cannot be cancelled: fingers that land ("may-begin"), fingers
// that lift ("ended" or "cancelled"), and a glide's end. A handler that counts
// events, or divides by a delta, skips them. The lift comes up to 100 ms after
// the fingers leave, as Chromium waits to see whether a glide follows. When
// one does, the lift comes just before the glide's first event.
//
// Nothing else changes: the page scrolls as before, and preventDefault() does
// what it does in Chrome. A page that is not the app's own (see "Trusted
// origins"), a page in a view made without the option, and a tab's page have
// neither the fields nor the events, so a website cannot tell Wren from Chrome
// by them. Only the Mac's trackpads give phases.

// --- Messages ---------------------------------------------------------------
//
// The page of a view made with `messages` and the app can call each other, as
// a page and the main process do in Electron. The page has one object, `wren`,
// which is there before its first script runs:
//
//   const tabs = await wren.invoke("tabs.list");       // the app answers
//   const stop = wren.on("tabs.changed", tabs => {});  // the app sends
//
// Wren carries the messages and defines none of them. Every name is the
// app's. Values cross as JSON, so they are whatever JSON can hold. The page
// passes and gets ordinary values.
//
// Besides the channel, the page has one function of Wren's own,
// wren.pathForFile(file). It returns the path on disk of a file the user
// dropped on the page, or null for any other File or Blob. With it, the app
// can open a dropped file as Chrome does. Wren opens nothing itself.
//
// The channel is for pages the app trusts: its own UI, which it wrote and
// serves. A page with `wren` reaches every handler the app set. It is not a
// way for websites, extensions or other content to reach the app. An
// extension reaches the app through native messaging ("Native messaging" in
// WrenExtensions.h).
//
// - Only the view's own page has `wren`, and only while it is on one of the
//   app's schemes (WrenSchemes.h) or a trusted origin. A tab on such a scheme,
//   a frame inside the view's page, and a view that has gone anywhere else
//   have no `wren`.
// - A call belongs to the page that made it. When that page reloads, goes
//   elsewhere or ends, its unanswered calls are over. The page's promise
//   never settles, and answering returns WREN_ERROR_CLOSED.
// - A call's value comes from a page. Treat it as input, as a scheme's
//   handler treats a URL.
// - The handler runs from the event loop, outside Chromium's notifications,
//   so it may call any function.

// One call from a view's page, through wren.invoke. The app answers it once,
// in the handler or later. The pointer is valid until then. Using it after
// that is safe, and returns WREN_ERROR_CLOSED.
typedef struct WrenPageCall WrenPageCall;

typedef void (*WrenPageCallHandler)(void* context,
                                    WrenWebView* view,
                                    WrenPageCall* call);

// Sets the handler for every call from the view's page, whatever its name.
// NULL removes the handler. Without a handler, calls fail in the page.
// Returns WREN_ERROR_NOT_ALLOWED for a view made without `messages`.
WREN_API WrenResult Wren_SetPageCallHandler(WrenWebView* view,
                                            WrenPageCallHandler handler,
                                            void* context);

// The next two calls copy the call's name, and its value as JSON text. The
// value is "null" when the page gave none. Each returns the size needed,
// including the NUL, and copies what fits. Each returns 0 for a call that is
// over.
WREN_API size_t Wren_CopyPageCallName(WrenPageCall* call,
                                      char* buffer,
                                      size_t size);
WREN_API size_t Wren_CopyPageCallValue(WrenPageCall* call,
                                       char* buffer,
                                       size_t size);

// Answers the call. The page's promise resolves with the value of `json`.
// NULL means null.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for text that is not JSON or is over 1
// MB. The call then stays open. Returns WREN_ERROR_CLOSED for a call that is
// over.
WREN_API WrenResult Wren_AnswerPageCall(WrenPageCall* call, const char* json);

// Fails the call. The page's promise rejects with an Error whose message is
// `message`. Returns WREN_ERROR_CLOSED for a call that is over.
WREN_API WrenResult Wren_FailPageCall(WrenPageCall* call, const char* message);

// Sends the page a message. The page's wren.on(name) listeners get the value
// of `json`. NULL means null.
//
// Nothing is kept for a page that is not there. Returns WREN_ERROR_CLOSED
// while the view has no page with the channel, because it is still loading,
// has gone elsewhere or has ended. A page that has made a call is there.
// Returns WREN_ERROR_INVALID_ARGUMENT for no name, or for text that is not
// JSON or is over 1 MB.
WREN_API WrenResult Wren_SendPageMessage(WrenWebView* view,
                                         const char* name,
                                         const char* json);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_WEB_VIEW_H_

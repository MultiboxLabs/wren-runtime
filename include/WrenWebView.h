// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: web views (C API). Wren.h includes this header; see Wren.h for
// threading and handles.
//
// A web view is a page the app makes and places in its own window, as it
// places a tab's view, and that is not a tab: no Browser has it, and the app
// needs no Browser to have one. It shows any URL: the app's own UI on one of
// its schemes (WrenSchemes.h), or a website.
// - It has no tab ID, and no change of a tab's or a Browser's is reported
//   for it. It is not in history, in a session, or in Chromium's lists of
//   tabs.
// - It is a bare page. What Chromium's UI gives a tab's page is missing:
//   alert() and other dialogs, permission prompts (a request is left
//   unanswered or denied), file choosers and window.open do nothing. Only a
//   text field has a context menu, Wren's own: spelling suggestions, undo,
//   cut, copy, paste, select all and, unless the view was made without
//   DevTools, Inspect. Elsewhere the page sends its own items from its
//   contextmenu event to the app, which shows them (Wren_ShowContextMenu,
//   WrenMenus.h). A site that needs the rest belongs in a tab.
// - A key the page does not handle goes to the app's main menu, as a tab's
//   does, so the menu's shortcuts work while the page has focus.
// - The app reads whether the page is loading, failed to load, has lost
//   its process or has drawn, and hears each change: see "Loading and
//   crashing".

#ifndef WREN_PUBLIC_WREN_WEB_VIEW_H_
#define WREN_PUBLIC_WREN_WEB_VIEW_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// One web view (WrenWebView, declared in Wren.h). The app owns it from
// Wren_CreateWebView to Wren_DestroyWebView; nothing else ends the handle. Its
// page ends earlier if its Profile closes, and every call then returns
// WREN_ERROR_CLOSED.

// What DevTools and remote debugging (Puppeteer, chromedriver,
// chrome://inspect) are told a web view's page is. Either kind can be
// inspected and driven in full; the kind decides where it is listed.
typedef enum WrenWebViewKind {
  // A page, as a tab's is ("page"): tools that list the browser's pages
  // list it among the tabs. Electron's windows are this.
  WREN_WEB_VIEW_PAGE = 0,
  // The browser's own UI ("browser_ui"), as Chromium's WebUI browser calls
  // its own: it is not offered as a tab, so a tool driving the user's tabs
  // does not find it there, and one that wants it attaches to it by ID.
  WREN_WEB_VIEW_BROWSER_UI = 1,
} WrenWebViewKind;

typedef struct WrenWebViewOptions {
  WrenWebViewKind kind;
  // The view has no DevTools: its text fields' menu has no Inspect, and
  // Wren_OpenWebViewDevTools refuses. For a UI that is inspected while it
  // is built and not once shipped. Remote debugging, if the app turns it
  // on, still reaches the page.
  bool disable_devtools;
  // The view's page may show tabs and side panels in itself: see "Views
  // in a web view".
  bool embeds_views;
  // The view's page and the app exchange messages: see "Messages".
  bool messages;
  // Origins besides the app's schemes whose pages this view trusts as the
  // app's own UI, each as "http://localhost:5173": see "Trusted origins".
  // May be NULL.
  const char* const* trusted_origins;
  size_t trusted_origin_count;
  // The view's page may mark elements as places for Chromium's bubbles: see
  // "Anchors in a web view".
  bool anchors;
  // The areas the view's page marks with the CSS property `app-region`
  // move the app's window: see "Dragging the window".
  bool drags_window;
  // The page is see-through where it draws nothing, so that what lies
  // behind the view in its window shows there, as behind Chromium's own
  // transparent WebUI. What the page draws, its CSS backgrounds among it,
  // still covers. Wren changes nothing of the window's: the app puts behind
  // the view what should show (a material, the window's own background).
  // Any page in the view is see-through, not only the app's own.
  bool transparent;
} WrenWebViewOptions;

// Makes a web view in `profile` and starts loading `url`. `options` may be
// NULL, for a WREN_WEB_VIEW_PAGE. While the view lives Chromium keeps a
// persistent or ephemeral Profile loaded, as a reference to it does. NULL
// for a URL that is not valid, a kind that does not exist, a trusted origin
// that is not an origin or is given with none of the options a trusted page
// has anything by, a Profile that has closed, or inside a callback that
// asks the app a question.
WREN_API WrenWebView* Wren_CreateWebView(WrenProfile* profile,
                                         const char* url,
                                         const WrenWebViewOptions* options);

// Ends the page and the handle. The view leaves its window.
WREN_API void Wren_DestroyWebView(WrenWebView* view);

// The view's NSView (borrowed; valid until Wren_DestroyWebView). The app
// places it like any view, and Chromium follows it as it follows a tab's
// (see "Tab views" in WrenTabs.h): the page is visible while the view is in a
// window and not hidden, its size is the view's, and its scale is the
// window's screen's.
WREN_API void* Wren_GetWebViewNativeView(WrenWebView* view);

// Loads `url` in the view, as Wren_Navigate does in a tab.
// WREN_ERROR_INVALID_ARGUMENT for a URL that is not valid.
WREN_API WrenResult Wren_LoadWebView(WrenWebView* view, const char* url);

// --- Loading and crashing ---------------------------------------------------

// What a web view's page is doing. The page has loaded when it is not
// loading, has no error and has not crashed.
typedef struct WrenWebViewState {
  // The page is loading, as WrenTabState.loading says of a tab.
  bool loading;
  // The page's last load failed: Chromium's network error for it, a
  // negative number (-105: the name was not found; -102: the connection was
  // refused; -106: no network; -6: the app's scheme handler found nothing),
  // or 0. The view then shows Chromium's error page, or the page it showed
  // before. A page a server answered, with any status, did not fail. 0
  // again from when the next load starts.
  int load_error;
  // Why the page is gone, if it is: its process crashed or was killed. The
  // view shows nothing until the page is loaded again.
  WrenTabCrash crash;
  // The page shown now has drawn once: its first frame with something in
  // it (enough text or images, or the whole document parsed) has reached
  // the view, as Electron's ready-to-show. An app shows a new window when
  // it turns true, so that the window is never blank. A page draws while
  // its view is in a window, shown or not, and not hidden. False from when
  // another page commits, Chromium's error page among them, and while the
  // page is crashed.
  bool drawn;
} WrenWebViewState;

// WREN_CHANGE_WEB_VIEW reports each change, one record per view in an
// on_changed call. WREN_ERROR_CLOSED once the view's page has ended.
WREN_API WrenResult Wren_GetWebViewState(WrenWebView* view,
                                         WrenWebViewState* state);

// Loads the page again at the address it has now, as Wren_Reload does a
// tab: after a crash, a failed load, or a change to the page's files.
// Wren_LoadWebView does the same for an address the app gives.
WREN_API WrenResult Wren_ReloadWebView(WrenWebView* view);

// Gives the page keyboard focus within its view's window, restoring the
// element that had it. Does not make the window key.
WREN_API WrenResult Wren_FocusWebView(WrenWebView* view);

// Opens DevTools for the view's page, of either kind, in a
// WREN_BROWSER_DEVTOOLS Browser of
// its own (on_browser_opened), which the app shows as it shows a tab's
// undocked DevTools; an open one is shown again. It cannot dock: a web view
// has no Browser. WREN_ERROR_NOT_ALLOWED if the view was made without
// DevTools, or Chromium's policy forbids it.
WREN_API WrenResult Wren_OpenWebViewDevTools(WrenWebView* view);

// --- Trusted origins --------------------------------------------------------
//
// What a web view's page has from the app, each by its own option (`wren`
// by `messages`, marks by `anchors`, <wren-view> by `embeds_views`, drag
// areas by `drags_window`), it has only while it is one of the app's own
// pages. By default those are the
// pages on the app's schemes (WrenSchemes.h). `trusted_origins` adds
// origins, for this view alone and fixed for its life: a page on one of them
// is the app's own to this view exactly as a page on an app scheme is. It
// turns nothing on: each thing still needs its option.
//
// It is for a development server that serves the UI while it is built
// ("http://localhost:5173"). Whoever serves a trusted origin reaches every
// handler the app set for the view, places its Browsers' anchors and shows
// any tab whose ID it writes, so a shipped app lists none.
// - It does not open the app's schemes: they stay closed to a trusted
//   origin as to any website (WrenSchemes.h), so such a page loads its own
//   files from its own server.
// - It is the view's: the same origin in a tab, in a frame of the view's
//   page, or in another view that did not list it has nothing.

// --- Views in a web view ----------------------------------------------------
//
// The page of a view made with `embeds_views` shows, in an element of its
// own, what an app with native UI has a view for:
//
//   <wren-view data-id="tab-5"></wren-view>
//   <wren-view data-id="sidepanel-2"></wren-view>
//
// "tab-" and a tab's ID is what the tab's own view (Wren_GetTabView) holds:
// the tab's page and, while DevTools is docked to it, DevTools, laid out as
// DevTools' own dock menu says, with its divider; and a scrim over the page
// while a dialog of the page's blocks it. "sidepanel-" and a Browser's ID
// (Wren_GetBrowserID) is what its side panel view (Wren_GetSidePanelView)
// holds: the open panel's page, or nothing.
//
// The pages are drawn in the element, at its size, and used there: clicks,
// keys and focus reach them. The element lies in the page like any other,
// styled by the page's CSS (give it `display: block` and a size), so the
// page's own HTML can be over it. Wren fills it and keeps it filled: the
// page does nothing when DevTools docks, a tab's page is replaced, or the
// panel changes. A tab stays a tab, with its ID, history and changes; the
// pages cannot script each other.
// - A tab's page arrives in its element a moment after the page asks for
//   it, when Chromium attaches it: WrenTabState.in_element says when, and
//   Wren_FocusTab reaches it from then.
// - The right is the view's page's, and only while that page is on one of
//   the app's schemes (WrenSchemes.h) or on a trusted origin. A tab on
//   either, a frame inside the view's page, and a view that has gone
//   anywhere else have none: the element is nothing to them.
// - A page is in an element or in its native view, not both. While an
//   element shows a tab, the tab's own view is empty, and the same for a
//   side panel; when the element goes or the web view's page ends, the
//   pages are back in their native views.
// - One element shows a thing at a time: a second element with the same
//   data-id takes it from the first.
// - An element whose data-id changes shows the other thing. One for a tab
//   that has closed, or an ID that names nothing, is empty.
// - The page learns the IDs from the app: in its address, or by asking
//   ("Messages").
// - Wren defines the element in the page, with a script of its own: a page
//   whose Content-Security-Policy forbids inline script or plugins
//   (script-src, object-src) has no <wren-view>.
// - What Chromium shows for a tab outside its page is not in the element:
//   context menus, <select> lists and dialogs are windows of their own. A
//   tab's dialog is centred over its element when the Browser is attached to
//   the web view's own view (Wren_AttachBrowserToView with
//   Wren_GetWebViewNativeView), and kept inside that view; attached to
//   another window, the Browser places it in that window.

// --- Anchors in a web view --------------------------------------------------
//
// The page of a view made with `anchors` marks the elements that Chromium's
// bubbles hang off, where an app with native UI names a view
// (Wren_SetAnchor in WrenToolbar.h): its site icon, its extensions button, a
// button for one extension's action, its address field. A mark is an
// element of a type of Wren's, with a name the page chooses, laid over the
// button by the page's CSS:
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
// The app says which Browser's anchor a name is, with
// Wren_SetAnchorElement. Wren then follows the element: Chromium's own
// layout reports where it is whenever the page lays out or scrolls, and
// when it goes, with no script of the page's or the app's. The mark draws
// nothing and takes no input.
// - The right is the view's page's, and only while that page is on one of
//   the app's schemes (WrenSchemes.h) or on a trusted origin. A tab on such
//   a scheme, a frame inside the view's page, and a view that has gone
//   anywhere else have none: the element is nothing to them. A mark does
//   nothing until the app names it for a Browser.
// - An anchor is no further than the web view: what lies outside the view
//   is cut off, and a mark wholly outside it, hidden, or in a view that is
//   not in the window its Browser is attached to, is no anchor. Bubbles
//   then open where they do when the app names none.
// - A mark's name is read when the element is made: to rename one, replace
//   the element. Of two with one name, the one made last counts.
// - A mark moved by an animation that the page does not lay out again for
//   (a transform) is followed only once it does.
// - A page whose Content-Security-Policy forbids plugins (object-src) has
//   no marks.

// --- Dragging the window ----------------------------------------------------
//
// The page of a view made with `drags_window` says in CSS which of its
// areas move the app's window, as a title bar does, for a window whose
// title bar is drawn in HTML:
//
//   .bar        { app-region: drag; }
//   .bar button { app-region: no-drag; }
//
// It is Chromium's own mechanism (the property is also spelt
// -webkit-app-region), as its own windows and Electron's use it. Blink
// finds the areas after each layout; over one, the web view behaves as a
// window's title bar does:
// - a left mouse-down starts moving the window, and does not reach the
//   page; a double click does what the user chose in System Settings for a
//   title bar's double click (zoom, minimise or nothing);
// - right clicks, control-clicks, scrolling and the pointer's movement
//   reach the page as anywhere else, so a drag area still has its
//   contextmenu event and its hover styles.
// An area is the box of the element that has the property. The property is
// not inherited, but what lies inside a drag area is covered by it: give
// `no-drag` to each button, tab or field in it. A <wren-view> is `no-drag`
// unless the page's CSS says otherwise, so a page whose whole body drags
// still has tabs that take their clicks.
// - The app makes the window: one with a hidden title bar whose content
//   view reaches the top (NSWindowStyleMaskFullSizeContentView, a
//   transparent title bar, a hidden title). Wren changes nothing of the
//   window's. The window's own buttons stay the window's, above the page:
//   leave them room.
// - Only the page's drag areas move the window. In a title bar's band macOS
//   moves a window from a press by itself wherever it sees nothing opaque,
//   and a page drawn out of process looks empty to it: the view keeps all
//   of itself from that, as Chromium's own windows do. A view made without
//   `drags_window` is left to macOS.
// - The right is the view's page's, and only while that page is on one of
//   the app's schemes (WrenSchemes.h) or on a trusted origin. A frame
//   inside the page declares nothing, and neither does a tab the page
//   shows. A view that has gone anywhere else drags nothing, and every
//   click reaches its page.
// - The areas are the page's that is there: they go when it reloads, goes
//   elsewhere or its process ends, and come back with the next page's
//   first layout.

// --- Messages ---------------------------------------------------------------
//
// The page of a view made with `messages` and the app call each other, as a
// page and the main process do in Electron. The page has one object, `wren`,
// there before its first script runs:
//
//   const tabs = await wren.invoke("tabs.list");       // the app answers
//   const stop = wren.on("tabs.changed", tabs => {});  // the app sends
//
// Wren defines the channel and nothing on it: every name is the app's, and
// nothing of Wren's API is in the page. Values cross as JSON, so they are
// what JSON holds; the page passes and gets ordinary values.
//
// It is for pages the app trusts: its own UI, which it wrote and serves. A
// page with `wren` reaches every handler the app set. It is not a way for
// websites, extensions or other content to reach the app.
// - The right is the view's page's, and only while that page is on one of
//   the app's schemes (WrenSchemes.h) or on a trusted origin. A tab on such
//   a scheme, a frame inside the view's page, and a view that has gone
//   anywhere else have no `wren`.
// - A call belongs to the page that made it. When that page reloads, goes
//   elsewhere or ends, its unanswered calls are over: the page's promise is
//   never settled, and answering reports WREN_ERROR_CLOSED.
// - A call's value comes from a page: a handler treats it as input, as a
//   scheme's handler treats a URL.
// - The handler runs from the event loop, outside Chromium's notifications,
//   so it may call anything.

// One call from a view's page (wren.invoke). The app answers it once, in
// the handler or later; the pointer is valid until then, and using it after
// is safe: it reports WREN_ERROR_CLOSED.
typedef struct WrenPageCall WrenPageCall;

typedef void (*WrenPageCallHandler)(void* context,
                                    WrenWebView* view,
                                    WrenPageCall* call);

// Sets the handler for every call from the view's page, whatever its name;
// NULL removes it. With none, a call fails in the page.
// WREN_ERROR_NOT_ALLOWED for a view made without `messages`.
WREN_API WrenResult Wren_SetPageCallHandler(WrenWebView* view,
                                            WrenPageCallHandler handler,
                                            void* context);

// The call's name, and its value as JSON text ("null" when the page gave
// none). Each returns the size needed, including the NUL, and copies what
// fits; 0 for a call that is over.
WREN_API size_t Wren_CopyPageCallName(WrenPageCall* call,
                                      char* buffer,
                                      size_t size);
WREN_API size_t Wren_CopyPageCallValue(WrenPageCall* call,
                                       char* buffer,
                                       size_t size);

// Answers the call: the page's promise resolves with `json`'s value (NULL
// is null). WREN_ERROR_INVALID_ARGUMENT for text that is not JSON or is
// over 1 MB, and the call stays open; WREN_ERROR_CLOSED for a call that is
// over.
WREN_API WrenResult Wren_AnswerPageCall(WrenPageCall* call, const char* json);

// Fails the call: the page's promise rejects with an Error whose message
// is `message`. WREN_ERROR_CLOSED for a call that is over.
WREN_API WrenResult Wren_FailPageCall(WrenPageCall* call, const char* message);

// Sends the page a message: its wren.on(name) listeners get `json`'s value
// (NULL is null). Nothing is kept for a page that is not there:
// WREN_ERROR_CLOSED while the view has no page with the channel (it is
// still loading, has gone elsewhere, or has ended). A page that has called
// is there. WREN_ERROR_INVALID_ARGUMENT for no name, or text that is not
// JSON or is over 1 MB.
WREN_API WrenResult Wren_SendPageMessage(WrenWebView* view,
                                         const char* name,
                                         const char* json);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_WEB_VIEW_H_

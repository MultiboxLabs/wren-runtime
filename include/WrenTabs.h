// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: tabs (C API). Wren.h includes this header; see Wren.h for threading,
// handles and strings.
//
// Tabs and what they show: navigation, groups, splits, a tab's view and
// DevTools.

#ifndef WREN_PUBLIC_WREN_TABS_H_
#define WREN_PUBLIC_WREN_TABS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Tabs -------------------------------------------------------------------

// 0 once the Browser has closed.
WREN_API int Wren_GetTabCount(WrenBrowser* browser);
// WREN_NO_TAB if `index` is out of range.
WREN_API WrenTabID Wren_GetTabAt(WrenBrowser* browser, int index);
WREN_API WrenTabID Wren_GetActiveTab(WrenBrowser* browser);

// The tab's Browser, borrowed, or NULL if no open tab has this ID.
WREN_API WrenBrowser* Wren_GetTabBrowser(WrenTabID tab);
// The tab's index in its Browser, or -1.
WREN_API int Wren_GetTabIndex(WrenTabID tab);

// Copies a UTF-8 string and returns the length needed, including the NUL.
// An unavailable tab copies "".
WREN_API size_t Wren_CopyTabTitle(WrenTabID tab, char* buffer, size_t size);
WREN_API size_t Wren_CopyTabURL(WrenTabID tab, char* buffer, size_t size);

typedef struct WrenTabOptions {
  // Where to insert the tab; -1 (or past the end) appends it.
  int index;
  bool activate;
} WrenTabOptions;

// Adds a tab showing `url`, or Chromium's New Tab page if it is NULL or
// empty. NULL `options` appends an active tab. `*tab` (optional) receives the
// new tab's ID.
WREN_API WrenResult Wren_AddTab(WrenBrowser* browser,
                                const char* url,
                                const WrenTabOptions* options,
                                WrenTabID* tab);

// Chromium's New Tab command, as Cmd-T runs it in Chrome: a foreground tab
// showing the New Tab page, or an extension's replacement for it. A Browser
// without a tab strip (a popup) opens it in a normal Browser instead.
WREN_API WrenResult Wren_NewTab(WrenBrowser* browser);

WREN_API WrenResult Wren_ActivateTab(WrenTabID tab);
// Requests that the tab close. Pages may cancel it with beforeunload.
WREN_API WrenResult Wren_CloseTab(WrenTabID tab);
WREN_API WrenResult Wren_Navigate(WrenTabID tab, const char* url);

// Where a tab's DevTools is.
typedef enum WrenDevTools {
  WREN_DEVTOOLS_CLOSED = 0,
  // Docked in the tab's view, on that side of the page.
  WREN_DEVTOOLS_RIGHT = 1,
  WREN_DEVTOOLS_LEFT = 2,
  WREN_DEVTOOLS_BOTTOM = 3,
  // In a WREN_BROWSER_DEVTOOLS Browser of its own (on_browser_opened).
  WREN_DEVTOOLS_UNDOCKED = 4,
} WrenDevTools;

// What Chrome's status bubble, at the bottom left of its window, would show
// for a tab. Chromium has no bubble in a Wren window: the app draws its own
// from this, or none.
typedef enum WrenTabStatus {
  WREN_TAB_STATUS_NONE = 0,
  // The address of the link under the pointer or with keyboard focus.
  WREN_TAB_STATUS_LINK = 1,
  // What Chromium is doing for a loading page ("Waiting for example.com…"),
  // while there is no link.
  WREN_TAB_STATUS_LOADING = 2,
} WrenTabStatus;

typedef struct WrenTabState {
  bool loading;
  bool can_go_back;
  bool can_go_forward;
  bool pinned;
  // Discarded to save memory; activating or reloading it loads it again.
  bool discarded;
  // All zero if the tab is in no group.
  WrenTabGroupID group;
  // Why the page is gone, if it is. A crashed tab's view shows nothing: the
  // app shows its own crashed state, and Wren_Reload loads the page again.
  WrenTabCrash crash;
  WrenDevTools devtools;
  // All zero if the tab is in no split.
  WrenSplitID split;
  // Only a tab the Browser shows has one: its active tab and the other tabs
  // of that tab's split.
  WrenTabStatus status;
  // The tab's page is in a <wren-view> element of a web view's page now
  // ("Views in a web view", WrenWebView.h), where Wren_FocusTab gives it the
  // keyboard. An element's page arrives after the page's script asks for it:
  // give a newly active tab the keyboard once this is true. False while the
  // page is in the tab's own view, and from when it leaves the element or
  // is replaced (discarded) until the new page is in one.
  bool in_element;
} WrenTabState;

WREN_API WrenResult Wren_GetTabState(WrenTabID tab, WrenTabState* state);

// Copies the text of WrenTabState.status, "" for none, and returns the length
// needed, including the NUL. A link's address is whole, formatted as Chrome
// shows it; Chrome's bubble elides it to a third of the page's width, and
// after the pointer has rested 1.6 seconds grows to fit it.
WREN_API size_t Wren_CopyTabStatusText(WrenTabID tab,
                                       char* buffer,
                                       size_t size);

// Copies the tab's favicon as PNG into `buffer` if it fits, and returns the
// PNG's size in bytes; 0 if the tab has no favicon (yet). The PNG is at the
// largest scale Chromium keeps (2x on a Mac): draw it at 16 points.
WREN_API size_t Wren_CopyTabFaviconPNG(WrenTabID tab,
                                       uint8_t* buffer,
                                       size_t size);

// Moves the tab within its Browser. Chromium keeps pinned tabs first and a
// group's tabs together, so the final index may differ; see
// Wren_GetTabIndex.
WREN_API WrenResult Wren_MoveTab(WrenTabID tab, int index);
// Pinning moves the tab to the end of the pinned tabs, unpinning to the start
// of the others.
WREN_API WrenResult Wren_SetTabPinned(WrenTabID tab, bool pinned);

// --- Navigation -------------------------------------------------------------

// WREN_ERROR_NAVIGATION_UNAVAILABLE if there is no page to go to.
WREN_API WrenResult Wren_GoBack(WrenTabID tab);
WREN_API WrenResult Wren_GoForward(WrenTabID tab);
// May show Chromium's form-resubmission prompt.
WREN_API WrenResult Wren_Reload(WrenTabID tab);
WREN_API WrenResult Wren_Stop(WrenTabID tab);

// --- Tab groups -------------------------------------------------------------

typedef enum WrenTabGroupColor {
  WREN_TAB_GROUP_GREY = 0,
  WREN_TAB_GROUP_BLUE = 1,
  WREN_TAB_GROUP_RED = 2,
  WREN_TAB_GROUP_YELLOW = 3,
  WREN_TAB_GROUP_GREEN = 4,
  WREN_TAB_GROUP_PINK = 5,
  WREN_TAB_GROUP_PURPLE = 6,
  WREN_TAB_GROUP_CYAN = 7,
  WREN_TAB_GROUP_ORANGE = 8,
} WrenTabGroupColor;

// Puts the tabs, all in one Browser, in a new group; `*group` receives its
// ID. Pinned tabs are unpinned, and the tabs are moved next to each other.
// WREN_ERROR_NOT_ALLOWED if the Browser has no tab strip (a popup).
WREN_API WrenResult Wren_GroupTabs(const WrenTabID* tabs,
                                   size_t count,
                                   WrenTabGroupID* group);
// Adds the tabs to an existing group in their Browser.
WREN_API WrenResult Wren_AddTabsToGroup(const WrenTabID* tabs,
                                        size_t count,
                                        WrenTabGroupID group);
// Removes the tabs from their groups. A group closes with its last tab.
WREN_API WrenResult Wren_UngroupTabs(const WrenTabID* tabs, size_t count);

// The group's Browser, borrowed, or NULL if no open group has this ID.
WREN_API WrenBrowser* Wren_GetTabGroupBrowser(WrenTabGroupID group);
// Copies the group's title (UTF-8; "" if it has none or is unavailable) and
// returns the length needed, including the NUL.
WREN_API size_t Wren_CopyTabGroupTitle(WrenTabGroupID group,
                                       char* buffer,
                                       size_t size);
WREN_API WrenResult Wren_GetTabGroupVisuals(WrenTabGroupID group,
                                            WrenTabGroupColor* color,
                                            bool* collapsed);
// `title` may be NULL or empty for an untitled group.
WREN_API WrenResult Wren_SetTabGroupVisuals(WrenTabGroupID group,
                                            const char* title,
                                            WrenTabGroupColor color,
                                            bool collapsed);

// --- Tab views --------------------------------------------------------------
//
// A tab's view is an NSView the app places in its own window, like any view.
// It holds the tab's page and, while DevTools is docked to the tab, DevTools,
// which Wren lays out as DevTools' own dock menu says (right, left or bottom
// of the page). Chromium follows the view:
// - The page is visible while the view is in a window, has no hidden
//   ancestor, and the window is not covered; otherwise it is hidden, and
//   stops rendering. Remove or hide the views of tabs not on screen.
// - The page's size is the view's; set its frame or autoresizing mask.
//   Chromium sizes a page in whole points and drops any fraction: in a view
//   600.5 points tall the page is 600, and what is behind the view shows in
//   a strip along its bottom edge (one pixel at 2x). Give the view, and the
//   side panel view, a whole number of points each way.
// - The page's scale follows the window's screen (devicePixelRatio).
// - The view stays the same while the tab lives, including when Chromium
//   replaces the page inside it (discarding) and when the tab moves to
//   another Browser.

// The tab's NSView (borrowed; valid until the tab closes), or NULL.
WREN_API void* Wren_GetTabView(WrenTabID tab);

// Gives the tab's page keyboard focus within its view's window, restoring the
// element that had it, as Chrome does when switching tabs. A page that wants
// the address field instead (the New Tab page) gets on_focus_location_bar.
// Does not make the window key, and does nothing while the view is in no
// window. A tab a web view's page shows (WrenWebView.h) takes focus there,
// in that view's window.
WREN_API WrenResult Wren_FocusTab(WrenTabID tab);

// --- Split tabs -------------------------------------------------------------
//
// A split shows two tabs of one Browser side by side, as one: Chromium's
// split view. Each side is an ordinary tab, with its own ID, page, history
// and DevTools; one of them is the active tab. While either side is active,
// show both tabs' views, divided by the ratio; the app draws its own divider
// and calls Wren_SetSplitRatio as it is dragged. Focusing a side's page makes
// it the active tab. Chromium keeps a split's tabs together: closing one side,
// or moving, grouping or pinning one side alone, ends the split.

typedef enum WrenSplitLayout {
  // The first tab on the left, the second on the right.
  WREN_SPLIT_SIDE_BY_SIDE = 0,
  // The first tab above the second.
  WREN_SPLIT_STACKED = 1,
} WrenSplitLayout;

typedef struct WrenSplitState {
  // The first (left or top) and second tab.
  WrenTabID tabs[2];
  WrenSplitLayout layout;
  // The first tab's share of the width (or height), between 0 and 1.
  double ratio;
} WrenSplitState;

// Puts two tabs of one Browser in a new split, `first` first, next to each
// other; `first`'s group and pinning apply to both. `*split` (optional)
// receives its ID. WREN_ERROR_NOT_ALLOWED if the Browser is not a normal one
// or either tab is already in a split.
WREN_API WrenResult Wren_SplitTabs(WrenTabID first,
                                   WrenTabID second,
                                   WrenSplitLayout layout,
                                   WrenSplitID* split);
// Chromium's New Split Tab command: a new tab next to the active one, in a
// split with it, showing Chromium's tab picker for the second side.
// WREN_ERROR_NOT_ALLOWED if the Browser is not a normal one or the active
// tab is already in a split.
WREN_API WrenResult Wren_NewSplitTab(WrenBrowser* browser);
// Ends the split; both tabs stay, in their places.
WREN_API WrenResult Wren_Unsplit(WrenSplitID split);
WREN_API WrenResult Wren_GetSplitState(WrenSplitID split,
                                       WrenSplitState* state);
// `ratio` is strictly between 0 and 1. Pass `dragging` while the user is
// still dragging the divider; Chromium saves the ratio once it is false.
WREN_API WrenResult Wren_SetSplitRatio(WrenSplitID split,
                                       double ratio,
                                       bool dragging);
WREN_API WrenResult Wren_SetSplitLayout(WrenSplitID split,
                                        WrenSplitLayout layout);
// Swaps the two tabs' places.
WREN_API WrenResult Wren_SwapSplitTabs(WrenSplitID split);

// --- DevTools ---------------------------------------------------------------
//
// DevTools stays with the page it inspects. Docked, it is part of the tab's
// view; DevTools' own dock menu moves it to another side or to its own
// window (a WREN_BROWSER_DEVTOOLS Browser, which reaches on_browser_opened),
// and from there back. Only tabs of a normal Browser dock; others always open
// undocked.

// Opens DevTools for the tab: docked on a side, or WREN_DEVTOOLS_UNDOCKED in
// its own window. WREN_DEVTOOLS_CLOSED opens it where DevTools was last
// (right at first). An open DevTools is shown, not moved.
// WREN_ERROR_NOT_ALLOWED if Chromium's policy forbids DevTools for the page.
WREN_API WrenResult Wren_OpenDevTools(WrenTabID tab, WrenDevTools place);
// Closes the tab's DevTools, docked or not. DevTools may cancel it, as its
// own close button can.
WREN_API WrenResult Wren_CloseDevTools(WrenTabID tab);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_TABS_H_

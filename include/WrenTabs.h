// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: tabs (C API). Wren.h includes this header. Wren.h also explains
// threads, handles and strings.
//
// This header covers tabs and what they show: navigation, groups, splits, a
// tab's view, and DevTools.

#ifndef WREN_PUBLIC_WREN_TABS_H_
#define WREN_PUBLIC_WREN_TABS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Tabs -------------------------------------------------------------------

// The number of the Browser's tabs. 0 once the Browser has closed.
WREN_API int Wren_GetTabCount(WrenBrowser* browser);
// The tab at `index`. WREN_NO_TAB if `index` is out of range.
WREN_API WrenTabID Wren_GetTabAt(WrenBrowser* browser, int index);
WREN_API WrenTabID Wren_GetActiveTab(WrenBrowser* browser);

// Returns the tab's Browser, borrowed. NULL if no open tab has this ID.
WREN_API WrenBrowser* Wren_GetTabBrowser(WrenTabID tab);
// Returns the tab's index in its Browser, or -1.
WREN_API int Wren_GetTabIndex(WrenTabID tab);

// Each copies a UTF-8 string and returns the length needed, including the
// NUL. A tab that is not available copies "".
WREN_API size_t Wren_CopyTabTitle(WrenTabID tab, char* buffer, size_t size);
WREN_API size_t Wren_CopyTabURL(WrenTabID tab, char* buffer, size_t size);

typedef struct WrenTabOptions {
  // Where to insert the tab. -1, or an index past the end, adds it at the end.
  int index;
  bool activate;
} WrenTabOptions;

// Adds a tab showing `url`. If `url` is NULL or empty, the tab shows
// Chromium's New Tab page. NULL `options` adds an active tab at the end. If
// `tab` is not NULL, it receives the new tab's ID.
WREN_API WrenResult Wren_AddTab(WrenBrowser* browser,
                                const char* url,
                                const WrenTabOptions* options,
                                WrenTabID* tab);

// Runs Chromium's New Tab command, as Cmd-T does in Chrome. It opens a tab in
// front, showing the New Tab page or an extension's replacement for it. A
// Browser without a tab strip, such as a popup, opens the tab in a normal
// Browser instead.
WREN_API WrenResult Wren_NewTab(WrenBrowser* browser);

WREN_API WrenResult Wren_ActivateTab(WrenTabID tab);
// Asks the tab to close. Its page may cancel the close with beforeunload. When
// a Browser's last tab closes, the Browser closes too (WrenBrowser.h).
WREN_API WrenResult Wren_CloseTab(WrenTabID tab);
// Loads `url` in the tab.
WREN_API WrenResult Wren_Navigate(WrenTabID tab, const char* url);

// Where a tab's DevTools is.
typedef enum WrenDevTools {
  WREN_DEVTOOLS_CLOSED = 0,
  // Docked in the tab's view, on that side of the page.
  WREN_DEVTOOLS_RIGHT = 1,
  WREN_DEVTOOLS_LEFT = 2,
  WREN_DEVTOOLS_BOTTOM = 3,
  // In a WREN_BROWSER_DEVTOOLS Browser of its own, which reaches
  // on_browser_opened.
  WREN_DEVTOOLS_UNDOCKED = 4,
} WrenDevTools;

// What Chrome's status bubble, at the bottom left of its window, would show
// for a tab. Chromium draws no status bubble in a Wren window. The app draws
// its own from this, or none.
typedef enum WrenTabStatus {
  WREN_TAB_STATUS_NONE = 0,
  // The address of the link under the pointer, or of the link with keyboard
  // focus.
  WREN_TAB_STATUS_LINK = 1,
  // What Chromium is doing for a loading page, such as "Waiting for
  // example.com…", while there is no link to show.
  WREN_TAB_STATUS_LOADING = 2,
} WrenTabStatus;

typedef struct WrenTabState {
  bool loading;
  bool can_go_back;
  bool can_go_forward;
  bool pinned;
  // True if Chromium discarded the page to save memory. Activating or
  // reloading the tab loads it again.
  bool discarded;
  // All zero if the tab is in no group.
  WrenTabGroupID group;
  // Why the page is gone, if it is. A crashed tab's view shows nothing. The
  // app shows its own crashed state, and Wren_Reload loads the page again.
  WrenTabCrash crash;

  WrenDevTools devtools;
  // All zero if the tab is in no split.
  WrenSplitID split;
  // Only a tab the Browser shows has a status. That is its active tab, and
  // the other tab of the active tab's split.
  WrenTabStatus status;
  // True while the tab's page is in a <wren-view> element of a web view's
  // page ("Views in a web view", WrenWebView.h). Wren_FocusTab gives it the
  // keyboard there.
  //
  // An element gets its page only after the page's script asks for it. So
  // give a newly active tab the keyboard once this is true. It is false while
  // the page is in the tab's own view. It is also false from when the page
  // leaves the element, or is replaced by a discard, until the new page is in
  // an element.
  bool in_element;
} WrenTabState;

WREN_API WrenResult Wren_GetTabState(WrenTabID tab, WrenTabState* state);

// Copies the text of WrenTabState.status, or "" for none. Returns the length
// needed, including the NUL.
//
// A link's address is whole, formatted as Chrome shows it. Chrome's bubble
// shortens it to a third of the page's width. Once the pointer has rested on
// the link for 1.6 seconds, the bubble grows to fit the whole address.
WREN_API size_t Wren_CopyTabStatusText(WrenTabID tab,
                                       char* buffer,
                                       size_t size);

// Copies the tab's favicon as a PNG into `buffer` if it fits, and returns the
// PNG's size in bytes. Returns 0 if the tab has no favicon, or none yet. The
// PNG is at the largest scale Chromium keeps, 2x on a Mac. Draw it at 16
// points.
WREN_API size_t Wren_CopyTabFaviconPNG(WrenTabID tab,
                                       uint8_t* buffer,
                                       size_t size);

// Moves the tab to `index` within its Browser. Chromium keeps pinned tabs
// first and a group's tabs together, so the tab may end up at another index.
// Wren_GetTabIndex gives the index it ends up at.
WREN_API WrenResult Wren_MoveTab(WrenTabID tab, int index);
// Pins or unpins the tab. Pinning moves the tab to the end of the pinned
// tabs. Unpinning moves it to the start of the others.
WREN_API WrenResult Wren_SetTabPinned(WrenTabID tab, bool pinned);

// --- Navigation -------------------------------------------------------------

// Each returns WREN_ERROR_NAVIGATION_UNAVAILABLE if there is no page to go to.
WREN_API WrenResult Wren_GoBack(WrenTabID tab);
WREN_API WrenResult Wren_GoForward(WrenTabID tab);
// Reloads the page. Chromium may ask the user before sending a form again.
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

// Puts the tabs in a new group. They must all be in one Browser. `*group`
// receives the group's ID. Pinned tabs are unpinned, and the tabs move next to
// each other. Returns WREN_ERROR_NOT_ALLOWED if the Browser has no tab strip,
// such as a popup.
WREN_API WrenResult Wren_GroupTabs(const WrenTabID* tabs,
                                   size_t count,
                                   WrenTabGroupID* group);
// Adds the tabs to an existing group in their Browser.
WREN_API WrenResult Wren_AddTabsToGroup(const WrenTabID* tabs,
                                        size_t count,
                                        WrenTabGroupID group);
// Takes the tabs out of their groups. A group closes when its last tab leaves
// it.
WREN_API WrenResult Wren_UngroupTabs(const WrenTabID* tabs, size_t count);

// Returns the group's Browser, borrowed. NULL if no open group has this ID.
WREN_API WrenBrowser* Wren_GetTabGroupBrowser(WrenTabGroupID group);
// Copies the group's title, in UTF-8, and returns the length needed, including
// the NUL. Copies "" if the group has no title or is not available.
WREN_API size_t Wren_CopyTabGroupTitle(WrenTabGroupID group,
                                       char* buffer,
                                       size_t size);
// Reads the group's colour, and whether it is collapsed.
WREN_API WrenResult Wren_GetTabGroupVisuals(WrenTabGroupID group,
                                            WrenTabGroupColor* color,
                                            bool* collapsed);
// Sets the group's title, colour and collapsed state. `title` may be NULL or
// empty for a group with no title.
WREN_API WrenResult Wren_SetTabGroupVisuals(WrenTabGroupID group,
                                            const char* title,
                                            WrenTabGroupColor color,
                                            bool collapsed);

// --- Tab views --------------------------------------------------------------
//
// A tab's view is an NSView. The app places it in its own window, like any
// other view. It holds the tab's page. While DevTools is docked to the tab, it
// holds DevTools too, which Wren lays out on the side that DevTools' own dock
// menu chose: right, left or below the page.
//
// - Chromium draws the page only while the view is in a window, no ancestor
//   of the view is hidden, and the window is not covered. Otherwise the page
//   is hidden and stops drawing. Remove or hide the views of tabs that are not
//   on screen.
// - The page is the view's size. Set the view's frame or its autoresizing
//   mask.
// - Give the view, and the side panel view, a whole number of points each
//   way. Chromium sizes a page in whole points and drops any fraction. In a
//   view 600.5 points tall the page is 600 points tall, and whatever is behind
//   the view shows in a strip along its bottom edge, one pixel at 2x.
// - The page's scale, its devicePixelRatio, follows the window's screen.
// - The view stays the same for the whole life of the tab. That includes when
//   Chromium replaces the page inside it by discarding it, and when the tab
//   moves to another Browser.

// Returns the tab's NSView, or NULL. Borrowed, and valid until the tab closes.
WREN_API void* Wren_GetTabView(WrenTabID tab);

// Gives the tab's page keyboard focus within its view's window. It restores
// focus to the element that had it, as Chrome does when switching tabs.
//
// A page that wants the address field to have focus instead, such as the New
// Tab page, gets on_focus_location_bar. The call does not make the window key.
// It does nothing while the view is in no window. A tab that a web view's page
// shows (WrenWebView.h) takes focus there, in that web view's window.
WREN_API WrenResult Wren_FocusTab(WrenTabID tab);

// --- Split tabs -------------------------------------------------------------
//
// A split shows two tabs of one Browser side by side, as one. It is Chromium's
// split view.
//
// - Each side is an ordinary tab, with its own ID, page, history and
//   DevTools. One of the two is the active tab.
// - While either side is active, show both tabs' views, divided by the
//   ratio. The app draws its own divider, and calls Wren_SetSplitRatio as the
//   user drags it.
// - Focusing a side's page makes that side the active tab.
// - Chromium keeps a split's tabs together. Closing one side ends the split.
//   So does moving, grouping or pinning one side alone.

typedef enum WrenSplitLayout {
  // The first tab on the left, the second on the right.
  WREN_SPLIT_SIDE_BY_SIDE = 0,
  // The first tab above the second.
  WREN_SPLIT_STACKED = 1,
} WrenSplitLayout;

typedef struct WrenSplitState {
  // The first tab, on the left or at the top, then the second.
  WrenTabID tabs[2];
  WrenSplitLayout layout;
  // The first tab's share of the width, or of the height, between 0 and 1.
  double ratio;
} WrenSplitState;

// Puts two tabs of one Browser in a new split, `first` first, and moves them
// next to each other. `first`'s group and pinning then apply to both. If
// `split` is not NULL, it receives the split's ID.
//
// Returns WREN_ERROR_NOT_ALLOWED if the Browser is not a normal one, or if
// either tab is already in a split.
WREN_API WrenResult Wren_SplitTabs(WrenTabID first,
                                   WrenTabID second,
                                   WrenSplitLayout layout,
                                   WrenSplitID* split);
// Runs Chromium's New Split Tab command. It adds a new tab next to the active
// one, in a split with it, and shows Chromium's tab picker on the new side.
//
// Returns WREN_ERROR_NOT_ALLOWED if the Browser is not a normal one, or if the
// active tab is already in a split.
WREN_API WrenResult Wren_NewSplitTab(WrenBrowser* browser);
// Ends the split. Both tabs stay open, where they are.
WREN_API WrenResult Wren_Unsplit(WrenSplitID split);
WREN_API WrenResult Wren_GetSplitState(WrenSplitID split,
                                       WrenSplitState* state);
// Sets the first tab's share. `ratio` must be strictly between 0 and 1. Pass
// `dragging` as true while the user is still dragging the divider. Chromium
// saves the ratio once a call passes false.
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
// view. DevTools' own dock menu moves it to another side, or to a window of
// its own, and back. That window is a WREN_BROWSER_DEVTOOLS Browser, which
// reaches on_browser_opened. Only tabs of a normal Browser can dock DevTools.
// For any other tab, DevTools always opens in its own window.

// Opens DevTools for the tab, docked on the side `place` names, or in its own
// window for WREN_DEVTOOLS_UNDOCKED. WREN_DEVTOOLS_CLOSED opens it where
// DevTools was last, which is on the right the first time. If DevTools is
// already open, it is shown where it is, not moved.
//
// Returns WREN_ERROR_NOT_ALLOWED if Chromium's policy forbids DevTools for the
// page.
WREN_API WrenResult Wren_OpenDevTools(WrenTabID tab, WrenDevTools place);
// Closes the tab's DevTools, docked or not. DevTools may cancel the close, as
// it can when the user clicks its own close button.
WREN_API WrenResult Wren_CloseDevTools(WrenTabID tab);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_TABS_H_

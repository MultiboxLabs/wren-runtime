// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: a Profile's bookmarks (C API). Wren.h includes this header. Wren.h
// also explains threads, handles and strings.
//
// The app reads and changes a Profile's bookmarks as state. It can draw its
// own bookmarks bar from them, or keep folders of its own. They are
// Chromium's bookmarks, the same ones chrome://bookmarks, the address field's
// suggestions, the star and sync use.
//
// - Each Profile has one tree of bookmarks. An off-the-record Profile uses its
//   original Profile's tree, as Chrome's incognito windows do.
// - Chromium loads the tree from disk after the Profile opens. Until then the
//   tree is empty, and WrenBookmarksState.loaded is false.
// - The top of the tree is Chromium's four folders, each with a constant ID.
//   They are the bookmarks bar, Other bookmarks, Mobile bookmarks and the
//   managed folder. Chrome shows Mobile bookmarks only once it holds
//   something. The managed folder holds the bookmarks of the administrator's
//   policy. It is read-only, and empty without the policy.
// - When the user keeps bookmarks in their Google Account, the bar, Other and
//   Mobile each hold both the account's items and the device's. Chromium
//   keeps them in one order, and Chrome shows them in that order.
// - Every other item, bookmark or folder, has Chromium's ID. The ID is saved
//   on disk and stays the same across launches, so an app may store it, for
//   example to find its own folder again.
// - Chromium gives an item a new ID only when its file has two items with the
//   same ID. The user may also rename or remove the app's folder in
//   chrome://bookmarks. So when a stored ID is gone, giving
//   WREN_ERROR_BOOKMARK_UNAVAILABLE, the app looks for its folder again.
// - A WrenBookmarkFolderID (WrenUI.h) is a WrenBookmarkID. For the bar, Other
//   and Mobile, Chromium's ID of the folder and its constant name the same
//   folder.

#ifndef WREN_PUBLIC_WREN_BOOKMARKS_H_
#define WREN_PUBLIC_WREN_BOOKMARKS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t WrenBookmarkID;
#define WREN_NO_BOOKMARK 0
// Chromium's four folders.
#define WREN_BOOKMARKS_BAR (-1)
#define WREN_OTHER_BOOKMARKS (-2)
#define WREN_MOBILE_BOOKMARKS (-3)
#define WREN_MANAGED_BOOKMARKS (-4)

typedef struct WrenBookmarksState {
  // True once Chromium has loaded the Profile's bookmarks. Until then every
  // folder is empty and Wren refuses changes. WREN_CHANGE_BOOKMARKS reports
  // the load.
  bool loaded;
  // True if the administrator's policy lets the user add, change and remove
  // bookmarks. This is Chromium's EditBookmarksEnabled. When it is false,
  // changes return WREN_ERROR_NOT_ALLOWED.
  bool editable;
} WrenBookmarksState;

// Reads the state of the Profile's bookmarks.
//
// Once the app has read them, WREN_CHANGE_BOOKMARKS reports each change: an
// item added, removed, moved, renamed or given another URL, an item's icon
// loaded, or the bookmarks loaded. The app then reads again what it shows.
WREN_API WrenResult Wren_GetBookmarksState(WrenProfile* profile,
                                           WrenBookmarksState* state);

typedef struct WrenBookmarkItem {
  // True for a folder. False for a bookmark, which has a URL.
  bool folder;
  // True for one of Chromium's four folders, which cannot be renamed, moved or
  // removed.
  bool permanent;
  // The folder that holds the item. One of Chromium's four folders is given
  // by its constant. WREN_NO_BOOKMARK for the four folders themselves.
  WrenBookmarkID parent;
  // True if the item is kept in the user's Google Account, not only on this
  // device.
  bool account;
  // True for the administrator's items, in the managed folder. They are
  // read-only.
  bool managed;
  // Times in milliseconds since 1970, UTC, as Chromium keeps them. 0 means
  // never, or not known.
  //
  // - `added_ms` is when the item was added.
  // - `last_used_ms` is when a bookmark was last opened, through
  //   Wren_OpenBookmarkItem or any of Chromium's own UI. Chromium records it
  //   once history has the visit, and tells no one. The app reads it when it
  //   needs it.
  // - `folder_changed_ms` is when a folder last had an item added, moved in,
  //   moved out or removed.
  int64_t added_ms;
  int64_t last_used_ms;
  int64_t folder_changed_ms;
} WrenBookmarkItem;

// Returns WREN_ERROR_BOOKMARK_UNAVAILABLE for an ID that names no item. That
// is an item that was removed, or any ID before the bookmarks have loaded.
WREN_API WrenResult Wren_GetBookmarkItem(WrenProfile* profile,
                                         WrenBookmarkID item,
                                         WrenBookmarkItem* state);
// The number of items in a folder. 0 for anything that is not a folder.
WREN_API size_t Wren_GetBookmarkChildCount(WrenProfile* profile,
                                           WrenBookmarkID folder);
// The folder's item at `index`, in the folder's order. WREN_NO_BOOKMARK past
// the end.
WREN_API WrenBookmarkID Wren_GetBookmarkChildAt(WrenProfile* profile,
                                                WrenBookmarkID folder,
                                                size_t index);
// Copies the item's title as Chrome shows it. For the four folders that is
// Chromium's own name for them.
WREN_API size_t Wren_CopyBookmarkItemTitle(WrenProfile* profile,
                                           WrenBookmarkID item,
                                           char* buffer,
                                           size_t size);
// Copies a bookmark's URL. "" for a folder.
WREN_API size_t Wren_CopyBookmarkItemURL(WrenProfile* profile,
                                         WrenBookmarkID item,
                                         char* buffer,
                                         size_t size);
// Copies a bookmark's favicon as a PNG into `buffer` if it fits, and returns
// the PNG's size in bytes, as Wren_CopyTabFaviconPNG (WrenTabs.h) does.
//
// Returns 0 for a folder, for a page Chromium has no icon for, and for an icon
// not loaded yet. Chromium loads an icon the first time it is asked for.
// WREN_CHANGE_BOOKMARKS follows once it has loaded.
WREN_API size_t Wren_CopyBookmarkItemIconPNG(WrenProfile* profile,
                                             WrenBookmarkID item,
                                             uint8_t* buffer,
                                             size_t size);

// --- Finding -----------------------------------------------------------------
//
// Each call copies the IDs of what it finds into `items`, as many as `size`
// holds, and returns how many it found. Neither ever finds Chromium's four
// folders.

// Finds the bookmarks of exactly `url`. The URL is compared as Chromium keeps
// it, normalised for a trailing slash and the scheme's case. Finds nothing for
// an invalid URL.
WREN_API size_t Wren_FindBookmarksByURL(WrenProfile* profile,
                                        const char* url,
                                        WrenBookmarkID* items,
                                        size_t size);
// Finds bookmarks and folders whose title has every word of `query`, and
// bookmarks whose URL does. The matches are those chrome://bookmarks' search
// finds, in no set order.
WREN_API size_t Wren_SearchBookmarks(WrenProfile* profile,
                                     const char* query,
                                     WrenBookmarkID* items,
                                     size_t size);

// --- Changes -----------------------------------------------------------------
//
// Each change is done before the call returns, as Chrome's bookmark manager
// does it.
//
// - WREN_ERROR_BOOKMARK_UNAVAILABLE means the item is gone.
// - WREN_ERROR_NOT_ALLOWED means the user could not make the change either.
//   That is before the bookmarks load, without EditBookmarksEnabled, in the
//   managed folder, and for a change to one of Chromium's four folders.
//
// An `index` is a place among the folder's items, starting at 0.

// Adds a bookmark to `folder` at `index`. With a NULL `url` it adds a folder.
// `index` may be up to the folder's count, which is its end. If `item` is not
// NULL, it gets the new item's ID.
//
// When the user keeps bookmarks in their account, an item added to one of
// Chromium's four folders goes to the account's side, as Chrome's do.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for an invalid URL, a `folder` that is
// not a folder, or an index past the end.
WREN_API WrenResult Wren_AddBookmarkItem(WrenProfile* profile,
                                         WrenBookmarkID folder,
                                         size_t index,
                                         const char* title,
                                         const char* url,
                                         WrenBookmarkID* item);
// Renames the item.
WREN_API WrenResult Wren_SetBookmarkItemTitle(WrenProfile* profile,
                                              WrenBookmarkID item,
                                              const char* title);
// Gives a bookmark another URL. Returns WREN_ERROR_INVALID_ARGUMENT for a
// folder or an invalid URL.
WREN_API WrenResult Wren_SetBookmarkItemURL(WrenProfile* profile,
                                            WrenBookmarkID item,
                                            const char* url);
// Moves the item into `folder`, where it ends up at `index`. `index` may be up
// to the folder's count. Within the same folder, it may be up to the count
// less one.
//
// An item moves between the account and this device at once. Chrome asks the
// user first, but Wren does not. The app may ask if it wants to.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for an index past the end, or for a
// `folder` that is not a folder, is the item itself, or is inside it.
WREN_API WrenResult Wren_MoveBookmarkItem(WrenProfile* profile,
                                          WrenBookmarkID item,
                                          WrenBookmarkID folder,
                                          size_t index);
// Removes the item. Removing a folder removes everything in it.
WREN_API WrenResult Wren_RemoveBookmarkItem(WrenProfile* profile,
                                            WrenBookmarkID item);

// Opens Chromium's bookmark editor for the item, as a browser-modal dialog
// over `browser`'s window. Chrome's bar opens the same editor from Edit… for
// a bookmark, to change its name, URL and folder, and from Rename… for a
// folder.
//
// What the user saves is a change like any other. Returns
// WREN_ERROR_NOT_ALLOWED wherever a change would be refused, as listed above,
// and inside on_popover.
WREN_API WrenResult Wren_EditBookmarkItem(WrenBrowser* browser,
                                          WrenBookmarkID item);
// Opens the same editor to add an item in `folder` at `index`. `index` may be
// up to the folder's count.
//
// Without `new_folder`, it adds a bookmark, as Chrome's bar's Add page… does.
// It starts with the active tab's page and title, which the user may change.
// With `new_folder`, it adds a folder, as Add folder… does.
//
// Returns the errors of Wren_AddBookmarkItem and Wren_EditBookmarkItem.
WREN_API WrenResult Wren_ShowAddBookmarkDialog(WrenBrowser* browser,
                                               WrenBookmarkID folder,
                                               size_t index,
                                               bool new_folder);

// Opens a bookmark in `browser` as Chrome's bookmarks bar does. History and
// the address field count it as a page the user chose, not a link followed.
// If `tab` is not NULL, it gets the tab that shows the page.
//
// WREN_OPEN_NEW_WINDOW opens a new Browser with the tab, and
// on_browser_opened gives that Browser to the app.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for WREN_OPEN_SWITCH_TO_TAB and for a
// folder, WREN_ERROR_BOOKMARK_UNAVAILABLE for an item that is gone, and
// WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API WrenResult Wren_OpenBookmarkItem(WrenBrowser* browser,
                                          WrenBookmarkID item,
                                          WrenOpenDisposition disposition,
                                          WrenTabID* tab);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_BOOKMARKS_H_

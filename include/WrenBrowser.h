// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Profiles and Browsers (C API). Wren.h includes this header. Wren.h
// also explains threads, handles and strings.
//
// This header covers Profiles, Browsers and the windows that show them. That
// includes several Browsers in one window, full screen and the side panel.

#ifndef WREN_PUBLIC_WREN_BROWSER_H_
#define WREN_PUBLIC_WREN_BROWSER_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Profiles ---------------------------------------------------------------

// Receives one reference to the Profile on success. On failure it receives
// NULL and the reason.
typedef void (*WrenProfileCallback)(void* context,
                                    WrenProfile* profile,
                                    WrenResult result);

// Opens a Profile by its ID. That may be a persistent Profile, which Chromium
// loads if needed, or an off-the-record or ephemeral Profile that is open.
// NULL opens the startup Profile: the one Chromium started this launch on.
// As in Chrome, that is the persistent Profile whose Browser was active last,
// and "Default" in a new user_data_dir. It stays the same until the app quits.
WREN_API void Wren_OpenProfile(const char* id,
                               WrenProfileCallback callback,
                               void* context);

// Creates a persistent Profile. With an empty or NULL name, Chromium chooses
// one.
WREN_API void Wren_CreateProfile(const char* display_name,
                                 WrenProfileCallback callback,
                                 void* context);

// Creates an ephemeral Profile.
WREN_API void Wren_CreateEphemeralProfile(WrenProfileCallback callback,
                                          void* context);

// Gets the off-the-record Profile of a persistent or ephemeral Profile.
// `*result` receives one reference.
WREN_API WrenResult Wren_GetOffTheRecordProfile(WrenProfile* original,
                                                WrenProfile** result);

// While the app holds a reference to a persistent or ephemeral Profile,
// Chromium keeps the Profile loaded.
WREN_API void Wren_RetainProfile(WrenProfile* profile);
WREN_API void Wren_ReleaseProfile(WrenProfile* profile);

WREN_API bool Wren_IsProfileOpen(WrenProfile* profile);
WREN_API WrenProfileKind Wren_GetProfileKind(WrenProfile* profile);
// Returns the Profile an off-the-record Profile belongs to. For any other
// Profile, returns `profile` itself. Borrowed.
WREN_API WrenProfile* Wren_GetOriginalProfile(WrenProfile* profile);

// Copies the Profile's ID, in UTF-8, and returns the length needed, including
// the NUL. A persistent or ephemeral Profile's ID is its directory name. An
// off-the-record Profile's ID is its original Profile's ID followed by
// ":off-the-record".
WREN_API size_t Wren_CopyProfileID(WrenProfile* profile,
                                   char* buffer,
                                   size_t size);

// Renames a persistent Profile, as Chrome's settings page does. `name` is
// UTF-8, and Wren trims whitespace at either end. The list has the new name
// when the call returns, and WREN_CHANGE_PROFILES reports it. Returns
// WREN_ERROR_INVALID_ARGUMENT for a name that is empty after trimming, or for
// an off-the-record or ephemeral Profile, and WREN_ERROR_CLOSED once the
// Profile has closed.
WREN_API WrenResult Wren_RenameProfile(WrenProfile* profile, const char* name);

// Deletes a persistent Profile, as Chrome's profile picker does. `callback`
// gets the result.
//
// First Wren closes the Profile's Browsers and those of its off-the-record
// Profiles, as quitting closes them. Each page's beforeunload may ask. If one
// cancels, nothing is deleted, and `callback` gets WREN_ERROR_CANCELLED. Then
// Chromium cancels the Profile's downloads in progress without asking, and
// removes its data. Its directory goes when the app quits.
//
// When `callback` gets WREN_OK, the Profile's handle and those of its
// off-the-record Profiles have closed, and so have their web views' pages.
// WREN_CHANGE_PROFILE_CLOSED reports each handle the app has. The list no
// longer has the Profile, and Wren_OpenProfile refuses its ID. The startup
// Profile can be deleted too. Wren_OpenProfile(NULL) then fails until the next
// launch, which starts on another Profile, as Chrome does.
//
// A delete from Chromium's own pages, such as chrome://profile-picker in a
// tab, closes the handles and pages the same way.
//
// `callback` gets one of these errors without anything being done:
//
// - WREN_ERROR_INVALID_ARGUMENT for an off-the-record or ephemeral Profile.
// - WREN_ERROR_CLOSED once the Profile has closed.
// - WREN_ERROR_NOT_ALLOWED for the last persistent Profile, which Chrome
//   would replace with a new one, one being deleted already, one with a
//   Browser in a Wren_CloseBrowsers request, or while a quit is under way.
WREN_API void Wren_DeleteProfile(WrenProfile* profile,
                                 WrenResultCallback callback,
                                 void* context);

// The persistent Profiles in user_data_dir, loaded or not: their number, and
// the ID and name of each.
WREN_API size_t Wren_GetProfileListCount(void);
WREN_API size_t Wren_CopyProfileListID(size_t index, char* buffer, size_t size);
WREN_API size_t Wren_CopyProfileListName(size_t index,
                                         char* buffer,
                                         size_t size);
// Gets the avatar ID of the Profile at `index` into `avatar`. Returns false
// if there is no Profile at `index`.
WREN_API bool Wren_GetProfileListAvatar(size_t index, size_t* avatar);
// Copies a listed Profile's avatar as a PNG, as Chromium draws it, loaded or
// not: the placeholder in the Profile's colours, or Chromium's picture.
// `id` is the Profile's ID. `points` is 1 to 1024 and `scale` is at most 4.
//
// Chromium downloads a picture's large image from www.gstatic.com once, as
// Chrome does, and draws the small one built into it until then.
// WREN_CHANGE_PROFILES reports the large one.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 if no listed Profile has the ID.
WREN_API size_t Wren_CopyProfileAvatarPNG(const char* id,
                                          int points,
                                          double scale,
                                          uint8_t* buffer,
                                          size_t size);

// --- Avatars and colours ----------------------------------------------------

// The avatars Chrome's settings page offers, in its order: their number, and
// the ID of each. The first is the placeholder, a silhouette in the
// Profile's colours. The rest are Chromium's pictures. An avatar ID is
// Chromium's own, the same from one version to the next.
// Wren_GetAvatarAt returns SIZE_MAX, an ID Chromium does not know, for an
// index out of range.
WREN_API size_t Wren_GetAvatarCount(void);
WREN_API size_t Wren_GetAvatarAt(size_t index);
// Copies the avatar's label in UTF-8, such as "Avatar cat", and returns the
// length needed, including the NUL. Returns 0 for an ID Chromium does not
// know.
WREN_API size_t Wren_CopyAvatarLabel(size_t avatar, char* buffer, size_t size);
// Copies the avatar as a PNG, `points` square at `scale`, as the settings
// page shows it: the placeholder in Chromium's default colours, and a
// picture's small image. `points` is 1 to 1024 and `scale` is at most 4.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 for an ID Chromium does not know.
WREN_API size_t Wren_CopyAvatarPNG(size_t avatar,
                                   int points,
                                   double scale,
                                   uint8_t* buffer,
                                   size_t size);

// Sets a persistent Profile's avatar, as Chrome's settings page does. Any ID
// Chromium knows is accepted, including older ones it no longer offers. The
// list has the new avatar when the call returns, and WREN_CHANGE_PROFILES
// reports it. Returns WREN_ERROR_INVALID_ARGUMENT for an unknown ID, or for
// an off-the-record or ephemeral Profile, and WREN_ERROR_CLOSED once the
// Profile has closed.
WREN_API WrenResult Wren_SetProfileAvatar(WrenProfile* profile, size_t avatar);

// A persistent Profile's colour is its theme's, as in Chrome, where
// chrome://settings/appearance sets it. Chromium makes its colours from it,
// in light and dark. It keeps colours made from it with the Profile's list
// entry, for its profile picker and the placeholder avatar. Wren keeps them
// following the theme, even for a Profile with no Browser. WREN_CHANGE_PROFILES
// reports a new colour, from the app or from Chromium's pages.
//
// Sets the colour, an opaque 0xFFRRGGBB, as Chrome's colour picker does.
// Returns WREN_ERROR_INVALID_ARGUMENT for a colour that is not opaque, or for
// an off-the-record or ephemeral Profile, and WREN_ERROR_CLOSED once the
// Profile has closed.
WREN_API WrenResult Wren_SetProfileColor(WrenProfile* profile, uint32_t color);
// Goes back to Chromium's default colours, as the picker's default does.
// Returns the errors Wren_SetProfileColor returns.
WREN_API WrenResult Wren_ResetProfileColor(WrenProfile* profile);
// Gets the colour set, by the app or in Chromium's pages, into `color`.
// Returns false if the Profile has none, or has closed, or is not
// persistent.
WREN_API bool Wren_GetProfileColor(WrenProfile* profile, uint32_t* color);

// --- Clearing browsing data -------------------------------------------------

// The kinds of data Chrome's Clear browsing data dialog clears.
typedef enum WrenBrowsingData {
  WREN_BROWSING_DATA_HISTORY = 1u << 0,
  WREN_BROWSING_DATA_DOWNLOADS = 1u << 1,
  // Cached images and files.
  WREN_BROWSING_DATA_CACHE = 1u << 2,
  // Cookies and the storage of websites.
  WREN_BROWSING_DATA_SITE_DATA = 1u << 3,
  // Autofill's form data.
  WREN_BROWSING_DATA_FORM_DATA = 1u << 4,
  // Permissions and other settings the user gave sites.
  WREN_BROWSING_DATA_SITE_SETTINGS = 1u << 5,
  // The storage of installed web apps' sites.
  WREN_BROWSING_DATA_HOSTED_APPS_DATA = 1u << 6,
} WrenBrowsingData;

// The periods the dialog offers. Each but the last ends now.
typedef enum WrenTimePeriod {
  WREN_TIME_PERIOD_LAST_15_MINUTES = 0,
  WREN_TIME_PERIOD_LAST_HOUR = 1,
  WREN_TIME_PERIOD_LAST_DAY = 2,
  WREN_TIME_PERIOD_LAST_WEEK = 3,
  WREN_TIME_PERIOD_FOUR_WEEKS = 4,
  WREN_TIME_PERIOD_ALL_TIME = 5,
  // Everything older than 30 days.
  WREN_TIME_PERIOD_OLDER_THAN_30_DAYS = 6,
} WrenTimePeriod;

// Receives the result of Wren_ClearBrowsingData. With WREN_OK, `failed` has
// the WrenBrowsingData bits of the kinds Chromium could not clear, or 0.
typedef void (*WrenClearBrowsingDataCallback)(void* context,
                                              WrenResult result,
                                              uint32_t failed);

// Clears the Profile's data of the kinds in `kinds`, WrenBrowsingData bits,
// for `period`, as Chrome's Clear browsing data dialog does. Any kind of
// Profile may be cleared. Site data is cleared for websites only: the app's
// own scheme pages keep their storage, as Chromium's chrome:// pages do.
// `callback` gets the result once Chromium has finished, from the event loop.
// It is not called if the app quits first.
//
// `callback` gets one of these errors without anything being cleared:
//
// - WREN_ERROR_INVALID_ARGUMENT for no kinds, an unknown bit or an unknown
//   period.
// - WREN_ERROR_CLOSED once the Profile has closed.
// - WREN_ERROR_NOT_ALLOWED for history or downloads where policy forbids
//   deleting history (AllowDeletingBrowserHistory). Chrome's dialog skips
//   them without saying.
WREN_API void Wren_ClearBrowsingData(WrenProfile* profile,
                                     uint32_t kinds,
                                     WrenTimePeriod period,
                                     WrenClearBrowsingDataCallback callback,
                                     void* context);

// --- Importing history ------------------------------------------------------
//
// For an app that brings the user over from another browser. The app reads
// that browser's history itself.

// A page of the user's history in another browser.
typedef struct WrenHistoryEntry {
  const char* url;
  // The page's title, or NULL.
  const char* title;
  // The last visit, in milliseconds since 1970, UTC.
  int64_t last_visit_ms;
  // How often the user visited the page, and how often they typed its
  // address. The address bar suggests a page typed once, visited 4 times, or
  // visited in the past 3 days.
  int32_t visit_count;
  int32_t typed_count;
} WrenHistoryEntry;

// Receives the result of Wren_AddHistory. With WREN_OK, `added` is the number
// of entries written.
typedef void (*WrenAddHistoryCallback)(void* context,
                                       WrenResult result,
                                       size_t added);

// Adds `entries` to the Profile's history, as Chromium's importer does. A page
// history already has keeps its title and counts, and gains a visit at
// `last_visit_ms`. Wren leaves out a URL history does not keep, such as
// chrome:// and about: pages, and a last visit older than 90 days, which
// Chromium would delete. `added` counts the rest. Chromium writes them to disk
// within seconds, and at quit. An off-the-record Profile adds to its
// original's. `callback` gets the result from the event loop.
//
// `callback` gets one of these errors without anything being added:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Profile, NULL `entries` with a count,
//   or an entry with a NULL or invalid URL, or a count below 0.
// - WREN_ERROR_CLOSED once the Profile has closed.
// - WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API void Wren_AddHistory(WrenProfile* profile,
                              const WrenHistoryEntry* entries,
                              size_t count,
                              WrenAddHistoryCallback callback,
                              void* context);

// --- Importing cookies ------------------------------------------------------

typedef enum WrenCookieSameSite {
  WREN_COOKIE_SAME_SITE_UNSPECIFIED = 0,
  WREN_COOKIE_SAME_SITE_NONE = 1,
  WREN_COOKIE_SAME_SITE_LAX = 2,
  WREN_COOKIE_SAME_SITE_STRICT = 3,
} WrenCookieSameSite;

// A cookie of the user's in another browser.
typedef struct WrenCookie {
  const char* name;
  const char* value;
  // ".example.com" for the domain and its subdomains, "example.com" for the
  // host only.
  const char* domain;
  const char* path;
  // Milliseconds since 1970, UTC. 0 for `expires_ms` is a session cookie, and
  // 0 for the others is now.
  int64_t created_ms;
  int64_t expires_ms;
  int64_t last_access_ms;
  bool secure;
  bool http_only;
  WrenCookieSameSite same_site;
  // The top-level site of a partitioned (CHIPS) cookie, such as
  // "https://example.com", or NULL.
  const char* partition_site;
  // True if the partitioned cookie was set in a frame with a cross-site
  // ancestor.
  bool partition_cross_site_ancestor;
} WrenCookie;

typedef enum WrenCookieResult {
  WREN_COOKIE_WRITTEN = 0,
  // Not a cookie Chromium keeps: its name, value, domain, path, prefix or
  // partition is invalid, or it is SameSite=None or partitioned without
  // Secure.
  WREN_COOKIE_INVALID = 1,
  // It had already expired.
  WREN_COOKIE_EXPIRED = 2,
  // A Secure cookie of the same name is there, which a cookie that is not
  // Secure may not replace.
  WREN_COOKIE_CONFLICT = 3,
  // Chromium could not store it.
  WREN_COOKIE_FAILED = 4,
} WrenCookieResult;

// Receives the result of Wren_AddCookies. With WREN_OK, `results` has one
// result for each cookie, in order, valid during the call.
typedef void (*WrenAddCookiesCallback)(void* context,
                                       WrenResult result,
                                       const WrenCookieResult* results,
                                       size_t count);

// Writes `cookies` into the Profile's cookie store, as a site setting them
// would, HttpOnly and SameSite included, and writes the store to disk before
// it answers. An off-the-record Profile's cookies are its own, and go when it
// closes. `callback` gets the results from the event loop. It is not called if
// the app quits first.
//
// `callback` gets one of these errors without anything being written:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Profile, NULL `cookies` with a count,
//   a cookie with a NULL name, value, domain or path, or an unknown
//   `same_site`.
// - WREN_ERROR_CLOSED once the Profile has closed. If it closes before the
//   cookies are written, `callback` gets it too.
// - WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API void Wren_AddCookies(WrenProfile* profile,
                              const WrenCookie* cookies,
                              size_t count,
                              WrenAddCookiesCallback callback,
                              void* context);

// --- The password manager ---------------------------------------------------

// Chromium's password manager in a Profile: saving and filling passwords, its
// key in the address bar (WREN_PAGE_ACTION_PASSWORDS) and its prompts
// (WREN_POPOVER_PASSWORDS). It is on unless the app turns it off. An app with
// a password manager of its own, or an extension's, turns it off.
typedef struct WrenPasswordManagerState {
  // On, as in Chrome. Off, Chromium offers to save no password, fills none,
  // suggests none, signs in to no site, and shows no key or prompt. What is
  // saved stays saved, and chrome://password-manager still lists it.
  bool on;
  // Policy sets the sites it is off for (PasswordManagerBlocklist), and
  // Wren_SetPasswordManager refuses to change it.
  bool managed;
} WrenPasswordManagerState;

// Fills `state`. An off-the-record Profile has its original's. Returns
// WREN_ERROR_CLOSED once the Profile has closed.
WREN_API WrenResult
Wren_GetPasswordManagerState(WrenProfile* profile,
                             WrenPasswordManagerState* state);

// Turns the password manager on or off for `profile` and its off-the-record
// Profile, and for later launches. Pages loaded from a moment after the call
// follow it; a page already filled keeps its values. Off also turns Chrome's
// "Offer to save passwords" and "Sign in automatically" off in Settings,
// unless the user turned them on. On an off-the-record Profile, it sets its
// original's.
//
// Returns WREN_ERROR_NOT_ALLOWED where policy sets it, or inside a callback
// that runs inside Chromium's call, and WREN_ERROR_CLOSED once the Profile
// has closed.
WREN_API WrenResult Wren_SetPasswordManager(WrenProfile* profile, bool on);

// --- Browsers ---------------------------------------------------------------

// Creates a Browser on `profile` with one tab showing `initial_url`. If
// `initial_url` is NULL or empty, the tab shows Chromium's New Tab page.
WREN_API void Wren_CreateBrowser(WrenProfile* profile,
                                 const char* initial_url,
                                 WrenBrowserCallback callback,
                                 void* context);

// Creates a popup Browser on `profile` with one tab showing `url`, as
// Chromium makes a page's popup: WREN_BROWSER_POPUP, with a title bar and a
// location bar (Wren_GetWindowFeatures), kept in the session and restored as
// a popup. The page has no opener. `x`, `y`, `width` and `height` are the
// frame Wren_GetRequestedBounds then gives, in its coordinates: `x` and `y`
// NAN for no position, and `width` and `height` 0 for Chromium's own size.
// The callback gives the Browser, as Wren_CreateBrowser's does, and
// on_browser_opened does not run for it.
//
// A link the page opens in a new tab, and Chromium's New Tab command in the
// popup, open in a normal Browser of the Profile, as Chrome's do; that
// Browser reaches on_browser_opened if Chromium makes it.
// Wren_MovePopupTabToBrowser and Wren_MovePopupTabToNewBrowser move its tab
// to a normal Browser.
//
// The callback gets WREN_ERROR_INVALID_ARGUMENT for a NULL, empty or invalid
// `url`, or a size below 0 or not a number, and otherwise
// Wren_CreateBrowser's errors.
WREN_API void Wren_CreatePopupBrowser(WrenProfile* profile,
                                      const char* url,
                                      double x,
                                      double y,
                                      double width,
                                      double height,
                                      WrenBrowserCallback callback,
                                      void* context);

WREN_API void Wren_RetainBrowser(WrenBrowser* browser);
WREN_API void Wren_ReleaseBrowser(WrenBrowser* browser);

// Closes `count` Browsers, all or none, as quitting closes them. `callback`
// gets the answer, from the event loop.
//
// First each page's beforeunload may ask, one Browser at a time. Then, if
// the close would leave an off-the-record Profile with no Browser while it
// has downloads in progress, Chromium asks whether to cancel them, through
// on_downloads_prompt with WREN_DOWNLOADS_PROMPT_CLOSE. Only once every page
// and that question have agreed do the Browsers close.
//
// - WREN_OK comes after on_browser_closed has run for each one.
// - WREN_ERROR_CANCELLED means a page or the downloads question kept them.
//   Every Browser stays open, none closed by the request, and each page asks
//   again next time. Keeping the downloads shows chrome://downloads in that
//   Profile's Browser, as Chrome does.
//
// A Browser that closes by itself meanwhile, because its last tab closed,
// counts as closed. A quit that starts while the request asks takes it over:
// `callback` gets WREN_ERROR_CANCELLED, and the quit asks the same pages.
//
// `callback` gets one of these errors without anything being done:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Browsers, or a NULL one.
// - WREN_ERROR_CLOSED if one has closed already.
// - WREN_ERROR_NOT_ALLOWED if one is in a request already or its Profile is
//   being deleted, while a quit is under way, or inside on_popover.
//
// Chromium also closes a Browser when its last tab closes, as Chrome closes
// the window. on_browser_closed reports that close too. Wren has no way to
// keep a Browser open with no tabs.
WREN_API void Wren_CloseBrowsers(WrenBrowser* const* browsers,
                                 size_t count,
                                 WrenResultCallback callback,
                                 void* context);

WREN_API bool Wren_IsBrowserOpen(WrenBrowser* browser);
// Returns WREN_NO_BROWSER once the Browser has closed.
WREN_API WrenBrowserID Wren_GetBrowserID(WrenBrowser* browser);
WREN_API WrenBrowserType Wren_GetBrowserType(WrenBrowser* browser);
// Returns the WrenWindowFeature bits that Chromium's own rule gives the
// Browser. The rule is Chromium's WindowFeatureController:
//
// - A page's popup has a title bar and a location bar.
// - An extension's popup window, or a window Chromium opened itself, has only
//   a title bar.
// - A normal Browser has everything except a title bar.
//
// The value does not change while the Browser is open. It ignores full
// screen, where Chrome hides them all (see Full screen below). Returns 0 once
// the Browser has closed.
WREN_API uint32_t Wren_GetWindowFeatures(WrenBrowser* browser);
// Borrowed. NULL once the Browser has closed.
WREN_API WrenProfile* Wren_GetBrowserProfile(WrenBrowser* browser);

// The open Browsers the app knows, oldest first. The app knows a Browser if it
// created it or Wren told it about it. Borrowed.
WREN_API size_t Wren_GetBrowserCount(void);
WREN_API WrenBrowser* Wren_GetBrowserAt(size_t index);

// Reads the screen rectangle the Browser's window should have, in AppKit
// screen coordinates. For a popup, that is what the page asked for in
// window.open's features. For a restored Browser, it is the frame its window
// had (WrenSession.h). x and y are NAN if the page gave no position. Returns
// false if there is no rectangle.
WREN_API bool Wren_GetRequestedBounds(WrenBrowser* browser,
                                      double* x,
                                      double* y,
                                      double* width,
                                      double* height);

// A window's state, as Chromium reads it and asks for it. Full screen is apart
// from these (Wren_SetFullscreen).
typedef enum WrenWindowState {
  WREN_WINDOW_STATE_NORMAL = 0,
  WREN_WINDOW_STATE_MINIMIZED = 1,
  // Zoomed, as by the green button or Window > Zoom. Chromium calls it
  // maximised.
  WREN_WINDOW_STATE_ZOOMED = 2,
} WrenWindowState;

// Reads the state Chromium wants the Browser's new window in, for
// on_browser_opened. A restored window that was minimised asks for
// WREN_WINDOW_STATE_MINIMIZED, and so does an extension's
// chrome.windows.create with state "minimized". Returns
// WREN_WINDOW_STATE_NORMAL once the Browser has closed.
WREN_API WrenWindowState Wren_GetRequestedWindowState(WrenBrowser* browser);

// What Chromium asks of a Browser's window (on_window_request, Wren.h).
typedef enum WrenWindowRequestKind {
  // Move or resize the window to the request's frame.
  WREN_WINDOW_REQUEST_FRAME = 0,
  // Put the window in the request's state.
  WREN_WINDOW_REQUEST_STATE = 1,
} WrenWindowRequestKind;

struct WrenWindowRequest {
  WrenWindowRequestKind kind;
  // STATE. WREN_WINDOW_STATE_NORMAL also takes the window out of full screen,
  // as Chrome does.
  WrenWindowState state;
  // FRAME. The window's frame, in AppKit screen coordinates.
  double x;
  double y;
  double width;
  double height;
};

// Places Chromium's own browser UI over `ns_view`, the app's view that holds
// the tab views. That UI is dialogs, and permission and passkey prompts.
// Attach every Browser the app shows.
//
// - The UI follows the view as the view or its ancestors move or resize. It
//   shows only while the view's window is on screen.
// - `ns_view` must be in a window. NULL detaches the Browser, and so does
//   closing the window. Attaching again moves the UI to the new view.
// - The view's window becomes the Browser's window. The Browser is active
//   while that window, or a Chromium window over it, is key.
// - When Chromium asks to activate the Browser, Wren makes the window key,
//   once the app has shown it. If the app is not active, Wren activates it
//   first, as Chrome does, so a hidden app comes forward. A window the app has
//   not shown yet, such as one waiting for its page to draw, is left to the
//   app. So is a window the app has taken off screen itself.
// - Chromium also asks when it shows a Browser that is already shown, for
//   example for a switch to one of its tabs, or for Settings opened from an
//   off-the-record Browser.
// - Browser-modal dialogs, and the file panels Chromium opens, are sheets of
//   the view's window. See Wren_HasModalDialog.
// - Bubbles hang off the app's views named with Wren_SetAnchor.
WREN_API void Wren_AttachBrowserToView(WrenBrowser* browser, void* ns_view);

// Returns true if the Browser is active, as Chromium counts it. It is active
// when its window, or one of Chromium's windows over it such as a bubble, a
// dialog or a sheet, is key, and it is not in the background.
//
// Chrome draws its window's frame as in use by this value, not by the key
// window alone. That way its bars do not look inactive while page info is
// open. Draw yours by it too.
//
// A Browser counts as active from when it is shown until its window's key
// state is known. It also counts as active while Chromium has asked to
// activate it and the app has not shown its window. WREN_CHANGE_ACTIVE reports
// each change. Returns false once the Browser has closed.
//
// In on_browser_opened, it is false only for a Browser Chromium showed
// without activating it. A request to stay back, such as an extension's
// chrome.windows.update with focused false, changes nothing, as in Chrome.
WREN_API bool Wren_IsBrowserActive(WrenBrowser* browser);

// Returns true while one of Chromium's browser-modal dialogs is open for the
// Browser, whether shown or waiting for its window to be on screen. Examples
// are an extension's install prompt and its uninstall confirmation.
// WREN_CHANGE_MODAL_DIALOG reports each change. Returns false once the Browser
// has closed.
//
// While such a dialog is open, Chrome blocks the whole window. Do the same in
// your UI: block New Tab, Close Tab, the address field and switching tabs.
// Leave New Window and Quit working. Wren already refuses clicks below the
// window's title bar, and disables Chromium's commands in its menus.
//
// Dialogs that belong to one page, such as HTTP authentication or a page's
// alert, block only that page. Chromium handles them.
WREN_API bool Wren_HasModalDialog(WrenBrowser* browser);

// --- Several Browsers in one window -----------------------------------------
//
// A window can show several Browsers in turn, for example one workspace per
// Profile. It attaches each of them to the same view and keeps all but one in
// the background.
//
// A Browser in the background still belongs to the window. Until it comes
// forward:
//
// - It is never active, whichever window is key.
// - Chromium's commands in the menus do not reach it. They reach the window's
//   Browser that is in front.
// - Its browser-modal dialogs, its pages' dialogs and its bubbles are not
//   shown and block nothing. They wait, and show once it is in front.
// - When Chromium asks to activate it, it stays where it is, and Wren tells
//   the app. The app decides whether to bring it forward. Such requests come
//   from a click on a notification, an extension's chrome.windows.update, a
//   page focusing a window it opened, a switch to one of its tabs, or Settings
//   opened from an off-the-record Browser.
//
// Its tabs stay open and loaded, like the tabs of a window behind another.
// What its tabs' views or elements show is up to the app and its page.
//
// Keep exactly one Browser of a window in front. Wren does not put the others
// in the background for you. Two Browsers in front share the window as two
// attached Browsers always have.

// Puts the Browser in the background, or brings it forward. Bringing it
// forward also forgets a request to come forward. A new Browser is in front.
WREN_API WrenResult Wren_SetBrowserBackgrounded(WrenBrowser* browser,
                                                bool backgrounded);
// Returns false once the Browser has closed.
WREN_API bool Wren_IsBrowserBackgrounded(WrenBrowser* browser);

// Returns true if Chromium has asked for the Browser to come forward since it
// went into the background, or since the app last called
// Wren_ClearForegroundRequest. WREN_CHANGE_FOREGROUND_REQUEST reports the
// first such request. Returns false once the Browser has closed.
WREN_API bool Wren_HasForegroundRequest(WrenBrowser* browser);
// Forgets the Browser's request to come forward, and leaves it in the
// background. Chromium's next request is reported again. Use it for a
// request the app will not act on, such as the one a session restore makes
// for the Browser it activates. Reports no change, and may be called from
// any callback. A Browser in front has no request: returns WREN_OK.
WREN_API WrenResult Wren_ClearForegroundRequest(WrenBrowser* browser);

// --- Full screen ------------------------------------------------------------
//
// A page in full screen has the app's whole window, as in Chrome. While
// Wren_GetFullscreenTab names a tab, the app hides its tab strip and toolbar
// and gives the tab's view the whole window.
//
// Chromium asks the app to take the window into full screen and out of it,
// through on_fullscreen. It shows its own "Press Esc to exit full screen"
// notice over the page. When the user presses Escape, it asks the app to
// leave. If the window is already in full screen, a page asks nothing. It just
// takes the window.

// Tells Wren that `browser`'s window entered full screen, if `fullscreen`, or
// left it. Call it whether the change came from on_fullscreen or from the
// user, through the window's green button or the app's menu.
//
// Call it once the change has finished, from windowDidEnterFullScreen: or
// windowDidExitFullScreen:, or once it has failed. Leaving full screen also
// takes the page out of full screen. Returns WREN_ERROR_CLOSED once the
// Browser has closed.
WREN_API WrenResult Wren_SetFullscreen(WrenBrowser* browser, bool fullscreen);

// Returns the tab whose page is in full screen in `browser`'s window, from the
// moment the app agrees to it. WREN_CHANGE_FULLSCREEN reports each change.
// Returns WREN_NO_TAB if there is none, or once the Browser has closed.
WREN_API WrenTabID Wren_GetFullscreenTab(WrenBrowser* browser);

// --- Side panels ------------------------------------------------------------
//
// Chromium's side panels include Customize Chromium on the New Tab page, the
// reading list, bookmarks, history, reading mode and extensions' panels.
//
// A Browser shows at most one panel at a time, as Chrome does. The app
// decides where. It puts the Browser's side panel view where the panel
// belongs, and shows it while WrenSidePanelState.open is true. Wren puts each
// panel's page in that view.
//
// Panels open from Chromium, such as from the New Tab page's Customize button,
// Chrome's menu or chrome.sidePanel.open. They also open with
// Wren_ShowSidePanel.
//
// On a tab switch, per-tab panels follow Chromium's rule. The new tab's own
// panel shows, or else the window's panel, or else none. Switching back shows
// the first tab's panel again.
//
// Panels that Chromium draws without a page, such as Lens and About this site,
// are not shown.

typedef struct WrenSidePanelState {
  bool open;
  // The tab a per-tab panel belongs to. WREN_NO_TAB for a window's panel.
  WrenTabID tab;
  // Chromium's width for the panel, in points, set when the panel opens.
  // Reading mode's is half its page's width at that moment.
  double default_width;
  // The width to show the panel at, in points. It is the width the user gave
  // this panel through Wren_SetSidePanelWidth, which Chromium remembers, or
  // else default_width.
  double width;
  // Chromium's setting for which side the panel goes on, from Settings >
  // Appearance. It is a hint the app may follow.
  bool right_aligned;
} WrenSidePanelState;

// Returns the Browser's side panel view, the same view for every panel.
// Borrowed, and valid while the Browser handle is. NULL once the Browser has
// closed.
WREN_API void* Wren_GetSidePanelView(WrenBrowser* browser);
WREN_API WrenResult Wren_GetSidePanelState(WrenBrowser* browser,
                                           WrenSidePanelState* state);
// Copies Chromium's name for the open panel, in UTF-8, or "" if none is open.
// Names include "ReadingList", "Bookmarks", "CustomizeChrome", and
// "Extension" followed by the extension's ID. Returns the length needed,
// including the NUL.
WREN_API size_t Wren_CopySidePanelID(WrenBrowser* browser,
                                     char* buffer,
                                     size_t size);
// Copies the open panel's title, in Chromium's language, or "".
WREN_API size_t Wren_CopySidePanelTitle(WrenBrowser* browser,
                                        char* buffer,
                                        size_t size);
// Opens the panel with this ID, as Chromium names it, in the Browser or for
// its active tab. WREN_CHANGE_SIDE_PANEL follows once the panel's page is
// ready.
//
// Returns WREN_ERROR_PANEL_UNAVAILABLE if there is no such panel here now. An
// extension's panel needs the extension loaded. Returns WREN_ERROR_NOT_ALLOWED
// in any Browser that is not a normal one.
WREN_API WrenResult Wren_ShowSidePanel(WrenBrowser* browser, const char* id);
// Closes the open panel.
WREN_API WrenResult Wren_CloseSidePanel(WrenBrowser* browser);
// Records the width the user gave the open panel, in points, as dragging the
// edge of Chrome's side panel does. Call it as the user drags the app's own
// resize handle.
//
// Chromium keeps one width per kind of panel in the Browser's Profile, across
// launches. Every extension's panel shares one width. Chromium makes it no
// narrower than its narrowest panel, 360 points. WREN_CHANGE_SIDE_PANEL
// follows in every Browser of the Profile with a panel open. Read
// WrenSidePanelState.width there.
//
// Returns WREN_ERROR_PANEL_UNAVAILABLE when no panel is open.
WREN_API WrenResult Wren_SetSidePanelWidth(WrenBrowser* browser, double width);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_BROWSER_H_

// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: a Profile's cookies and its cookie settings (C API). Wren.h includes
// this header. Wren.h also explains threads and handles.
//
// Each Profile has Chromium's own cookie store, which its pages use. An
// off-the-record Profile has a store of its own, which goes when the Profile
// closes. The app reads, sets and deletes cookies there, and watches their
// changes. Importing another browser's cookies is setting them.
//
// - The calls on the store answer later from the event loop, never inside
//   the call. An answer is not given if the app quits first. The settings'
//   calls take effect before they return.
// - Inside on_popover, and the other callbacks that run inside Chromium's
//   call, the calls answer WREN_ERROR_NOT_ALLOWED and do nothing.
// - The app's own writes and deletions ignore the site's cookie settings, as
//   an extension's do in Chrome.

#ifndef WREN_PUBLIC_WREN_COOKIES_H_
#define WREN_PUBLIC_WREN_COOKIES_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Cookies ----------------------------------------------------------------

typedef enum WrenCookieSameSite {
  WREN_COOKIE_SAME_SITE_UNSPECIFIED = 0,
  WREN_COOKIE_SAME_SITE_NONE = 1,
  WREN_COOKIE_SAME_SITE_LAX = 2,
  WREN_COOKIE_SAME_SITE_STRICT = 3,
} WrenCookieSameSite;

// Which cookies Chromium removes first when a site has too many: low ones
// go before medium ones, and medium before high.
typedef enum WrenCookiePriority {
  WREN_COOKIE_PRIORITY_MEDIUM = 0,
  WREN_COOKIE_PRIORITY_LOW = 1,
  WREN_COOKIE_PRIORITY_HIGH = 2,
} WrenCookiePriority;

// A cookie. Its name, domain, path and partition make it one cookie: setting
// another with the same four replaces it.
typedef struct WrenCookie {
  const char* name;
  const char* value;
  // ".example.com" for the domain and its subdomains, "example.com" for the
  // host only.
  const char* domain;

  const char* path;
  // Milliseconds since 1970, UTC. 0 for `expires_ms` is a session cookie, and
  // 0 for the others is now. Chromium keeps a cookie at most 400 days past its
  // creation date, and Chrome's own cookies can be older. So Wren_SetCookies
  // keeps `expires_ms` up to 400 days from now. It moves a creation date more
  // than 400 days before that expiry to 400 days before it, and a last access
  // earlier than that creation date to it.
  int64_t created_ms;
  int64_t expires_ms;
  int64_t last_access_ms;

  bool secure;
  bool http_only;
  WrenCookieSameSite same_site;
  WrenCookiePriority priority;
  // The top-level site of a partitioned (CHIPS) cookie, such as
  // "https://example.com", or NULL.
  const char* partition_site;
  // True if the partitioned cookie was set in a frame with a cross-site
  // ancestor.
  bool partition_cross_site_ancestor;
} WrenCookie;

// Which cookies Wren_GetCookies and Wren_DeleteCookies are about. Each field
// that is not NULL narrows the match. A filter whose fields are all NULL
// matches every cookie. Strings are UTF-8, and are read during the call.
typedef struct WrenCookieFilter {
  // The cookies a request to this URL would send, in every partition. SameSite
  // and the blocking of third-party cookies are ignored.
  const char* url;
  // Exactly this domain, as WrenCookie.domain has it: ".example.com" does not
  // match "example.com".
  const char* domain;

  const char* name;
  const char* path;
  // The top-level site of the cookie's partition, or "" for unpartitioned
  // cookies only.
  const char* partition_site;
} WrenCookieFilter;

// --- Setting cookies --------------------------------------------------------

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

// Receives the result of Wren_SetCookies. With WREN_OK, `results` has one
// result for each cookie, in order, valid during the call.
typedef void (*WrenSetCookiesCallback)(void* context,
                                       WrenResult result,
                                       const WrenCookieResult* results,
                                       size_t count);

// Sets `cookies` in the Profile's cookie store, as a site setting them would,
// HttpOnly and SameSite included. Each replaces the cookie with the same
// name, domain, path and partition. Wren writes the store to disk before it
// answers. `callback` gets the results from the event loop.
//
// `callback` gets one of these errors without anything being written:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Profile, NULL `cookies` with a count,
//   a cookie with a NULL name, value, domain or path, or an unknown
//   `same_site` or `priority`.
// - WREN_ERROR_CLOSED once the Profile has closed. If it closes before the
//   cookies are written, `callback` gets it too.
// - WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API void Wren_SetCookies(WrenProfile* profile,
                              const WrenCookie* cookies,
                              size_t count,
                              WrenSetCookiesCallback callback,
                              void* context);

// --- Reading cookies --------------------------------------------------------

// Receives the cookies of Wren_GetCookies. With WREN_OK, `cookies` has every
// match, longest path first and then oldest first, as Chromium orders them.
// The cookies and their strings are valid during the call.
typedef void (*WrenGetCookiesCallback)(void* context,
                                       WrenResult result,
                                       const WrenCookie* cookies,
                                       size_t count);

// Reads the Profile's cookies that `filter` matches, or every cookie for a
// NULL filter. Reading does not change a cookie's last access. A cookie in a
// partition no top-level site names, such as an anonymous frame's, is left
// out. `callback` gets the cookies from the event loop.
//
// `callback` gets one of these errors and no cookies:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Profile, or a filter URL that is not
//   valid.
// - WREN_ERROR_CLOSED once the Profile has closed, or if it closes first.
// - WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API void Wren_GetCookies(WrenProfile* profile,
                              const WrenCookieFilter* filter,
                              WrenGetCookiesCallback callback,
                              void* context);

// --- Deleting cookies -------------------------------------------------------

// Receives the result of Wren_DeleteCookies, with how many cookies it deleted.
typedef void (*WrenDeleteCookiesCallback)(void* context,
                                          WrenResult result,
                                          size_t deleted);

// Deletes the Profile's cookies that `filter` matches, or every cookie for a
// NULL filter. Wren reads the matches, then deletes each. A cookie a site
// changes between the two stays, and is not counted. Wren writes the store to
// disk before it answers. `callback` gets the count from the event loop.
// Wren_ClearBrowsingData (WrenBrowser.h) deletes cookies with the rest of the
// sites' data, over a period.
//
// `callback` gets one of these errors without anything being deleted:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Profile, or a filter URL that is not
//   valid.
// - WREN_ERROR_CLOSED once the Profile has closed. If it closes before the
//   store is written, `callback` gets it too.
// - WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API void Wren_DeleteCookies(WrenProfile* profile,
                                 const WrenCookieFilter* filter,
                                 WrenDeleteCookiesCallback callback,
                                 void* context);

// --- Watching cookies -------------------------------------------------------

// Why a cookie changed.
typedef enum WrenCookieChangeCause {
  // A site or the app set it, new or changed.
  WREN_COOKIE_CHANGE_SET = 0,
  // A site or the app deleted it, or clearing data did.
  WREN_COOKIE_CHANGE_DELETED = 1,
  // A cookie with the same name, domain, path and partition replaced it. The
  // new one's WREN_COOKIE_CHANGE_SET follows.
  WREN_COOKIE_CHANGE_REPLACED = 2,
  // It expired.
  WREN_COOKIE_CHANGE_EXPIRED = 3,
  // Chromium removed it because its site or the Profile had too many.
  WREN_COOKIE_CHANGE_EVICTED = 4,
  // A site set it again with an expiry in the past, which deletes it.
  WREN_COOKIE_CHANGE_SET_EXPIRED = 5,
} WrenCookieChangeCause;

// One change. Every cause but WREN_COOKIE_CHANGE_SET means the cookie is gone.
typedef struct WrenCookieChange {
  // As it was set, or as it was before it went.
  WrenCookie cookie;
  WrenCookieChangeCause cause;
} WrenCookieChange;

// A watch (Wren_WatchCookies). Unique for the life of the process, and never
// reused. WREN_NO_COOKIE_WATCH means none.
typedef uint64_t WrenCookieWatchID;
#define WREN_NO_COOKIE_WATCH 0

// Receives a watch's changes, in the order Chromium made them. The changes and
// their strings are valid during the call.
typedef void (*WrenCookieChangesCallback)(void* context,
                                          const WrenCookieChange* changes,
                                          size_t count);

// Watches the Profile's cookies, until Wren_StopWatchingCookies or until the
// Profile closes. `callback` gets each turn of the event loop's changes
// together, from the event loop, so it may call any function. The changes of
// the app's own calls come too, but Chromium sends them apart from the calls'
// answers, so a watch may hear them before or after those answers. A cookie
// in a partition no top-level site names is left out. If Chromium's
// network service restarts, changes made meanwhile are not reported, as in
// Chrome's chrome.cookies.onChanged.
//
// Returns WREN_NO_COOKIE_WATCH, and never calls `callback`, for no Profile or
// no callback, once the Profile has closed, and inside on_popover.
WREN_API WrenCookieWatchID Wren_WatchCookies(WrenProfile* profile,
                                             WrenCookieChangesCallback callback,
                                             void* context);
// Ends a watch. Its callback is not called again, even for changes already
// made. A watch that has ended, or WREN_NO_COOKIE_WATCH, is ignored.
WREN_API void Wren_StopWatchingCookies(WrenCookieWatchID watch);

// --- Cookie settings --------------------------------------------------------
//
// The Profile's two settings in Chrome's Settings > Privacy and security:
// whether third-party cookies are blocked, and what sites may save by
// default. Chromium's own defaults apply until the app or the user changes
// them: third-party cookies are blocked only off the record, and sites may
// save data. The setters write what Settings writes, so Settings shows the
// app's choice, and the user may change it there. An off-the-record Profile
// has its original's settings, and Chromium blocks third-party cookies off
// the record whatever they are. Per-site exceptions stay Chromium's.

// What sites may save, cookies and site data alike, unless a site has an
// exception.
typedef enum WrenSiteDataDefault {
  WREN_SITE_DATA_ALLOW = 0,
  // Sites save data, and Chromium deletes it when the last Browser of the
  // Profile closes, as Chrome's "Delete data sites have saved to your device
  // when you close all windows" does.
  WREN_SITE_DATA_DELETE_ON_CLOSE = 1,
  // Sites save nothing, and many stop working, as Chrome warns.
  WREN_SITE_DATA_BLOCK = 2,
} WrenSiteDataDefault;

typedef struct WrenCookieSettings {
  // True if third-party cookies are blocked everywhere, false if only off the
  // record. Pages see it from their next load.
  bool third_party_blocked;
  // Policy or an extension sets it, and Wren_SetThirdPartyCookiesBlocked
  // refuses to change it.
  bool third_party_managed;

  WrenSiteDataDefault site_data;
  // Policy or an extension sets it, and Wren_SetSiteDataDefault refuses to
  // change it.
  bool site_data_managed;
} WrenCookieSettings;

// Fills `settings`. From the first call for `profile`, the app hears
// WREN_CHANGE_COOKIE_SETTINGS for it. Returns WREN_ERROR_CLOSED once the
// Profile has closed.
WREN_API WrenResult Wren_GetCookieSettings(WrenProfile* profile,
                                           WrenCookieSettings* settings);

// Blocks third-party cookies everywhere, or only off the record, as Settings'
// two choices do, for `profile` and its off-the-record Profile, and for later
// launches. It may be called inside on_popover.
//
// Returns WREN_ERROR_NOT_ALLOWED where policy or an extension sets it, and
// WREN_ERROR_CLOSED once the Profile has closed.
WREN_API WrenResult Wren_SetThirdPartyCookiesBlocked(WrenProfile* profile,
                                                     bool blocked);

// Sets what sites may save by default, as Settings' three choices do, for
// `profile` and its off-the-record Profile, and for later launches. Data
// sites already saved stays. It may be called inside on_popover.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for an unknown value,
// WREN_ERROR_NOT_ALLOWED where policy or an extension sets it, and
// WREN_ERROR_CLOSED once the Profile has closed.
WREN_API WrenResult Wren_SetSiteDataDefault(WrenProfile* profile,
                                            WrenSiteDataDefault value);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_COOKIES_H_

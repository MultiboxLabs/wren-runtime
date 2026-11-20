// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the user agent pages see, by site and by tab (C API). Wren.h includes
// this header. Wren.h also explains threads and handles.
//
// A WrenUserAgent (Wren.h) is the User-Agent header, navigator.userAgent and
// the client hints. Wren chooses one at the start of each load of a page's
// top frame, from the first of these that applies:
//
// 1. The tab's own setting (Wren_SetTabUserAgent).
// 2. The site rule whose domain matches the page's host most closely
//    (Wren_SetUserAgentRules).
// 3. The app's default (WrenStartup.user_agent).
// 4. Chromium's.
//
// Web views follow 2 to 4. The choice covers the page's own request, its
// navigator, its frames' requests, and its dedicated workers.
//
// - A redirect keeps the choice made at the start of the load, as in Chrome.
//   A link through a short-link service to a site with a rule sends that site
//   the choice for the link's own site, and the next load chooses again.
// - Shared workers and service workers send the app's default whatever the
//   page's choice. So does navigator in a frame from another site that runs
//   in a process of its own. When the app's default has
//   WREN_CLIENT_HINTS_NONE, they send empty client hints, because only a
//   page's choice can leave the headers out.
// - A change applies from the next load. Wren_Reload applies it to the page
//   shown.
// - DevTools' own user agent, in its Network conditions, comes first.

#ifndef WREN_PUBLIC_WREN_USER_AGENT_H_
#define WREN_PUBLIC_WREN_USER_AGENT_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// The user agent of a site's pages.
typedef struct WrenUserAgentRule {
  // A host name, such as "example.com", which matches that host and every
  // host under it, as a cookie's domain does. An IP address, such as
  // "127.0.0.1" or "[::1]", matches only itself. Lower-case, with no scheme,
  // port or path. Only http and https pages match.
  const char* domain;
  WrenUserAgent user_agent;
} WrenUserAgentRule;

// Replaces the site rules with `rules`, for every Profile. Wren copies them.
// A count of 0 removes them all. When several rules match a host, the one
// with the longest domain applies, whatever their order. The rules apply from
// each page's next load.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for an invalid domain or user agent,
// or for two rules with one domain, and then keeps the rules it had. Returns
// WREN_ERROR_CLOSED before on_ready and after quitting.
WREN_API WrenResult Wren_SetUserAgentRules(const WrenUserAgentRule* rules,
                                           size_t count);

// Sets the user agent of `tab`'s pages, ahead of the site rules, from its next
// load. Wren copies it. NULL clears it. The setting lasts as long as the tab,
// through a discard. A duplicate of the tab, a popup it opens and a tab
// restored from it start without it.
//
// Returns WREN_ERROR_TAB_UNAVAILABLE for an unknown tab, and
// WREN_ERROR_INVALID_ARGUMENT for an invalid user agent, which leaves the
// tab's setting as it was.
WREN_API WrenResult Wren_SetTabUserAgent(WrenTabID tab,
                                         const WrenUserAgent* user_agent);

// Fills `user_agent` with the user agent a page gets when no site rule and no
// tab setting applies, for an app to start from, such as to make a rule's
// value. That is WrenStartup.user_agent as the app gave it, with its kind of
// client hints. Without one, it is Chromium's, with WREN_CLIENT_HINTS_CUSTOM
// and Chromium's brands and fields. Site rules, tab settings and DevTools'
// own user agent never change it. The optional fields that send an empty
// value are NULL. Wren keeps the strings until the process ends.
//
// Returns WREN_ERROR_CLOSED before on_ready and after quitting, because
// Chromium's own is known only once Chromium has started.
WREN_API WrenResult Wren_GetDefaultUserAgent(WrenUserAgent* user_agent);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_USER_AGENT_H_

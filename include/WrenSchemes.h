// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the app's own URL schemes (C API). Wren.h includes this header; see
// Wren.h for threading and handles.
//
// An app declares its schemes in WrenStartup.schemes and answers their
// requests itself: "app://ui/index.html" in a tab, and that page's scripts,
// styles, images and fetch() calls, all ask the scheme's handler.
// - There is one handler per scheme for the whole app. A request names the
//   Profile and the tab or web view it comes from.
// - A scheme is closed to other pages: the handler is asked only for what
//   the app or the user opens and for what the scheme's own pages ask. With
//   WREN_SCHEME_WEB_ACCESSIBLE any page may link to it or load it as an
//   image, a script or a frame, as it may an https URL.
// - Only pages of the same origin can read a response with fetch(), and only
//   with WREN_SCHEME_FETCH.
// - Not offered yet: a request's body, responses that stream, redirects, and
//   service workers and workers started from a declared scheme's URL.

#ifndef WREN_PUBLIC_WREN_SCHEMES_H_
#define WREN_PUBLIC_WREN_SCHEMES_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// One request for a URL on a declared scheme. It is valid from the handler's
// call until the app answers it, which it must do exactly once: with
// Wren_RespondWithData, Wren_RespondWithFile or Wren_FailSchemeRequest, inside
// the handler or later. A request left unanswered keeps its page loading.
// Requests still unanswered when the app quits are dropped.
typedef struct WrenSchemeRequest WrenSchemeRequest;

// Runs on the main thread, from the event loop, for each request. A page's
// first request can come before the callback of the Wren_CreateBrowser that
// opened it.
typedef void (*WrenSchemeHandler)(void* context, WrenSchemeRequest* request);

// Sets the handler of a scheme declared in WrenStartup.schemes, replacing
// the one before; NULL removes it. Without a handler the scheme's requests
// fail. WREN_ERROR_INVALID_ARGUMENT for a scheme that was not declared.
WREN_API WrenResult Wren_SetSchemeHandler(const char* scheme,
                                          WrenSchemeHandler handler,
                                          void* context);

typedef struct WrenSchemeRequestInfo {
  // Borrowed; NULL if the Profile has closed since the request was made.
  WrenProfile* profile;
  // The tab whose page asks, or WREN_NO_TAB: a page that is not a tab (a
  // web view, a side panel, an extension's popup), or the tab has closed.
  WrenTabID tab;
  // The web view whose page asks (borrowed), or NULL.
  WrenWebView* web_view;
  // The request loads a page: a tab's, or a frame's inside one.
  bool is_document;
  // Whoever asked has stopped waiting (the load was stopped, the tab
  // closed). The request must still be answered, which then does nothing.
  bool cancelled;
} WrenSchemeRequestInfo;

// WREN_ERROR_CLOSED for a request that has been answered, here and below.
WREN_API WrenResult Wren_GetSchemeRequestInfo(WrenSchemeRequest* request,
                                              WrenSchemeRequestInfo* info);

// These copy as Wren_CopyTabStatusText does and return the length needed,
// including the NUL; 0 for a request that has been answered.
// The whole URL, as Chromium canonicalizes it: "app://ui/index.html?x=1".
WREN_API size_t Wren_CopySchemeRequestURL(WrenSchemeRequest* request,
                                          char* buffer,
                                          size_t size);
// "GET", "POST", ...
WREN_API size_t Wren_CopySchemeRequestMethod(WrenSchemeRequest* request,
                                             char* buffer,
                                             size_t size);
// The value of the request header `name`, in any case; "" if it has none.
// Pages send few on their own: Accept, Range for media, and what fetch()
// was given.
WREN_API size_t Wren_CopySchemeRequestHeader(WrenSchemeRequest* request,
                                             const char* name,
                                             char* buffer,
                                             size_t size);

typedef struct WrenSchemeHeader {
  const char* name;
  const char* value;
} WrenSchemeHeader;

// Answers with `body_size` bytes at `body`, copied before the call returns.
// `status` is an HTTP status, 200 to 599; a 3xx is shown, not followed.
// `headers` are the response's, as a server's: Content-Type names the body's
// type ("text/html; charset=utf-8"), and without one it is text/plain.
// WREN_ERROR_INVALID_ARGUMENT, with the request left unanswered, for a status
// out of range or a header that is not valid in HTTP.
WREN_API WrenResult Wren_RespondWithData(WrenSchemeRequest* request,
                                         int status,
                                         const WrenSchemeHeader* headers,
                                         size_t header_count,
                                         const void* body,
                                         size_t body_size);

// Answers with the file at `path`, an absolute path. Chromium reads it off
// the main thread, as it reads a file: URL, so a large file costs the app
// nothing, and Range requests (audio, video) are answered. The type is
// `headers`' Content-Type, or else the file extension's. A missing file, or
// a directory, fails the load as WREN_SCHEME_FAILURE_NOT_FOUND does.
// Wren serves whatever file the handler names: a handler that builds the
// path from the URL must keep it inside its own directory.
// WREN_ERROR_INVALID_ARGUMENT, with the request left unanswered, for a path
// that is not absolute or has a ".." component, or a header that is not
// valid in HTTP.
WREN_API WrenResult Wren_RespondWithFile(WrenSchemeRequest* request,
                                         const char* path,
                                         const WrenSchemeHeader* headers,
                                         size_t header_count);

typedef enum WrenSchemeFailure {
  // Chromium's ERR_FAILED: a page load shows Chromium's error page.
  WREN_SCHEME_FAILURE_FAILED = 0,
  // ERR_FILE_NOT_FOUND.
  WREN_SCHEME_FAILURE_NOT_FOUND = 1,
  // ERR_ACCESS_DENIED.
  WREN_SCHEME_FAILURE_ACCESS_DENIED = 2,
} WrenSchemeFailure;

// Answers with a failed load: no response at all, as when a server cannot
// be reached. To show a page of the app's for a missing URL, answer with
// data and status 404.
WREN_API WrenResult Wren_FailSchemeRequest(WrenSchemeRequest* request,
                                           WrenSchemeFailure failure);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_SCHEMES_H_

// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the app's own URL schemes (C API). Wren.h includes this header. Wren.h
// also explains threads and handles.
//
// An app declares its schemes in WrenStartup.schemes and answers their
// requests itself. If a tab opens "app://ui/index.html", the scheme's handler
// gets that request, and one for each script, style sheet, image and fetch()
// call the page makes on the scheme.
//
// - The app has one handler per scheme. Each request names the Profile and
//   the tab or web view it comes from.
// - By default other pages cannot reach a scheme. The handler gets requests
//   only for what the app or the user opens, and for what the scheme's own
//   pages ask for.
// - With WREN_SCHEME_WEB_ACCESSIBLE, any page may link to the scheme, or load
//   from it as an image, a script or a frame, as it may from an https URL.
// - fetch() can read a response only from a page of the same origin, and only
//   with WREN_SCHEME_FETCH.
// - Not offered yet: the request's body, streamed responses, redirects, and
//   service workers or workers started from a declared scheme's URL.

#ifndef WREN_PUBLIC_WREN_SCHEMES_H_
#define WREN_PUBLIC_WREN_SCHEMES_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// One request for a URL on a declared scheme.
//
// The app must answer each request exactly once, with Wren_RespondWithData,
// Wren_RespondWithFile or Wren_FailSchemeRequest. It may answer inside the
// handler or later. The request is valid from the handler's call until it is
// answered. While a request has no answer, its page keeps loading. Requests
// that still have no answer when the app quits are dropped.
typedef struct WrenSchemeRequest WrenSchemeRequest;

// Runs on the main thread, from the event loop, once for each request. A
// page's first request can arrive before the callback of the
// Wren_CreateBrowser call that opened the page.
typedef void (*WrenSchemeHandler)(void* context, WrenSchemeRequest* request);

// Sets the handler of a scheme declared in WrenStartup.schemes, replacing any
// handler set before. NULL removes the handler. Without a handler, every
// request on the scheme fails. Returns WREN_ERROR_INVALID_ARGUMENT for a
// scheme that was not declared.
WREN_API WrenResult Wren_SetSchemeHandler(const char* scheme,
                                          WrenSchemeHandler handler,
                                          void* context);

typedef struct WrenSchemeRequestInfo {
  // Borrowed. NULL if the Profile has closed since the request was made.
  WrenProfile* profile;
  // The tab whose page made the request. WREN_NO_TAB if the tab has closed,
  // or if the page is not in a tab, such as a web view, a side panel or an
  // extension's popup.
  WrenTabID tab;
  // The web view whose page made the request, or NULL. Borrowed.
  WrenWebView* web_view;
  // True if the request loads a page, either a tab's page or a frame inside
  // one.
  bool is_document;
  // True if whoever asked has stopped waiting, because the load was stopped
  // or the tab closed. The app must still answer the request. The answer then
  // does nothing.
  bool cancelled;
} WrenSchemeRequestInfo;

// Returns WREN_ERROR_CLOSED for a request that has been answered. The same is
// true of every call below that takes a request.
WREN_API WrenResult Wren_GetSchemeRequestInfo(WrenSchemeRequest* request,
                                              WrenSchemeRequestInfo* info);

// The three calls below copy text as Wren_CopyTabStatusText does. Each returns
// the length needed, including the NUL, or 0 for a request that has been
// answered.

// The whole URL, as Chromium canonicalizes it, such as
// "app://ui/index.html?x=1".
WREN_API size_t Wren_CopySchemeRequestURL(WrenSchemeRequest* request,
                                          char* buffer,
                                          size_t size);
// The method, such as "GET" or "POST".
WREN_API size_t Wren_CopySchemeRequestMethod(WrenSchemeRequest* request,
                                             char* buffer,
                                             size_t size);
// The value of the request header `name`, matched in any case. "" if the
// request has no such header. Pages send few headers on their own: Accept,
// Range for media, and whatever was passed to fetch().
WREN_API size_t Wren_CopySchemeRequestHeader(WrenSchemeRequest* request,
                                             const char* name,
                                             char* buffer,
                                             size_t size);

typedef struct WrenSchemeHeader {
  const char* name;
  const char* value;
} WrenSchemeHeader;

// Answers with the `body_size` bytes at `body`. Wren copies them before the
// call returns.
//
// `status` is an HTTP status from 200 to 599. A 3xx status is shown as a
// response. It is not followed as a redirect. `headers` are the response's
// headers, as a server would send them. Content-Type gives the body's type,
// such as "text/html; charset=utf-8". Without it, the body is text/plain.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for a status out of range or a header
// that is not valid in HTTP. The request then still needs an answer.
WREN_API WrenResult Wren_RespondWithData(WrenSchemeRequest* request,
                                         int status,
                                         const WrenSchemeHeader* headers,
                                         size_t header_count,
                                         const void* body,
                                         size_t body_size);

// Answers with the file at `path`, which must be absolute.
//
// Chromium reads the file off the main thread, as it reads a file: URL. So a
// large file costs the app nothing, and Range requests for audio and video
// get answers. The type is the Content-Type in `headers`, or else the one the
// file's extension implies. A missing file or a directory fails the load, as
// WREN_SCHEME_FAILURE_NOT_FOUND does.
//
// Wren serves whatever file the handler names. A handler that builds the path
// from the URL must keep the path inside its own directory.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for a path that is not absolute or has a
// ".." component, or for a header that is not valid in HTTP. The request then
// still needs an answer.
WREN_API WrenResult Wren_RespondWithFile(WrenSchemeRequest* request,
                                         const char* path,
                                         const WrenSchemeHeader* headers,
                                         size_t header_count);

typedef enum WrenSchemeFailure {
  // Chromium's ERR_FAILED. A page load shows Chromium's error page.
  WREN_SCHEME_FAILURE_FAILED = 0,
  // Chromium's ERR_FILE_NOT_FOUND.
  WREN_SCHEME_FAILURE_NOT_FOUND = 1,
  // Chromium's ERR_ACCESS_DENIED.
  WREN_SCHEME_FAILURE_ACCESS_DENIED = 2,
} WrenSchemeFailure;

// Answers with a failed load. The page gets no response at all, as when a
// server cannot be reached. To show the app's own page for a missing URL,
// answer with data and status 404 instead.
WREN_API WrenResult Wren_FailSchemeRequest(WrenSchemeRequest* request,
                                           WrenSchemeFailure failure);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_SCHEMES_H_

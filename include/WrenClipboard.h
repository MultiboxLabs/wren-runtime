// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: the system clipboard (C API). Wren.h includes this header. Wren.h
// also explains threads and handles.
//
// An app copies and pastes through Chromium's clipboard, as Chrome's own
// commands do, so it needs no platform call for it. Wren writes text, and a
// URL with its title, and reads them back.
//
// - A write replaces everything the clipboard held, in one go.
// - A read answers later, from the event loop, as reads are asynchronous on
//   some platforms.
// - Chromium's own copies, from a page's menu, a page's script and DevTools,
//   go to the same clipboard.
// - Not offered yet: HTML, RTF, images, file names, and hearing when the
//   clipboard changes.

#ifndef WREN_PUBLIC_WREN_CLIPBOARD_H_
#define WREN_PUBLIC_WREN_CLIPBOARD_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// The kinds Wren_ReadClipboard reads, as bits.
typedef enum WrenClipboardKinds {
  WREN_CLIPBOARD_TEXT = 1u << 0,
  WREN_CLIPBOARD_URL = 1u << 1,
} WrenClipboardKinds;

// What the system clipboard holds, or what the app writes to it. Strings are
// UTF-8.
typedef struct WrenClipboardData {
  // Plain text. NULL for none.
  const char* text;
  // A URL. NULL for none.
  const char* url;
  // The URL's title, which a paste into rich text may show. NULL or "" for
  // none.
  const char* url_title;
  // Hidden from clipboard viewers and history, as Chrome's password manager
  // marks a copied password. A read says whether the writer marked it so.
  // Chrome's Paste and Go refuses such data.
  bool concealed;
  // Kept out of clipboard history and on this device, as Chromium marks a copy
  // from an off-the-record Profile. On a Mac, macOS deletes it from the
  // clipboard 5 minutes after the write. Set it for a copy from an
  // off-the-record Profile's Browser. A read gives false.
  bool off_the_record;
} WrenClipboardData;

// Replaces what the system clipboard holds with `data`, in one write, as
// Chrome's commands do. A URL with no text is also written as its text, so a
// paste into a plain text field gives the URL on every platform. Text given
// with a URL is the plain text. The call may be made inside any callback.
//
// Returns, and writes nothing:
//
// - WREN_ERROR_INVALID_ARGUMENT for NULL `data`, no text and no URL, a title
//   without a URL, a URL Chromium cannot parse, or a string that is not
//   UTF-8.
// - WREN_ERROR_NOT_ALLOWED before on_ready, or once the app quits.
WREN_API WrenResult Wren_WriteClipboard(const WrenClipboardData* data);

// Receives the result of Wren_ReadClipboard, from the event loop. `data` and
// its strings are borrowed until the callback returns. `data` is NULL unless
// `result` is WREN_OK.
typedef void (*WrenClipboardCallback)(void* context,
                                      const WrenClipboardData* data,
                                      WrenResult result);

// Reads `kinds`, WrenClipboardKinds bits, from the system clipboard as it is
// at the call, and gives them to `callback` from the event loop. The call
// returns at once, and may be made inside any callback. `callback` is
// required. It is not called before on_ready, or once the app quits.
//
// A kind not asked for, or that the clipboard does not hold, is NULL. Empty
// text is NULL. `concealed` is always read.
//
// On a Mac, macOS may ask the person whether the app may paste, before the
// read, unless they have allowed it. Read only when the person asks for it,
// as with a Paste and Go command, never to show what the clipboard holds.
//
// `callback` gets WREN_ERROR_INVALID_ARGUMENT without data for `kinds` of 0,
// or with a bit it does not know.
WREN_API void Wren_ReadClipboard(uint32_t kinds,
                                 WrenClipboardCallback callback,
                                 void* context);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_CLIPBOARD_H_

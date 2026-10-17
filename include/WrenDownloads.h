// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: downloads (C API). Wren.h includes this header; see Wren.h for
// threading, handles and strings.
//
// Chromium downloads files as Chrome does: where its download settings say,
// asking with its save panel when they say to (an off-the-record Profile
// always asks), and warning about dangerous files. Chrome shows them from a
// button in its toolbar and a bubble below it; here the app reads them as
// state and draws them as it likes, or leaves the bubble to Chromium
// (WREN_POPOVER_DOWNLOADS, WrenUI.h).
//
// - A Profile's downloads are Chrome's bubble's: those of the last 24 hours,
//   at most 30, in progress first, then paused, then the rest, newest first.
//   An off-the-record Profile's has its original's too, as Chrome's
//   incognito windows do. chrome://downloads has the whole history.
// - The download button is Chrome's, per Browser: whether Chrome shows it,
//   its icon and its progress ring. Chromium decides, as for Chrome's.
// - Before a Browser closes, or the app quits, with downloads in progress
//   that would be cancelled, on_downloads_prompt asks (Wren.h). On macOS a
//   Browser asks only when it is the last of an off-the-record Profile: a
//   normal Profile's downloads go on while the app runs.

#ifndef WREN_PUBLIC_WREN_DOWNLOADS_H_
#define WREN_PUBLIC_WREN_DOWNLOADS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Downloads --------------------------------------------------------------

typedef enum WrenDownloadStatus {
  // Downloading, or paused (WrenDownloadState.paused).
  WREN_DOWNLOAD_IN_PROGRESS = 0,
  WREN_DOWNLOAD_DONE = 1,
  WREN_DOWNLOAD_CANCELLED = 2,
  // It stopped with an error (WrenDownloadState.failure); it may be
  // resumed or retried.
  WREN_DOWNLOAD_FAILED = 3,
} WrenDownloadStatus;

// How Chrome warns about it, by the colour of its warning.
typedef enum WrenDownloadWarning {
  WREN_DOWNLOAD_WARNING_NONE = 0,
  // Red: a dangerous file, site or content. Chrome keeps it unless the user
  // chooses to (WREN_DOWNLOAD_KEEP).
  WREN_DOWNLOAD_WARNING_DANGEROUS = 1,
  // Grey: uncommon, unverified, or insecure (http).
  WREN_DOWNLOAD_WARNING_SUSPICIOUS = 2,
} WrenDownloadWarning;

// Why a WREN_DOWNLOAD_FAILED download stopped.
typedef enum WrenDownloadFailure {
  WREN_DOWNLOAD_FAILURE_NONE = 0,
  // The file could not be written: no space, no permission, a name too long.
  WREN_DOWNLOAD_FAILURE_FILE = 1,
  WREN_DOWNLOAD_FAILURE_NETWORK = 2,
  // The server refused or ended it.
  WREN_DOWNLOAD_FAILURE_SERVER = 3,
  // Blocked: a virus, the administrator's policy, a failed security check.
  WREN_DOWNLOAD_FAILURE_BLOCKED = 4,
  // The browser went away while it downloaded.
  WREN_DOWNLOAD_FAILURE_CRASH = 5,
} WrenDownloadFailure;

// What may be done to a download now, as Chrome's bubble offers it. Bits of
// WrenDownloadState.actions; Wren_DoDownloadAction takes one.
typedef enum WrenDownloadAction {
  WREN_DOWNLOAD_PAUSE = 1u << 0,
  WREN_DOWNLOAD_RESUME = 1u << 1,
  WREN_DOWNLOAD_CANCEL = 1u << 2,
  // Opens the finished file with its app.
  WREN_DOWNLOAD_OPEN = 1u << 3,
  // Shows the file in the Finder.
  WREN_DOWNLOAD_SHOW_IN_FINDER = 1u << 4,
  // Opens the file when it is done, or no longer; see open_when_done.
  WREN_DOWNLOAD_OPEN_WHEN_DONE = 1u << 5,
  // Keeps a file Chrome warns about, or discards it (the file is deleted and
  // the download leaves the list).
  WREN_DOWNLOAD_KEEP = 1u << 6,
  WREN_DOWNLOAD_DISCARD = 1u << 7,
  // Downloads a failed file again from the start.
  WREN_DOWNLOAD_RETRY = 1u << 8,
} WrenDownloadAction;

typedef struct WrenDownloadState {
  WrenDownloadStatus status;
  bool paused;
  // Bytes so far, and in all; 0 when the size is not known.
  int64_t received_bytes;
  int64_t total_bytes;
  // Chromium's estimate of the time left, in whole seconds; -1 when it has
  // none (paused, done, not started, an unknown size).
  int64_t seconds_left;
  WrenDownloadWarning warning;
  WrenDownloadFailure failure;
  // It opens when done (WREN_DOWNLOAD_OPEN_WHEN_DONE).
  bool open_when_done;
  // Done, but its file has since been moved or deleted.
  bool file_removed;
  // Bits of WrenDownloadAction.
  uint32_t actions;
  // The tab that started it, or WREN_NO_TAB (a web view's page, a tab that
  // has closed, one from a past session).
  WrenTabID tab;
} WrenDownloadState;

// The Profile's downloads (see above), in order. WREN_CHANGE_DOWNLOADS
// reports changes to them, once the app has read them: one added or gone,
// or one's progress or state.
WREN_API size_t Wren_GetDownloadCount(WrenProfile* profile);
// WREN_NO_DOWNLOAD past the end.
WREN_API WrenDownloadID Wren_GetDownloadAt(WrenProfile* profile, size_t index);

// WREN_ERROR_DOWNLOAD_UNAVAILABLE for a download Chromium no longer has
// (discarded, or its Profile closed).
WREN_API WrenResult Wren_GetDownloadState(WrenDownloadID download,
                                          WrenDownloadState* state);
// The file's name, as Chrome shows it.
WREN_API size_t Wren_CopyDownloadFileName(WrenDownloadID download,
                                          char* buffer,
                                          size_t size);
// Where the file is, or will be once done; "" before Chromium has chosen.
WREN_API size_t Wren_CopyDownloadPath(WrenDownloadID download,
                                      char* buffer,
                                      size_t size);
// The URL it came from, after redirects.
WREN_API size_t Wren_CopyDownloadURL(WrenDownloadID download,
                                     char* buffer,
                                     size_t size);

// Does what Chrome's bubble does for `action`. WREN_ERROR_NOT_ALLOWED when
// it is not among the download's actions, and inside on_popover.
WREN_API WrenResult Wren_DoDownloadAction(WrenDownloadID download,
                                          WrenDownloadAction action);

// --- The download button ----------------------------------------------------

typedef enum WrenDownloadIcon {
  // Downloads in progress: Chrome's arrow with the ring.
  WREN_DOWNLOAD_ICON_PROGRESS = 0,
  // None in progress: Chrome's plain download icon.
  WREN_DOWNLOAD_ICON_DONE = 1,
  // A file is being scanned: the ring spins.
  WREN_DOWNLOAD_ICON_SCANNING = 2,
} WrenDownloadIcon;

typedef struct WrenDownloadButtonState {
  // Chrome shows its button: from a download's start until an hour after the
  // last ended.
  bool visible;
  // Drawn in the accent colour: something new for a minute after a download
  // ends, until the button is clicked, or while one downloads unpaused.
  bool active;
  WrenDownloadIcon icon;
  // The downloads in progress, and their progress together, 0 to 100.
  int in_progress;
  int percent;
  // All their sizes are known; otherwise the ring spins.
  bool percent_known;
} WrenDownloadButtonState;

// The Browser's download button, as Chrome's toolbar would show it.
// WREN_CHANGE_DOWNLOAD_BUTTON reports its changes, once the app has read
// it.
WREN_API WrenResult Wren_GetDownloadButtonState(WrenBrowser* browser,
                                                WrenDownloadButtonState* state);
// As a click on Chrome's button: opens the bubble of the Browser's downloads
// (WREN_POPOVER_DOWNLOADS asks whose), or closes Chromium's if it is open;
// with none to list, chrome://downloads in a tab. The button is no longer
// active. WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API WrenResult Wren_ClickDownloadButton(WrenBrowser* browser);

// --- Closing and quitting with downloads in progress ------------------------

typedef enum WrenDownloadsPromptKind {
  // The app is quitting: Wren_Quit, Cmd-Q, logging out.
  WREN_DOWNLOADS_PROMPT_QUIT = 0,
  // `browser`, the last Browser of the off-the-record `profile`, is closing.
  WREN_DOWNLOADS_PROMPT_CLOSE = 1,
} WrenDownloadsPromptKind;

// What Chromium is about to ask. Valid during on_downloads_prompt.
struct WrenDownloadsPrompt {
  WrenDownloadsPromptID prompt;
  WrenDownloadsPromptKind kind;
  // CLOSE: the Browser, and its Profile. Borrowed.
  WrenBrowser* browser;
  WrenProfile* profile;
  // The downloads that would be cancelled.
  int count;
};

// The app's answer to a prompt it answered WREN_POPOVER_APP: cancel the
// downloads and go on closing or quitting, or keep them, and the Browser or
// the app. Keeping them at a close shows chrome://downloads in the Browser,
// as Chrome does. WREN_ERROR_POPOVER_CLOSED for a prompt answered already,
// or whose Browser has closed.
WREN_API WrenResult Wren_AnswerDownloadsPrompt(WrenDownloadsPromptID prompt,
                                               bool cancel_downloads);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_DOWNLOADS_H_

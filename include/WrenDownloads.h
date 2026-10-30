// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: downloads (C API). Wren.h includes this header. Wren.h also explains
// threads, handles and strings.
//
// Chromium downloads files as Chrome does. It saves them where its download
// settings say. When the settings say to ask, it asks with its save panel. An
// off-the-record Profile always asks. It warns about dangerous files.
//
// Chrome shows downloads from a button in its toolbar and a bubble below it.
// A Wren app reads them as state and draws them as it likes. It may also
// leave the bubble to Chromium, through WREN_POPOVER_DOWNLOADS (WrenUI.h).
//
// - A Profile's downloads are the ones Chrome's bubble lists. That is those
//   of the last 24 hours, at most 30. Downloads in progress come first, then
//   paused ones, then the rest, newest first. The whole history, as
//   chrome://downloads lists it, is below.
// - An off-the-record Profile's list also has its original Profile's
//   downloads, as Chrome's incognito windows do.
// - Each Browser has Chrome's download button: whether Chrome would show it,
//   its icon and its progress ring. Chromium decides all of these, as it does
//   for Chrome.
// - Before a Browser closes, or the app quits, with downloads in progress
//   that would be cancelled, Wren asks on_downloads_prompt (Wren.h). On macOS
//   a closing Browser asks only if it is the last Browser of an off-the-record
//   Profile. A normal Profile's downloads go on for as long as the app runs.

#ifndef WREN_PUBLIC_WREN_DOWNLOADS_H_
#define WREN_PUBLIC_WREN_DOWNLOADS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Downloads --------------------------------------------------------------

typedef enum WrenDownloadStatus {
  // Downloading, or paused. WrenDownloadState.paused says which.
  WREN_DOWNLOAD_IN_PROGRESS = 0,

  WREN_DOWNLOAD_DONE = 1,
  WREN_DOWNLOAD_CANCELLED = 2,
  // Stopped with an error, given by WrenDownloadState.failure. The download
  // may be resumed or retried.
  WREN_DOWNLOAD_FAILED = 3,
} WrenDownloadStatus;

// How Chrome warns about a download, named by the colour of its warning.
typedef enum WrenDownloadWarning {
  WREN_DOWNLOAD_WARNING_NONE = 0,
  // Red. The file, the site or the content is dangerous. Chrome does not keep
  // the file unless the user chooses to, with WREN_DOWNLOAD_KEEP.
  WREN_DOWNLOAD_WARNING_DANGEROUS = 1,
  // Grey. The file is uncommon or unverified, or came over insecure http.
  WREN_DOWNLOAD_WARNING_SUSPICIOUS = 2,
} WrenDownloadWarning;

// Why a WREN_DOWNLOAD_FAILED download stopped.
typedef enum WrenDownloadFailure {
  WREN_DOWNLOAD_FAILURE_NONE = 0,
  // The file could not be written. The disk may be full, the folder not
  // writable, or the name too long.
  WREN_DOWNLOAD_FAILURE_FILE = 1,

  WREN_DOWNLOAD_FAILURE_NETWORK = 2,
  // The server refused the download or ended it.
  WREN_DOWNLOAD_FAILURE_SERVER = 3,
  // Something blocked it: a virus, the administrator's policy, or a failed
  // security check.
  WREN_DOWNLOAD_FAILURE_BLOCKED = 4,
  // The browser went away during the download.
  WREN_DOWNLOAD_FAILURE_CRASH = 5,
} WrenDownloadFailure;

// What the user may do to a download now, as Chrome's bubble offers it, and
// chrome://downloads's Remove. WrenDownloadState.actions holds these as bits.
// Wren_DoDownloadAction takes one.
typedef enum WrenDownloadAction {
  WREN_DOWNLOAD_PAUSE = 1u << 0,
  WREN_DOWNLOAD_RESUME = 1u << 1,
  WREN_DOWNLOAD_CANCEL = 1u << 2,
  // Opens the finished file with its app.
  WREN_DOWNLOAD_OPEN = 1u << 3,
  // Shows the file in the Finder.
  WREN_DOWNLOAD_SHOW_IN_FINDER = 1u << 4,
  // Turns opening the file when it is done on, or off again. See
  // open_when_done.
  WREN_DOWNLOAD_OPEN_WHEN_DONE = 1u << 5,
  // Keeps a file Chrome warns about.
  WREN_DOWNLOAD_KEEP = 1u << 6,
  // Discards a file Chrome warns about. The file is deleted and the download
  // leaves the list.
  WREN_DOWNLOAD_DISCARD = 1u << 7,
  // Downloads a failed file again from the start.
  WREN_DOWNLOAD_RETRY = 1u << 8,
  // Takes the download off the Profile's lists and its history, as Remove
  // from List on chrome://downloads, with no Undo. The file stays. Offered
  // once the download is not in progress or paused, with no warning
  // unanswered, where policy allows deleting history.
  WREN_DOWNLOAD_REMOVE = 1u << 9,
} WrenDownloadAction;

typedef struct WrenDownloadState {
  WrenDownloadStatus status;
  bool paused;
  // The bytes received so far, and the total. Each is 0 when not known.
  int64_t received_bytes;
  int64_t total_bytes;
  // Chromium's estimate of the time left, in whole seconds. -1 when it has
  // none: paused, done, not started, or of unknown size.
  int64_t seconds_left;

  WrenDownloadWarning warning;
  WrenDownloadFailure failure;
  // True if the file opens when the download is done. See
  // WREN_DOWNLOAD_OPEN_WHEN_DONE.
  bool open_when_done;
  // True if the download is done but its file has since been moved or
  // deleted.
  bool file_removed;
  // Bits of WrenDownloadAction.
  uint32_t actions;
  // The tab that started the download. WREN_NO_TAB if a web view's page
  // started it, if the tab has closed, or if it is from a past session.
  WrenTabID tab;
  // When the download started and, once done, when it finished, in
  // milliseconds since 1970, UTC. end_ms is 0 for a download not done: in
  // progress, cancelled or failed.
  int64_t start_ms;
  int64_t end_ms;
  // True if an off-the-record Profile made the download. Only that Profile's
  // lists have it, beside its original's downloads; chrome://downloads marks
  // it "In Incognito".
  bool off_the_record;
} WrenDownloadState;

// The number of the Profile's downloads, as listed above, in order.
//
// Once the app has read them, WREN_CHANGE_DOWNLOADS reports each change to
// the list: a download added or gone, or one entering, leaving or moving in
// it. WREN_CHANGE_DOWNLOAD reports a download's progress or state.
WREN_API size_t Wren_GetDownloadCount(WrenProfile* profile);
// The download at `index`. WREN_NO_DOWNLOAD past the end.
WREN_API WrenDownloadID Wren_GetDownloadAt(WrenProfile* profile, size_t index);

// Returns WREN_ERROR_DOWNLOAD_UNAVAILABLE for a download Chromium no longer
// has, because it was discarded or removed, or its Profile closed.
WREN_API WrenResult Wren_GetDownloadState(WrenDownloadID download,
                                          WrenDownloadState* state);
// Copies the file's name, as Chrome shows it.
WREN_API size_t Wren_CopyDownloadFileName(WrenDownloadID download,
                                          char* buffer,
                                          size_t size);
// Copies the file's path, where it is or where it will be once done. "" until
// Chromium has chosen it.
WREN_API size_t Wren_CopyDownloadPath(WrenDownloadID download,
                                      char* buffer,
                                      size_t size);
// Copies the URL the file came from, after redirects.
WREN_API size_t Wren_CopyDownloadURL(WrenDownloadID download,
                                     char* buffer,
                                     size_t size);

// Does what Chrome's bubble does for `action`. Returns WREN_ERROR_NOT_ALLOWED
// when `action` is not among the download's actions, and inside on_popover.
WREN_API WrenResult Wren_DoDownloadAction(WrenDownloadID download,
                                          WrenDownloadAction action);

// --- The whole history ------------------------------------------------------

typedef struct WrenDownloadHistoryState {
  // True once Chromium has loaded the Profile's history, and for an
  // off-the-record Profile its original's. Until then the history has what
  // Chromium has so far. WREN_CHANGE_DOWNLOADS reports the load.
  bool loaded;
  // True if the administrator's policy lets the user delete history. This is
  // Chromium's AllowDeletingBrowserHistory. When it is false, removing and
  // clearing return WREN_ERROR_NOT_ALLOWED. Read when asked; not reported.
  bool removable;
} WrenDownloadHistoryState;

// Reads the state of the Profile's download history.
WREN_API WrenResult
Wren_GetDownloadHistoryState(WrenProfile* profile,
                             WrenDownloadHistoryState* state);

// Copies up to `capacity` of the Profile's downloads, as chrome://downloads
// lists them, newest start first, and returns how many there are. An
// off-the-record Profile's history has its original's too. Call it with
// `capacity` 0 for the count, then again to fill: within one turn the list
// cannot change between.
//
// Once the app has read it, WREN_CHANGE_DOWNLOADS reports each change to the
// list, and WREN_CHANGE_DOWNLOAD each download's progress or state.
WREN_API size_t Wren_GetDownloadHistory(WrenProfile* profile,
                                        WrenDownloadID* downloads,
                                        size_t capacity);

// As Clear All on chrome://downloads, with no Undo: removes every download
// of the history but those in progress or paused. One with a warning
// unanswered is removed and its unfinished file deleted. An off-the-record
// Profile's clear removes its original's downloads too. Returns
// WREN_ERROR_NOT_ALLOWED where policy forbids deleting history, and inside
// on_popover.
//
// Wren_ClearBrowsingData's WREN_BROWSING_DATA_DOWNLOADS differs: it clears
// a time period of one Profile, downloads chrome://downloads does not list
// included.
WREN_API WrenResult Wren_ClearDownloadHistory(WrenProfile* profile);

// --- The download button ----------------------------------------------------

typedef enum WrenDownloadIcon {
  // Downloads are in progress. Chrome shows its arrow with the ring.
  WREN_DOWNLOAD_ICON_PROGRESS = 0,
  // None is in progress. Chrome shows its plain download icon.
  WREN_DOWNLOAD_ICON_DONE = 1,
  // A file is being scanned. The ring spins.
  WREN_DOWNLOAD_ICON_SCANNING = 2,
} WrenDownloadIcon;

typedef struct WrenDownloadButtonState {
  // True while Chrome would show its button. That is from the start of a
  // download until an hour after the last one ended.
  bool visible;
  // True while Chrome draws the button in the accent colour. That is while a
  // download runs unpaused, and for a minute after a download ends, unless
  // the button is clicked first.
  bool active;

  WrenDownloadIcon icon;
  // The number of downloads in progress, and their progress together, from 0
  // to 100.
  int in_progress;
  int percent;
  // True if the sizes of all of them are known. If not, the ring spins.
  bool percent_known;
} WrenDownloadButtonState;

// Reads the Browser's download button, as Chrome's toolbar would show it.
// Once the app has read it, WREN_CHANGE_DOWNLOAD_BUTTON reports each change.
WREN_API WrenResult Wren_GetDownloadButtonState(WrenBrowser* browser,
                                                WrenDownloadButtonState* state);
// Does what a click on Chrome's button does.
//
// It opens the bubble of the Browser's downloads, and WREN_POPOVER_DOWNLOADS
// asks whether the bubble is Chromium's or the app's. If Chromium's bubble is
// open, it closes it. With no downloads to list, it opens chrome://downloads
// in a tab. After the click the button is no longer active.
//
// Returns WREN_ERROR_NOT_ALLOWED inside on_popover.
WREN_API WrenResult Wren_ClickDownloadButton(WrenBrowser* browser);

// --- Closing and quitting with downloads in progress ------------------------

typedef enum WrenDownloadsPromptKind {
  // The app is quitting, through Wren_Quit, Cmd-Q or logging out.
  WREN_DOWNLOADS_PROMPT_QUIT = 0,
  // `browser` is closing, and it is the last Browser of the off-the-record
  // `profile`.
  WREN_DOWNLOADS_PROMPT_CLOSE = 1,
} WrenDownloadsPromptKind;

// What Chromium is about to ask. Valid only during on_downloads_prompt.
struct WrenDownloadsPrompt {
  WrenDownloadsPromptID prompt;
  WrenDownloadsPromptKind kind;
  // For WREN_DOWNLOADS_PROMPT_CLOSE, the Browser and its Profile. Borrowed.
  WrenBrowser* browser;
  WrenProfile* profile;
  // The number of downloads that would be cancelled.
  int count;
};

// Answers a prompt for which the app answered WREN_POPOVER_APP.
//
// With `cancel_downloads`, Chromium cancels the downloads and goes on closing
// or quitting. Without it, the downloads go on, and so does the Browser or the
// app. Keeping the downloads at a close shows chrome://downloads in the
// Browser, as Chrome does.
//
// Returns WREN_ERROR_POPOVER_CLOSED for a prompt already answered, or whose
// Browser has closed.
WREN_API WrenResult Wren_AnswerDownloadsPrompt(WrenDownloadsPromptID prompt,
                                               bool cancel_downloads);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_DOWNLOADS_H_

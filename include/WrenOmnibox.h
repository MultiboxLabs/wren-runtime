// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's omnibox as data (C API). Wren.h includes this header;
// see Wren.h for threading, handles and strings.
//
// The app draws its own address field and suggestion list. Wren gives it
// what Chrome's show, and Chromium draws none of it:
// - the location state: the address to show while the field is not being
//   edited, from Chromium's location bar model, as Chrome trims it;
// - suggestions: Chromium's autocomplete results for the text the app
//   queries, as Chrome's list shows them, from history, bookmarks, open
//   tabs, search engines and extensions' keywords. Not those that open
//   Chrome's own UI instead of a page ("Ask Google about this page", Lens,
//   AI Mode), which a Wren browser does not have;
// - opening or deleting a suggestion as Chrome does, so Chromium's history,
//   shortcuts and search engines learn from it.
//
// The app handles typing itself, as Chrome's field does: it queries after
// each edit, shows the default suggestion's inline completion selected after
// the caret, moves through the list with the arrow keys putting each
// suggestion's text in the field, brings back what was typed on Escape, and
// enters keyword mode on Tab for a suggestion with a keyword ("Search
// YouTube"). A normal or popup Browser has an omnibox; an app Browser has
// none (WREN_ERROR_NOT_ALLOWED).
//
// Offsets in the text (the cursor, highlight ranges) count UTF-16 code
// units, as NSString does; text is copied out as UTF-8.

#ifndef WREN_PUBLIC_WREN_OMNIBOX_H_
#define WREN_PUBLIC_WREN_OMNIBOX_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Location state ---------------------------------------------------------

typedef enum WrenLocationText {
  // The active tab's address as Chrome shows it in a field that is not being
  // edited: "example.com/page", without "https://" or "www.".
  WREN_LOCATION_TEXT_DISPLAY = 0,
  // The address to put in the field when editing starts, as Chrome does:
  // the whole URL, leaving out only "http://", a user name and password, and
  // a bare host's trailing slash ("https://www.example.com/page").
  WREN_LOCATION_TEXT_EDIT = 1,
  // What Chrome shows in an empty field: "Search Google or type a URL",
  // naming the default search engine; "" with none.
  WREN_LOCATION_TEXT_PLACEHOLDER = 2,
  // The URL the address is about, in full: the page's, or the one a tab is
  // loading at the user's request; "about:blank" while show_url is false.
  WREN_LOCATION_TEXT_URL = 3,
} WrenLocationText;

typedef struct WrenLocationState {
  // Chrome shows the address. Not on the New Tab page, whose field is empty
  // with the placeholder.
  bool show_url;
} WrenLocationState;

// The location state of the Browser's active tab.
// WREN_ERROR_NAVIGATION_UNAVAILABLE while the Browser has no tab.
// WREN_CHANGE_LOCATION reports its changes once the app has read it.
WREN_API WrenResult Wren_GetLocationState(WrenBrowser* browser,
                                          WrenLocationState* state);
// UTF-8; returns the length needed, including the NUL. "" when the state
// cannot be read.
WREN_API size_t Wren_CopyLocationText(WrenBrowser* browser,
                                      WrenLocationText text,
                                      char* buffer,
                                      size_t size);

// --- Suggestions ------------------------------------------------------------

// One set of suggestions: it changes whenever the list does, and actions
// take it back, returning WREN_ERROR_STALE_RESULT for an older one, so a
// click as the list changes never opens the suggestion that took its place.
typedef uint64_t WrenOmniboxResult;

typedef enum WrenOmniboxFlag {
  // Do not complete inline, as Chrome's field after a deletion or a paste.
  WREN_OMNIBOX_NO_INLINE = 1u << 0,
  // Control is held (Ctrl-Enter in Chrome): a typed name becomes
  // "www.<name>.com".
  WREN_OMNIBOX_ADD_COM = 1u << 1,
  // The field has just been focused, not edited: Chrome lists suggestions
  // for an empty field or the page's address (recent and most visited pages,
  // when Chromium has them).
  WREN_OMNIBOX_ON_FOCUS = 1u << 2,
} WrenOmniboxFlag;

typedef struct WrenOmniboxQuery {
  // What the field holds, UTF-8; in keyword mode, the text after the chip.
  const char* text;
  // The caret, in UTF-16 code units; SIZE_MAX for the end.
  size_t cursor;
  // The keyword whose chip the field shows (WREN_SUGGESTION_TEXT_KEYWORD of
  // the suggestion Tab was pressed on), or NULL.
  const char* keyword;
  // WrenOmniboxFlag bits.
  uint32_t flags;
} WrenOmniboxQuery;

// Starts Chromium's autocomplete for `query`, replacing any query running in
// the Browser. Suggestions come at once and again as slower ones arrive (a
// search engine's); WREN_CHANGE_OMNIBOX reports each change.
WREN_API WrenResult Wren_OmniboxQuery(WrenBrowser* browser,
                                      const WrenOmniboxQuery* query);
// Stops the query and clears the suggestions, as Chrome does when its list
// closes (Escape, the field losing focus). WREN_CHANGE_OMNIBOX follows.
WREN_API WrenResult Wren_OmniboxStop(WrenBrowser* browser);

typedef struct WrenOmniboxState {
  WrenOmniboxResult result;
  size_t suggestion_count;
  // Chromium expects no more suggestions for this query.
  bool done;
} WrenOmniboxState;

// The Browser's current suggestions; none before a query and after
// Wren_OmniboxStop.
WREN_API WrenResult Wren_GetOmniboxState(WrenBrowser* browser,
                                         WrenOmniboxState* state);

typedef enum WrenSuggestionKind {
  // A search: what was typed, or one the search engine suggests.
  WREN_SUGGESTION_SEARCH = 0,
  // A search made before.
  WREN_SUGGESTION_SEARCH_HISTORY = 1,
  // An address: what was typed, or one a search engine suggests.
  WREN_SUGGESTION_URL = 2,
  // A page visited before.
  WREN_SUGGESTION_HISTORY = 3,
  // A bookmark.
  WREN_SUGGESTION_BOOKMARK = 4,
  // A tab already open: opening it switches to it.
  WREN_SUGGESTION_OPEN_TAB = 5,
  // A calculation's answer.
  WREN_SUGGESTION_CALCULATOR = 6,
  // What the clipboard holds.
  WREN_SUGGESTION_CLIPBOARD = 7,
  // Anything else Chromium offers, such as an extension's keyword.
  WREN_SUGGESTION_OTHER = 8,
} WrenSuggestionKind;

typedef struct WrenSuggestion {
  WrenSuggestionKind kind;
  // Enter opens this one: the first suggestion, when Chromium allows it to
  // be the default. Its inline completion goes after the caret.
  bool is_default;
  // Its page is bookmarked (Chrome draws a star).
  bool bookmarked;
  // Wren_DeleteSuggestion removes it (from history, or a search engine's
  // record of past searches); Chrome shows an X.
  bool deletable;
  // Its page is open in a tab: WREN_OPEN_SWITCH_TO_TAB switches to it
  // (Chrome's "Switch to this tab" button).
  bool has_tab;
  // Chrome shows the description before the contents (an answer to a
  // search).
  bool swap_contents_and_description;
} WrenSuggestion;

WREN_API WrenResult Wren_GetSuggestion(WrenBrowser* browser,
                                       size_t index,
                                       WrenSuggestion* suggestion);

typedef enum WrenSuggestionText {
  // The main line ("youtube.com", "weather paris").
  WREN_SUGGESTION_TEXT_CONTENTS = 0,
  // The second part ("YouTube", "Google Search").
  WREN_SUGGESTION_TEXT_DESCRIPTION = 1,
  // What the field shows while it is selected with the arrow keys.
  WREN_SUGGESTION_TEXT_FILL = 2,
  // For the default suggestion: the text to add after the caret, selected,
  // as Chrome completes inline; "" if none.
  WREN_SUGGESTION_TEXT_INLINE = 3,
  // A keyword Tab enters keyword mode with ("youtube.com"), and the name
  // for its chip ("YouTube": Chrome shows "Search YouTube"); "" if none.
  WREN_SUGGESTION_TEXT_KEYWORD = 4,
  WREN_SUGGESTION_TEXT_KEYWORD_NAME = 5,
  // The header of the group it starts or belongs to ("Recent searches"),
  // if Chrome shows one; "" if none.
  WREN_SUGGESTION_TEXT_GROUP = 6,
  // Where it goes: a URL. "" for one that does not navigate.
  WREN_SUGGESTION_TEXT_URL = 7,
} WrenSuggestionText;

WREN_API size_t Wren_CopySuggestionText(WrenBrowser* browser,
                                        size_t index,
                                        WrenSuggestionText text,
                                        char* buffer,
                                        size_t size);

// Bits of WrenTextStyle.style, as Chrome draws a suggestion's text.
typedef enum WrenTextStyleBits {
  // An address.
  WREN_TEXT_URL = 1u << 0,
  // Matches what was typed: Chrome draws it bold.
  WREN_TEXT_MATCH = 1u << 1,
  // Helper text, drawn dimmed.
  WREN_TEXT_DIM = 1u << 2,
} WrenTextStyleBits;

// A style from `start` (UTF-16 code units) to the next one's start, or the
// end of the text.
typedef struct WrenTextStyle {
  size_t start;
  uint32_t style;
} WrenTextStyle;

// The styles of a suggestion's CONTENTS or DESCRIPTION, in order. Copies up
// to `capacity` and returns how many there are; 0 for other texts.
WREN_API size_t Wren_GetSuggestionStyles(WrenBrowser* browser,
                                         size_t index,
                                         WrenSuggestionText text,
                                         WrenTextStyle* styles,
                                         size_t capacity);

// The page's favicon for a suggestion of a page (URL, HISTORY, BOOKMARK,
// OPEN_TAB), as PNG at the largest scale Chromium keeps (2x on a Mac), to
// draw at 16 points; returns the length needed, or 0 while there is none.
// Wren loads it from Chromium's favicon store when the suggestion arrives,
// and WREN_CHANGE_OMNIBOX reports it, with the same WrenOmniboxResult. The
// app draws its own icon for the others, by kind.
WREN_API size_t Wren_CopySuggestionIconPNG(WrenBrowser* browser,
                                           size_t index,
                                           uint8_t* buffer,
                                           size_t size);

typedef enum WrenOpenDisposition {
  WREN_OPEN_CURRENT_TAB = 0,
  // A new tab, shown (Chrome: Alt-Enter, a middle click).
  WREN_OPEN_NEW_TAB = 1,
  // A new tab, behind the current one.
  WREN_OPEN_BACKGROUND_TAB = 2,
  // A new window (Chrome: Shift-Enter): a new normal Browser, which
  // on_browser_opened gives the app.
  WREN_OPEN_NEW_WINDOW = 3,
  // The tab that has the suggestion's page (has_tab), in whichever Browser.
  WREN_OPEN_SWITCH_TO_TAB = 4,
} WrenOpenDisposition;

// Opens a suggestion as Chrome does when it is chosen. The app clears its
// field and calls Wren_OmniboxStop, as Chrome reverts its field.
// WREN_ERROR_INVALID_ARGUMENT for SWITCH_TO_TAB without has_tab, or an
// index out of range.
WREN_API WrenResult Wren_OpenSuggestion(WrenBrowser* browser,
                                        WrenOmniboxResult result,
                                        size_t index,
                                        WrenOpenDisposition disposition);
// Removes a deletable suggestion from where Chromium keeps it, as Chrome's
// X does; the suggestions change. WREN_ERROR_NOT_ALLOWED if it is not
// deletable.
WREN_API WrenResult Wren_DeleteSuggestion(WrenBrowser* browser,
                                          WrenOmniboxResult result,
                                          size_t index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_OMNIBOX_H_

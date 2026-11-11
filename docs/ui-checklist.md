# The browser's UI, piece by piece

Chrome's window holds a lot: tabs, an address field with suggestions, buttons for extensions, and bubbles for passwords, permissions and downloads. In a Wren Runtime app you choose, piece by piece, whether Chromium draws it, your own UI draws it, or it doesn't appear at all. This page goes through every piece, and names the API for each.

## Chromium's, yours, or none

Each piece of UI comes one of these ways.

- **Chromium draws it.** You give it a place, such as a button for its bubble to open from, and Chromium draws the rest.
- **You draw it.** You read its state, redraw when its change arrives, and call its actions.
- **You choose each time.** Before Chromium opens one of its popovers, it asks `on_popover`, and you answer whether it shows Chromium's, your own, or nothing.

Chromium's bubbles open from a button, which you name as an anchor. For a native button, call `Wren_SetAnchor`. For a button drawn in a web page, call `Wren_SetAnchorElement`, as [Anchors in a web view](anchors-in-web-views.md) shows. `WrenToolbar.h` lists the anchors' names, such as "passwords", "zoom" and "cast". Name the buttons you have, and the address field stands in for the rest. `Wren_GetAnchorState` says whether a bubble is open from an anchor, so you can draw its button pressed, and `WREN_CHANGE_ANCHORS` reports changes.

This page names the C functions, because it's a map to the headers. The Swift SDK and the Rust SDK have each one under their own name. For example, `Wren_GetSiteState` is `browser.siteState()` in Swift and `Browser::site_state` in Rust. Each Swift and Rust item's comment names its C function, so a search for the C name finds it.

The native sample draws its own site panel, extensions panel and password prompts, and lets Chromium draw the rest. [What Wren Runtime can't do yet](limitations.md) lists the parts that aren't available.

## The window and tabs

Your app draws all of these, because Chrome's versions are part of Chrome's window. [Windows and tabs](windows-and-tabs.md) covers them in detail.

| UI | What you draw | API |
| --- | --- | --- |
| Tab strip | The tabs, with their titles, favicons, spinners and pins, and a New Tab button | `Wren_GetTabCount`, `Wren_GetTabAt`, `Wren_GetTabState`, `Wren_NewTab`, `WREN_CHANGE_TABS`, `WREN_CHANGE_TAB` |
| Tab views | Each tab's view in its window, at whole-point sizes, with the active one showing | `Wren_GetTabView`, `Wren_AttachBrowserToView`, `Wren_FocusTab` |
| Crashed tab | Something of your own over the tab's view, with a way to reload | `WrenTabState.crash`, `Wren_Reload` |
| Tab alerts and the speaker | Chrome's icon for what a tab is doing, which is the first of its alerts in the header's order, or a muted speaker for a muted tab playing sound. Clicking the speaker mutes or unmutes the tab. | `WrenTabState.alerts`, `WrenTabState.muted`, `Wren_SetTabMuted`, `WREN_TAB_FIELD_ALERTS`, `WREN_TAB_FIELD_MUTED` |
| Tab groups | Group labels and colours, and menu items to group and ungroup | `Wren_GroupTabs`, `Wren_SetTabGroupVisuals`, `WREN_CHANGE_GROUP` |
| Moving tabs to another window | Menu items, or a drag between windows. Bring the other window forward yourself. | `Wren_MoveTabsToBrowser`, `Wren_MoveTabsToNewBrowser` |
| Pictures of tabs | A tab's picture wherever you want one, such as on a hover card or in a tab switcher | `Wren_CaptureTab`, `WrenTabState.unloaded` |
| A tab under your own UI | Nothing. Make the tab ignore input while your UI covers it, and keep the keyboard off it. | `Wren_SetTabIgnoresInput`, `WrenTabState.ignores_input`, `WREN_TAB_FIELD_IGNORES_INPUT` |
| Split tabs | Both sides, and the divider between them | `Wren_SplitTabs`, `Wren_SetSplitRatio`, `WREN_CHANGE_SPLIT` |
| Side panels | A place for the side panel's view, at the width Chromium remembers, and a resize handle. Chromium puts each panel's page in the view. | `Wren_GetSidePanelView`, `Wren_ShowSidePanel`, `Wren_SetSidePanelWidth`, `WREN_CHANGE_SIDE_PANEL` |
| Status bubble | Chrome's bubble at the bottom left of a page, with the link under the pointer or what a loading page waits for, if you want it | `WrenTabState.status`, `WREN_TAB_TEXT_STATUS`, `WREN_TAB_FIELD_STATUS` |
| Popups and other windows Chromium opens | A window for each, with the parts Chrome gives that kind of window | `on_browser_opened`, `Wren_GetBrowserType`, `Wren_GetWindowFeatures`, `Wren_GetRequestedBounds` |
| DevTools | Nothing. Docked DevTools is part of the tab's view, and undocked DevTools arrives as a Browser of its own. | `Wren_OpenDevTools`, `on_browser_opened` |
| Main menu | Nothing, or a menu of your own. Wren Runtime's default and Chrome's own are ready to use. | `WrenStartup.menu`, `Wren_SetMainMenu` |
| Dialogs that block a window | Your own UI, turned off while one is open | `Wren_HasModalDialog`, `WREN_CHANGE_MODAL_DIALOG` |
| Restoring the last session | A way to ask for it, at launch, from a button, or never. Each window arrives as a Browser, with the frame it had. | `Wren_GetLastSession`, `Wren_RestoreLastSession`, `on_browser_opened` |
| Bookmarks bar and folders | The bar, with its folders as menus, and as much of Chrome's bookmark manager as you want to offer. Chromium's own bar isn't shown. | `Wren_GetBookmarksState`, `Wren_GetBookmarkChildAt`, `Wren_GetBookmarkItem`, `Wren_AddBookmarkItem`, `Wren_MoveBookmarkItem`, `Wren_OpenBookmarkItem`, `WREN_CHANGE_BOOKMARKS` |
| Reopen Closed Tab | A menu item or button, turned on while there's something to reopen | `Wren_CanReopenClosedTab`, `Wren_ReopenClosedTab`, `WREN_CHANGE_CLOSED_TABS` |
| Full screen | Your window in full screen when a page asks, showing only the page. Chromium shows its "Press Esc" notice over it. | `on_fullscreen`, `Wren_SetFullscreen`, `Wren_GetFullscreenTab`, `WREN_CHANGE_FULLSCREEN` |
| Moving and resizing windows | Whatever you choose to do when Chromium asks, such as for an extension's `chrome.windows.update` or a popup's `resizeTo` | `on_window_request`, `Wren_GetRequestedWindowState` |

Chrome's status bubble appears 80 ms after its text arrives, and fades 250 ms after the text goes. It's a third of the page wide, with the address shortened, and grows to fit after 1.6 seconds. It also moves out of the pointer's way.

### A window drawn in HTML

When a window's UI is one [web view](web-views.md)'s page, the page draws the pieces above from what your app sends it. A few pieces are new. [A browser drawn in HTML](html-browser.md) goes through them.

| UI | What you do | API |
| --- | --- | --- |
| The window | Make it, with a hidden title bar if the page draws its own. Put the web view in it, and attach each Browser to the web view's view. | `Wren_CreateWebView`, `Wren_AttachBrowserToView` |
| Title bar | Turn on the option. The page marks what drags the window with CSS `app-region`, and leaves room for the window's buttons. | `WrenWebViewOptions.drags_window` |
| Tabs and side panels | Tell the page which IDs to show, and the page shows them with `<wren-view>` | `WrenWebViewOptions.embeds_views` |
| Toolbar buttons | Answer the page's calls, and name the page's buttons as each Browser's anchors | `WrenWebViewOptions.messages`, `WrenWebViewOptions.anchors`, `Wren_SetAnchorElement` |
| [Workspaces](workspaces.md), several Browsers shown in turn | Keep all but one in the background, and decide what to do when Chromium wants another in front | `Wren_SetBrowserBackgrounded`, `WREN_CHANGE_FOREGROUND_REQUEST` |
| The page failing or crashing | Reload it, or show something of your own | `Wren_GetWebViewState`, `Wren_ReloadWebView`, `WREN_CHANGE_WEB_VIEW` |
| A development server | Trust its origin, in development builds only | `WrenWebViewOptions.trusted_origins` |
| Context menus | Show the items the page sends for its right clicks. Text fields get Wren Runtime's editing menu. | `Wren_ShowContextMenu`, and [Menus](menus.md) |

## The address field

You draw the address field itself. Chromium gives you what it shows, its suggestions, and the icons inside it.

| UI | What you get | API |
| --- | --- | --- |
| Address and focus | Chromium asks for focus, such as on the New Tab page. For Search the Web, it asks for the default search engine's keyword mode. | `on_focus_location_bar`, `Wren_GetLocationFocus`, `WREN_LOCATION_TEXT_SEARCH_KEYWORD` |
| What the field shows | The address shortened for display, the whole address for editing, and a placeholder that names the search engine | `Wren_GetLocationState`, `Wren_CopyLocationText`, `WREN_CHANGE_LOCATION` |
| Suggestions | Chromium's autocomplete for the typed text, as data to draw as a list. Open a row with `Wren_OpenSuggestion`, so history and search engines learn from it as in Chrome. | `Wren_OmniboxQuery`, `Wren_GetOmniboxState`, `Wren_GetSuggestion`, `Wren_CopySuggestionText`, `WREN_CHANGE_OMNIBOX` |
| Site icon and security | The site's security, to show always. Name your icon `"location"`, and its click opens page info. | `Wren_GetSiteState`, `Wren_ShowPageInfo`, `WREN_CHANGE_SITE` |
| Page actions, such as the key, zoom, star, translate and the app picker | The icons to draw, and their clicks. You can also draw your own icons instead. | `Wren_GetPageActionState`, `Wren_CopyPageActionIconPNG`, `Wren_ClickPageAction`, `WREN_CHANGE_PAGE_ACTIONS` |
| Site setting icons, such as a blocked popup or location in use | The icons to draw, highlighted while their bubble is open, and their clicks | `Wren_GetContentSettingIcon`, `Wren_IsContentSettingBubbleOpen`, `Wren_ShowContentSettingBubble`, `WREN_CHANGE_CONTENT_SETTINGS` |
| Cast button | A button that shows while Chrome would show its own, pressed while Cast's dialog is open. Its click opens or closes the dialog. Cast is also a command for your menu. | `WREN_PAGE_ACTION_CAST`, `WREN_CHROMIUM_COMMAND_CAST`, `WREN_CHANGE_PAGE_ACTIONS` |
| Where the field's bubbles open from | Name the whole field as `"address-field"` | `Wren_SetAnchor` |

## Popovers

Whatever opens one of Chromium's popovers, Chromium asks `on_popover` first, and you give one of three answers.

- **`WREN_POPOVER_CHROMIUM`** shows Chromium's, from the anchor named. Without a handler, this is what happens.
- **`WREN_POPOVER_APP`** means you draw your own, from the data.
- **`WREN_POPOVER_NONE`** shows nothing.

A request you take stays open until you close it with `Wren_ClosePopover`, or until Chromium withdraws it, and `WREN_CHANGE_POPOVER` reports both. Show a request marked `WREN_POPOVER_AUTOMATIC` without taking focus, because nobody clicked anything. Inside `on_popover`, only decide. Wren Runtime refuses calls that change Browsers or tabs until it returns.

| Popover | Chromium asks when | Draw your own from |
| --- | --- | --- |
| Page info | `Wren_ShowPageInfo` is called | The site's state: its origin, security, permissions, devices and cookies, with their actions (`Wren_GetSiteState`, `Wren_SetSitePermission`, `Wren_SiteAction`) |
| Extensions menu | `Wren_ShowExtensionsMenu` or one of Chromium's commands opens it | What each extension may do on the site, with its requests (`Wren_GetExtensionsSite`, `Wren_GetExtensionOnSite`) |
| Zoom | ⌘+ and the like, an extension zooms, or someone clicks the zoom icon | The zoom state (`Wren_GetZoomState`, `Wren_CopyZoomExtensionID`, `Wren_Zoom`). Close it after 1.5 seconds, as Chrome does. |
| Find bar | ⌘F, Find Next, or `Wren_Find`, or someone returns to a tab with a find open | The find state (`Wren_GetFindState`, `Wren_CopyFindString`, `Wren_SetFindString`, `Wren_Find`). Return and Shift-Return step through matches, Escape ends the find, and Control-Return clicks a link's match. |
| Download bubble | All of a Profile's downloads finish, `Wren_ClickDownloadButton` is called, or chrome://downloads shows a warning | The Profile's downloads (`Wren_GetDownloadCount`, `Wren_GetDownloadAt`, `Wren_GetDownloadState`, `Wren_DoDownloadAction`). Close the short one after 5 seconds unless it's used, as Chrome does. |
| Bookmark star | ⌘D or the star, in an active window | The bookmark, with suggested folders (`Wren_GetBookmark`, `Wren_SetBookmarkTitle`, `Wren_MoveBookmark`) |
| Site setting bubble | `Wren_ShowContentSettingBubble` is called | What was blocked, and the site's setting (`Wren_GetContentSettingState`, `Wren_CopyContentSettingItemURL`, `Wren_SetContentSettingValue`) |
| Translate | Translate offers itself, or someone clicks its icon | The language data (`Wren_GetTranslateState`, `Wren_CopyPopoverText`). Translate needs a Google API key. |
| Saving or updating a password, and saved passwords | After a sign-in, or a click on the key. Never while the Profile's password manager is off. | The passwords data (`Wren_GetPasswordsState`, `Wren_RevealPasswords`, `Wren_SavePassword`) |
| Saving or updating an address | After a form | The address data (`Wren_GetAddressState`, `Wren_AnswerAddressPrompt`) |
| Saving a card on this Mac | After a payment form | The card's network and last digits, never its number (`Wren_GetCardState`, `Wren_CopyPopoverText`, `Wren_SaveCard`) |
| Permission prompt | A page asks for a permission | The permission data (`Wren_GetPermissionPromptState`, `Wren_CopyPopoverText`, `Wren_AnswerPermissionPrompt`). Most apps let Chromium show these. |
| Toast, such as "Link copied" | Chromium shows one, such as after copying a link or adding to the reading list | The toast data (`WrenPopoverRequest.toast`, `Wren_GetToastState`, `Wren_PressToastButton`). Chromium closes it when its own would go. Chromium's opens from the "toast" anchor. Your own toasts from `Wren_ShowToast` come the same way. |

Every popover's text is Chrome's own, in `WrenUnstableUI.h`, which can change in any release. Everything in this table is in the stable API. See [Compatibility](compatibility.md).

## Extensions

| UI | Chromium's | Your own |
| --- | --- | --- |
| Extensions' buttons | None. Draw them, and name each as `"extension:<id>"`. | `Wren_GetExtensionActionCount`, `Wren_CopyExtensionActionIconPNG`, `Wren_ClickExtensionAction`, `WREN_CHANGE_EXTENSION_ACTIONS` |
| Extensions' popups | Chromium's, below the button, because they're web pages | None |
| A button's context menu | Chromium's, from `Wren_ShowExtensionActionMenu` | None |
| Extensions button and menu | Name your button `"extensions"`. Its menu is a popover, above. | The button's state, with `Wren_GetExtensionsButtonState` and `Wren_GrantRequestedSiteAccess` |
| Installed extensions | Chromium's `chrome://extensions` page | A list of a Profile's extensions, to turn on or off or allow in incognito, with `Wren_GetExtensionCount`, `Wren_GetExtensionState`, `Wren_CopyExtensionIconPNG`, `Wren_SetExtensionEnabled`, `Wren_SetExtensionAllowedInIncognito` and `WREN_CHANGE_EXTENSIONS` |

## Chromium's own, which you give a place

These stay Chromium's, and need only a place to appear. Attach every Browser you show, and name your anchors.

- **Dialogs and sheets,** such as installing and removing extensions, the bookmark and address editors, the certificate viewer, site data, and resubmitting a form.
- **Prompts from the system and the page,** such as device choosers for USB, HID, serial and Bluetooth, passkeys, HTTP authentication, and a page's alerts and confirms.
- **Prompts for a page's `<permission>` elements,** and for the New Tab page's microphone when you let Chromium show it. They're sheets over the page, centred or beside the element. Wren Runtime dims the page inside the tab's view, so the dimming stays inside however you clip the page.
- **The find bar,** unless you draw your own. It opens below the right end of the `"address-field"` anchor, or at the page's top right without one.
- **The download bubble,** unless you draw your own. It opens below the `"downloads"` anchor, or else the `"address-field"` one.
- **Chrome's prompts about downloads in progress,** before an incognito Profile's last Browser closes or the app quits. To ask in your own way, answer `WREN_POPOVER_APP` in `on_downloads_prompt`, then give the answer with `Wren_AnswerDownloadsPrompt`.

Some pieces belong to Chrome's toolbar, so they're yours to draw:

- **The download button.** Draw it from `Wren_GetDownloadButtonState`, which says whether it shows, its icon, its progress ring, and whether it's active. Name it `"downloads"`, and click it with `Wren_ClickDownloadButton`.
- **The downloads page,** if you'd rather not use chrome://downloads. `Wren_GetDownloadHistory` lists downloads, newest first, with each one's `start_ms` and `end_ms` to group them by day, and `Wren_GetDownloadHistoryState` says whether the list has loaded. Remove from List is `WREN_DOWNLOAD_REMOVE`, and Clear All is `Wren_ClearDownloadHistory`. Neither can be undone, so confirm first. Hide both where the state says the history isn't `removable`.

## Not available yet

Infobars aren't available, including page info's offer to reload, so show your own from the site state's `reload_needed`. Lens's overlay isn't available, and Chrome's commands that need UI Wren Runtime doesn't have are turned off. [What Wren Runtime can't do yet](limitations.md) lists the rest.

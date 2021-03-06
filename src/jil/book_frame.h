#ifndef JIL_BOOK_FRAME_H_
#define JIL_BOOK_FRAME_H_

#include <list>
#include <vector>

#include "wx/arrstr.h"
#include "wx/filename.h"
#include "wx/frame.h"

#include "editor/binding.h"
#include "editor/compile_config.h"
#include "editor/theme.h"

#include "jil/compile_config.h"
#include "jil/defs.h"
#include "jil/id.h"
#include "jil/option.h"

namespace editor {
class Binding;
class FtPlugin;
class Options;
class Style;
class TextBuffer;
class TextPoint;
class TextRange;
class TextWindow;
}  // namespace editor

namespace jil {

class BookCtrl;
class BookPage;
class FindResultPage;
class FindPanel;
class FindThreadBase;
class PageWindow;
class Session;
class Splitter;
class SplitNode;
class StatusBar;
class TextBook;
class TextPage;
class ToolBook;

class BookFrame : public wxFrame {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    COLOR_BG = 0,
    COLORS,
  };

  friend class FindThreadBase;

public:
  BookFrame();
  virtual ~BookFrame();

  virtual bool Destroy() override;

  void set_options(Options* options) {
    options_ = options;
  }

  void set_editor_options(editor::Options* editor_options) {
    editor_options_ = editor_options;
  }

  void set_session(Session* session) {
    session_ = session;
  }

  bool Create(wxWindow* parent, wxWindowID id, const wxString& title);

  virtual bool Show(bool show = true) override;

  // Must be called before Create().
  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  void set_style(editor::Style* style) {
    style_ = style;
  }

  void set_binding(editor::Binding* binding) {
    binding_ = binding;
  }

  //----------------------------------------------------------------------------

  // \param silent Don't popup error message box on failure.
  // Return the text page of this file.
  TextPage* OpenFile(const wxString& file_name, bool active, bool silent);

  // \param silent Don't popup error message box on failure.
  void OpenFiles(const wxArrayString& file_names, bool silent);

  // Restore last opened files from session.
  void RestoreOpenedFiles();

  // File menu operations.
  void FileNew();

  void FileOpen();

  void FileClose();
  void FileCloseAll();
  void FileCloseAllButThis();

  void FileSave();
  void FileSaveAs();
  void FileSaveAll();
  void FileCopyPath();
  void FileOpenFolder();

  void SwitchToNextStackPage();
  void SwitchToPrevStackPage();

  void ShowFind();
  void ShowReplace();

  void FindNext();
  void FindPrev();

  // Toggle the wrap state of the active text page.
  void Wrap();
  // Toggle the line number show state of the active text page.
  void ShowNumber();
  // Toggle the white space show state of the active text page.
  void ShowSpace();

  // Show full screen or not.
  void FullScreen();

  TextPage* ActiveTextPage() const;
  editor::TextBuffer* ActiveBuffer() const;

  //----------------------------------------------------------------------------
  // Find & Replace

  // Set last find string and flags for finding with shortcut keys (menu) later.
  void SetLastFindStringAndFlags(const std::wstring& str, int flags);

  // Find string in the active text page "incrementally".
  // Usually when the user is typing.
  void FindInActivePageIncrementally(const std::wstring& str, int flags);

  // Find string in the active text page, select it and update the caret point.
  void FindInActivePage(const std::wstring& str, int flags);

  void FindAllInActivePage(const std::wstring& str, int flags);

  void FindAllInAllPages(const std::wstring& str, int flags);

  void FindAllInFolder(const std::wstring& str,
                       int flags,
                       const wxString& folder);

  // Find string in the file from another thread (async).
  // NOTE: Don't call or trigger any GUI operation (e.g., refresh).
  // Return the number of new lines added to the find result buffer.
  // See FindThread.
  int AsyncFindInFile(const std::wstring& str,
                      int flags,
                      const wxString& file);

  void ReplaceInActivePage(const std::wstring& str,
                           const std::wstring& replace_str,
                           int flags);

  void ReplaceAllInActivePage(const std::wstring& str,
                              const std::wstring& replace_str,
                              int flags);

  void ReplaceAllInAllPages(const std::wstring& str,
                            const std::wstring& replace_str,
                            int flags);

  void ReplaceAllInFolder(const std::wstring& str,
                          const std::wstring& replace_str,
                          int flags,
                          const wxString& folder);

  // Return the ID of the new buffer being replaced.
  int AsyncReplaceInFile(const std::wstring& str,
                         const std::wstring& replace_str,
                         int flags,
                         const wxString& file);

protected:
  void Init();

  void OnSize(wxSizeEvent& evt);

  // Layout child windows.
  void UpdateLayout();

  void RestoreSplitTree(SplitNode* n);
  SplitNode* CreateDefaultSplitTree();

  void OnKeyDownHook(wxKeyEvent& evt);
  // Return true if the event is handled.
  bool HandleKeyDownHook(wxKeyEvent& evt);

  void OnAbout(wxCommandEvent& evt);
  void ShowAboutDialog();

  void OnGlobalPrefs(wxCommandEvent& evt);

  // Compare global options and apply the changes.
  void ApplyGlobalOptionChanges(const Options& old_options);

  void ApplyLinePadding(int line_padding);
  void ApplyTextFont(const wxFont& font);

  void OnCurrentSyntaxPrefs(wxCommandEvent& evt);
  void OnCurrentSyntaxPrefsUpdateUI(wxUpdateUIEvent& evt);

  void OnSyntaxSpecificPrefs(wxCommandEvent& evt);

  // Show syntax specific options dialog.
  void HandleSyntaxSpecificPrefs(editor::FtPlugin* ft_plugin);

  // Apply changes to the text pages/buffers of the given file type.
  void ApplySyntaxSpecificOptionChanges(const wxString& ft_id,
                                        const editor::Options& options,
                                        const editor::Options& old_options);

  void OnMenuTheme(wxCommandEvent& evt);
  void ReloadTheme(const wxString& theme_name);

  void OnQuit(wxCommandEvent& evt);

  void OnMenuFile(wxCommandEvent& evt);
  void OnMenuFileRecentFile(wxCommandEvent& evt);

  void OnMenuEdit(wxCommandEvent& evt);
  void OnMenuView(wxCommandEvent& evt);
  void OnMenuTools(wxCommandEvent& evt);
  void OnMenuHelp(wxCommandEvent& evt);

  // Return true if the command function is found and executed.
  bool ExecFuncByMenu(int menu);

  void OnMenuFileUpdateUI(wxUpdateUIEvent& evt);
  void OnMenuEditUpdateUI(wxUpdateUIEvent& evt);
  void OnMenuViewUpdateUI(wxUpdateUIEvent& evt);
  void OnMenuToolsUpdateUI(wxUpdateUIEvent& evt);
  void OnMenuThemeUpdateUI(wxUpdateUIEvent& evt);

  void OnClose(wxCloseEvent& evt);

  // Text page(s) added or removed.
  void OnTextBookPageChange(wxCommandEvent& evt);

  // Active text page switch.
  void OnTextBookPageSwitch(wxCommandEvent& evt);

  // Tool page(s) added or removed.
  void OnToolBookPageChange(wxCommandEvent& evt);

  // Update status fields according to active text page.
  void UpdateStatusFields();

  // Update status fields with the given text page.
  void UpdateStatusCaret(PageWindow* pw, bool refresh);
  void UpdateStatusTabOptions(PageWindow* pw, bool refresh);
  void UpdateStatusEncoding(PageWindow* pw, bool refresh);
  void UpdateStatusFileFormat(PageWindow* pw, bool refresh);
  void UpdateStatusFileType(PageWindow* pw, bool refresh);

  // Update title with the file path of the active text page.
  void UpdateTitleWithPath();

  // Example: "3/120, 27"
  wxString GetStatusCaretString(PageWindow* pw) const;
  // Example: "Tab: 4"
  wxString GetStatusTabOptionsString(PageWindow* pw) const;

  // Handle events from text window.
  void OnTextWindowEvent(wxCommandEvent& evt);

  // Create edit menu items according to current page.
  void RecreateEditMenu();

  // Handle events from find result page.
  void OnFindResultPageEvent(wxCommandEvent& evt);

  void HandleFindResultPageLocalize(FindResultPage* fr_page);
  void HandleFindResultPageDestroy();

  // Status field event handler.
  void OnStatusFieldClick(wxCommandEvent& evt);

  void PopupStatusTabOptionsMenu();
  void PopupStatusEncodingMenu();
  void PopupStatusFileFormatMenu();
  void PopupStatusFileTypeMenu();

  void OnStatusTabOptionsMenu(wxCommandEvent& evt);
  void OnStatusEncodingMenu(wxCommandEvent& evt);
  void OnStatusFileFormatMenu(wxCommandEvent& evt);
  void OnStatusFileTypeMenu(wxCommandEvent& evt);

  void UpdateStatusIndentOptionsField(TextPage* text_page);

  // Show message on status bar.
  // See StatusBar::SetMessage() for details.
  void ShowStatusMessage(const wxString& msg, int sec = 0);

  // Return find panel if it's shown.
  FindPanel* GetFindPanel() const;

  // \param mode See FindPanel::Mode
  void ShowFindPanel(int mode);

  // Close find panel, update layout, etc.
  void CloseFindPanel();

  // Find panel changed its layout.
  void OnFindPanelLayoutEvent(wxCommandEvent& evt);

  // Get find result page, create it if necessary.
  FindResultPage* GetFindResultPage(bool create);

  // Show tool book and activate the given page.
  void ActivateToolPage(BookPage* page);

  // Get the page which has focus.
  BookPage* GetFocusedPage();

  // The focused page must be the current page, but the current page might
  // not be focused.
  // Only used by Save As.
  BookPage* GetCurrentPage();

private:
  //----------------------------------------------------------------------------
  // Find & Replace

  // Return the matched result range.
  editor::TextRange Find(TextPage* text_page,
                         const std::wstring& str,
                         const editor::TextPoint& point,
                         int flags,
                         bool cycle);

  // Find all in the given buffer, add the lines to the find result buffer.
  // \param notify Notify the listeners of find result buffer.
  void FindAll(const std::wstring& str,
               int flags,
               editor::TextBuffer* buffer,
               bool notify,
               editor::TextBuffer* fr_buffer);

  // Find all in the given buffer, return the result text ranges.
  void FindAll(const std::wstring& str,
               int flags,
               editor::TextBuffer* buffer,
               std::list<editor::TextRange>* result_ranges);

  // Replace all in the given text page.
  void ReplaceAll(const std::wstring& str,
                  const std::wstring& replace_str,
                  int flags,
                  TextPage* text_page);

  // Replace all in the given text buffer.
  // Used to replace in a folder before create the text page.
  // Return true if any string was really replaced.
  bool ReplaceAll(const std::wstring& str,
                  const std::wstring& replace_str,
                  int flags,
                  editor::TextBuffer* buffer);

  void AddFrFilePathLine(editor::TextBuffer* buffer, editor::TextBuffer* fr_buffer);

  void AddFrMatchLines(editor::TextBuffer* buffer,
                       std::list<editor::TextRange>& result_ranges,
                       editor::TextBuffer* fr_buffer);

  void AddFrMatchCountLine(editor::TextBuffer* buffer, std::size_t count,
                           editor::TextBuffer* fr_buffer);

  // The find result won't be selected and the caret point won't be updated
  // if it's incremental.
  void SetFindResult(PageWindow* page_window,
                     const editor::TextRange& find_result,
                     bool incremental);

  void ClearFindResult(FindResultPage* fr_page);

  void OnFindThreadEvent(wxThreadEvent& evt);
  void OnReplaceThreadEvent(wxThreadEvent& evt);

  // Terminate find thread if it's running.
  void StopFindThread();

  void CreateFrBuffer();
  void DeleteFrBuffer();

  //----------------------------------------------------------------------------
  // Menu

  // TODO: TextWindow also has AppendMenuItem, remove one of them.
  wxMenuItem* AppendMenuItem(wxMenu* menu, int id, const wxString& label, wxItemKind kind = wxITEM_NORMAL);

  void LoadMenus();

  // Some void commands don't have menu items though they should always have menu IDs.
  // A void command without menu item can only be executed by shortcut keys.
  // When create menus, wxWidgets automatically builds and updates accelerator tables
  // with the shortcut keys specified in the menu's label. This is a bad design though
  // for most cases it's really convenient.
  // This function will overwrite the accelerator table built during creating menus.
  // TODO: Avoid building accelerator table when create menus.
  void SetAccelForVoidCmds();

  bool GetFileMenuState(int menu_id);
  bool GetEditMenuState(int menu_id);
  bool GetViewMenuState(int menu_id, bool* check = NULL);
  bool GetToolsMenuState(int menu_id);

#if JIL_ENABLE_LEADER_KEY
  bool GetMenuEnableState(int menu_id);
#endif  // JIL_ENABLE_LEADER_KEY

  // Add installed themes to the theme menu.
  void InitThemeMenu(wxMenu* theme_menu);

  void InitFileTypeMenu(wxMenu* ft_menu, int id_begin);

  // Clear and recreate the items for Recent Files menu.
  void UpdateRecentFilesMenu();

  //----------------------------------------------------------------------------
  // Text Page

  TextPage* TextPageByFileName(const wxFileName& fn_object) const;

  TextPage* TextPageByBufferId(std::size_t buffer_id) const;

  // Remove all text pages from text book.
  // \param from_destroy It's going to be destroyed.
  // \param except_page If specified, this page won't be removed;
  //                    Must be NULL if from_destroy is true.
  //                    E.g., Close All But This.
  void RemoveAllTextPages(bool from_destroy, const TextPage* except_page);

  void SwitchStackPage(bool forward);

  //----------------------------------------------------------------------------
  // File - Open, Save

  // Create a buffer according to the given file name.
  editor::TextBuffer* CreateBuffer(const wxFileName& fn, std::size_t id,
                                   bool scan_lex);

  TextPage* DoOpenFile(const wxString& file_name,
                       bool active,
                       bool silent,
                       bool update_recent_files,
                       bool update_recent_files_menu);

  // \param silent Don't popup error message box on failure.
  // Return the text page of this file.
  TextPage* DoOpenFile(const wxFileName& fn_object,
                       bool active,
                       bool silent,
                       bool update_recent_files,
                       bool update_recent_files_menu);

  void DoSaveBuffer(editor::TextBuffer* buffer);

  //----------------------------------------------------------------------------

  void AddRecentFile(const wxString& recent_file, bool update_menu);
  void RemoveRecentFile(const wxString& recent_file, bool update_menu);

  //----------------------------------------------------------------------------

  // Rescan the lex for the active text page.
  void RescanLex();

  // Show errors and warnings.
  // E.g., indent function errors.
  void ShowErrors();

private:
  Options* options_;
  editor::Options* editor_options_;

  Session* session_;

  editor::Style* style_;
  editor::SharedTheme theme_;

  editor::Binding* binding_;

  // Splitter splits sub windows.
  Splitter* splitter_;

  TextBook* text_book_;
  ToolBook* tool_book_;

  StatusBar* status_bar_;

  FindPanel* find_panel_;

  // Current page type.
  // See BookPage::Page_Type() and enum PageType.
  int page_type_;

  // Find result buffer.
  // Always keep a find result buffer to simplify the handling of find thread.
  editor::TextBuffer* fr_buffer_;

  std::wstring find_str_;  // Last find string.
  int find_flags_;  // Last find flags (excluding kFind_Reversely).

  // The thread to search or replace text in a given folder.
  FindThreadBase* find_thread_;

  // Critical section to guard the find thread.
  wxCriticalSection find_thread_cs_;

  // Temporary buffers which have been changed by the replace thread.
  std::map<std::size_t, editor::TextBuffer*> replaced_buffers_;

  // Critical section to guard the replaced buffers.
  wxCriticalSection replaced_buffers_cs_;

#if JIL_ENABLE_LEADER_KEY
  editor::Key leader_key_;
#endif  // JIL_ENABLE_LEADER_KEY

  // Edit menu will be updated according to the current page.
  wxMenu* edit_menu_;

  // File menu -> Recent Files
  std::list<wxString> recent_files_;
  wxMenu* recent_files_menu_;

#if defined (__WXGTK__)
  // Client size before show full screen.
  wxSize last_client_size_;
#else
  // Screen rect before show full screen.
  wxRect last_screen_rect_;
#endif  // __WXGTK__
};

}  // namespace jil

#endif  // JIL_BOOK_FRAME_H_

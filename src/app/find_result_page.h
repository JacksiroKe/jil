#ifndef JIL_FIND_RESULT_PAGE_H_
#define JIL_FIND_RESULT_PAGE_H_
#pragma once

// Text window as find result page in the tool book.

#include "editor/text_window.h"
#include "app/book_ctrl.h"

namespace jil {

class FindResultPage : public editor::TextWindow, public BookPage {
  DECLARE_CLASS(FindResultPage)

public:
  // Detailed event types of kFindResultPageEvent.
  enum EventType {
    kLocalizeEvent = 1,
  };

public:
  explicit FindResultPage(editor::TextBuffer* buffer);
  bool Create(wxWindow* parent, wxWindowID id, bool hide = false);
  virtual ~FindResultPage();

  // OVERRIDE of BookPage:
  virtual wxWindow* Page_Window() override { return this; }
  virtual void Page_Activate(bool active) override;
  virtual bool Page_Close() override;
  virtual wxString Page_Type() const override;
  virtual wxString Page_Label() const override;
  virtual wxString Page_Description() const override;
  virtual int Page_Flags() const override;

  virtual void Page_EditMenu(wxMenu* edit_menu) override;
  virtual bool Page_EditMenuState(int menu_id) override;

protected:
  // OVERRIDE of editor::TextWindow:
  virtual void HandleTextLeftDClick(wxMouseEvent& evt) override;
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;
};

BEGIN_DECLARE_EVENT_TYPES()
// Check GetInt(), which returns enum FindResultPage::EventType, for the
// details.
DECLARE_EVENT_TYPE(kFindResultPageEvent, 0)
END_DECLARE_EVENT_TYPES()

}  // namespace jil

#define EVT_FIND_RESULT_PAGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(jil::kFindResultPageEvent, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_FIND_RESULT_PAGE_H_

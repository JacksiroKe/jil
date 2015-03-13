#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/log.h"

#include "ui/bitmap_toggle_button.h"
#include "ui/color.h"
#include "ui/text_button.h"

#include "editor/text_buffer.h"

#include "app/id.h"
#include "app/session.h"
#include "app/skin.h"
#include "app/text_page.h"
#include "app/util.h"

#define kTrUseRegex _("Regular expression")
#define kTrCaseSensitive _("Case sensitive")
#define kTrMatchWholeWord _("Match whole word")

#define kTrFind _("Find")
#define kTrReplace _("Replace")
#define kTrFindAll _("Find All")
#define kTrReplaceAll _("Replace All")

namespace jil {

const int kPadding = 5;

DEFINE_EVENT_TYPE(kFindPanelEvent)

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
EVT_PAINT(FindPanel::OnPaint)
EVT_TOGGLEBUTTON(ID_REGEX_TOGGLE_BUTTON, FindPanel::OnRegexToggle)
EVT_TOGGLEBUTTON(ID_CASE_TOGGLE_BUTTON, FindPanel::OnCaseToggle)
EVT_TOGGLEBUTTON(ID_WHOLE_WORD_TOGGLE_BUTTON, FindPanel::OnWholeWordToggle)
EVT_BUTTON(ID_FIND_BUTTON, FindPanel::OnFind)
EVT_BUTTON(ID_FIND_ALL_BUTTON, FindPanel::OnFindAll)
EVT_BUTTON(ID_REPLACE_BUTTON, FindPanel::OnReplace)
EVT_BUTTON(ID_REPLACE_ALL_BUTTON, FindPanel::OnReplaceAll)
EVT_TEXT(ID_FIND_COMBOBOX, FindPanel::OnFindText)
EVT_TEXT_ENTER(ID_FIND_COMBOBOX, FindPanel::OnFindTextEnter)
END_EVENT_TABLE()

FindPanel::FindPanel()
    : session_(NULL), mode_(kFindMode) {
}

FindPanel::FindPanel(Session* session, int mode)
    : session_(session), mode_(mode) {
}

bool FindPanel::Create(wxWindow* parent, wxWindowID id) {
  assert(theme_);
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(theme_->GetColor(BG_TOP));

  // Create text button style.
  InitButtonStyle();

  //------------------------------------

  regex_toggle_button_ = NewToggleButton(ID_REGEX_TOGGLE_BUTTON,
                                         wxT("find_regex"));
  regex_toggle_button_->SetToolTip(kTrUseRegex);

  case_toggle_button_ = NewToggleButton(ID_CASE_TOGGLE_BUTTON,
                                        wxT("find_case"));
  case_toggle_button_->SetToolTip(kTrCaseSensitive);

  whole_word_toggle_button_ = NewToggleButton(ID_WHOLE_WORD_TOGGLE_BUTTON,
                                              wxT("find_whole_word"));
  whole_word_toggle_button_->SetToolTip(kTrMatchWholeWord);

  // Initialize toggle button states.
  regex_toggle_button_->set_toggle(GetBit(flags_, kFindUseRegex));
  case_toggle_button_->set_toggle(GetBit(flags_, kFindCaseSensitive));
  whole_word_toggle_button_->set_toggle(GetBit(flags_, kFindMatchWholeWord));

  //------------------------------------

  find_combobox_ = new wxComboBox(this, ID_FIND_COMBOBOX);
  find_combobox_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);

  // Initialize find combobox with find history.
  const std::list<wxString>& find_strings = session_->find_strings();
  {
    std::list<wxString>::const_iterator it = find_strings.begin();
    for (; it != find_strings.end(); ++it) {
      find_combobox_->Append(*it);
    }
  }

  if (!find_combobox_->IsListEmpty()) {
    find_combobox_->Select(0);
  }

  //------------------------------------

  replace_combobox_ = new wxComboBox(this, ID_REPLACE_COMBOBOX);

  // Initialize replace combobox with replace history.
  const std::list<wxString>& replace_strings = session_->replace_strings();
  {
    std::list<wxString>::const_iterator it = replace_strings.begin();
    for (; it != replace_strings.end(); ++it) {
      replace_combobox_->Append(*it);
    }
  }

  if (!replace_combobox_->IsListEmpty()) {
    replace_combobox_->Select(0);
  }

  //------------------------------------

  find_button_ = NewTextButton(ID_FIND_BUTTON, kTrFind);
  find_all_button_ = NewTextButton(ID_FIND_ALL_BUTTON, kTrFindAll);
  replace_button_ = NewTextButton(ID_REPLACE_BUTTON, kTrReplace);
  replace_all_button_ = NewTextButton(ID_REPLACE_ALL_BUTTON, kTrReplaceAll);

  //find_button_->SetDefault();  // Set default for ENTER key.

  //------------------------------------

  if (mode_ == kFindMode) {
    LayoutAsFind();
  } else {
    LayoutAsReplace();
  }

  return true;
}

FindPanel::~FindPanel() {
}

bool FindPanel::Destroy() {
  session_->set_find_flags(flags_);
  return wxPanel::Destroy();
}

void FindPanel::UpdateLayout() {
  if (mode_ == kFindMode) {
    LayoutAsFind();
  } else {
    LayoutAsReplace();
  }
}

void FindPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  wxRect rect = GetClientRect();
  wxRect update_rect = GetUpdateClientRect();

  // Background
  wxRect bg_rect(update_rect.x, 0, update_rect.width, 0);
  bg_rect.y = rect.y + 2;
  bg_rect.height = rect.height - 2;
  wxColour bg_top = theme_->GetColor(BG_TOP);
  wxColour bg_bottom = theme_->GetColor(BG_BOTTOM);
  dc.GradientFillLinear(bg_rect, bg_bottom, bg_top, wxNORTH);

  // Borders
  int border_y = rect.y;
  dc.SetPen(wxPen(theme_->GetColor(BORDER_OUTER)));
  dc.DrawLine(bg_rect.x, border_y, bg_rect.GetRight() + 1, border_y);
  ++border_y;
  dc.SetPen(wxPen(theme_->GetColor(BORDER_INNER)));
  dc.DrawLine(bg_rect.x, border_y, bg_rect.GetRight() + 1, border_y);
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_combobox_->SetValue(find_string);
  AddFindString(find_string);
}

void FindPanel::SetFocus() {
  wxPanel::SetFocus();

  find_combobox_->SetFocus();
  if (!find_combobox_->GetValue().IsEmpty()) {
    find_combobox_->SelectAll();
  }
}

void FindPanel::OnRegexToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindUseRegex, evt.IsChecked());
}

void FindPanel::OnCaseToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindCaseSensitive, evt.IsChecked());
}

void FindPanel::OnWholeWordToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindMatchWholeWord, evt.IsChecked());
}

void FindPanel::OnFind(wxCommandEvent& evt) {
  HandleFind(false);
}

void FindPanel::OnFindAll(wxCommandEvent& evt) {
  HandleFind(true);
}

void FindPanel::OnReplace(wxCommandEvent& evt) {
  HandleReplace(false);
}

void FindPanel::OnReplaceAll(wxCommandEvent& evt) {
  HandleReplace(true);
}

void FindPanel::OnFindText(wxCommandEvent& evt) {
  wxString find_str = find_combobox_->GetValue();
  // Post event even if the find string is empty so that the previous matching
  // results can be cleared.
  PostEvent(kFindTextEvent, find_str, wxEmptyString);
}

void FindPanel::OnFindTextEnter(wxCommandEvent& evt) {
  HandleFind(false);
}

void FindPanel::HandleFind(bool all) {
  wxString find_str = find_combobox_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    int event_type = all ? kFindAllEvent : kFindEvent;
    PostEvent(event_type, find_str, wxEmptyString);
  }
}

void FindPanel::HandleReplace(bool all) {
  wxString find_str = find_combobox_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    wxString replace_str = replace_combobox_->GetValue();
    if (!replace_str.IsEmpty()) {
      AddReplaceString(replace_str);
    }

    int event_type = all ? kReplaceAllEvent : kReplaceEvent;
    PostEvent(kReplaceAllEvent, find_str, replace_str);
  }
}

void FindPanel::AddFindString(const wxString& string) {
  if (session_->AddFindString(string)) {
    // The find string is new. Simply push to find combobox at front.
    find_combobox_->Insert(string, 0);
  } else {
    // Move the find string to front in the find combobox.
    int old_index = find_combobox_->FindString(string, true);
    if (old_index != wxNOT_FOUND) {
      find_combobox_->Delete(old_index);
    }
    find_combobox_->Insert(string, 0);
    find_combobox_->Select(0);
  }
}

void FindPanel::AddReplaceString(const wxString& string) {
  if (session_->AddReplaceString(string)) {
    // The replace string is new. Simply push to replace combobox at front.
    replace_combobox_->Insert(string, 0);
  } else {
    // Move the find string to front in the replace combobox.
    int old_index = replace_combobox_->FindString(string, true);
    if (old_index != wxNOT_FOUND) {
      replace_combobox_->Delete(old_index);
    }
    replace_combobox_->Insert(string, 0);
    replace_combobox_->Select(0);
  }
}

void FindPanel::LayoutAsFind() {
  replace_combobox_->Hide();
  replace_button_->Hide();
  replace_all_button_->Hide();

  int flags = wxALIGN_CENTER_VERTICAL | wxLEFT;

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);

  ctrl_hsizer->Add(regex_toggle_button_, 0, flags, 0);
  ctrl_hsizer->Add(case_toggle_button_, 0, flags, 2);
  ctrl_hsizer->Add(whole_word_toggle_button_, 0, flags, 2);

  ctrl_hsizer->Add(find_combobox_, 1, flags, kPadding);
  ctrl_hsizer->Add(find_button_, 0, flags, kPadding);
  ctrl_hsizer->Add(find_all_button_, 0, flags, kPadding);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->AddSpacer(2);  // Top borders
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
  SetSizer(vsizer);

  Layout();
}

void FindPanel::LayoutAsReplace() {
  replace_combobox_->Show();
  replace_button_->Show();
  replace_all_button_->Show();

  int flags = wxALIGN_CENTER_VERTICAL | wxLEFT;

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);

  wxSizer* flag_vsizer = new wxBoxSizer(wxVERTICAL);

  wxSizer* find_flag_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_flag_hsizer->Add(regex_toggle_button_, 0, flags, 0);
  find_flag_hsizer->Add(case_toggle_button_, 0, flags, 2);
  find_flag_hsizer->Add(whole_word_toggle_button_, 0, flags, 2);

  flag_vsizer->Add(find_flag_hsizer, 0, wxEXPAND);

  wxSizer* find_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_hsizer->Add(find_combobox_, 1, flags, kPadding);
  find_hsizer->Add(find_button_, 0, flags, kPadding);
  find_hsizer->Add(find_all_button_, 0, flags, kPadding);

  wxSizer* replace_hsizer = new wxBoxSizer(wxHORIZONTAL);
  replace_hsizer->Add(replace_combobox_, 1, flags, kPadding);
  replace_hsizer->Add(replace_button_, 0, flags, kPadding);
  replace_hsizer->Add(replace_all_button_, 0, flags, kPadding);

  ctrl_hsizer->Add(flag_vsizer, 0, wxEXPAND);

  wxSizer* body_vsizer = new wxBoxSizer(wxVERTICAL);
  body_vsizer->Add(find_hsizer, 0, wxEXPAND);
  body_vsizer->AddSpacer(kPadding);
  body_vsizer->Add(replace_hsizer, 0, wxEXPAND);

  ctrl_hsizer->Add(body_vsizer, 1, wxEXPAND);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->AddSpacer(2);  // Top borders
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
  SetSizer(vsizer);

  Layout();
}

void FindPanel::InitButtonStyle() {
  button_style_.reset(new ui::ButtonStyle);

  editor::SharedTheme button_theme = theme_->GetTheme(BUTTON);
  if (button_theme.get() == NULL) {
    return;
  }

  for (int part = 0; part < ui::ButtonStyle::PARTS; ++part) {
    editor::SharedTheme part_theme = button_theme->GetTheme(part);
    if (part_theme) {
      for (int state = 0; state < ui::ButtonStyle::STATES; ++state) {
        button_style_->SetColor(part, state, part_theme->GetColor(state));
      }
    }
  }

  button_style_->Fix();
}

ui::BitmapToggleButton* FindPanel::NewToggleButton(int id,
                                                   const wxString& bitmap) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(button_style_);
  button->Create(this, id);
  button->SetBitmap(skin::GetIcon(bitmap));
  button->set_user_best_size(wxSize(24, 24));
  return button;
}

ui::TextButton* FindPanel::NewTextButton(int id, const wxString& label) {
  ui::TextButton* button = new ui::TextButton(button_style_);
  button->Create(this, id, label);
  button->SetMinSize(wxSize(80, -1));
  return button;
}

void FindPanel::PostEvent(int event_type,
                          const wxString& find_str,
                          const wxString& replace_str) {
  FindPanelEvent evt(GetId());
  evt.SetEventObject(this);
  evt.SetInt(event_type);
  evt.set_flags(flags_);
  evt.set_find_str(find_str.ToStdWstring());
  evt.set_replace_str(replace_str.ToStdWstring());
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace jil

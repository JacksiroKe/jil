#ifndef JIL_STATUS_BAR_H_
#define JIL_STATUS_BAR_H_

#include <vector>

#include "wx/panel.h"

#include "editor/theme.h"

class wxTimer;
class wxTimerEvent;

namespace jil {

class StatusBar : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    COLOR_FG = 0,
    COLOR_BORDER_OUTER,
    COLOR_BORDER_INNER,
    COLOR_BG_TOP,
    COLOR_BG_BOTTOM,
    COLOR_SEPARATOR,
    COLORS,
  };

  enum FieldId {
    kField_Cwd = 0,
    kField_TabOptions,
    kField_Encoding,
    kField_FileFormat,
    kField_FileType,
    kField_Caret,
#if JIL_ENABLE_LEADER_KEY
    kField_KeyStroke,
#endif
    kField_Space,

    kField_Count,
  };

  enum SizeType {
    kFit = 0,
    kFixed,
    kPercentage,
    kStretch,
  };

  struct FieldInfo {
    FieldId id;

    wxAlignment align;

    SizeType size_type;

    // For different size types, different meanings of size value:
    // kFit         -> extra padding
    // kFixed       -> fixed size in char
    // kPercentage  -> (% * 100)
    // kStretch     -> stretch factor
    int size_value;

    // Minimal size.
    // Usually specified when size_type is kFit.
    // The meaning is also determined by size type.
    int min_size;

    // Actual size in pixel.
    int size;
  };

public:
  StatusBar();
  virtual ~StatusBar();

  bool Create(wxWindow* parent, wxWindowID id);

  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  virtual bool SetFont(const wxFont& font) override;

  void AddField(FieldId id, wxAlignment align, SizeType size_type, int size_value);

  bool HasField(FieldId id) const;

  void SetFields(const std::vector<FieldInfo>& field_infos);

  void UpdateFieldSizes();

  void SetFieldValue(FieldId id, const wxString& value, bool refresh);

  void ClearFieldValues();

  // Set a message to display in the first field.
  // Examples:
  //   SetMessage(wxT("Some message..."), 1000) -> Show a message and clear it after 1s.
  //   SetMessage(wxT("Some message..."), 0) -> Just show a message.
  //   SetMessage(wxEmptyString, 1000) -> Clear last message after 1s.
  //   SetMessage(wxEmptyString, 0) -> Do nothing.
  void SetMessage(const wxString& msg, int time_ms = 0);

protected:
  virtual wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);

  void OnSize(wxSizeEvent& evt);

  void OnMouseLeftUp(wxMouseEvent& evt);

  void OnMsgTimer(wxTimerEvent& evt);

private:
  void UpdateFontDetermined();

  wxString GetFieldValue(FieldId id);

  // Get field rect according to its size and the client rect.
  // If the field is not found, the rect will be empty.
  wxRect GetFieldRect(FieldId id) const;

  wxRect GetFieldRectByIndex(std::size_t index) const;

  void RefreshFieldById(FieldId id);
  void RefreshFieldByIndex(std::size_t index);

  const FieldInfo* GetFieldByPos(int pos_x) const;

  const FieldInfo* GetFieldById(FieldId id) const;

  void PostFieldClickEvent(const wxPoint& pos);

private:
  editor::SharedTheme theme_;

  std::vector<FieldInfo> field_infos_;
  wxString field_values_[kField_Count];

  wxSize char_size_;
  wxSize padding_;

  // Status message (temporarily) displayed in the first field.
  wxString msg_;
  wxTimer* msg_timer_;
};

////////////////////////////////////////////////////////////////////////////////

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(kEvtStatusFieldClick, 0)
END_DECLARE_EVENT_TYPES()

}  // namespace jil

#define EVT_STATUS_FIELD_CLICK(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::kEvtStatusFieldClick, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_STATUS_BAR_H_

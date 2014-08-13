#include "editor/wrap.h"
#include "wx/log.h"
#include "editor/text_extent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

namespace {

size_t __WrapLine(TextExtent* text_extent,
                  const std::wstring& line,
                  size_t off,
                  size_t begin,
                  size_t end,
                  Coord max_width) {
  if (begin >= end) {
    return std::wstring::npos;
  }

  int m = begin + (end - begin) / 2;
  int width = text_extent->GetWidth(line.substr(off, m));

  if (width == max_width) {
    return m + off;
  }
  if (width < max_width) {
    int m_char_width = text_extent->GetWidth(std::wstring(1, line[m]));
    if (max_width - width < m_char_width) {
      // The space left not enough for an extra char.
      return m + off;
    } else {
    return __WrapLine(text_extent, line, off, m + 1, end, max_width);
    }
  } else {
    return __WrapLine(text_extent, line, off, begin, m, max_width);
  }
}

} // namespace

WrapOffsets WrapLineByChar(const std::wstring& line,
                           TextExtent* text_extent,
                           Coord max_width) {
  WrapOffsets offsets;
  size_t wrap = 0;
  while (true) {
    wrap = __WrapLine(text_extent,
                      line,
                      wrap,
                      0,
                      line.length() - wrap,
                      max_width);
    if (wrap == std::wstring::npos) {
      break;
    }
    offsets.push_back(CoordCast(wrap));
  }
  return offsets;
}

bool WrapInfo::Wrap(const std::wstring& line,
                    TextExtent* text_extent,
                    Coord max_width,
                    int* delta) {
  WrapOffsets new_offsets = WrapLineByChar(line, text_extent, max_width);

  *delta = static_cast<int>(new_offsets.size()) -
           static_cast<int>(offsets_.size());

  if (delta != 0 || new_offsets != offsets_) {
    offsets_.swap(new_offsets);
    return true;
  }

  return false;
}

Coord WrapInfo::Unwrap() {
  Coord wrap_count = WrapCount();
  offsets_.clear();
  return wrap_count;
}

std::vector<CharRange> WrapInfo::GetSubRanges() const {
  std::vector<CharRange> sub_ranges;
  sub_ranges.resize(offsets_.size() + 1);

  size_t off = 0;
  size_t i = 0;
  for (; i < offsets_.size(); ++i) {
    sub_ranges[i].Set(off, offsets_[i]);
    off = offsets_[i];
  }

  sub_ranges[i].Set(off, kInvalidCoord);

  return sub_ranges;
}

Coord WrapInfo::GetSubLineNr(Coord x, Coord* sub_off) const {
  Coord sub_ln = 1;

  for (size_t i = 0; i < offsets_.size(); ++i) {
    if (x <= offsets_[i]) {
      break;
    }

    if (sub_off != NULL) {
      *sub_off = offsets_[i];
    }

    ++sub_ln;
  }

  return sub_ln;
}

////////////////////////////////////////////////////////////////////////////////

WrapHelper::WrapHelper(TextBuffer* buffer, TextExtent* text_extent)
    : buffer_(buffer)
    , text_extent_(text_extent)
    , client_width_(0)
    , wrapped_line_count_(0) {
  // Init with the existing lines.
  wrap_infos_.resize(buffer->LineCount());
}

WrapHelper::~WrapHelper() {
}

int WrapHelper::AddLineWrap(Coord ln) {
  assert(ln >= 1 && ln <= buffer_->LineCount());

  const std::wstring& line_data = buffer_->LineData(ln);

  WrapInfo wrap_info;

  int delta = 0;
  if (client_width_ > 0) {
    wrap_info.Wrap(line_data, text_extent_, client_width_, &delta);
  }

  WrapInfos::iterator insert_pos(wrap_infos_.begin());
  std::advance(insert_pos, ln - 1);
  wrap_infos_.insert(insert_pos, wrap_info);

  wrapped_line_count_ += wrap_info.WrapCount() + 1;

  return delta;
}

void WrapHelper::RemoveLineWrap(Coord ln) {
  assert(ln >= 1 && ln <= CoordCast(wrap_infos_.size()));

  WrapInfos::iterator erase_pos(wrap_infos_.begin());
  std::advance(erase_pos, ln - 1);

  wrapped_line_count_ -= erase_pos->WrapCount() + 1;

  wrap_infos_.erase(erase_pos);
}

int WrapHelper::UpdateLineWrap(Coord ln) {
  assert(ln >= 1 && ln <= buffer_->LineCount());

  if (client_width_ <= 0) {
    return 0;
  }

  const std::wstring& line_data = buffer_->LineData(ln);
  int delta = 0;
  wrap_infos_[ln - 1].Wrap(line_data, text_extent_, client_width_, &delta);

  wrapped_line_count_ += delta;

  return delta;
}

bool WrapHelper::Wrap(int* wrap_delta) {
  *wrap_delta = 0;
  bool wrap_changed = false;

  if (client_width_ <= 0) {
    return wrap_changed;
  }

  const Coord line_count = buffer_->LineCount();
  assert(line_count == CoordCast(wrap_infos_.size()));

  wrapped_line_count_ = 0;

  for (Coord ln = 1; ln <= line_count; ++ln) {
    int line_wrap_delta = 0;
    WrapInfo& wrap_info = wrap_infos_[ln - 1];
    if (wrap_info.Wrap(buffer_->LineData(ln),
                       text_extent_,
                       client_width_,
                       &line_wrap_delta)) {
      wrap_changed = true;
      *wrap_delta += line_wrap_delta;
    }

    wrapped_line_count_ += wrap_info.WrapCount() + 1;
  }

  return wrap_changed;
}

bool WrapHelper::Unwrap(int* wrap_delta) {
  const Coord line_count = buffer_->LineCount();
  assert(line_count == CoordCast(wrap_infos_.size()));

  *wrap_delta = 0;

  for (Coord ln = 1; ln <= line_count; ++ln) {
    *wrap_delta -= wrap_infos_[ln - 1].Unwrap();
  }

  wrapped_line_count_ = 0;

  return *wrap_delta != 0;
}

Coord WrapHelper::WrapLineNr(Coord ln) const {
  Coord wrapped_ln = 0;

  const Coord line_count = buffer_->LineCount();

  if (ln > line_count) {
    assert(false && "Invalid unwrapped ln!");
    return wrapped_ln;
  }

  for (Coord unwrapped_ln = 1; unwrapped_ln < ln; ++unwrapped_ln) {
    wrapped_ln += WrappedLineCount(unwrapped_ln);
  }

  return wrapped_ln + 1;
}

Coord WrapHelper::UnwrapLineNr(Coord wrapped_ln, Coord* sub_ln) const {
  const Coord line_count = buffer_->LineCount();

  for (Coord ln = 1; ln <= line_count && wrapped_ln > 0; ++ln) {
    int wlc = WrappedLineCount(ln);
    if (wrapped_ln <= wlc) {
      *sub_ln = wrapped_ln;
      return ln;
    }
    wrapped_ln -= wlc;
  }

  assert(false && "Invalid wrapped ln!");
  return 0;
}

LineRange WrapHelper::WrapLineRange(const LineRange& line_range) const {
  LineRange wrapped_line_range;

  const Coord line_count = buffer_->LineCount();
  assert(line_count == CoordCast(wrap_infos_.size()));

  if (line_range.first() > line_count) {
    assert(false && "Invalid line range!");
    return wrapped_line_range;
  }

  Coord wrapped_ln = 0;
  Coord ln = 1;
  for (; ln < line_range.first(); ++ln) {
    wrapped_ln += WrappedLineCount(ln);
  }

  // Now we have (ln == line_range.first()).
  wrapped_line_range.set_first(wrapped_ln + 1);
  wrapped_ln += WrappedLineCount(ln);

  // Go to next line.
  ++ln;

  for (; ln <= line_range.last() && ln <= line_count; ++ln) {
    wrapped_ln += WrappedLineCount(ln);
  }
  wrapped_line_range.set_last(wrapped_ln);

  return wrapped_line_range;
}

LineRange WrapHelper::UnwrapLineRange(
    const LineRange& wrapped_line_range) const {
  LineRange line_range;

  const Coord line_count = buffer_->LineCount();
  Coord wrapped_ln = 0;
  Coord ln = 1;

  for (; ln <= line_count; ++ln) {
    wrapped_ln += WrappedLineCount(ln);
    if (wrapped_ln >= wrapped_line_range.first()) {
      line_range.set_first(ln);
      break;
    }
  }

  if (wrapped_ln >= wrapped_line_range.last()) {
    line_range.set_last(ln);
    return line_range;
  }

  for (++ln; ln <= line_count; ++ln) {
    wrapped_ln += WrappedLineCount(ln);
    if (wrapped_ln >= wrapped_line_range.last()) {
      line_range.set_last(ln);
      break;
    }
  }

  if (line_range.last() == 0 && line_range.first() != 0) {
    line_range.set_last(line_count);
  }

  return line_range;
}

}  // namespace editor
}  // namespace jil

#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

bool IsContinuationByte(unsigned char value) {
    return (value & 0xC0U) == 0x80U;
}

std::size_t FindPreviousCodepointStart(const std::string& text, std::size_t byteIndex) {
    std::size_t cursor = std::min(byteIndex, text.size());
    if (cursor == 0) {
        return 0;
    }

    --cursor;
    while (cursor > 0 && IsContinuationByte(static_cast<unsigned char>(text[cursor]))) {
        --cursor;
    }
    return cursor;
}

std::size_t FindNextCodepointStart(const std::string& text, std::size_t byteIndex) {
    std::size_t cursor = std::min(byteIndex, text.size());
    if (cursor >= text.size()) {
        return text.size();
    }

    ++cursor;
    while (cursor < text.size() && IsContinuationByte(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
    return cursor;
}

std::string EncodeCodepointUtf8(unsigned int codepoint) {
    if (codepoint > 0x10FFFFU) {
        return {};
    }

    std::string utf8;
    if (codepoint <= 0x7FU) {
        utf8.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFU) {
        utf8.push_back(static_cast<char>(0xC0U | ((codepoint >> 6U) & 0x1FU)));
        utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else if (codepoint <= 0xFFFFU) {
        utf8.push_back(static_cast<char>(0xE0U | ((codepoint >> 12U) & 0x0FU)));
        utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else {
        utf8.push_back(static_cast<char>(0xF0U | ((codepoint >> 18U) & 0x07U)));
        utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        utf8.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        utf8.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }

    return utf8;
}

FVector2 MeasureTextWithFont(const std::string& text, float fontSize) {
    if (text.empty()) {
        return FVector2(0.0f, fontSize);
    }

    if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr) {
        return FVector2(static_cast<float>(text.size()) * fontSize * 0.55f, fontSize);
    }

    ImFont* font = ImGui::GetFont();
    const ImVec2 size = font->CalcTextSizeA(
        fontSize,
        FLT_MAX,
        0.0f,
        text.c_str(),
        text.c_str() + text.size());
    return FVector2(size.x, size.y);
}

} // namespace

ImEditableText::ImEditableText()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

void ImEditableText::SetText(const std::string& text) {
    if (m_Text == text) {
        return;
    }

    m_Text = text;
    m_CursorByteIndex = ClampByteIndex(m_CursorByteIndex);
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::SetHintText(const std::string& hintText) {
    if (m_HintText == hintText) {
        return;
    }

    m_HintText = hintText;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImEditableText::SetStyle(const FEditableTextStyle& style) {
    m_Style = style;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImEditableText::SetDisabled(bool bDisabled) {
    if (m_bDisabled == bDisabled) {
        return;
    }

    m_bDisabled = bDisabled;
    Invalidate(EInvalidateReason::Paint);
}

void ImEditableText::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    FColor background = m_Style.BackgroundColor;
    if (m_bDisabled) {
        background = m_Style.DisabledBackgroundColor;
    } else if (HasKeyboardFocus()) {
        background = m_Style.FocusedBackgroundColor;
    } else if (m_bHovered) {
        background = m_Style.HoveredBackgroundColor;
    }

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.Position,
        m_Geometry.Position + m_Geometry.Size,
        background,
        m_Style.CornerRadius
    );

    paintContext.DrawContext_.DrawRect(
        m_Geometry.Position,
        m_Geometry.Position + m_Geometry.Size,
        HasKeyboardFocus() ? m_Style.FocusedOutlineColor : m_Style.BorderColor,
        m_Style.CornerRadius,
        m_Style.BorderThickness
    );

    const FVector2 innerMin(
        m_Geometry.Position.X + m_Style.Padding.Left,
        m_Geometry.Position.Y + m_Style.Padding.Top
    );
    const FVector2 innerMax(
        m_Geometry.Position.X + m_Geometry.Size.X - m_Style.Padding.Right,
        m_Geometry.Position.Y + m_Geometry.Size.Y - m_Style.Padding.Bottom
    );

    paintContext.DrawContext_.PushClipRect(innerMin, innerMax, true);

    const std::string& displayText = m_Text.empty() ? m_HintText : m_Text;
    const FColor textColor = m_Text.empty()
        ? m_Style.HintTextColor
        : (m_bDisabled ? m_Style.DisabledTextColor : m_Style.TextColor);
    const FVector2 textSize = MeasureText(displayText);
    const FVector2 textPosition(
        innerMin.X - m_HorizontalScrollOffset,
        innerMin.Y + std::max(0.0f, (innerMax.Y - innerMin.Y - textSize.Y) * 0.5f));

    if (!displayText.empty()) {
        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
            paintContext.DrawContext_.GetImDrawList()->AddText(
                ImGui::GetFont(),
                m_Style.FontSize,
                textPosition.ToImVec2(),
                textColor.ToImU32(),
                displayText.c_str()
            );
        } else {
            paintContext.DrawContext_.DrawText(
                textPosition,
                textColor,
                displayText,
                m_Style.FontSize
            );
        }
    }

    if (HasKeyboardFocus() && !m_bDisabled) {
        const float caretX = innerMin.X + MeasureCaretX(m_CursorByteIndex) - m_HorizontalScrollOffset;
        const FVector2 caretMin(caretX, innerMin.Y + 1.0f);
        const FVector2 caretMax(caretX, innerMax.Y - 1.0f);
        paintContext.DrawContext_.DrawLine(caretMin, caretMax, m_Style.CaretColor, 1.5f);
    }

    paintContext.DrawContext_.PopClipRect();
}

FVector2 ImEditableText::GetMinSize() const {
    const FVector2 textSize = MeasureText(m_Text.empty() ? m_HintText : m_Text);
    const float width = m_Style.Padding.Left + textSize.X + m_Style.Padding.Right;
    const float height = m_Style.Padding.Top + std::max(textSize.Y, m_Style.FontSize) + m_Style.Padding.Bottom;

    return FVector2(
        std::max(width, m_Style.MinDesiredSize.X),
        std::max(height, m_Style.MinDesiredSize.Y)
    );
}

FReply ImEditableText::OnInputEvent(const FInputEvent& event) {
    if (m_bDisabled) {
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseEnter) {
        SetHovered(true);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        SetHovered(false);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {
        SetCursorByteIndex(ResolveCursorByteIndexAt(event.MousePosition));
        return FReply::Handled().SetKeyboardFocus(shared_from_this());
    }

    if (!HasKeyboardFocus()) {
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::TextInput) {
        if (event.Codepoint >= 32U && event.Codepoint != '\r' && event.Codepoint != '\n') {
            InsertCodepoint(event.Codepoint);
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

    if (event.Type != EInputEventType::KeyDown) {
        return FReply::Unhandled();
    }

    switch (event.Key) {
    case EKey::Left:
        MoveCursorLeft();
        return FReply::Handled();
    case EKey::Right:
        MoveCursorRight();
        return FReply::Handled();
    case EKey::Home:
        MoveCursorToStart();
        return FReply::Handled();
    case EKey::End:
        MoveCursorToEnd();
        return FReply::Handled();
    case EKey::Backspace:
        DeletePreviousCodepoint();
        return FReply::Handled();
    case EKey::DeleteKey:
        DeleteNextCodepoint();
        return FReply::Handled();
    case EKey::Enter:
        CommitText();
        return FReply::Handled();
    default:
        break;
    }

    return FReply::Unhandled();
}

void ImEditableText::OnFocusChanged(bool bHasFocus) {
    ImWidget::OnFocusChanged(bHasFocus);

    if (bHasFocus) {
        EnsureCursorVisible();
        return;
    }

    if (m_bTextDirty) {
        CommitText();
    }
}

std::size_t ImEditableText::ClampByteIndex(std::size_t byteIndex) const {
    return std::min(byteIndex, m_Text.size());
}

std::size_t ImEditableText::ResolveCursorByteIndexAt(const FVector2& mousePosition) const {
    const float localX =
        mousePosition.X - (m_Geometry.Position.X + m_Style.Padding.Left) + m_HorizontalScrollOffset;
    if (localX <= 0.0f || m_Text.empty()) {
        return 0;
    }

    std::size_t current = 0;
    while (current < m_Text.size()) {
        const std::size_t next = FindNextCodepointStart(m_Text, current);
        const float currentX = MeasureCaretX(current);
        const float nextX = MeasureCaretX(next);
        if (localX < (currentX + nextX) * 0.5f) {
            return current;
        }
        current = next;
    }

    return m_Text.size();
}

void ImEditableText::SetCursorByteIndex(std::size_t byteIndex) {
    const std::size_t clampedIndex = ClampByteIndex(byteIndex);
    if (m_CursorByteIndex == clampedIndex) {
        EnsureCursorVisible();
        return;
    }

    m_CursorByteIndex = clampedIndex;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Paint);
}

void ImEditableText::EnsureCursorVisible() {
    const float visibleWidth = std::max(0.0f, m_Geometry.Size.X - m_Style.Padding.Left - m_Style.Padding.Right);
    if (visibleWidth <= 0.0f) {
        m_HorizontalScrollOffset = 0.0f;
        return;
    }

    const float textWidth = MeasureCaretX(m_Text.size());
    const float caretX = MeasureCaretX(m_CursorByteIndex);

    if (caretX < m_HorizontalScrollOffset) {
        m_HorizontalScrollOffset = caretX;
    } else if (caretX > m_HorizontalScrollOffset + visibleWidth) {
        m_HorizontalScrollOffset = caretX - visibleWidth;
    }

    const float maxScroll = std::max(0.0f, textWidth - visibleWidth);
    m_HorizontalScrollOffset = std::clamp(m_HorizontalScrollOffset, 0.0f, maxScroll);
}

void ImEditableText::NotifyTextChanged() {
    m_bTextDirty = true;
    OnTextChanged.Broadcast(*this, m_Text);
}

void ImEditableText::CommitText() {
    m_bTextDirty = false;
    OnTextCommitted.Broadcast(*this, m_Text);
}

void ImEditableText::InsertCodepoint(unsigned int codepoint) {
    const std::string utf8 = EncodeCodepointUtf8(codepoint);
    if (utf8.empty()) {
        return;
    }

    InsertText(utf8);
}

void ImEditableText::InsertText(const std::string& text) {
    if (text.empty()) {
        return;
    }

    m_Text.insert(m_CursorByteIndex, text);
    m_CursorByteIndex += text.size();
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::DeletePreviousCodepoint() {
    if (m_CursorByteIndex == 0 || m_Text.empty()) {
        return;
    }

    const std::size_t previous = FindPreviousCodepointStart(m_Text, m_CursorByteIndex);
    m_Text.erase(previous, m_CursorByteIndex - previous);
    m_CursorByteIndex = previous;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::DeleteNextCodepoint() {
    if (m_CursorByteIndex >= m_Text.size() || m_Text.empty()) {
        return;
    }

    const std::size_t next = FindNextCodepointStart(m_Text, m_CursorByteIndex);
    m_Text.erase(m_CursorByteIndex, next - m_CursorByteIndex);
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::MoveCursorLeft() {
    SetCursorByteIndex(FindPreviousCodepointStart(m_Text, m_CursorByteIndex));
}

void ImEditableText::MoveCursorRight() {
    SetCursorByteIndex(FindNextCodepointStart(m_Text, m_CursorByteIndex));
}

void ImEditableText::MoveCursorToStart() {
    SetCursorByteIndex(0);
}

void ImEditableText::MoveCursorToEnd() {
    SetCursorByteIndex(m_Text.size());
}

float ImEditableText::MeasureCaretX(std::size_t byteIndex) const {
    return MeasureText(m_Text.substr(0, ClampByteIndex(byteIndex))).X;
}

FVector2 ImEditableText::MeasureText(const std::string& text) const {
    return MeasureTextWithFont(text, m_Style.FontSize);
}

void ImEditableText::SetHovered(bool bHovered) {
    if (m_bHovered == bHovered) {
        return;
    }

    m_bHovered = bHovered;
    Invalidate(EInvalidateReason::Paint);
}

} // namespace ImWidgetV4

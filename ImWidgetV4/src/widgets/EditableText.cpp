#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

std::string GFallbackClipboardText;

float ResolveContentInset(float borderThickness)
{
    return std::max(0.0f, borderThickness);
}

bool IsContinuationByte(unsigned char value) {
    return (value & 0xC0U) == 0x80U;
}

bool IsAsciiWordCharacter(unsigned char value) {
    return (value >= '0' && value <= '9') ||
           (value >= 'A' && value <= 'Z') ||
           (value >= 'a' && value <= 'z') ||
           value == '_';
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

bool IsWordCharacterAt(const std::string& text, std::size_t byteIndex) {
    if (byteIndex >= text.size()) {
        return false;
    }

    const unsigned char value = static_cast<unsigned char>(text[byteIndex]);
    return value >= 0x80U || IsAsciiWordCharacter(value);
}

std::size_t FindPreviousWordBoundary(const std::string& text, std::size_t byteIndex) {
    std::size_t cursor = std::min(byteIndex, text.size());

    while (cursor > 0) {
        const std::size_t previous = FindPreviousCodepointStart(text, cursor);
        if (IsWordCharacterAt(text, previous)) {
            break;
        }
        cursor = previous;
    }

    while (cursor > 0) {
        const std::size_t previous = FindPreviousCodepointStart(text, cursor);
        if (!IsWordCharacterAt(text, previous)) {
            break;
        }
        cursor = previous;
    }

    return cursor;
}

std::size_t FindNextWordBoundary(const std::string& text, std::size_t byteIndex) {
    std::size_t cursor = std::min(byteIndex, text.size());

    if (cursor < text.size() && IsWordCharacterAt(text, cursor)) {
        while (cursor < text.size() && IsWordCharacterAt(text, cursor)) {
            cursor = FindNextCodepointStart(text, cursor);
        }
    }

    while (cursor < text.size() && !IsWordCharacterAt(text, cursor)) {
        cursor = FindNextCodepointStart(text, cursor);
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

const char* GetClipboardTextSafe() {
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.GetClipboardTextFn != nullptr) {
            const char* text = io.GetClipboardTextFn(io.ClipboardUserData);
            return text != nullptr ? text : "";
        }
    }

    return GFallbackClipboardText.c_str();
}

void SetClipboardTextSafe(const std::string& text) {
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.SetClipboardTextFn != nullptr) {
            io.SetClipboardTextFn(io.ClipboardUserData, text.c_str());
        }
    }

    GFallbackClipboardText = text;
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
    m_SelectionAnchorByteIndex = m_CursorByteIndex;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::SetHintText(const std::string& hintText) {
    if (m_HintText == hintText) {
        return;
    }

    m_HintText = hintText;
    m_HintTextValue = FText::FromString(hintText);
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImEditableText::SetHintText(const FText& hintText) {
    if (m_HintTextValue == hintText) {
        return;
    }

    m_HintTextValue = hintText;
    m_HintText = hintText.IsLocalized()
        ? (hintText.GetDefaultText().empty() ? hintText.GetKey() : hintText.GetDefaultText())
        : hintText.GetInvariantText();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImEditableText::SetStyle(const FEditableTextStyle& style) {
    m_Style = style;
    m_bHasExplicitStyle = true;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImEditableText::SetDisabled(bool bDisabled) {
    if (m_bDisabled == bDisabled) {
        return;
    }

    m_bDisabled = bDisabled;
    if (m_bDisabled) {
        m_bDraggingSelection = false;
    }
    Invalidate(EInvalidateReason::Paint);
}

std::size_t ImEditableText::GetSelectionStartByteIndex() const {
    return std::min(m_SelectionAnchorByteIndex, m_CursorByteIndex);
}

std::size_t ImEditableText::GetSelectionEndByteIndex() const {
    return std::max(m_SelectionAnchorByteIndex, m_CursorByteIndex);
}

void ImEditableText::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    const FEditableTextStyle& style = GetEffectiveStyle();
    FColor background = style.BackgroundColor;
    if (m_bDisabled) {
        background = style.DisabledBackgroundColor;
    } else if (HasKeyboardFocus()) {
        background = style.FocusedBackgroundColor;
    } else if (m_bHovered) {
        background = style.HoveredBackgroundColor;
    }

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.Position,
        m_Geometry.Position + m_Geometry.Size,
        background,
        style.CornerRadius
    );

    paintContext.DrawContext_.DrawRect(
        m_Geometry.Position,
        m_Geometry.Position + m_Geometry.Size,
        HasKeyboardFocus() ? style.FocusedOutlineColor : style.BorderColor,
        style.CornerRadius,
        style.BorderThickness
    );

    const float borderInset = ResolveContentInset(style.BorderThickness);
    const FVector2 innerMin(
        m_Geometry.Position.X + borderInset + style.Padding.Left,
        m_Geometry.Position.Y + borderInset + style.Padding.Top
    );
    const FVector2 innerMax(
        m_Geometry.Position.X + m_Geometry.Size.X - borderInset - style.Padding.Right,
        m_Geometry.Position.Y + m_Geometry.Size.Y - borderInset - style.Padding.Bottom
    );

    paintContext.DrawContext_.PushClipRect(innerMin, innerMax, true);

    const bool bShowingHint = m_Text.empty();
    const std::string resolvedHintText = bShowingHint ? ResolveHintText() : std::string();
    const std::string& displayText = bShowingHint ? resolvedHintText : m_Text;
    const FColor baseTextColor = bShowingHint
        ? style.HintTextColor
        : (m_bDisabled ? style.DisabledTextColor : style.TextColor);
    const FVector2 textSize = MeasureText(displayText);
    const float textBaseY = innerMin.Y + std::max(0.0f, (innerMax.Y - innerMin.Y - textSize.Y) * 0.5f);
    const float textBaseX = innerMin.X - m_HorizontalScrollOffset;

    if (!displayText.empty()) {
        const auto drawText = [&](const FColor& color) {
            if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
                paintContext.DrawContext_.GetImDrawList()->AddText(
                    ImGui::GetFont(),
                    style.FontSize,
                    ImVec2(textBaseX, textBaseY),
                    color.ToImU32(),
                    displayText.c_str()
                );
            } else {
                paintContext.DrawContext_.DrawText(
                    FVector2(textBaseX, textBaseY),
                    color,
                    displayText,
                    style.FontSize
                );
            }
        };
        const auto drawTextClipped = [&](const FVector2& clipMin, const FVector2& clipMax, const FColor& color) {
            if (clipMax.X <= clipMin.X || clipMax.Y <= clipMin.Y) {
                return;
            }

            paintContext.DrawContext_.PushClipRect(clipMin, clipMax, true);
            drawText(color);
            paintContext.DrawContext_.PopClipRect();
        };

        if (!bShowingHint && HasSelection()) {
            const std::size_t selectionStart = GetSelectionStartByteIndex();
            const std::size_t selectionEnd = GetSelectionEndByteIndex();
            const float beforeWidth = MeasureText(m_Text.substr(0, selectionStart)).X;
            const float selectedWidth = MeasureText(m_Text.substr(selectionStart, selectionEnd - selectionStart)).X;
            const FVector2 selectionTextClipMin(
                textBaseX + beforeWidth,
                innerMin.Y
            );
            const FVector2 selectionTextClipMax(
                selectionTextClipMin.X + selectedWidth,
                innerMax.Y
            );
            const FVector2 selectionBackgroundMin(
                selectionTextClipMin.X,
                innerMin.Y + 1.0f
            );
            const FVector2 selectionBackgroundMax(
                selectionTextClipMax.X,
                innerMax.Y - 1.0f
            );

            paintContext.DrawContext_.DrawRectFilled(
                selectionBackgroundMin,
                selectionBackgroundMax,
                style.SelectionBackgroundColor,
                3.0f
            );

            drawTextClipped(innerMin, FVector2(selectionTextClipMin.X, innerMax.Y), baseTextColor);
            drawTextClipped(selectionTextClipMin, selectionTextClipMax, style.SelectedTextColor);
            drawTextClipped(FVector2(selectionTextClipMax.X, innerMin.Y), innerMax, baseTextColor);
        } else {
            drawText(baseTextColor);
        }
    }

    if (HasKeyboardFocus() && !m_bDisabled) {
        const float caretX = innerMin.X + MeasureCaretX(m_CursorByteIndex) - m_HorizontalScrollOffset;
        const FVector2 caretMin(caretX, innerMin.Y + 1.0f);
        const FVector2 caretMax(caretX, innerMax.Y - 1.0f);
        paintContext.DrawContext_.DrawLine(caretMin, caretMax, style.CaretColor, 1.5f);
    }

    paintContext.DrawContext_.PopClipRect();
}

FVector2 ImEditableText::GetMinSize() const {
    const FEditableTextStyle& style = GetEffectiveStyle();
    const FVector2 textSize = MeasureText(m_Text.empty() ? ResolveHintText() : m_Text);
    const float borderInset = ResolveContentInset(style.BorderThickness);
    const float width = borderInset * 2.0f + style.Padding.Left + textSize.X + style.Padding.Right;
    const float height = borderInset * 2.0f + style.Padding.Top + std::max(textSize.Y, style.FontSize) + style.Padding.Bottom;

    return FVector2(
        std::max(width, style.MinDesiredSize.X),
        std::max(height, style.MinDesiredSize.Y)
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
        SetCursorByteIndex(
            ResolveCursorByteIndexAt(event.MousePosition),
            event.Modifiers.bShift
        );
        SetDraggingSelection(true);
        return FReply::Handled()
            .SetKeyboardFocus(shared_from_this())
            .CaptureMouse(shared_from_this(), EMouseButton::Left);
    }

    if (event.Type == EInputEventType::MouseMove && m_bDraggingSelection) {
        SetCursorByteIndex(ResolveCursorByteIndexAt(event.MousePosition), true);
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_bDraggingSelection) {
        SetCursorByteIndex(ResolveCursorByteIndexAt(event.MousePosition), true);
        SetDraggingSelection(false);
        return FReply::Handled().ReleaseMouseCapture();
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

    if (event.Modifiers.bCtrl) {
        switch (event.Key) {
        case EKey::A:
            SelectAll();
            return FReply::Handled();
        case EKey::C:
            CopySelectionToClipboard();
            return FReply::Handled();
        case EKey::X:
            CutSelectionToClipboard();
            return FReply::Handled();
        case EKey::V:
            PasteFromClipboard();
            return FReply::Handled();
        case EKey::Left:
            MoveCursorWordLeft(event.Modifiers.bShift);
            return FReply::Handled();
        case EKey::Right:
            MoveCursorWordRight(event.Modifiers.bShift);
            return FReply::Handled();
        case EKey::Backspace:
            DeletePreviousWord();
            return FReply::Handled();
        case EKey::DeleteKey:
            DeleteNextWord();
            return FReply::Handled();
        default:
            break;
        }
    }

    switch (event.Key) {
    case EKey::Left:
        MoveCursorLeft(event.Modifiers.bShift);
        return FReply::Handled();
    case EKey::Right:
        MoveCursorRight(event.Modifiers.bShift);
        return FReply::Handled();
    case EKey::Home:
        MoveCursorToStart(event.Modifiers.bShift);
        return FReply::Handled();
    case EKey::End:
        MoveCursorToEnd(event.Modifiers.bShift);
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

    SetDraggingSelection(false);
    if (m_bTextDirty) {
        CommitText();
    }
}

std::size_t ImEditableText::ClampByteIndex(std::size_t byteIndex) const {
    return std::min(byteIndex, m_Text.size());
}

std::size_t ImEditableText::ResolveCursorByteIndexAt(const FVector2& mousePosition) const {
    const FEditableTextStyle& style = GetEffectiveStyle();
    const float borderInset = ResolveContentInset(style.BorderThickness);
    const float localX =
        mousePosition.X - (m_Geometry.Position.X + borderInset + style.Padding.Left) + m_HorizontalScrollOffset;
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

void ImEditableText::SetCursorByteIndex(std::size_t byteIndex, bool bExtendSelection) {
    const std::size_t clampedIndex = ClampByteIndex(byteIndex);

    if (!bExtendSelection) {
        m_SelectionAnchorByteIndex = clampedIndex;
    }

    if (m_CursorByteIndex == clampedIndex && (!bExtendSelection || m_SelectionAnchorByteIndex == clampedIndex)) {
        EnsureCursorVisible();
        return;
    }

    m_CursorByteIndex = clampedIndex;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Paint);
}

void ImEditableText::EnsureCursorVisible() {
    const FEditableTextStyle& style = GetEffectiveStyle();
    const float borderInset = ResolveContentInset(style.BorderThickness);
    const float visibleWidth = std::max(
        0.0f,
        m_Geometry.Size.X - borderInset * 2.0f - style.Padding.Left - style.Padding.Right);
    if (visibleWidth <= 0.0f) {
        m_HorizontalScrollOffset = 0.0f;
        return;
    }

    const float textWidth = MeasureCaretX(m_Text.size());
    const float caretX = MeasureCaretX(m_CursorByteIndex);
    const float selectionAnchorX = MeasureCaretX(m_SelectionAnchorByteIndex);
    const float visibleMinX = std::min(caretX, selectionAnchorX);
    const float visibleMaxX = std::max(caretX, selectionAnchorX);

    if (visibleMinX < m_HorizontalScrollOffset) {
        m_HorizontalScrollOffset = visibleMinX;
    } else if (visibleMaxX > m_HorizontalScrollOffset + visibleWidth) {
        m_HorizontalScrollOffset = visibleMaxX - visibleWidth;
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
    const std::shared_ptr<ImWidget> keepAlive = weak_from_this().lock();
    (void)keepAlive;
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

    if (HasSelection()) {
        DeleteSelection();
    }

    m_Text.insert(m_CursorByteIndex, text);
    m_CursorByteIndex += text.size();
    m_SelectionAnchorByteIndex = m_CursorByteIndex;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::DeleteSelection() {
    if (!HasSelection()) {
        return;
    }

    const std::size_t selectionStart = GetSelectionStartByteIndex();
    const std::size_t selectionEnd = GetSelectionEndByteIndex();
    m_Text.erase(selectionStart, selectionEnd - selectionStart);
    m_CursorByteIndex = selectionStart;
    m_SelectionAnchorByteIndex = selectionStart;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImEditableText::DeletePreviousCodepoint() {
    if (HasSelection()) {
        DeleteSelection();
        NotifyTextChanged();
        return;
    }

    if (m_CursorByteIndex == 0 || m_Text.empty()) {
        return;
    }

    const std::size_t previous = FindPreviousCodepointStart(m_Text, m_CursorByteIndex);
    m_Text.erase(previous, m_CursorByteIndex - previous);
    m_CursorByteIndex = previous;
    m_SelectionAnchorByteIndex = previous;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::DeleteNextCodepoint() {
    if (HasSelection()) {
        DeleteSelection();
        NotifyTextChanged();
        return;
    }

    if (m_CursorByteIndex >= m_Text.size() || m_Text.empty()) {
        return;
    }

    const std::size_t next = FindNextCodepointStart(m_Text, m_CursorByteIndex);
    m_Text.erase(m_CursorByteIndex, next - m_CursorByteIndex);
    m_SelectionAnchorByteIndex = m_CursorByteIndex;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::DeletePreviousWord() {
    if (HasSelection()) {
        DeleteSelection();
        NotifyTextChanged();
        return;
    }

    if (m_CursorByteIndex == 0 || m_Text.empty()) {
        return;
    }

    const std::size_t previous = FindPreviousWordBoundary(m_Text, m_CursorByteIndex);
    m_Text.erase(previous, m_CursorByteIndex - previous);
    m_CursorByteIndex = previous;
    m_SelectionAnchorByteIndex = previous;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::DeleteNextWord() {
    if (HasSelection()) {
        DeleteSelection();
        NotifyTextChanged();
        return;
    }

    if (m_CursorByteIndex >= m_Text.size() || m_Text.empty()) {
        return;
    }

    const std::size_t next = FindNextWordBoundary(m_Text, m_CursorByteIndex);
    m_Text.erase(m_CursorByteIndex, next - m_CursorByteIndex);
    m_SelectionAnchorByteIndex = m_CursorByteIndex;
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    NotifyTextChanged();
}

void ImEditableText::MoveCursorLeft(bool bExtendSelection) {
    SetCursorByteIndex(FindPreviousCodepointStart(m_Text, m_CursorByteIndex), bExtendSelection);
}

void ImEditableText::MoveCursorRight(bool bExtendSelection) {
    SetCursorByteIndex(FindNextCodepointStart(m_Text, m_CursorByteIndex), bExtendSelection);
}

void ImEditableText::MoveCursorWordLeft(bool bExtendSelection) {
    SetCursorByteIndex(FindPreviousWordBoundary(m_Text, m_CursorByteIndex), bExtendSelection);
}

void ImEditableText::MoveCursorWordRight(bool bExtendSelection) {
    SetCursorByteIndex(FindNextWordBoundary(m_Text, m_CursorByteIndex), bExtendSelection);
}

void ImEditableText::MoveCursorToStart(bool bExtendSelection) {
    SetCursorByteIndex(0, bExtendSelection);
}

void ImEditableText::MoveCursorToEnd(bool bExtendSelection) {
    SetCursorByteIndex(m_Text.size(), bExtendSelection);
}

void ImEditableText::SelectAll() {
    m_SelectionAnchorByteIndex = 0;
    m_CursorByteIndex = m_Text.size();
    EnsureCursorVisible();
    Invalidate(EInvalidateReason::Paint);
}

std::string ImEditableText::GetSelectedText() const {
    if (!HasSelection()) {
        return {};
    }

    const std::size_t selectionStart = GetSelectionStartByteIndex();
    const std::size_t selectionEnd = GetSelectionEndByteIndex();
    return m_Text.substr(selectionStart, selectionEnd - selectionStart);
}

void ImEditableText::CopySelectionToClipboard() const {
    if (!HasSelection()) {
        return;
    }

    SetClipboardTextSafe(GetSelectedText());
}

void ImEditableText::CutSelectionToClipboard() {
    if (!HasSelection()) {
        return;
    }

    CopySelectionToClipboard();
    DeleteSelection();
    NotifyTextChanged();
}

void ImEditableText::PasteFromClipboard() {
    const char* clipboardText = GetClipboardTextSafe();
    if (clipboardText == nullptr || clipboardText[0] == '\0') {
        return;
    }

    InsertText(clipboardText);
}

float ImEditableText::MeasureCaretX(std::size_t byteIndex) const {
    return MeasureText(m_Text.substr(0, ClampByteIndex(byteIndex))).X;
}

FVector2 ImEditableText::MeasureText(const std::string& text) const {
    return MeasureTextWithFont(text, GetEffectiveStyle().FontSize);
}

const FEditableTextStyle& ImEditableText::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveEditableTextStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

std::string ImEditableText::ResolveHintText() const {
    if (m_HintTextValue.IsLocalized() || !m_HintTextValue.GetInvariantText().empty()) {
        return m_HintTextValue.Resolve();
    }

    return m_HintText;
}

void ImEditableText::SetHovered(bool bHovered) {
    if (m_bHovered == bHovered) {
        return;
    }

    m_bHovered = bHovered;
    Invalidate(EInvalidateReason::Paint);
}

void ImEditableText::SetDraggingSelection(bool bDraggingSelection) {
    m_bDraggingSelection = bDraggingSelection;
}

} // namespace ImWidgetV4

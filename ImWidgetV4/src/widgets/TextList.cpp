#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

std::string GFallbackClipboardText;

void SetImGuiMouseCursor(ImGuiMouseCursor cursor)
{
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::SetMouseCursor(cursor);
    }
}

FGeometry InsetGeometry(const FGeometry& geometry, float insetLeft, float insetTop, float insetRight, float insetBottom)
{
    const FVector2 min(
        geometry.Position.X + insetLeft,
        geometry.Position.Y + insetTop);
    const FVector2 max(
        geometry.Position.X + geometry.Size.X - insetRight,
        geometry.Position.Y + geometry.Size.Y - insetBottom);

    return FGeometry(
        min,
        FVector2(
            std::max(0.0f, max.X - min.X),
            std::max(0.0f, max.Y - min.Y)));
}

float ResolveVisibleOffset(float targetStart, float targetSize, float viewportSize, float currentOffset, bool bCenterIfLarger)
{
    if (viewportSize <= 0.0f) {
        return currentOffset;
    }

    if (targetSize > viewportSize) {
        if (bCenterIfLarger) {
            return targetStart - (viewportSize - targetSize) * 0.5f;
        }

        if (targetStart < currentOffset || targetStart + targetSize > currentOffset + viewportSize) {
            return targetStart;
        }

        return currentOffset;
    }

    if (targetStart < currentOffset) {
        return targetStart;
    }

    const float targetEnd = targetStart + targetSize;
    const float viewportEnd = currentOffset + viewportSize;
    if (targetEnd > viewportEnd) {
        return targetEnd - viewportSize;
    }

    return currentOffset;
}

float MeasureTextWidth(const std::string& text, float fontSize)
{
    if (text.empty()) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(
            fontSize,
            FLT_MAX,
            0.0f,
            text.c_str(),
            text.c_str() + text.size()).x;
    }

    return static_cast<float>(text.size()) * fontSize * 0.55f;
}

float MeasureLineHeight(float fontSize)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(
            fontSize,
            FLT_MAX,
            0.0f,
            "Ag",
            nullptr).y;
    }

    return fontSize;
}

float ResolveFontWrapScale(float fontSize)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        const ImFont* font = ImGui::GetFont();
        if (font->FontSize > 0.0f) {
            return fontSize / font->FontSize;
        }
    }

    return 1.0f;
}

const char* GetClipboardTextSafe()
{
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.GetClipboardTextFn != nullptr) {
            const char* text = io.GetClipboardTextFn(io.ClipboardUserData);
            return text != nullptr ? text : "";
        }
    }

    return GFallbackClipboardText.c_str();
}

void SetClipboardTextSafe(const std::string& text)
{
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.SetClipboardTextFn != nullptr) {
            io.SetClipboardTextFn(io.ClipboardUserData, text.c_str());
        }
    }

    GFallbackClipboardText = text;
}

void AppendWrappedLines(
    const std::string& paragraph,
    int itemIndex,
    float wrapWidth,
    float fontSize,
    std::vector<ImTextList::FTextLine>& outLines)
{
    if (paragraph.empty()) {
        ImTextList::FTextLine emptyLine;
        emptyLine.Text = "";
        emptyLine.ItemIndex = itemIndex;
        emptyLine.Size = FVector2(0.0f, MeasureLineHeight(fontSize));
        emptyLine.CharOffsets = {0.0f};
        outLines.push_back(std::move(emptyLine));
        return;
    }

    const char* start = paragraph.c_str();
    const char* end = start + paragraph.size();

    while (start < end) {
        const char* wrapPos = nullptr;
        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr && wrapWidth > 0.0f) {
            wrapPos = ImGui::GetFont()->CalcWordWrapPositionA(
                ResolveFontWrapScale(fontSize),
                start,
                end,
                wrapWidth);
            if (wrapPos == start) {
                wrapPos = start + 1;
            }
        }

        if (wrapPos == nullptr || wrapPos <= start) {
            wrapPos = end;
        }

        std::string lineText(start, wrapPos);
        while (!lineText.empty() && (lineText.back() == ' ' || lineText.back() == '\t')) {
            lineText.pop_back();
        }

        ImTextList::FTextLine line;
        line.Text = lineText;
        line.ItemIndex = itemIndex;
        line.Size = FVector2(MeasureTextWidth(lineText, fontSize), MeasureLineHeight(fontSize));
        line.CharOffsets.reserve(lineText.size() + 1);
        line.CharOffsets.push_back(0.0f);
        for (std::size_t index = 0; index < lineText.size(); ++index) {
            line.CharOffsets.push_back(MeasureTextWidth(lineText.substr(0, index + 1), fontSize));
        }
        outLines.push_back(std::move(line));

        start = wrapPos;
        while (start < end && (*start == ' ' || *start == '\t')) {
            ++start;
        }
    }
}

} // namespace

ImTextList::ImTextList()
    : ImWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

void ImTextList::SetItems(const std::vector<std::string>& items)
{
    m_Items = items;
    m_ItemColors.assign(m_Items.size(), m_Style.TextColor);
    ClearSelection();
    m_bLayoutDirty = true;
    m_LastLayoutWrapWidth = -1.0f;
    m_LastLayoutHeight = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::AddItem(const std::string& item)
{
    m_Items.push_back(item);
    m_ItemColors.push_back(m_Style.TextColor);
    m_bLayoutDirty = true;
    m_LastLayoutWrapWidth = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::ClearItems()
{
    m_Items.clear();
    m_ItemColors.clear();
    m_Lines.clear();
    m_ItemLayouts.clear();
    m_ContentHeight = 0.0f;
    m_ScrollOffsetY = 0.0f;
    m_MaxScrollOffsetY = 0.0f;
    ClearSelection();
    m_bLayoutDirty = true;
    m_LastLayoutWrapWidth = -1.0f;
    m_LastLayoutHeight = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::ModifyItem(int index, const std::string& item)
{
    if (index < 0 || index >= static_cast<int>(m_Items.size())) {
        return;
    }

    m_Items[static_cast<std::size_t>(index)] = item;
    ClearSelection();
    m_bLayoutDirty = true;
    m_LastLayoutWrapWidth = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::RemoveItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_Items.size())) {
        return;
    }

    m_Items.erase(m_Items.begin() + index);
    m_ItemColors.erase(m_ItemColors.begin() + index);
    ClearSelection();
    m_bLayoutDirty = true;
    m_LastLayoutWrapWidth = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::SetStyle(const FTextListStyle& style)
{
    m_Style = style;
    for (FColor& color : m_ItemColors) {
        color = m_Style.TextColor;
    }
    m_bLayoutDirty = true;
    m_LastLayoutWrapWidth = -1.0f;
    m_LastLayoutHeight = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::SetTextColor(const FColor& color)
{
    if (m_Style.TextColor.R == color.R &&
        m_Style.TextColor.G == color.G &&
        m_Style.TextColor.B == color.B &&
        m_Style.TextColor.A == color.A) {
        return;
    }

    m_Style.TextColor = color;
    for (FColor& itemColor : m_ItemColors) {
        itemColor = color;
    }
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::SetItemColor(int index, const FColor& color)
{
    if (index < 0 || index >= static_cast<int>(m_ItemColors.size())) {
        return;
    }

    m_ItemColors[static_cast<std::size_t>(index)] = color;
    Invalidate(EInvalidateReason::Paint);
}

FColor ImTextList::GetItemColor(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_ItemColors.size())) {
        return m_Style.TextColor;
    }

    return m_ItemColors[static_cast<std::size_t>(index)];
}

void ImTextList::SetAllItemsColor(const FColor& color)
{
    for (FColor& itemColor : m_ItemColors) {
        itemColor = color;
    }
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::SetLineSpacing(float spacing)
{
    if (m_Style.LineSpacing == spacing) {
        return;
    }

    m_Style.LineSpacing = spacing;
    m_bLayoutDirty = true;
    m_LastLayoutHeight = -1.0f;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextList::SetScrollOffset(float offset)
{
    Relayout();
    const float clamped = std::clamp(offset, 0.0f, m_MaxScrollOffsetY);
    if (m_ScrollOffsetY == clamped) {
        return;
    }

    m_ScrollOffsetY = clamped;
    Invalidate(EInvalidateReason::Paint);
}

bool ImTextList::ScrollToItem(int index, bool bCenterIfLarger)
{
    Relayout();
    if (index < 0 || index >= static_cast<int>(m_ItemLayouts.size()) || !m_CachedViewportGeometry.IsValid()) {
        return false;
    }

    const FItemLayout& layout = m_ItemLayouts[static_cast<std::size_t>(index)];
    const float nextOffset = ResolveVisibleOffset(
        layout.Top,
        std::max(0.0f, layout.Bottom - layout.Top),
        m_CachedViewportGeometry.Size.Y,
        m_ScrollOffsetY,
        bCenterIfLarger);
    SetScrollOffset(nextOffset);
    return true;
}

bool ImTextList::HasSelection() const
{
    return m_SelectionAnchor.IsValid() && m_SelectionCursor.IsValid() && !(m_SelectionAnchor == m_SelectionCursor);
}

void ImTextList::ClearSelection()
{
    const bool hadSelection = m_SelectionAnchor.IsValid() || m_SelectionCursor.IsValid();
    m_SelectionAnchor = {};
    m_SelectionCursor = {};
    m_bDraggingSelection = false;
    if (hadSelection) {
        Invalidate(EInvalidateReason::Paint);
    }
}

std::string ImTextList::GetSelectedText() const
{
    if (!HasSelection()) {
        return {};
    }

    FSelectionPoint start = m_SelectionAnchor;
    FSelectionPoint end = m_SelectionCursor;
    NormalizeSelection(start, end);

    std::string selected;
    for (int lineIndex = start.LineIndex; lineIndex <= end.LineIndex; ++lineIndex) {
        if (lineIndex < 0 || lineIndex >= static_cast<int>(m_Lines.size())) {
            continue;
        }

        const FTextLine& line = m_Lines[static_cast<std::size_t>(lineIndex)];
        const int startChar = lineIndex == start.LineIndex ? start.CharIndex : 0;
        const int endChar = lineIndex == end.LineIndex ? end.CharIndex : static_cast<int>(line.Text.size());
        if (endChar > startChar) {
            if (!selected.empty()) {
                selected += '\n';
            }
            selected += line.Text.substr(static_cast<std::size_t>(startChar), static_cast<std::size_t>(endChar - startChar));
        } else if (lineIndex != end.LineIndex) {
            if (!selected.empty()) {
                selected += '\n';
            }
        }
    }

    return selected;
}

void ImTextList::CopySelectionToClipboard() const
{
    const std::string selected = GetSelectedText();
    if (!selected.empty()) {
        SetClipboardTextSafe(selected);
    }
}

void ImTextList::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        m_Style.BackgroundColor,
        m_Style.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        HasKeyboardFocus() ? m_Style.FocusedOutlineColor : m_Style.BorderColor,
        m_Style.CornerRadius,
        m_Style.BorderThickness);

    if (m_CachedViewportGeometry.IsValid()) {
        paintContext.DrawContext_.PushClipRect(
            m_CachedViewportGeometry.GetMin(),
            m_CachedViewportGeometry.GetMax(),
            true);

        if (HasSelection()) {
            FSelectionPoint start = m_SelectionAnchor;
            FSelectionPoint end = m_SelectionCursor;
            NormalizeSelection(start, end);
            const float lineStride = ResolveLineStride(ResolveWrappedLineHeight());

            for (int lineIndex = start.LineIndex; lineIndex <= end.LineIndex; ++lineIndex) {
                if (lineIndex < 0 || lineIndex >= static_cast<int>(m_Lines.size())) {
                    continue;
                }

                const FTextLine& line = m_Lines[static_cast<std::size_t>(lineIndex)];
                const float screenY = m_CachedViewportGeometry.Position.Y + line.ContentY - m_ScrollOffsetY;
                if (screenY + lineStride < m_CachedViewportGeometry.Position.Y ||
                    screenY > m_CachedViewportGeometry.GetMax().Y) {
                    continue;
                }

                const int startChar = lineIndex == start.LineIndex ? start.CharIndex : 0;
                const int endChar = lineIndex == end.LineIndex ? end.CharIndex : static_cast<int>(line.Text.size());
                if (endChar < startChar || startChar < 0 || endChar < 0 ||
                    startChar >= static_cast<int>(line.CharOffsets.size()) ||
                    endChar >= static_cast<int>(line.CharOffsets.size())) {
                    continue;
                }

                const float selectionStartX = m_CachedViewportGeometry.Position.X + line.CharOffsets[static_cast<std::size_t>(startChar)];
                const float selectionEndX = m_CachedViewportGeometry.Position.X + line.CharOffsets[static_cast<std::size_t>(endChar)];
                paintContext.DrawContext_.DrawRectFilled(
                    FVector2(selectionStartX, screenY),
                    FVector2(selectionEndX, screenY + lineStride),
                    m_Style.SelectionBackgroundColor);
            }
        }

        for (const FTextLine& line : m_Lines) {
            const float screenY = m_CachedViewportGeometry.Position.Y + line.ContentY - m_ScrollOffsetY;
            if (screenY + line.Size.Y < m_CachedViewportGeometry.Position.Y ||
                screenY > m_CachedViewportGeometry.GetMax().Y) {
                continue;
            }

            paintContext.DrawContext_.DrawText(
                FVector2(m_CachedViewportGeometry.Position.X, screenY),
                GetItemColor(line.ItemIndex),
                line.Text,
                m_Style.FontSize);
        }

        paintContext.DrawContext_.PopClipRect();
    }

    if (m_VerticalScrollbarGeometry.IsValid()) {
        paintContext.DrawContext_.DrawRectFilled(
            m_VerticalScrollbarGeometry.GetMin(),
            m_VerticalScrollbarGeometry.GetMax(),
            m_Style.ScrollbarTrackColor,
            m_Style.ScrollbarThickness * 0.5f);
        paintContext.DrawContext_.DrawRectFilled(
            m_VerticalThumbGeometry.GetMin(),
            m_VerticalThumbGeometry.GetMax(),
            (m_bDraggingScrollbar || m_bHoveredScrollbar)
                ? m_Style.ScrollbarThumbHoveredColor
                : m_Style.ScrollbarThumbColor,
            m_Style.ScrollbarThickness * 0.5f);
    }

    if (m_bDraggingScrollbar || m_bHoveredScrollbar) {
        SetImGuiMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
}

FVector2 ImTextList::GetMinSize() const
{
    return m_Style.MinDesiredSize;
}

FReply ImTextList::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    switch (event.Type) {
    case EInputEventType::MouseButtonDown:
        if (event.MouseButton != EMouseButton::Left || !m_Geometry.Contains(event.MousePosition)) {
            return FReply::Unhandled();
        }

        if (m_VerticalThumbGeometry.IsValid() && m_VerticalThumbGeometry.Contains(event.MousePosition)) {
            BeginScrollbarDrag(event.MousePosition.Y - m_VerticalThumbGeometry.Position.Y);
            return FReply::Handled()
                .SetKeyboardFocus(shared_from_this())
                .CaptureMouse(shared_from_this(), EMouseButton::Left);
        }

        BeginTextSelection(event.MousePosition);
        return FReply::Handled()
            .SetKeyboardFocus(shared_from_this())
            .CaptureMouse(shared_from_this(), EMouseButton::Left);

    case EInputEventType::MouseMove:
    case EInputEventType::MouseEnter:
        m_bHoveredScrollbar = m_VerticalThumbGeometry.IsValid() && m_VerticalThumbGeometry.Contains(event.MousePosition);
        if (m_bDraggingScrollbar) {
            UpdateScrollbarDrag(event.MousePosition);
            return FReply::Handled();
        }
        if (m_bDraggingSelection) {
            UpdateTextSelection(event.MousePosition);
            ApplyAutoScrollForSelection(event.MousePosition);
            return FReply::Handled();
        }
        Invalidate(EInvalidateReason::Paint);
        return FReply::Unhandled();

    case EInputEventType::MouseLeave:
        if (!m_bDraggingScrollbar) {
            m_bHoveredScrollbar = false;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();

    case EInputEventType::MouseButtonUp:
        if (event.MouseButton != EMouseButton::Left) {
            return FReply::Unhandled();
        }
        if (m_bDraggingScrollbar) {
            UpdateScrollbarDrag(event.MousePosition);
            EndScrollbarDrag();
            return FReply::Handled().ReleaseMouseCapture();
        }
        if (m_bDraggingSelection) {
            UpdateTextSelection(event.MousePosition);
            EndTextSelection();
            return FReply::Handled().ReleaseMouseCapture();
        }
        return FReply::Unhandled();

    case EInputEventType::MouseWheel:
        if (!m_Geometry.Contains(event.MousePosition) || m_MaxScrollOffsetY <= 0.0f) {
            return FReply::Unhandled();
        }
        SetScrollOffset(m_ScrollOffsetY - event.ScrollDelta.Y * m_Style.WheelScrollStep);
        return FReply::Handled();

    case EInputEventType::KeyDown:
        if (!HasKeyboardFocus()) {
            return FReply::Unhandled();
        }
        if (event.Modifiers.bCtrl && !event.Modifiers.bAlt && !event.Modifiers.bSuper) {
            if (event.Key == EKey::C) {
                CopySelectionToClipboard();
                return FReply::Handled();
            }
            if (event.Key == EKey::A && !m_Lines.empty()) {
                m_SelectionAnchor = {0, 0};
                m_SelectionCursor = {
                    static_cast<int>(m_Lines.size()) - 1,
                    static_cast<int>(m_Lines.back().Text.size())
                };
                Invalidate(EInvalidateReason::Paint);
                return FReply::Handled();
            }
        }
        return FReply::Unhandled();

    default:
        return FReply::Unhandled();
    }
}

bool ImTextList::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    outPath.push_back(shared_from_this());
    return true;
}

void ImTextList::OnFocusChanged(bool bHasFocus)
{
    (void)bHasFocus;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::Relayout()
{
    const float borderInset = std::max(0.0f, m_Style.BorderThickness);
    const FGeometry innerGeometry = InsetGeometry(
        m_Geometry,
        borderInset + m_Style.Padding.Left,
        borderInset + m_Style.Padding.Top,
        borderInset + m_Style.Padding.Right,
        borderInset + m_Style.Padding.Bottom);

    const float estimatedWidthWithoutScrollbar = std::max(0.0f, innerGeometry.Size.X);
    const float estimatedWidthWithScrollbar = std::max(
        0.0f,
        estimatedWidthWithoutScrollbar - m_Style.ScrollbarThickness - m_Style.ScrollbarPadding);
    const float firstPassWrapWidth = std::max(1.0f, estimatedWidthWithoutScrollbar);
    const float secondPassWrapWidth = std::max(1.0f, estimatedWidthWithScrollbar);
    float resolvedWrapWidth = firstPassWrapWidth;

    const bool bLayoutWidthMatchesCache =
        m_LastLayoutWrapWidth == firstPassWrapWidth ||
        m_LastLayoutWrapWidth == secondPassWrapWidth;

    if (m_bLayoutDirty ||
        !bLayoutWidthMatchesCache ||
        m_LastLayoutHeight != innerGeometry.Size.Y) {
        RebuildLayout(firstPassWrapWidth);
        if (firstPassWrapWidth != secondPassWrapWidth &&
            m_ContentHeight > innerGeometry.Size.Y + 0.5f) {
            RebuildLayout(secondPassWrapWidth);
            resolvedWrapWidth = secondPassWrapWidth;
        }
        m_LastLayoutWrapWidth = resolvedWrapWidth;
        m_LastLayoutHeight = innerGeometry.Size.Y;
    } else {
        resolvedWrapWidth = m_LastLayoutWrapWidth;
    }

    const bool bShowScrollbar = m_ContentHeight > innerGeometry.Size.Y + 0.5f;
    const float viewportWidth = bShowScrollbar ? estimatedWidthWithScrollbar : estimatedWidthWithoutScrollbar;
    m_CachedViewportGeometry = FGeometry(innerGeometry.Position, FVector2(std::max(0.0f, viewportWidth), innerGeometry.Size.Y));
    m_MaxScrollOffsetY = std::max(0.0f, m_ContentHeight - m_CachedViewportGeometry.Size.Y);
    ClampScrollOffset();

    m_VerticalScrollbarGeometry = FGeometry();
    m_VerticalThumbGeometry = FGeometry();
    if (bShowScrollbar && innerGeometry.Size.Y > 0.0f) {
        const float scrollbarX = innerGeometry.Position.X + std::max(0.0f, resolvedWrapWidth) + m_Style.ScrollbarPadding;
        m_VerticalScrollbarGeometry = FGeometry(
            FVector2(scrollbarX, innerGeometry.Position.Y),
            FVector2(m_Style.ScrollbarThickness, innerGeometry.Size.Y));

        const float trackHeight = m_VerticalScrollbarGeometry.Size.Y;
        const float thumbHeight = std::min(
            trackHeight,
            std::max(
                std::min(trackHeight, m_Style.ThumbMinLength),
                m_ContentHeight > 0.0f ? trackHeight * (m_CachedViewportGeometry.Size.Y / m_ContentHeight) : trackHeight));
        const float availableTrack = std::max(0.0f, trackHeight - thumbHeight);
        const float thumbOffset = m_MaxScrollOffsetY > 0.0f
            ? (m_ScrollOffsetY / m_MaxScrollOffsetY) * availableTrack
            : 0.0f;

        m_VerticalThumbGeometry = FGeometry(
            FVector2(m_VerticalScrollbarGeometry.Position.X, m_VerticalScrollbarGeometry.Position.Y + thumbOffset),
            FVector2(m_Style.ScrollbarThickness, thumbHeight));
    }
}

void ImTextList::RebuildLayout(float wrapWidth)
{
    wrapWidth = std::max(1.0f, wrapWidth);

    m_Lines.clear();
    m_ItemLayouts.clear();
    m_ItemLayouts.resize(m_Items.size());

    float cursorY = 0.0f;
    const float lineStride = ResolveLineStride(ResolveWrappedLineHeight());

    for (std::size_t itemIndex = 0; itemIndex < m_Items.size(); ++itemIndex) {
        FItemLayout layout;
        layout.FirstLineIndex = m_Lines.size();
        layout.Top = cursorY;

        std::size_t start = 0;
        const std::string& item = m_Items[itemIndex];
        while (start <= item.size()) {
            const std::size_t end = item.find('\n', start);
            const std::size_t count = (end == std::string::npos ? item.size() : end) - start;
            AppendWrappedLines(item.substr(start, count), static_cast<int>(itemIndex), wrapWidth, m_Style.FontSize, m_Lines);
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }

        layout.LineCount = m_Lines.size() - layout.FirstLineIndex;
        for (std::size_t index = layout.FirstLineIndex; index < layout.FirstLineIndex + layout.LineCount; ++index) {
            m_Lines[index].ContentY = cursorY;
            cursorY += lineStride;
        }
        layout.Bottom = cursorY;
        m_ItemLayouts[itemIndex] = layout;
    }

    m_ContentHeight = cursorY > 0.0f ? (cursorY - std::max(0.0f, lineStride - ResolveWrappedLineHeight())) : 0.0f;
    m_bLayoutDirty = false;
}

void ImTextList::ClampScrollOffset()
{
    m_ScrollOffsetY = std::clamp(m_ScrollOffsetY, 0.0f, m_MaxScrollOffsetY);
}

void ImTextList::BeginScrollbarDrag(float grabOffset)
{
    m_bDraggingScrollbar = true;
    m_ActiveGrabOffset = std::max(0.0f, grabOffset);
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::UpdateScrollbarDrag(const FVector2& cursorPosition)
{
    if (!m_bDraggingScrollbar || !m_VerticalScrollbarGeometry.IsValid()) {
        return;
    }

    const float availableTrack = std::max(0.0f, m_VerticalScrollbarGeometry.Size.Y - m_VerticalThumbGeometry.Size.Y);
    if (availableTrack <= 0.0f || m_MaxScrollOffsetY <= 0.0f) {
        SetScrollOffset(0.0f);
        return;
    }

    const float thumbPosition = std::clamp(
        cursorPosition.Y - m_VerticalScrollbarGeometry.Position.Y - m_ActiveGrabOffset,
        0.0f,
        availableTrack);
    SetScrollOffset((thumbPosition / availableTrack) * m_MaxScrollOffsetY);
}

void ImTextList::EndScrollbarDrag()
{
    m_bDraggingScrollbar = false;
    m_ActiveGrabOffset = 0.0f;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::BeginTextSelection(const FVector2& cursorPosition)
{
    m_bDraggingSelection = true;
    m_SelectionAnchor = ResolveSelectionPointAt(cursorPosition);
    m_SelectionCursor = m_SelectionAnchor;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::UpdateTextSelection(const FVector2& cursorPosition)
{
    if (!m_bDraggingSelection) {
        return;
    }

    const FSelectionPoint nextPoint = ResolveSelectionPointAt(cursorPosition);
    if (!(m_SelectionCursor == nextPoint)) {
        m_SelectionCursor = nextPoint;
        Invalidate(EInvalidateReason::Paint);
    }
}

void ImTextList::EndTextSelection()
{
    m_bDraggingSelection = false;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextList::ApplyAutoScrollForSelection(const FVector2& cursorPosition)
{
    if (!m_bDraggingSelection || !m_CachedViewportGeometry.IsValid()) {
        return;
    }

    float delta = 0.0f;
    const float localY = cursorPosition.Y - m_CachedViewportGeometry.Position.Y;
    if (localY < m_Style.AutoScrollEdgePadding) {
        delta = (localY - m_Style.AutoScrollEdgePadding) * (m_Style.AutoScrollSpeed / std::max(1.0f, m_Style.AutoScrollEdgePadding));
    } else if (localY > m_CachedViewportGeometry.Size.Y - m_Style.AutoScrollEdgePadding) {
        delta = (localY - (m_CachedViewportGeometry.Size.Y - m_Style.AutoScrollEdgePadding)) *
            (m_Style.AutoScrollSpeed / std::max(1.0f, m_Style.AutoScrollEdgePadding));
    }

    if (delta != 0.0f) {
        SetScrollOffset(m_ScrollOffsetY + delta);
    }
}

ImTextList::FSelectionPoint ImTextList::ResolveSelectionPointAt(const FVector2& cursorPosition) const
{
    if (m_Lines.empty()) {
        return {};
    }

    const float localY = std::clamp(
        cursorPosition.Y - m_CachedViewportGeometry.Position.Y + m_ScrollOffsetY,
        0.0f,
        std::max(0.0f, m_ContentHeight));
    const float lineStride = ResolveLineStride(ResolveWrappedLineHeight());

    int chosenLineIndex = static_cast<int>(m_Lines.size()) - 1;
    for (std::size_t index = 0; index < m_Lines.size(); ++index) {
        const FTextLine& line = m_Lines[index];
        const float lineTop = line.ContentY;
        const float lineBottom = lineTop + lineStride;
        if (localY >= lineTop && localY <= lineBottom) {
            chosenLineIndex = static_cast<int>(index);
            break;
        }
        if (localY < lineTop) {
            chosenLineIndex = static_cast<int>(index);
            break;
        }
    }

    const FTextLine& line = m_Lines[static_cast<std::size_t>(chosenLineIndex)];
    const float localX = cursorPosition.X - m_CachedViewportGeometry.Position.X;

    int charIndex = 0;
    if (localX <= 0.0f) {
        charIndex = 0;
    } else if (localX >= line.Size.X) {
        charIndex = static_cast<int>(line.Text.size());
    } else {
        for (std::size_t index = 1; index < line.CharOffsets.size(); ++index) {
            if (localX < line.CharOffsets[index]) {
                charIndex = static_cast<int>(index - 1);
                break;
            }
            charIndex = static_cast<int>(index);
        }
    }

    return {chosenLineIndex, charIndex};
}

void ImTextList::NormalizeSelection(FSelectionPoint& start, FSelectionPoint& end) const
{
    if (!start.IsValid() || !end.IsValid()) {
        return;
    }

    if (start.LineIndex > end.LineIndex ||
        (start.LineIndex == end.LineIndex && start.CharIndex > end.CharIndex)) {
        std::swap(start, end);
    }
}

bool ImTextList::IsNavigationShortcut(const FInputEvent& event) const
{
    return event.Modifiers.bCtrl && !event.Modifiers.bAlt && !event.Modifiers.bSuper;
}

float ImTextList::ResolveWrappedLineHeight() const
{
    return MeasureLineHeight(m_Style.FontSize);
}

float ImTextList::ResolveLineStride(float lineHeight) const
{
    return std::max(lineHeight, lineHeight * std::max(0.0f, m_Style.LineSpacing));
}

} // namespace ImWidgetV4

#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

namespace {

constexpr float LayoutEpsilon = 0.001f;

float SanitizeThickness(float thickness) {
    return std::max(0.0f, thickness);
}

void SetImGuiMouseCursor(ImGuiMouseCursor cursor)
{
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::SetMouseCursor(cursor);
    }
}

void DistributeSizes(
    const std::vector<float>& weights,
    const std::vector<float>& minSizes,
    float availableSize,
    std::vector<float>& outSizes) {
    const std::size_t count = weights.size();
    outSizes.assign(count, 0.0f);
    if (count == 0) {
        return;
    }

    availableSize = std::max(0.0f, availableSize);

    float totalMinSize = 0.0f;
    for (float minSize : minSizes) {
        totalMinSize += std::max(0.0f, minSize);
    }

    if (availableSize <= LayoutEpsilon) {
        return;
    }

    if (totalMinSize > LayoutEpsilon && availableSize < totalMinSize) {
        for (std::size_t index = 0; index < count; ++index) {
            outSizes[index] = availableSize * std::max(0.0f, minSizes[index]) / totalMinSize;
        }
        return;
    }

    std::vector<bool> locked(count, false);
    float remainingSize = availableSize;

    while (true) {
        float totalUnlockedWeight = 0.0f;
        int unlockedCount = 0;
        for (std::size_t index = 0; index < count; ++index) {
            if (!locked[index]) {
                totalUnlockedWeight += std::max(0.0f, weights[index]);
                ++unlockedCount;
            }
        }

        if (unlockedCount == 0) {
            break;
        }

        bool changed = false;
        for (std::size_t index = 0; index < count; ++index) {
            if (locked[index]) {
                continue;
            }

            const float minSize = std::max(0.0f, minSizes[index]);
            const float proposedSize = totalUnlockedWeight > LayoutEpsilon
                ? remainingSize * std::max(0.0f, weights[index]) / totalUnlockedWeight
                : remainingSize / static_cast<float>(unlockedCount);

            if (proposedSize + LayoutEpsilon < minSize) {
                outSizes[index] = minSize;
                remainingSize -= minSize;
                locked[index] = true;
                changed = true;
            }
        }

        if (!changed) {
            totalUnlockedWeight = 0.0f;
            unlockedCount = 0;
            for (std::size_t index = 0; index < count; ++index) {
                if (!locked[index]) {
                    totalUnlockedWeight += std::max(0.0f, weights[index]);
                    ++unlockedCount;
                }
            }

            for (std::size_t index = 0; index < count; ++index) {
                if (!locked[index]) {
                    outSizes[index] = totalUnlockedWeight > LayoutEpsilon
                        ? remainingSize * std::max(0.0f, weights[index]) / totalUnlockedWeight
                        : remainingSize / static_cast<float>(unlockedCount);
                }
            }
            break;
        }
    }
}

} // namespace

const Reflection::FTypeDesc& FHorizontalSplitterStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FHorizontalSplitterStyle, float, &FHorizontalSplitterStyle::BarWidth>(
            "FHorizontalSplitterStyle", "BarWidth", Reflection::EPropertyKind::Float, "float", "Bar width"),
        Reflection::MakeMemberProperty<FHorizontalSplitterStyle, FColor, &FHorizontalSplitterStyle::Color>(
            "FHorizontalSplitterStyle", "Color", Reflection::EPropertyKind::Color, "FColor", "Bar color"),
        Reflection::MakeMemberProperty<FHorizontalSplitterStyle, FColor, &FHorizontalSplitterStyle::HoveredColor>(
            "FHorizontalSplitterStyle", "HoveredColor", Reflection::EPropertyKind::Color, "FColor", "Hovered bar color"),
        Reflection::MakeMemberProperty<FHorizontalSplitterStyle, FColor, &FHorizontalSplitterStyle::ActiveColor>(
            "FHorizontalSplitterStyle", "ActiveColor", Reflection::EPropertyKind::Color, "FColor", "Active bar color"),
        Reflection::MakeMemberProperty<FHorizontalSplitterStyle, float, &FHorizontalSplitterStyle::Rounding>(
            "FHorizontalSplitterStyle", "Rounding", Reflection::EPropertyKind::Float, "float", "Bar rounding")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FHorizontalSplitterStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImHorizontalSplitterSlot::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImHorizontalSplitterSlot, float, &ImHorizontalSplitterSlot::m_Ratio>(
            "ImHorizontalSplitterSlot", "Ratio", Reflection::EPropertyKind::Float, "float", "Part ratio"),
        Reflection::MakeMemberProperty<ImHorizontalSplitterSlot, float, &ImHorizontalSplitterSlot::m_MinSize>(
            "ImHorizontalSplitterSlot", "MinSize", Reflection::EPropertyKind::Float, "float", "Minimum part size")
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImHorizontalSplitterSlot",
        &ImPaddingSlot::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImHorizontalSplitter::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImHorizontalSplitter, FHorizontalSplitterStyle, &ImHorizontalSplitter::m_Style>(
            "ImHorizontalSplitter",
            "Style",
            Reflection::EPropertyKind::Struct,
            "FHorizontalSplitterStyle",
            "Horizontal splitter style",
            &FHorizontalSplitterStyle::StaticTypeDesc())
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImHorizontalSplitter",
        &ImPanelWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FHorizontalSplitterStyleReflectionTypeDesc = FHorizontalSplitterStyle::StaticTypeDesc();
const Reflection::FTypeDesc& ImHorizontalSplitterSlotReflectionTypeDesc = ImHorizontalSplitterSlot::StaticTypeDesc();
const Reflection::FTypeDesc& ImHorizontalSplitterReflectionTypeDesc = ImHorizontalSplitter::StaticTypeDesc();

} // namespace

ImHorizontalSplitter::ImHorizontalSplitter()
    : ImPanelWidget() {
}

void ImHorizontalSplitter::SetSplitterStyle(const FHorizontalSplitterStyle& style)
{
    m_Style = style;
    m_bHasExplicitStyle = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImHorizontalSplitter::AddChild(const Ptr& child) {
    AddPart(child);
}

void ImHorizontalSplitter::AddPartWithSlot(
    const Ptr& child,
    std::unique_ptr<ImHorizontalSplitterSlot> slot) {
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImHorizontalSplitterSlot>();
    }

    ImWidget::AddChild(child);
    m_Slots.push_back(std::move(slot));
}

ImHorizontalSplitterSlot* ImHorizontalSplitter::AddPart(
    const Ptr& child,
    float ratio,
    float minSize,
    const FMargin& padding) {
    if (!child) {
        return nullptr;
    }

    auto slot = std::make_unique<ImHorizontalSplitterSlot>();
    slot->PaddingLeft = padding.Left;
    slot->PaddingRight = padding.Right;
    slot->PaddingTop = padding.Top;
    slot->PaddingBottom = padding.Bottom;
    slot->SetRatio(ratio);
    slot->SetMinSize(minSize);

    ImHorizontalSplitterSlot* slotPtr = slot.get();
    AddPartWithSlot(child, std::move(slot));
    return slotPtr;
}

void ImHorizontalSplitter::SetPartMinSize(int index, float minSize) {
    if (auto* slot = dynamic_cast<ImHorizontalSplitterSlot*>(GetSlotAt(index))) {
        slot->SetMinSize(minSize);
    }
}

std::unique_ptr<ImSlot> ImHorizontalSplitter::CreateSlot() {
    return std::make_unique<ImHorizontalSplitterSlot>();
}

void ImHorizontalSplitter::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    Relayout();
    RenderChildren(paintContext);
    RenderBars(paintContext);

    if (m_DraggingBarIndex >= 0 || m_HoveredBarIndex >= 0) {
        SetImGuiMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
}

FVector2 ImHorizontalSplitter::GetMinSize() const {
    const auto& children = GetChildren();
    if (children.empty()) {
        return FVector2(0.0f, 0.0f);
    }

    const FHorizontalSplitterStyle& style = GetEffectiveStyle();
    const float barWidth = SanitizeThickness(style.BarWidth);
    float totalWidth = barWidth * static_cast<float>(children.size() > 0 ? children.size() - 1 : 0);
    float maxHeight = 0.0f;

    for (std::size_t index = 0; index < children.size(); ++index) {
        const auto* slot = dynamic_cast<const ImHorizontalSplitterSlot*>(GetSlotAt(static_cast<int>(index)));
        if (!children[index] || !slot) {
            continue;
        }

        const FVector2 childMinSize = children[index]->GetMinSize();
        const float childHeight = childMinSize.Y + slot->PaddingTop + slot->PaddingBottom;
        totalWidth += std::max(0.0f, slot->GetMinSize());
        maxHeight = std::max(maxHeight, childHeight);
    }

    return FVector2(totalWidth, maxHeight);
}

FReply ImHorizontalSplitter::OnInputEvent(const FInputEvent& event) {
    if (event.Type == EInputEventType::MouseEnter ||
        event.Type == EInputEventType::MouseMove) {
        UpdateHoveredBar(event.MousePosition);
    } else if (event.Type == EInputEventType::MouseLeave) {
        if (m_Geometry.Contains(event.MousePosition)) {
            UpdateHoveredBar(event.MousePosition);
        } else {
            ClearHoveredBar();
        }
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left) {
        const int barIndex = HitTestBarIndex(event.MousePosition);
        if (barIndex >= 0) {
            BeginDrag(barIndex, event.MousePosition);
            return FReply::Handled().CaptureMouse(shared_from_this(), EMouseButton::Left);
        }
    }

    if (event.Type == EInputEventType::MouseMove && m_DraggingBarIndex >= 0) {
        UpdateDrag(event.MousePosition);
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_DraggingBarIndex >= 0) {
        UpdateDrag(event.MousePosition);
        EndDrag();
        UpdateHoveredBar(event.MousePosition);
        return FReply::Handled().ReleaseMouseCapture();
    }

    return FReply::Unhandled();
}

bool ImHorizontalSplitter::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) {
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    outPath.push_back(shared_from_this());

    if (HitTestBarIndex(position) >= 0) {
        return true;
    }

    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        if ((*it)->BuildHitTestPath(position, outPath)) {
            return true;
        }
    }

    return true;
}

void ImHorizontalSplitter::Relayout() {
    const auto& children = GetChildren();
    m_PartGeometries.clear();
    m_BarGeometries.clear();

    if (children.empty()) {
        return;
    }

    const FHorizontalSplitterStyle& style = GetEffectiveStyle();
    const float barWidth = SanitizeThickness(style.BarWidth);
    const float totalBarWidth = barWidth * static_cast<float>(children.size() > 0 ? children.size() - 1 : 0);
    const float contentWidth = std::max(0.0f, m_Geometry.Size.X - totalBarWidth);

    std::vector<float> weights;
    std::vector<float> minSizes;
    weights.reserve(children.size());
    minSizes.reserve(children.size());

    float totalWeight = 0.0f;
    for (std::size_t index = 0; index < children.size(); ++index) {
        const auto* slot = dynamic_cast<const ImHorizontalSplitterSlot*>(GetSlotAt(static_cast<int>(index)));
        const float weight = slot ? std::max(0.0f, slot->GetRatio()) : 0.0f;
        weights.push_back(weight);
        totalWeight += weight;

        float paddedMinSize = 0.0f;
        if (slot) {
            paddedMinSize = std::max(0.0f, slot->GetMinSize());
        }
        minSizes.push_back(paddedMinSize);
    }

    if (totalWeight <= LayoutEpsilon) {
        std::fill(weights.begin(), weights.end(), 1.0f);
    }

    std::vector<float> widths;
    DistributeSizes(weights, minSizes, contentWidth, widths);

    float currentX = m_Geometry.Position.X;
    const float top = m_Geometry.Position.Y;
    const float height = m_Geometry.Size.Y;

    for (std::size_t index = 0; index < children.size(); ++index) {
        const float width = index < widths.size() ? std::max(0.0f, widths[index]) : 0.0f;
        m_PartGeometries.emplace_back(currentX, top, width, height);

        if (auto* slot = dynamic_cast<ImHorizontalSplitterSlot*>(GetSlotAt(static_cast<int>(index)))) {
            slot->SetSlotPosition(FVector2(currentX, top));
            slot->SetSlotSize(FVector2(width, height));
            slot->ApplyLayout(children[index].get());
        }

        currentX += width;
        if (index + 1 < children.size()) {
            m_BarGeometries.emplace_back(currentX, top, barWidth, height);
            currentX += barWidth;
        }
    }
}

void ImHorizontalSplitter::ClearHoveredBar() {
    m_HoveredBarIndex = -1;
}

void ImHorizontalSplitter::UpdateHoveredBar(const FVector2& mousePosition) {
    if (!m_Geometry.Contains(mousePosition)) {
        ClearHoveredBar();
        return;
    }

    m_HoveredBarIndex = HitTestBarIndex(mousePosition);
}

int ImHorizontalSplitter::HitTestBarIndex(const FVector2& mousePosition) const {
    for (std::size_t index = 0; index < m_BarGeometries.size(); ++index) {
        if (m_BarGeometries[index].Contains(mousePosition)) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

void ImHorizontalSplitter::BeginDrag(int barIndex, const FVector2& mousePosition) {
    if (barIndex < 0 ||
        barIndex + 1 >= static_cast<int>(m_PartGeometries.size())) {
        return;
    }

    auto* leftSlot = dynamic_cast<ImHorizontalSplitterSlot*>(GetSlotAt(barIndex));
    auto* rightSlot = dynamic_cast<ImHorizontalSplitterSlot*>(GetSlotAt(barIndex + 1));
    if (!leftSlot || !rightSlot) {
        return;
    }

    m_DraggingBarIndex = barIndex;
    m_DragStartMouseX = mousePosition.X;
    m_DragStartLeftWidth = m_PartGeometries[barIndex].Size.X;
    m_DragStartRightWidth = m_PartGeometries[barIndex + 1].Size.X;
    const float totalRatio =
        std::max(0.0f, leftSlot->GetRatio()) +
        std::max(0.0f, rightSlot->GetRatio());
    m_DragStartTotalRatio = totalRatio > LayoutEpsilon ? totalRatio : 2.0f;
    m_HoveredBarIndex = barIndex;
}

void ImHorizontalSplitter::UpdateDrag(const FVector2& mousePosition) {
    if (m_DraggingBarIndex < 0) {
        return;
    }

    auto* leftSlot = dynamic_cast<ImHorizontalSplitterSlot*>(GetSlotAt(m_DraggingBarIndex));
    auto* rightSlot = dynamic_cast<ImHorizontalSplitterSlot*>(GetSlotAt(m_DraggingBarIndex + 1));
    if (!leftSlot || !rightSlot) {
        return;
    }

    const float pairWidth = m_DragStartLeftWidth + m_DragStartRightWidth;
    if (pairWidth <= LayoutEpsilon) {
        return;
    }

    const float minLeft = std::max(0.0f, leftSlot->GetMinSize());
    const float minRight = std::max(0.0f, rightSlot->GetMinSize());
    float newLeftWidth = m_DragStartLeftWidth + (mousePosition.X - m_DragStartMouseX);

    if (pairWidth <= minLeft + minRight + LayoutEpsilon) {
        const float totalMin = std::max(LayoutEpsilon, minLeft + minRight);
        newLeftWidth = pairWidth * minLeft / totalMin;
    } else {
        newLeftWidth = std::clamp(newLeftWidth, minLeft, pairWidth - minRight);
    }

    const float normalizedTotalRatio = m_DragStartTotalRatio > LayoutEpsilon
        ? m_DragStartTotalRatio
        : 2.0f;
    const float leftRatio = normalizedTotalRatio * newLeftWidth / pairWidth;

    leftSlot->SetRatio(leftRatio);
    rightSlot->SetRatio(normalizedTotalRatio - leftRatio);
}

void ImHorizontalSplitter::EndDrag() {
    m_DraggingBarIndex = -1;
    m_DragStartTotalRatio = 2.0f;
}

const FHorizontalSplitterStyle& ImHorizontalSplitter::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveHorizontalSplitterStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

void ImHorizontalSplitter::RenderBars(const FPaintContext& paintContext) const {
    const FHorizontalSplitterStyle& style = GetEffectiveStyle();
    for (std::size_t index = 0; index < m_BarGeometries.size(); ++index) {
        const FGeometry& barGeometry = m_BarGeometries[index];
        const FColor& color =
            static_cast<int>(index) == m_DraggingBarIndex ? style.ActiveColor :
            static_cast<int>(index) == m_HoveredBarIndex ? style.HoveredColor :
            style.Color;

        paintContext.DrawContext_.DrawRectFilled(
            barGeometry.GetMin(),
            barGeometry.GetMax(),
            color,
            style.Rounding
        );
    }
}

} // namespace ImWidgetV4

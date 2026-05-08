#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>

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

ImVerticalSplitter::ImVerticalSplitter()
    : ImPanelWidget() {
}

void ImVerticalSplitter::AddChild(const Ptr& child) {
    AddPart(child);
}

void ImVerticalSplitter::AddPartWithSlot(
    const Ptr& child,
    std::unique_ptr<ImVerticalSplitterSlot> slot) {
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImVerticalSplitterSlot>();
    }

    ImWidget::AddChild(child);
    m_Slots.push_back(std::move(slot));
}

ImVerticalSplitterSlot* ImVerticalSplitter::AddPart(
    const Ptr& child,
    float ratio,
    float minSize,
    const FMargin& padding) {
    if (!child) {
        return nullptr;
    }

    auto slot = std::make_unique<ImVerticalSplitterSlot>();
    slot->PaddingLeft = padding.Left;
    slot->PaddingRight = padding.Right;
    slot->PaddingTop = padding.Top;
    slot->PaddingBottom = padding.Bottom;
    slot->SetRatio(ratio);
    slot->SetMinSize(minSize);

    ImVerticalSplitterSlot* slotPtr = slot.get();
    AddPartWithSlot(child, std::move(slot));
    return slotPtr;
}

void ImVerticalSplitter::SetPartMinSize(int index, float minSize) {
    if (auto* slot = dynamic_cast<ImVerticalSplitterSlot*>(GetSlotAt(index))) {
        slot->SetMinSize(minSize);
    }
}

std::unique_ptr<ImSlot> ImVerticalSplitter::CreateSlot() {
    return std::make_unique<ImVerticalSplitterSlot>();
}

void ImVerticalSplitter::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    Relayout();
    RenderChildren(paintContext);
    RenderBars(paintContext);

    if (m_DraggingBarIndex >= 0 || m_HoveredBarIndex >= 0) {
        SetImGuiMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
}

FVector2 ImVerticalSplitter::GetMinSize() const {
    const auto& children = GetChildren();
    if (children.empty()) {
        return FVector2(0.0f, 0.0f);
    }

    const float barHeight = SanitizeThickness(m_Style.BarHeight);
    float totalHeight = barHeight * static_cast<float>(children.size() > 0 ? children.size() - 1 : 0);
    float maxWidth = 0.0f;

    for (std::size_t index = 0; index < children.size(); ++index) {
        const auto* slot = dynamic_cast<const ImVerticalSplitterSlot*>(GetSlotAt(static_cast<int>(index)));
        if (!children[index] || !slot) {
            continue;
        }

        const FVector2 childMinSize = children[index]->GetMinSize();
        const float childWidth = childMinSize.X + slot->PaddingLeft + slot->PaddingRight;
        maxWidth = std::max(maxWidth, childWidth);
        totalHeight += std::max(0.0f, slot->GetMinSize());
    }

    return FVector2(maxWidth, totalHeight);
}

FReply ImVerticalSplitter::OnInputEvent(const FInputEvent& event) {
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

bool ImVerticalSplitter::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) {
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

void ImVerticalSplitter::Relayout() {
    const auto& children = GetChildren();
    m_PartGeometries.clear();
    m_BarGeometries.clear();

    if (children.empty()) {
        return;
    }

    const float barHeight = SanitizeThickness(m_Style.BarHeight);
    const float totalBarHeight = barHeight * static_cast<float>(children.size() > 0 ? children.size() - 1 : 0);
    const float contentHeight = std::max(0.0f, m_Geometry.Size.Y - totalBarHeight);

    std::vector<float> weights;
    std::vector<float> minSizes;
    weights.reserve(children.size());
    minSizes.reserve(children.size());

    float totalWeight = 0.0f;
    for (std::size_t index = 0; index < children.size(); ++index) {
        const auto* slot = dynamic_cast<const ImVerticalSplitterSlot*>(GetSlotAt(static_cast<int>(index)));
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

    std::vector<float> heights;
    DistributeSizes(weights, minSizes, contentHeight, heights);

    float currentY = m_Geometry.Position.Y;
    const float left = m_Geometry.Position.X;
    const float width = m_Geometry.Size.X;

    for (std::size_t index = 0; index < children.size(); ++index) {
        const float height = index < heights.size() ? std::max(0.0f, heights[index]) : 0.0f;
        m_PartGeometries.emplace_back(left, currentY, width, height);

        if (auto* slot = dynamic_cast<ImVerticalSplitterSlot*>(GetSlotAt(static_cast<int>(index)))) {
            slot->SetSlotPosition(FVector2(left, currentY));
            slot->SetSlotSize(FVector2(width, height));
            slot->ApplyLayout(children[index].get());
        }

        currentY += height;
        if (index + 1 < children.size()) {
            m_BarGeometries.emplace_back(left, currentY, width, barHeight);
            currentY += barHeight;
        }
    }
}

void ImVerticalSplitter::ClearHoveredBar() {
    m_HoveredBarIndex = -1;
}

void ImVerticalSplitter::UpdateHoveredBar(const FVector2& mousePosition) {
    if (!m_Geometry.Contains(mousePosition)) {
        ClearHoveredBar();
        return;
    }

    m_HoveredBarIndex = HitTestBarIndex(mousePosition);
}

int ImVerticalSplitter::HitTestBarIndex(const FVector2& mousePosition) const {
    for (std::size_t index = 0; index < m_BarGeometries.size(); ++index) {
        if (m_BarGeometries[index].Contains(mousePosition)) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

void ImVerticalSplitter::BeginDrag(int barIndex, const FVector2& mousePosition) {
    if (barIndex < 0 ||
        barIndex + 1 >= static_cast<int>(m_PartGeometries.size())) {
        return;
    }

    auto* topSlot = dynamic_cast<ImVerticalSplitterSlot*>(GetSlotAt(barIndex));
    auto* bottomSlot = dynamic_cast<ImVerticalSplitterSlot*>(GetSlotAt(barIndex + 1));
    if (!topSlot || !bottomSlot) {
        return;
    }

    m_DraggingBarIndex = barIndex;
    m_DragStartMouseY = mousePosition.Y;
    m_DragStartTopHeight = m_PartGeometries[barIndex].Size.Y;
    m_DragStartBottomHeight = m_PartGeometries[barIndex + 1].Size.Y;
    const float totalRatio =
        std::max(0.0f, topSlot->GetRatio()) +
        std::max(0.0f, bottomSlot->GetRatio());
    m_DragStartTotalRatio = totalRatio > LayoutEpsilon ? totalRatio : 2.0f;
    m_HoveredBarIndex = barIndex;
}

void ImVerticalSplitter::UpdateDrag(const FVector2& mousePosition) {
    if (m_DraggingBarIndex < 0) {
        return;
    }

    auto* topSlot = dynamic_cast<ImVerticalSplitterSlot*>(GetSlotAt(m_DraggingBarIndex));
    auto* bottomSlot = dynamic_cast<ImVerticalSplitterSlot*>(GetSlotAt(m_DraggingBarIndex + 1));
    if (!topSlot || !bottomSlot) {
        return;
    }

    const float pairHeight = m_DragStartTopHeight + m_DragStartBottomHeight;
    if (pairHeight <= LayoutEpsilon) {
        return;
    }

    const float minTop = std::max(0.0f, topSlot->GetMinSize());
    const float minBottom = std::max(0.0f, bottomSlot->GetMinSize());
    float newTopHeight = m_DragStartTopHeight + (mousePosition.Y - m_DragStartMouseY);

    if (pairHeight <= minTop + minBottom + LayoutEpsilon) {
        const float totalMin = std::max(LayoutEpsilon, minTop + minBottom);
        newTopHeight = pairHeight * minTop / totalMin;
    } else {
        newTopHeight = std::clamp(newTopHeight, minTop, pairHeight - minBottom);
    }

    const float normalizedTotalRatio = m_DragStartTotalRatio > LayoutEpsilon
        ? m_DragStartTotalRatio
        : 2.0f;
    const float topRatio = normalizedTotalRatio * newTopHeight / pairHeight;

    topSlot->SetRatio(topRatio);
    bottomSlot->SetRatio(normalizedTotalRatio - topRatio);
}

void ImVerticalSplitter::EndDrag() {
    m_DraggingBarIndex = -1;
    m_DragStartTotalRatio = 2.0f;
}

void ImVerticalSplitter::RenderBars(const FPaintContext& paintContext) const {
    for (std::size_t index = 0; index < m_BarGeometries.size(); ++index) {
        const FGeometry& barGeometry = m_BarGeometries[index];
        const FColor& color =
            static_cast<int>(index) == m_DraggingBarIndex ? m_Style.ActiveColor :
            static_cast<int>(index) == m_HoveredBarIndex ? m_Style.HoveredColor :
            m_Style.Color;

        paintContext.DrawContext_.DrawRectFilled(
            barGeometry.GetMin(),
            barGeometry.GetMax(),
            color,
            m_Style.Rounding
        );
    }
}

} // namespace ImWidgetV4

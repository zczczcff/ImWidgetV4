#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

namespace {

float Clamp01Value(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float WrapHue(float hue)
{
    while (hue < 0.0f) {
        hue += 1.0f;
    }
    while (hue >= 1.0f) {
        hue -= 1.0f;
    }
    return hue;
}

FColor HsvToColor(float hue, float saturation, float value, float alpha)
{
    hue = WrapHue(hue);
    saturation = Clamp01Value(saturation);
    value = Clamp01Value(value);
    alpha = Clamp01Value(alpha);

    if (saturation <= 0.0001f) {
        return FColor(value, value, value, alpha);
    }

    const float scaledHue = hue * 6.0f;
    const int sector = static_cast<int>(std::floor(scaledHue)) % 6;
    const float fraction = scaledHue - std::floor(scaledHue);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - saturation * fraction);
    const float t = value * (1.0f - saturation * (1.0f - fraction));

    switch (sector) {
    case 0: return FColor(value, t, p, alpha);
    case 1: return FColor(q, value, p, alpha);
    case 2: return FColor(p, value, t, alpha);
    case 3: return FColor(p, q, value, alpha);
    case 4: return FColor(t, p, value, alpha);
    default: return FColor(value, p, q, alpha);
    }
}

void ColorToHsv(const FColor& color, float& outHue, float& outSaturation, float& outValue)
{
    const float r = Clamp01Value(color.R);
    const float g = Clamp01Value(color.G);
    const float b = Clamp01Value(color.B);

    const float maxComponent = std::max({r, g, b});
    const float minComponent = std::min({r, g, b});
    const float delta = maxComponent - minComponent;

    outValue = maxComponent;
    if (delta <= 0.0001f) {
        outHue = 0.0f;
        outSaturation = 0.0f;
        return;
    }

    outSaturation = maxComponent <= 0.0001f ? 0.0f : delta / maxComponent;
    if (maxComponent == r) {
        outHue = std::fmod(((g - b) / delta), 6.0f) / 6.0f;
    } else if (maxComponent == g) {
        outHue = (((b - r) / delta) + 2.0f) / 6.0f;
    } else {
        outHue = (((r - g) / delta) + 4.0f) / 6.0f;
    }

    outHue = WrapHue(outHue);
}

FColor LerpColor(const FColor& a, const FColor& b, float t)
{
    return FColor(
        a.R + (b.R - a.R) * t,
        a.G + (b.G - a.G) * t,
        a.B + (b.B - a.B) * t,
        a.A + (b.A - a.A) * t);
}

void DrawCheckerboard(
    DrawContext& drawContext,
    const FGeometry& geometry,
    const FColor& lightColor,
    const FColor& darkColor,
    float cellSize)
{
    const float clampedCell = std::max(2.0f, cellSize);
    const int columns = std::max(1, static_cast<int>(std::ceil(geometry.Size.X / clampedCell)));
    const int rows = std::max(1, static_cast<int>(std::ceil(geometry.Size.Y / clampedCell)));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const bool light = ((row + column) % 2) == 0;
            const FVector2 min(
                geometry.Position.X + column * clampedCell,
                geometry.Position.Y + row * clampedCell);
            const FVector2 max(
                std::min(geometry.Position.X + (column + 1) * clampedCell, geometry.Position.X + geometry.Size.X),
                std::min(geometry.Position.Y + (row + 1) * clampedCell, geometry.Position.Y + geometry.Size.Y));
            drawContext.DrawRectFilled(min, max, light ? lightColor : darkColor, 0.0f);
        }
    }
}

void DrawVerticalColorRamp(
    DrawContext& drawContext,
    const FGeometry& geometry,
    const std::vector<FColor>& stops)
{
    if (stops.size() < 2) {
        return;
    }

    const int segments = static_cast<int>(stops.size()) - 1;
    for (int index = 0; index < segments; ++index) {
        const float t0 = static_cast<float>(index) / static_cast<float>(segments);
        const float t1 = static_cast<float>(index + 1) / static_cast<float>(segments);
        const FVector2 min(
            geometry.Position.X,
            geometry.Position.Y + geometry.Size.Y * t0);
        const FVector2 max(
            geometry.Position.X + geometry.Size.X,
            geometry.Position.Y + geometry.Size.Y * t1);
        drawContext.DrawRectFilled(min, max, LerpColor(stops[index], stops[index + 1], 0.5f), 0.0f);
    }
}

} // namespace

ImColorPicker::ImColorPicker()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
    SyncHsvFromColor();
}

void ImColorPicker::SetColor(const FColor& color)
{
    m_Color = FColor(
        Clamp01(color.R),
        Clamp01(color.G),
        Clamp01(color.B),
        Clamp01(color.A));
    SyncHsvFromColor();
    Invalidate(EInvalidateReason::Paint);
}

void ImColorPicker::SetStyle(const FColorPickerStyle& style)
{
    m_Style = style;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImColorPicker::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    DrawContext& drawContext = paintContext.DrawContext_;
    const FPickerLayout layout = ResolveLayout();

    drawContext.DrawRectFilled(
        m_Geometry.Position,
        m_Geometry.Position + m_Geometry.Size,
        m_Style.BackgroundColor,
        m_Style.CornerRadius);
    drawContext.DrawRect(
        m_Geometry.Position,
        m_Geometry.Position + m_Geometry.Size,
        HasKeyboardFocus() ? m_Style.FocusedOutlineColor : m_Style.BorderColor,
        m_Style.CornerRadius,
        m_Style.BorderThickness);

    const FColor hueColor = HsvToColor(m_Hue, 1.0f, 1.0f, 1.0f);
    const int svColumns = std::max(1, static_cast<int>(std::ceil(layout.SaturationValueRect.Size.X)));
    const int svRows = std::max(1, static_cast<int>(std::ceil(layout.SaturationValueRect.Size.Y)));
    for (int row = 0; row < svRows; ++row) {
        const float value0 = 1.0f - static_cast<float>(row) / static_cast<float>(svRows);
        const float value1 = 1.0f - static_cast<float>(row + 1) / static_cast<float>(svRows);
        for (int column = 0; column < svColumns; ++column) {
            const float saturation0 = static_cast<float>(column) / static_cast<float>(svColumns);
            const float saturation1 = static_cast<float>(column + 1) / static_cast<float>(svColumns);
            const FColor c0 = HsvToColor(m_Hue, saturation0, value0, 1.0f);
            const FColor c1 = HsvToColor(m_Hue, saturation1, value0, 1.0f);
            const FColor c2 = HsvToColor(m_Hue, saturation0, value1, 1.0f);
            const FColor c3 = HsvToColor(m_Hue, saturation1, value1, 1.0f);
            const FColor average(
                (c0.R + c1.R + c2.R + c3.R) * 0.25f,
                (c0.G + c1.G + c2.G + c3.G) * 0.25f,
                (c0.B + c1.B + c2.B + c3.B) * 0.25f,
                1.0f);
            const FVector2 min(
                layout.SaturationValueRect.Position.X + static_cast<float>(column),
                layout.SaturationValueRect.Position.Y + static_cast<float>(row));
            const FVector2 max(min.X + 1.0f, min.Y + 1.0f);
            drawContext.DrawRectFilled(min, max, average, 0.0f);
        }
    }

    const FVector2 svSelector(
        layout.SaturationValueRect.Position.X + m_Saturation * layout.SaturationValueRect.Size.X,
        layout.SaturationValueRect.Position.Y + (1.0f - m_Value) * layout.SaturationValueRect.Size.Y);
    drawContext.DrawCircle(svSelector, m_Style.SelectorRadius + 1.5f, m_Style.SelectorOuterColor, 0, 2.0f);
    drawContext.DrawCircle(svSelector, m_Style.SelectorRadius - 1.0f, m_Style.SelectorInnerColor, 0, 1.0f);

    DrawVerticalColorRamp(
        drawContext,
        layout.HueRect,
        {
            FColor::Red,
            FColor::Yellow,
            FColor::Green,
            FColor::Cyan,
            FColor::Blue,
            FColor::Magenta,
            FColor::Red
        });
    const float hueY = layout.HueRect.Position.Y + m_Hue * layout.HueRect.Size.Y;
    drawContext.DrawRect(
        FVector2(layout.HueRect.Position.X - 1.0f, hueY - 2.0f),
        FVector2(layout.HueRect.Position.X + layout.HueRect.Size.X + 1.0f, hueY + 2.0f),
        m_Style.SelectorOuterColor,
        2.0f,
        2.0f);

    if (layout.bHasAlphaRect) {
        DrawCheckerboard(
            drawContext,
            layout.AlphaRect,
            m_Style.CheckerLightColor,
            m_Style.CheckerDarkColor,
            6.0f);
        const FColor opaque = HsvToColor(m_Hue, m_Saturation, m_Value, 1.0f);
        const int alphaRows = std::max(1, static_cast<int>(std::ceil(layout.AlphaRect.Size.Y)));
        for (int row = 0; row < alphaRows; ++row) {
            const float alpha0 = 1.0f - static_cast<float>(row) / static_cast<float>(alphaRows);
            const float alpha1 = 1.0f - static_cast<float>(row + 1) / static_cast<float>(alphaRows);
            const FColor average = LerpColor(
                FColor(opaque.R, opaque.G, opaque.B, alpha0),
                FColor(opaque.R, opaque.G, opaque.B, alpha1),
                0.5f);
            const FVector2 min(
                layout.AlphaRect.Position.X,
                layout.AlphaRect.Position.Y + static_cast<float>(row));
            const FVector2 max(
                layout.AlphaRect.Position.X + layout.AlphaRect.Size.X,
                min.Y + 1.0f);
            drawContext.DrawRectFilled(min, max, average, 0.0f);
        }

        const float alphaY = layout.AlphaRect.Position.Y + (1.0f - m_Alpha) * layout.AlphaRect.Size.Y;
        drawContext.DrawRect(
            FVector2(layout.AlphaRect.Position.X - 1.0f, alphaY - 2.0f),
            FVector2(layout.AlphaRect.Position.X + layout.AlphaRect.Size.X + 1.0f, alphaY + 2.0f),
            m_Style.SelectorOuterColor,
            2.0f,
            2.0f);
    }

}

FVector2 ImColorPicker::GetMinSize() const
{
    return m_Style.MinDesiredSize;
}

FReply ImColorPicker::OnInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {
        if (BeginInteraction(event.MousePosition)) {
            return FReply::Handled()
                .SetKeyboardFocus(shared_from_this())
                .CaptureMouse(shared_from_this(), EMouseButton::Left);
        }
    }

    if (event.Type == EInputEventType::MouseMove &&
        m_ActiveRegion != EActiveRegion::None) {
        UpdateFromMouse(event.MousePosition, true);
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_ActiveRegion != EActiveRegion::None) {
        UpdateFromMouse(event.MousePosition, true);
        EndInteraction(true);
        return FReply::Handled().ReleaseMouseCapture();
    }

    if (event.Type == EInputEventType::KeyDown &&
        HasKeyboardFocus() &&
        HandleKeyboardAdjust(event)) {
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

void ImColorPicker::OnFocusChanged(bool bHasFocus)
{
    ImWidget::OnFocusChanged(bHasFocus);
    Invalidate(EInvalidateReason::Paint);
}

ImColorPicker::FPickerLayout ImColorPicker::ResolveLayout() const
{
    FPickerLayout layout;

    const FVector2 contentOrigin(
        m_Geometry.Position.X + m_Style.Padding.Left,
        m_Geometry.Position.Y + m_Style.Padding.Top);
    const float contentWidth = std::max(
        1.0f,
        m_Geometry.Size.X - (m_Style.Padding.Left + m_Style.Padding.Right));
    const float contentHeight = std::max(
        1.0f,
        m_Geometry.Size.Y - (m_Style.Padding.Top + m_Style.Padding.Bottom));

    const float alphaWidth = m_Style.bShowAlphaBar ? m_Style.AlphaBarWidth : 0.0f;
    const float totalBarWidth =
        m_Style.HueBarWidth +
        (m_Style.bShowAlphaBar ? (m_Style.BarSpacing + alphaWidth) : 0.0f);
    const float availableSvWidth = std::max(1.0f, contentWidth - totalBarWidth - m_Style.BarSpacing);
    const float squareSize = std::max(1.0f, std::min(contentHeight, availableSvWidth));
    const FVector2 origin = contentOrigin;

    layout.SaturationValueRect = FGeometry(origin, FVector2(squareSize, squareSize));
    layout.HueRect = FGeometry(
        FVector2(origin.X + squareSize + m_Style.BarSpacing, origin.Y),
        FVector2(m_Style.HueBarWidth, squareSize));

    if (m_Style.bShowAlphaBar) {
        layout.bHasAlphaRect = true;
        layout.AlphaRect = FGeometry(
            FVector2(layout.HueRect.Position.X + layout.HueRect.Size.X + m_Style.BarSpacing, origin.Y),
            FVector2(m_Style.AlphaBarWidth, squareSize));
    }

    return layout;
}

void ImColorPicker::SyncHsvFromColor()
{
    ColorToHsv(m_Color, m_Hue, m_Saturation, m_Value);
    m_Alpha = Clamp01(m_Color.A);
}

void ImColorPicker::SyncColorFromHsv(bool bBroadcastChanged)
{
    m_Color = HsvToColor(m_Hue, m_Saturation, m_Value, m_Alpha);
    Invalidate(EInvalidateReason::Paint);
    if (bBroadcastChanged) {
        const std::shared_ptr<ImWidget> keepAlive = weak_from_this().lock();
        (void)keepAlive;
        OnColorChanged.Broadcast(*this, m_Color);
    }
}

void ImColorPicker::UpdateFromMouse(const FVector2& mousePosition, bool bBroadcastChanged)
{
    const FPickerLayout layout = ResolveLayout();
    if (m_ActiveRegion == EActiveRegion::SaturationValue) {
        m_Saturation = Clamp01((mousePosition.X - layout.SaturationValueRect.Position.X) / layout.SaturationValueRect.Size.X);
        m_Value = 1.0f - Clamp01((mousePosition.Y - layout.SaturationValueRect.Position.Y) / layout.SaturationValueRect.Size.Y);
    } else if (m_ActiveRegion == EActiveRegion::Hue) {
        m_Hue = Clamp01((mousePosition.Y - layout.HueRect.Position.Y) / layout.HueRect.Size.Y);
    } else if (m_ActiveRegion == EActiveRegion::Alpha && layout.bHasAlphaRect) {
        m_Alpha = 1.0f - Clamp01((mousePosition.Y - layout.AlphaRect.Position.Y) / layout.AlphaRect.Size.Y);
    } else {
        return;
    }

    m_bInteractionChanged = true;
    SyncColorFromHsv(bBroadcastChanged);
}

bool ImColorPicker::BeginInteraction(const FVector2& mousePosition)
{
    const FPickerLayout layout = ResolveLayout();
    if (layout.SaturationValueRect.Contains(mousePosition)) {
        m_ActiveRegion = EActiveRegion::SaturationValue;
    } else if (layout.HueRect.Contains(mousePosition)) {
        m_ActiveRegion = EActiveRegion::Hue;
    } else if (layout.bHasAlphaRect && layout.AlphaRect.Contains(mousePosition)) {
        m_ActiveRegion = EActiveRegion::Alpha;
    } else {
        m_ActiveRegion = EActiveRegion::None;
        return false;
    }

    m_bInteractionChanged = false;
    UpdateFromMouse(mousePosition, true);
    return true;
}

void ImColorPicker::EndInteraction(bool bCommit)
{
    const bool bShouldCommit = bCommit && m_bInteractionChanged;
    m_ActiveRegion = EActiveRegion::None;
    if (bShouldCommit) {
        const std::shared_ptr<ImWidget> keepAlive = weak_from_this().lock();
        (void)keepAlive;
        OnColorCommitted.Broadcast(*this, m_Color);
    }
    m_bInteractionChanged = false;
}

float ImColorPicker::Clamp01(float value) const
{
    return Clamp01Value(value);
}

bool ImColorPicker::HandleKeyboardAdjust(const FInputEvent& event)
{
    const float fineStep = event.Modifiers.bShift ? (1.0f / 255.0f) : (4.0f / 255.0f);
    bool bChanged = false;

    switch (event.Key) {
    case EKey::Left:
        m_Saturation = Clamp01(m_Saturation - fineStep);
        bChanged = true;
        break;
    case EKey::Right:
        m_Saturation = Clamp01(m_Saturation + fineStep);
        bChanged = true;
        break;
    case EKey::Up:
        if (event.Modifiers.bCtrl) {
            m_Hue = WrapHue(m_Hue - fineStep);
        } else {
            m_Value = Clamp01(m_Value + fineStep);
        }
        bChanged = true;
        break;
    case EKey::Down:
        if (event.Modifiers.bCtrl) {
            m_Hue = WrapHue(m_Hue + fineStep);
        } else {
            m_Value = Clamp01(m_Value - fineStep);
        }
        bChanged = true;
        break;
    case EKey::PageUp:
        m_Alpha = Clamp01(m_Alpha + fineStep);
        bChanged = true;
        break;
    case EKey::PageDown:
        m_Alpha = Clamp01(m_Alpha - fineStep);
        bChanged = true;
        break;
    case EKey::Home:
        m_Saturation = 0.0f;
        m_Value = 1.0f;
        bChanged = true;
        break;
    case EKey::End:
        m_Saturation = 1.0f;
        m_Value = 0.0f;
        bChanged = true;
        break;
    default:
        break;
    }

    if (!bChanged) {
        return false;
    }

    SyncColorFromHsv(true);
    const std::shared_ptr<ImWidget> keepAlive = weak_from_this().lock();
    (void)keepAlive;
    OnColorCommitted.Broadcast(*this, m_Color);
    return true;
}

} // namespace ImWidgetV4

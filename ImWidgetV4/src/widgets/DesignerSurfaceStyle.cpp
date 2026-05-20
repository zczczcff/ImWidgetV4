#include <imwidgetv4/widgets/DesignerSurface.h>

#include <imwidgetv4/style/StyleSet.h>

namespace ImWidgetV4 {

FDesignerSurfaceStyle ResolveDesignerSurfaceStyle(const FStyleSet& styleSet)
{
    FDesignerSurfaceStyle style;

    style.SelectionBorderColor = styleSet.GetColor(
        "Color.DesignerSurface.SelectionBorder",
        style.SelectionBorderColor);
    style.SelectionFillColor = styleSet.GetColor(
        "Color.DesignerSurface.SelectionFill",
        style.SelectionFillColor);
    style.TransformHandleColor = styleSet.GetColor(
        "Color.DesignerSurface.TransformHandle",
        style.TransformHandleColor);
    style.TransformHandleHoveredColor = styleSet.GetColor(
        "Color.DesignerSurface.TransformHandleHovered",
        style.TransformHandleHoveredColor);
    style.TransformHandleActiveColor = styleSet.GetColor(
        "Color.DesignerSurface.TransformHandleActive",
        style.TransformHandleActiveColor);
    style.TransformHandleBorderColor = styleSet.GetColor(
        "Color.DesignerSurface.TransformHandleBorder",
        style.TransformHandleBorderColor);
    style.DropPreviewBorderColor = styleSet.GetColor(
        "Color.DesignerSurface.DropPreviewBorder",
        style.DropPreviewBorderColor);
    style.DropPreviewFillColor = styleSet.GetColor(
        "Color.DesignerSurface.DropPreviewFill",
        style.DropPreviewFillColor);
    style.SelectionBorderThickness = styleSet.GetFloat(
        "Float.DesignerSurface.SelectionBorderThickness",
        style.SelectionBorderThickness);
    style.TransformHandleSize = styleSet.GetFloat(
        "Float.DesignerSurface.TransformHandleSize",
        style.TransformHandleSize);
    style.TransformHandleBorderThickness = styleSet.GetFloat(
        "Float.DesignerSurface.TransformHandleBorderThickness",
        style.TransformHandleBorderThickness);
    style.DropPreviewBorderThickness = styleSet.GetFloat(
        "Float.DesignerSurface.DropPreviewBorderThickness",
        style.DropPreviewBorderThickness);

    return style;
}

} // namespace ImWidgetV4

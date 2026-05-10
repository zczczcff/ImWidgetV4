#include "imwidgetv4/rendering/DrawContext.h"

namespace ImWidgetV4 {

DrawContext::DrawContext(ImDrawList* pDrawList)
    : m_pDrawList(pDrawList) {
}

// ==================== 矩形绘制 ====================

void DrawContext::DrawRectFilled(const ImVec2& vMin, const ImVec2& vMax, ImU32 uColor,
                                 float fRounding, ImDrawFlags nFlags) {
    if (m_pDrawList && uColor != 0) {
        m_pDrawList->AddRectFilled(vMin, vMax, uColor, fRounding, nFlags);
    }
}

void DrawContext::DrawRect(const ImVec2& vMin, const ImVec2& vMax, ImU32 uColor,
                           float fRounding, ImDrawFlags nFlags, float fThickness) {
    if (m_pDrawList && uColor != 0 && fThickness > 0.0f) {
        m_pDrawList->AddRect(vMin, vMax, uColor, fRounding, nFlags, fThickness);
    }
}

void DrawContext::DrawRectangle(const ImVec2& vMin, const ImVec2& vMax,
                                ImU32 uFillColor, ImU32 uBorderColor,
                                float fRounding, ImDrawFlags nFlags,
                                float fBorderThickness) {
    if (!m_pDrawList) {
        return;
    }

    // 先绘制填充
    if (uFillColor != 0) {
        m_pDrawList->AddRectFilled(vMin, vMax, uFillColor, fRounding, nFlags);
    }

    // 再绘制边框
    if (uBorderColor != 0) {
        if (fBorderThickness > 0.0f) {
            m_pDrawList->AddRect(vMin, vMax, uBorderColor, fRounding, nFlags, fBorderThickness);
        }
    }
}

// ==================== 圆形绘制 ====================

void DrawContext::DrawCircleFilled(const ImVec2& vCenter, float fRadius, ImU32 uColor,
                                   int nNumSegments) {
    if (m_pDrawList && uColor != 0 && fRadius > 0.0f) {
        m_pDrawList->AddCircleFilled(vCenter, fRadius, uColor, nNumSegments);
    }
}

void DrawContext::DrawCircle(const ImVec2& vCenter, float fRadius, ImU32 uColor,
                             int nNumSegments, float fThickness) {
    if (m_pDrawList && uColor != 0 && fRadius > 0.0f) {
        m_pDrawList->AddCircle(vCenter, fRadius, uColor, nNumSegments, fThickness);
    }
}

// ==================== 三角形绘制 ====================

void DrawContext::DrawTriangleFilled(const ImVec2& vP1, const ImVec2& vP2, const ImVec2& vP3,
                                     ImU32 uColor) {
    if (m_pDrawList && uColor != 0) {
        m_pDrawList->AddTriangleFilled(vP1, vP2, vP3, uColor);
    }
}

void DrawContext::DrawTriangle(const ImVec2& vP1, const ImVec2& vP2, const ImVec2& vP3,
                               ImU32 uColor, float fThickness) {
    if (m_pDrawList && uColor != 0) {
        m_pDrawList->AddTriangle(vP1, vP2, vP3, uColor, fThickness);
    }
}

// ==================== 线条绘制 ====================

void DrawContext::DrawLine(const ImVec2& vP1, const ImVec2& vP2, ImU32 uColor,
                           float fThickness) {
    if (m_pDrawList && uColor != 0) {
        m_pDrawList->AddLine(vP1, vP2, uColor, fThickness);
    }
}

// ==================== 文本绘制 ====================

void DrawContext::DrawText(const ImVec2& vPos, ImU32 uColor, const std::string& strText,
                           ImFont* pFont) {
    if (!m_pDrawList || strText.empty() || uColor == 0) {
        return;
    }

    if (pFont) {
        m_pDrawList->AddText(pFont, pFont->FontSize, vPos, uColor, strText.c_str());
    } else {
        m_pDrawList->AddText(vPos, uColor, strText.c_str());
    }
}

void DrawContext::DrawText(ImFont* pFont, float fFontSize, const ImVec2& vPos,
                           ImU32 uColor, const std::string& strText) {
    if (!m_pDrawList || strText.empty() || uColor == 0) {
        return;
    }

    if (pFont) {
        m_pDrawList->AddText(pFont, fFontSize, vPos, uColor, strText.c_str());
    } else {
        m_pDrawList->AddText(vPos, uColor, strText.c_str());
    }
}

// ==================== 图像绘制 ====================

void DrawContext::DrawImage(ImTextureID pTextureId, const ImVec2& vMin, const ImVec2& vMax,
                            const ImVec2& vUv0, const ImVec2& vUv1, ImU32 uTintColor) {
    if (m_pDrawList && pTextureId) {
        m_pDrawList->AddImage(pTextureId, vMin, vMax, vUv0, vUv1, uTintColor);
    }
}

// ==================== 裁剪区域管理 ====================

void DrawContext::PushClipRect(const ImVec2& vMin, const ImVec2& vMax,
                               bool bIntersectWithCurrentClip) {
    if (m_pDrawList) {
        m_pDrawList->PushClipRect(vMin, vMax, bIntersectWithCurrentClip);
    }
}

void DrawContext::PopClipRect() {
    if (m_pDrawList) {
        m_pDrawList->PopClipRect();
    }
}

} // namespace ImWidgetV4

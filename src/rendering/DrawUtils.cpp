#include "imwidgetv4/rendering/DrawUtils.h"
#include <cmath>
#include <algorithm>

namespace ImWidgetV4 {

// ==================== 颜色常量 ====================

const ImU32 DrawUtils::COLOR_WHITE = IM_COL32(255, 255, 255, 255);
const ImU32 DrawUtils::COLOR_BLACK = IM_COL32(0, 0, 0, 255);
const ImU32 DrawUtils::COLOR_RED = IM_COL32(255, 0, 0, 255);
const ImU32 DrawUtils::COLOR_GREEN = IM_COL32(0, 255, 0, 255);
const ImU32 DrawUtils::COLOR_BLUE = IM_COL32(0, 0, 255, 255);
const ImU32 DrawUtils::COLOR_YELLOW = IM_COL32(255, 255, 0, 255);
const ImU32 DrawUtils::COLOR_CYAN = IM_COL32(0, 255, 255, 255);
const ImU32 DrawUtils::COLOR_MAGENTA = IM_COL32(255, 0, 255, 255);
const ImU32 DrawUtils::COLOR_TRANSPARENT = IM_COL32(0, 0, 0, 0);

// ==================== 颜色辅助函数 ====================

ImU32 DrawUtils::ColorFromRGBA(int nR, int nG, int nB, int nA) {
    return IM_COL32(nR, nG, nB, nA);
}

ImU32 DrawUtils::ColorFromRGBAf(float fR, float fG, float fB, float fA) {
    return IM_COL32(
        static_cast<int>(fR * 255.0f),
        static_cast<int>(fG * 255.0f),
        static_cast<int>(fB * 255.0f),
        static_cast<int>(fA * 255.0f)
    );
}

void DrawUtils::ColorToRGBA(ImU32 uColor, int& nR, int& nG, int& nB, int& nA) {
    nR = (uColor >> IM_COL32_R_SHIFT) & 0xFF;
    nG = (uColor >> IM_COL32_G_SHIFT) & 0xFF;
    nB = (uColor >> IM_COL32_B_SHIFT) & 0xFF;
    nA = (uColor >> IM_COL32_A_SHIFT) & 0xFF;
}

void DrawUtils::ColorToRGBAf(ImU32 uColor, float& fR, float& fG, float& fB, float& fA) {
    int nR, nG, nB, nA;
    ColorToRGBA(uColor, nR, nG, nB, nA);
    fR = nR / 255.0f;
    fG = nG / 255.0f;
    fB = nB / 255.0f;
    fA = nA / 255.0f;
}

ImU32 DrawUtils::ColorWithAlpha(ImU32 uColor, int nAlpha) {
    return (uColor & 0x00FFFFFF) | (static_cast<ImU32>(nAlpha) << IM_COL32_A_SHIFT);
}

ImU32 DrawUtils::ColorWithAlphaf(ImU32 uColor, float fAlpha) {
    return ColorWithAlpha(uColor, static_cast<int>(fAlpha * 255.0f));
}

ImU32 DrawUtils::ColorLerp(ImU32 uColor1, ImU32 uColor2, float fT) {
    // 限制 t 在 [0, 1] 范围内
    fT = std::max(0.0f, std::min(1.0f, fT));

    int nR1, nG1, nB1, nA1;
    int nR2, nG2, nB2, nA2;
    ColorToRGBA(uColor1, nR1, nG1, nB1, nA1);
    ColorToRGBA(uColor2, nR2, nG2, nB2, nA2);

    int nR = static_cast<int>(nR1 + (nR2 - nR1) * fT);
    int nG = static_cast<int>(nG1 + (nG2 - nG1) * fT);
    int nB = static_cast<int>(nB1 + (nB2 - nB1) * fT);
    int nA = static_cast<int>(nA1 + (nA2 - nA1) * fT);

    return ColorFromRGBA(nR, nG, nB, nA);
}

// ==================== 坐标辅助函数 ====================

bool DrawUtils::RectContains(const ImVec2& vPoint, const ImVec2& vMin, const ImVec2& vMax) {
    return vPoint.x >= vMin.x && vPoint.x <= vMax.x &&
           vPoint.y >= vMin.y && vPoint.y <= vMax.y;
}

bool DrawUtils::RectIntersects(const ImVec2& vMin1, const ImVec2& vMax1,
                                const ImVec2& vMin2, const ImVec2& vMax2) {
    return !(vMax1.x < vMin2.x || vMin1.x > vMax2.x ||
             vMax1.y < vMin2.y || vMin1.y > vMax2.y);
}

void DrawUtils::RectExpand(ImVec2& vMin, ImVec2& vMax, float fAmount) {
    vMin.x -= fAmount;
    vMin.y -= fAmount;
    vMax.x += fAmount;
    vMax.y += fAmount;
}

ImVec2 DrawUtils::RectCenter(const ImVec2& vMin, const ImVec2& vMax) {
    return ImVec2((vMin.x + vMax.x) * 0.5f, (vMin.y + vMax.y) * 0.5f);
}

ImVec2 DrawUtils::RectSize(const ImVec2& vMin, const ImVec2& vMax) {
    return ImVec2(vMax.x - vMin.x, vMax.y - vMin.y);
}

void DrawUtils::RectClip(ImVec2& vMin, ImVec2& vMax,
                         const ImVec2& vClipMin, const ImVec2& vClipMax) {
    vMin.x = std::max(vMin.x, vClipMin.x);
    vMin.y = std::max(vMin.y, vClipMin.y);
    vMax.x = std::min(vMax.x, vClipMax.x);
    vMax.y = std::min(vMax.y, vClipMax.y);
}

// ==================== 向量辅助函数 ====================

float DrawUtils::Distance(const ImVec2& vP1, const ImVec2& vP2) {
    float fDx = vP2.x - vP1.x;
    float fDy = vP2.y - vP1.y;
    return std::sqrt(fDx * fDx + fDy * fDy);
}

float DrawUtils::DistanceSquared(const ImVec2& vP1, const ImVec2& vP2) {
    float fDx = vP2.x - vP1.x;
    float fDy = vP2.y - vP1.y;
    return fDx * fDx + fDy * fDy;
}

ImVec2 DrawUtils::Lerp(const ImVec2& vA, const ImVec2& vB, float fT) {
    return ImVec2(vA.x + (vB.x - vA.x) * fT, vA.y + (vB.y - vA.y) * fT);
}

// ==================== 文本辅助函数 ====================

ImVec2 DrawUtils::CalcTextSize(const std::string& strText,
                               ImFont* pFont,
                               float fFontSize) {
    if (strText.empty()) {
        return ImVec2(0.0f, 0.0f);
    }

    // 检查 ImGui 上下文是否已初始化
    if (ImGui::GetCurrentContext() == nullptr) {
        return ImVec2(0.0f, 0.0f);
    }

    if (pFont) {
        if (fFontSize <= 0.0f) {
            fFontSize = pFont->FontSize;
        }
        return pFont->CalcTextSizeA(fFontSize, FLT_MAX, 0.0f, strText.c_str());
    } else {
        return ImGui::CalcTextSize(strText.c_str());
    }
}

ImFont* DrawUtils::GetDefaultFont() {
    // 检查 ImGui 上下文是否已初始化
    if (ImGui::GetCurrentContext() == nullptr) {
        return nullptr;
    }

    return ImGui::GetFont();
}

ImVec2 DrawUtils::CalcTextAlignedPos(const std::string& strText,
                                     const ImVec2& vMin, const ImVec2& vMax,
                                     float fAlignX, float fAlignY,
                                     ImFont* pFont,
                                     float fFontSize) {
    ImVec2 vTextSize = CalcTextSize(strText, pFont, fFontSize);
    ImVec2 vRectSize = RectSize(vMin, vMax);

    float fX = vMin.x + (vRectSize.x - vTextSize.x) * fAlignX;
    float fY = vMin.y + (vRectSize.y - vTextSize.y) * fAlignY;

    return ImVec2(fX, fY);
}

} // namespace ImWidgetV4

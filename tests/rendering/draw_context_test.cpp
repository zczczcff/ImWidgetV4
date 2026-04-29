#include <gtest/gtest.h>
#include "imwidgetv4/rendering/DrawContext.h"
#include "imwidgetv4/rendering/DrawUtils.h"
#include <imgui.h>

using namespace ImWidgetV4;

/**
 * @brief DrawContext 测试套件
 *
 * 注意：由于 ImDrawList 需要完整的 ImGui 后端才能正常工作，
 * 这些测试主要验证 DrawContext 的 API 接口和参数传递的正确性。
 * 完整的渲染测试需要在实际的 ImGui 应用程序环境中进行。
 */
class DrawContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建一个简单的 DrawContext 用于测试 API
        m_pDrawList = nullptr;
        m_pDrawContext = new DrawContext(m_pDrawList);
    }

    void TearDown() override {
        delete m_pDrawContext;
    }

    ImDrawList* m_pDrawList;
    DrawContext* m_pDrawContext;
};

// ==================== DrawContext 基础测试 ====================

TEST_F(DrawContextTest, ConstructorAndGetDrawList) {
    // 测试构造函数和 GetDrawList 方法
    ASSERT_NE(m_pDrawContext, nullptr);
    EXPECT_EQ(m_pDrawContext->GetDrawList(), m_pDrawList);
}

TEST_F(DrawContextTest, DrawMethodsWithNullDrawList) {
    // 测试当 DrawList 为 nullptr 时，所有绘制方法都不会崩溃
    ImVec2 vMin(10.0f, 10.0f);
    ImVec2 vMax(100.0f, 100.0f);
    ImU32 uColor = IM_COL32(255, 0, 0, 255);

    // 矩形绘制
    EXPECT_NO_THROW(m_pDrawContext->DrawRectFilled(vMin, vMax, uColor));
    EXPECT_NO_THROW(m_pDrawContext->DrawRect(vMin, vMax, uColor));
    EXPECT_NO_THROW(m_pDrawContext->DrawRectangle(vMin, vMax, uColor, uColor));

    // 圆形绘制
    ImVec2 vCenter(50.0f, 50.0f);
    float fRadius = 25.0f;
    EXPECT_NO_THROW(m_pDrawContext->DrawCircleFilled(vCenter, fRadius, uColor));
    EXPECT_NO_THROW(m_pDrawContext->DrawCircle(vCenter, fRadius, uColor));

    // 三角形绘制
    ImVec2 vP1(10.0f, 10.0f);
    ImVec2 vP2(50.0f, 10.0f);
    ImVec2 vP3(30.0f, 50.0f);
    EXPECT_NO_THROW(m_pDrawContext->DrawTriangleFilled(vP1, vP2, vP3, uColor));
    EXPECT_NO_THROW(m_pDrawContext->DrawTriangle(vP1, vP2, vP3, uColor));

    // 线条绘制
    EXPECT_NO_THROW(m_pDrawContext->DrawLine(vP1, vP2, uColor));

    // 文本绘制
    EXPECT_NO_THROW(m_pDrawContext->DrawText(vMin, uColor, "Test"));

    // 图像绘制
    ImTextureID pTextureId = reinterpret_cast<ImTextureID>(1);
    EXPECT_NO_THROW(m_pDrawContext->DrawImage(pTextureId, vMin, vMax));

    // 裁剪区域
    EXPECT_NO_THROW(m_pDrawContext->PushClipRect(vMin, vMax));
    EXPECT_NO_THROW(m_pDrawContext->PopClipRect());
}

// ==================== DrawUtils 颜色测试 ====================

TEST(DrawUtilsTest, ColorFromRGBA) {
    ImU32 uColor = DrawUtils::ColorFromRGBA(255, 128, 64, 255);
    EXPECT_NE(uColor, 0u);

    int nR, nG, nB, nA;
    DrawUtils::ColorToRGBA(uColor, nR, nG, nB, nA);
    EXPECT_EQ(nR, 255);
    EXPECT_EQ(nG, 128);
    EXPECT_EQ(nB, 64);
    EXPECT_EQ(nA, 255);
}

TEST(DrawUtilsTest, ColorFromRGBAf) {
    ImU32 uColor = DrawUtils::ColorFromRGBAf(1.0f, 0.5f, 0.25f, 1.0f);
    EXPECT_NE(uColor, 0u);

    float fR, fG, fB, fA;
    DrawUtils::ColorToRGBAf(uColor, fR, fG, fB, fA);
    EXPECT_NEAR(fR, 1.0f, 0.01f);
    EXPECT_NEAR(fG, 0.5f, 0.01f);
    EXPECT_NEAR(fB, 0.25f, 0.01f);
    EXPECT_NEAR(fA, 1.0f, 0.01f);
}

TEST(DrawUtilsTest, ColorWithAlpha) {
    ImU32 uColor = DrawUtils::ColorFromRGBA(255, 128, 64, 255);
    ImU32 uColorWithAlpha = DrawUtils::ColorWithAlpha(uColor, 128);

    int nR, nG, nB, nA;
    DrawUtils::ColorToRGBA(uColorWithAlpha, nR, nG, nB, nA);
    EXPECT_EQ(nR, 255);
    EXPECT_EQ(nG, 128);
    EXPECT_EQ(nB, 64);
    EXPECT_EQ(nA, 128);
}

TEST(DrawUtilsTest, ColorWithAlphaf) {
    ImU32 uColor = DrawUtils::ColorFromRGBA(255, 128, 64, 255);
    ImU32 uColorWithAlpha = DrawUtils::ColorWithAlphaf(uColor, 0.5f);

    int nR, nG, nB, nA;
    DrawUtils::ColorToRGBA(uColorWithAlpha, nR, nG, nB, nA);
    EXPECT_EQ(nR, 255);
    EXPECT_EQ(nG, 128);
    EXPECT_EQ(nB, 64);
    EXPECT_NEAR(nA, 127, 1); // 允许 1 的误差
}

TEST(DrawUtilsTest, ColorLerp) {
    ImU32 uColor1 = DrawUtils::ColorFromRGBA(0, 0, 0, 255);
    ImU32 uColor2 = DrawUtils::ColorFromRGBA(255, 255, 255, 255);

    // 测试中间值
    ImU32 uColorMid = DrawUtils::ColorLerp(uColor1, uColor2, 0.5f);
    int nR, nG, nB, nA;
    DrawUtils::ColorToRGBA(uColorMid, nR, nG, nB, nA);
    EXPECT_NEAR(nR, 127, 1);
    EXPECT_NEAR(nG, 127, 1);
    EXPECT_NEAR(nB, 127, 1);

    // 测试边界值
    ImU32 uColor0 = DrawUtils::ColorLerp(uColor1, uColor2, 0.0f);
    EXPECT_EQ(uColor0, uColor1);

    ImU32 uColor1Full = DrawUtils::ColorLerp(uColor1, uColor2, 1.0f);
    EXPECT_EQ(uColor1Full, uColor2);
}

TEST(DrawUtilsTest, ColorConstants) {
    EXPECT_NE(DrawUtils::COLOR_WHITE, 0u);
    EXPECT_NE(DrawUtils::COLOR_BLACK, 0u);
    EXPECT_NE(DrawUtils::COLOR_RED, 0u);
    EXPECT_NE(DrawUtils::COLOR_GREEN, 0u);
    EXPECT_NE(DrawUtils::COLOR_BLUE, 0u);
    EXPECT_EQ(DrawUtils::COLOR_TRANSPARENT, 0u);
}

// ==================== DrawUtils 坐标测试 ====================

TEST(DrawUtilsTest, RectContains) {
    ImVec2 vMin(10.0f, 10.0f);
    ImVec2 vMax(100.0f, 100.0f);

    EXPECT_TRUE(DrawUtils::RectContains(ImVec2(50.0f, 50.0f), vMin, vMax));
    EXPECT_TRUE(DrawUtils::RectContains(ImVec2(10.0f, 10.0f), vMin, vMax));
    EXPECT_TRUE(DrawUtils::RectContains(ImVec2(100.0f, 100.0f), vMin, vMax));
    EXPECT_FALSE(DrawUtils::RectContains(ImVec2(5.0f, 50.0f), vMin, vMax));
    EXPECT_FALSE(DrawUtils::RectContains(ImVec2(50.0f, 105.0f), vMin, vMax));
}

TEST(DrawUtilsTest, RectIntersects) {
    ImVec2 vMin1(10.0f, 10.0f);
    ImVec2 vMax1(100.0f, 100.0f);

    // 相交
    EXPECT_TRUE(DrawUtils::RectIntersects(vMin1, vMax1, ImVec2(50.0f, 50.0f), ImVec2(150.0f, 150.0f)));

    // 不相交
    EXPECT_FALSE(DrawUtils::RectIntersects(vMin1, vMax1, ImVec2(110.0f, 110.0f), ImVec2(200.0f, 200.0f)));

    // 完全包含
    EXPECT_TRUE(DrawUtils::RectIntersects(vMin1, vMax1, ImVec2(20.0f, 20.0f), ImVec2(80.0f, 80.0f)));
}

TEST(DrawUtilsTest, RectExpand) {
    ImVec2 vMin(10.0f, 10.0f);
    ImVec2 vMax(100.0f, 100.0f);

    // 扩大
    DrawUtils::RectExpand(vMin, vMax, 5.0f);
    EXPECT_FLOAT_EQ(vMin.x, 5.0f);
    EXPECT_FLOAT_EQ(vMin.y, 5.0f);
    EXPECT_FLOAT_EQ(vMax.x, 105.0f);
    EXPECT_FLOAT_EQ(vMax.y, 105.0f);

    // 缩小
    DrawUtils::RectExpand(vMin, vMax, -5.0f);
    EXPECT_FLOAT_EQ(vMin.x, 10.0f);
    EXPECT_FLOAT_EQ(vMin.y, 10.0f);
    EXPECT_FLOAT_EQ(vMax.x, 100.0f);
    EXPECT_FLOAT_EQ(vMax.y, 100.0f);
}

TEST(DrawUtilsTest, RectCenter) {
    ImVec2 vMin(10.0f, 10.0f);
    ImVec2 vMax(100.0f, 100.0f);

    ImVec2 vCenter = DrawUtils::RectCenter(vMin, vMax);
    EXPECT_FLOAT_EQ(vCenter.x, 55.0f);
    EXPECT_FLOAT_EQ(vCenter.y, 55.0f);
}

TEST(DrawUtilsTest, RectSize) {
    ImVec2 vMin(10.0f, 10.0f);
    ImVec2 vMax(100.0f, 100.0f);

    ImVec2 vSize = DrawUtils::RectSize(vMin, vMax);
    EXPECT_FLOAT_EQ(vSize.x, 90.0f);
    EXPECT_FLOAT_EQ(vSize.y, 90.0f);
}

TEST(DrawUtilsTest, RectClip) {
    ImVec2 vMin(10.0f, 10.0f);
    ImVec2 vMax(100.0f, 100.0f);
    ImVec2 vClipMin(20.0f, 20.0f);
    ImVec2 vClipMax(80.0f, 80.0f);

    DrawUtils::RectClip(vMin, vMax, vClipMin, vClipMax);
    EXPECT_FLOAT_EQ(vMin.x, 20.0f);
    EXPECT_FLOAT_EQ(vMin.y, 20.0f);
    EXPECT_FLOAT_EQ(vMax.x, 80.0f);
    EXPECT_FLOAT_EQ(vMax.y, 80.0f);
}

// ==================== DrawUtils 向量测试 ====================

TEST(DrawUtilsTest, Distance) {
    ImVec2 vP1(0.0f, 0.0f);
    ImVec2 vP2(3.0f, 4.0f);

    float fDistance = DrawUtils::Distance(vP1, vP2);
    EXPECT_FLOAT_EQ(fDistance, 5.0f);
}

TEST(DrawUtilsTest, DistanceSquared) {
    ImVec2 vP1(0.0f, 0.0f);
    ImVec2 vP2(3.0f, 4.0f);

    float fDistanceSquared = DrawUtils::DistanceSquared(vP1, vP2);
    EXPECT_FLOAT_EQ(fDistanceSquared, 25.0f);
}

TEST(DrawUtilsTest, Lerp) {
    ImVec2 vA(0.0f, 0.0f);
    ImVec2 vB(100.0f, 100.0f);

    ImVec2 vMid = DrawUtils::Lerp(vA, vB, 0.5f);
    EXPECT_FLOAT_EQ(vMid.x, 50.0f);
    EXPECT_FLOAT_EQ(vMid.y, 50.0f);

    ImVec2 v0 = DrawUtils::Lerp(vA, vB, 0.0f);
    EXPECT_FLOAT_EQ(v0.x, vA.x);
    EXPECT_FLOAT_EQ(v0.y, vA.y);

    ImVec2 v1 = DrawUtils::Lerp(vA, vB, 1.0f);
    EXPECT_FLOAT_EQ(v1.x, vB.x);
    EXPECT_FLOAT_EQ(v1.y, vB.y);
}

// ==================== DrawUtils 文本测试 ====================

// 注意：文本相关的测试需要完整的 ImGui 后端支持，
// 在单元测试环境中无法正常运行。这些功能应该在实际的 ImGui 应用程序中测试。

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

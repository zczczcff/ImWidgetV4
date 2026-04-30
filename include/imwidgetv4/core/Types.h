#pragma once
#include <imgui.h>
#include <cmath>

namespace ImWidgetV4 {

// 二维向量
struct FVector2 {
    float X = 0.0f;
    float Y = 0.0f;

    FVector2() = default;
    FVector2(float x, float y) : X(x), Y(y) {}

    // 从 ImVec2 转换
    explicit FVector2(const ImVec2& v) : X(v.x), Y(v.y) {}

    // 转换为 ImVec2
    ImVec2 ToImVec2() const { return ImVec2(X, Y); }

    // 运算符重载
    FVector2 operator+(const FVector2& other) const {
        return FVector2(X + other.X, Y + other.Y);
    }

    FVector2 operator-(const FVector2& other) const {
        return FVector2(X - other.X, Y - other.Y);
    }

    FVector2 operator*(float scalar) const {
        return FVector2(X * scalar, Y * scalar);
    }

    FVector2 operator/(float scalar) const {
        return FVector2(X / scalar, Y / scalar);
    }

    FVector2& operator+=(const FVector2& other) {
        X += other.X;
        Y += other.Y;
        return *this;
    }

    FVector2& operator-=(const FVector2& other) {
        X -= other.X;
        Y -= other.Y;
        return *this;
    }

    FVector2& operator*=(float scalar) {
        X *= scalar;
        Y *= scalar;
        return *this;
    }

    FVector2& operator/=(float scalar) {
        X /= scalar;
        Y /= scalar;
        return *this;
    }

    bool operator==(const FVector2& other) const {
        return X == other.X && Y == other.Y;
    }

    bool operator!=(const FVector2& other) const {
        return !(*this == other);
    }

    // 工具方法
    float Length() const {
        return std::sqrt(X * X + Y * Y);
    }

    float LengthSquared() const {
        return X * X + Y * Y;
    }

    FVector2 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return *this / len;
        }
        return FVector2(0.0f, 0.0f);
    }

    float Dot(const FVector2& other) const {
        return X * other.X + Y * other.Y;
    }

    // 静态常量
    static const FVector2 Zero;
    static const FVector2 One;
    static const FVector2 UnitX;
    static const FVector2 UnitY;
};

// 几何信息
struct FGeometry {
    FVector2 Position;
    FVector2 Size;

    FGeometry() = default;
    FGeometry(const FVector2& pos, const FVector2& size)
        : Position(pos), Size(size) {}
    FGeometry(float x, float y, float width, float height)
        : Position(x, y), Size(width, height) {}

    // 工具方法
    bool Contains(const FVector2& point) const {
        return point.X >= Position.X && point.X <= Position.X + Size.X &&
               point.Y >= Position.Y && point.Y <= Position.Y + Size.Y;
    }

    FVector2 GetCenter() const {
        return Position + Size * 0.5f;
    }

    FVector2 GetMin() const {
        return Position;
    }

    FVector2 GetMax() const {
        return Position + Size;
    }

    float GetWidth() const {
        return Size.X;
    }

    float GetHeight() const {
        return Size.Y;
    }

    float GetArea() const {
        return Size.X * Size.Y;
    }

    bool IsValid() const {
        return Size.X > 0.0f && Size.Y > 0.0f;
    }
};

// 颜色
struct FColor {
    float R = 1.0f;
    float G = 1.0f;
    float B = 1.0f;
    float A = 1.0f;

    FColor() = default;
    FColor(float r, float g, float b, float a = 1.0f)
        : R(r), G(g), B(b), A(a) {}

    // 从字节值创建 (0-255)
    static FColor FromBytes(int r, int g, int b, int a = 255) {
        return FColor(
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f
        );
    }

    // 从浮点值创建 (0.0-1.0)
    static FColor FromFloat(float r, float g, float b, float a = 1.0f) {
        return FColor(r, g, b, a);
    }

    // 转换为 ImU32
    ImU32 ToImU32() const {
        return IM_COL32(
            static_cast<int>(R * 255.0f),
            static_cast<int>(G * 255.0f),
            static_cast<int>(B * 255.0f),
            static_cast<int>(A * 255.0f)
        );
    }

    // 从 ImU32 创建
    static FColor FromImU32(ImU32 color) {
        return FColor(
            ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
            ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
            ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
            ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
        );
    }

    // 从 ImVec4 创建
    static FColor FromImVec4(const ImVec4& color) {
        return FColor(color.x, color.y, color.z, color.w);
    }

    // 转换为 ImVec4
    ImVec4 ToImVec4() const {
        return ImVec4(R, G, B, A);
    }

    // 颜色混合
    FColor Lerp(const FColor& other, float t) const {
        return FColor(
            R + (other.R - R) * t,
            G + (other.G - G) * t,
            B + (other.B - B) * t,
            A + (other.A - A) * t
        );
    }

    // 预定义颜色
    static const FColor White;
    static const FColor Black;
    static const FColor Red;
    static const FColor Green;
    static const FColor Blue;
    static const FColor Yellow;
    static const FColor Cyan;
    static const FColor Magenta;
    static const FColor Transparent;
    static const FColor Gray;
};

// 帧信息
struct FFrameInfo {
    FVector2 ViewportPosition {0.0f, 0.0f};
    FVector2 ViewportSize {0.0f, 0.0f};
    float DeltaTime = 0.0f;
    double CurrentTime = 0.0;
    int FrameCount = 0;

    FFrameInfo() = default;
};

// 帧上下文
struct FFrameContext {
    FFrameInfo FrameInfo {};
    ImDrawList* DrawList = nullptr;
    ImGuiIO* ImGuiIo = nullptr;

    FFrameContext() = default;

    bool IsValid() const {
        return DrawList != nullptr && ImGuiIo != nullptr;
    }
};

// 前向声明
class DrawContext;
struct FStyleSet;

// 绘制上下文
struct FPaintContext {
    DrawContext& DrawContext_;
    FGeometry Geometry;
    const FStyleSet* StyleSet;
    FVector2 CursorPosition;
    bool bHasCursorPosition;
    float DeltaTime;

    FPaintContext(
        class DrawContext& drawContext,
        const FGeometry& geometry,
        const FStyleSet* styleSet,
        const FVector2& cursorPosition,
        bool bHasCursorPosition,
        float deltaTime
    )
        : DrawContext_(drawContext)
        , Geometry(geometry)
        , StyleSet(styleSet)
        , CursorPosition(cursorPosition)
        , bHasCursorPosition(bHasCursorPosition)
        , DeltaTime(deltaTime)
    {}
};

} // namespace ImWidgetV4

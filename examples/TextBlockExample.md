# ImTextBlock 使用示例

## 概述

ImTextBlock 是 ImWidgetV4 中的基础文本显示控件，支持文本内容、颜色、字体大小、对齐方式等功能。

## 基本用法

### 1. 创建简单的文本控件

```cpp
#include <imwidgetv4/widgets/TextBlock.h>

// 创建文本控件
auto textBlock = std::make_shared<ImWidgetV4::ImTextBlock>();
textBlock->SetText("Hello, ImWidgetV4!");
textBlock->SetVisible(true);

// 设置几何信息（位置和大小）
ImWidgetV4::FGeometry geometry;
geometry.Position = ImWidgetV4::FVector2(100.0f, 100.0f);
geometry.Size = ImWidgetV4::FVector2(200.0f, 50.0f);
textBlock->SetGeometry(geometry);

// 渲染
textBlock->Render();
```

### 2. 设置文本颜色

```cpp
// 使用预定义颜色
textBlock->SetTextColor(ImWidgetV4::FColor::White);
textBlock->SetTextColor(ImWidgetV4::FColor::Red);
textBlock->SetTextColor(ImWidgetV4::FColor::Yellow);

// 使用自定义颜色（RGBA，范围 0.0-1.0）
textBlock->SetTextColor(ImWidgetV4::FColor(1.0f, 0.5f, 0.0f, 1.0f));  // 橙色

// 从字节值创建颜色（0-255）
textBlock->SetTextColor(ImWidgetV4::FColor::FromBytes(255, 128, 0, 255));  // 橙色
```

### 3. 设置字体大小

```cpp
// 设置字体大小（像素）
textBlock->SetFontSize(16.0f);  // 默认大小
textBlock->SetFontSize(24.0f);  // 大字体
textBlock->SetFontSize(12.0f);  // 小字体
```

### 4. 设置文本对齐

```cpp
// 水平对齐
textBlock->SetTextAlignment(ImWidgetV4::ETextAlignment::Left);    // 左对齐
textBlock->SetTextAlignment(ImWidgetV4::ETextAlignment::Center);  // 居中对齐
textBlock->SetTextAlignment(ImWidgetV4::ETextAlignment::Right);   // 右对齐

// 垂直对齐
textBlock->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Top);     // 顶部对齐
textBlock->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Center);  // 居中对齐
textBlock->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Bottom);  // 底部对齐
```

### 5. 自动换行

```cpp
// 启用自动换行
textBlock->SetWrapText(true);
textBlock->SetText("这是一段很长的文本，当文本超过控件宽度时会自动换行显示。");

// 禁用自动换行（默认）
textBlock->SetWrapText(false);
```

## 完整示例

```cpp
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/Application.h>

void CreateTextBlockExample() {
    // 创建标题文本
    auto titleText = std::make_shared<ImWidgetV4::ImTextBlock>();
    titleText->SetText("ImWidgetV4 示例");
    titleText->SetTextColor(ImWidgetV4::FColor::Yellow);
    titleText->SetFontSize(32.0f);
    titleText->SetTextAlignment(ImWidgetV4::ETextAlignment::Center);
    titleText->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Top);
    
    ImWidgetV4::FGeometry titleGeometry;
    titleGeometry.Position = ImWidgetV4::FVector2(0.0f, 20.0f);
    titleGeometry.Size = ImWidgetV4::FVector2(800.0f, 50.0f);
    titleText->SetGeometry(titleGeometry);
    
    // 创建描述文本
    auto descText = std::make_shared<ImWidgetV4::ImTextBlock>();
    descText->SetText("这是一个基于 Dear ImGui 的现代化控件库");
    descText->SetTextColor(ImWidgetV4::FColor::White);
    descText->SetFontSize(16.0f);
    descText->SetTextAlignment(ImWidgetV4::ETextAlignment::Center);
    descText->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Center);
    
    ImWidgetV4::FGeometry descGeometry;
    descGeometry.Position = ImWidgetV4::FVector2(0.0f, 80.0f);
    descGeometry.Size = ImWidgetV4::FVector2(800.0f, 30.0f);
    descText->SetGeometry(descGeometry);
    
    // 创建多行文本
    auto multilineText = std::make_shared<ImWidgetV4::ImTextBlock>();
    multilineText->SetText(
        "ImTextBlock 支持以下功能：\n"
        "- 文本内容设置\n"
        "- 文本颜色自定义\n"
        "- 字体大小调整\n"
        "- 水平和垂直对齐\n"
        "- 自动换行"
    );
    multilineText->SetTextColor(ImWidgetV4::FColor::FromBytes(200, 200, 200, 255));
    multilineText->SetFontSize(14.0f);
    multilineText->SetTextAlignment(ImWidgetV4::ETextAlignment::Left);
    multilineText->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Top);
    multilineText->SetWrapText(true);
    
    ImWidgetV4::FGeometry multilineGeometry;
    multilineGeometry.Position = ImWidgetV4::FVector2(50.0f, 150.0f);
    multilineGeometry.Size = ImWidgetV4::FVector2(700.0f, 200.0f);
    multilineText->SetGeometry(multilineGeometry);
    
    // 渲染所有文本控件
    titleText->Render();
    descText->Render();
    multilineText->Render();
}
```

## 属性说明

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| Text | std::string | "" | 文本内容 |
| TextColor | FColor | White | 文本颜色 |
| FontSize | float | 16.0f | 字体大小（像素） |
| TextAlignment | ETextAlignment | Left | 水平对齐方式 |
| VerticalAlignment | EVerticalAlignment | Top | 垂直对齐方式 |
| WrapText | bool | false | 是否自动换行 |
| Visible | bool | true | 是否可见 |

## 对齐方式枚举

### ETextAlignment（水平对齐）

- `Left` - 左对齐
- `Center` - 居中对齐
- `Right` - 右对齐

### EVerticalAlignment（垂直对齐）

- `Top` - 顶部对齐
- `Center` - 居中对齐
- `Bottom` - 底部对齐

## 注意事项

1. **几何信息设置**：在调用 `Render()` 之前，必须先设置控件的几何信息（位置和大小）。

2. **文本换行**：启用 `WrapText` 时，文本会根据控件的宽度自动换行。如果控件宽度为 0，则不会换行。

3. **字体大小**：字体大小以像素为单位。默认使用 ImGui 的默认字体。

4. **颜色格式**：颜色使用 RGBA 格式，每个分量的范围是 0.0-1.0。也可以使用 `FromBytes()` 方法从 0-255 的字节值创建颜色。

5. **最小尺寸**：`GetMinSize()` 方法返回文本所需的最小尺寸，可用于布局计算。

## 下一步

- 查看 `ImButton` 控件示例（即将推出）
- 查看布局容器示例（即将推出）
- 查看样式系统文档（即将推出）

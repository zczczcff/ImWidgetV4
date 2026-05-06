# ImTextBlock 浣跨敤绀轰緥

## 姒傝堪

ImTextBlock 鏄?ImWidgetV4 涓殑鍩虹鏂囨湰鏄剧ず鎺т欢锛屾敮鎸佹枃鏈唴瀹广€侀鑹层€佸瓧浣撳ぇ灏忋€佸榻愭柟寮忕瓑鍔熻兘銆?
## 鍩烘湰鐢ㄦ硶

### 1. 鍒涘缓绠€鍗曠殑鏂囨湰鎺т欢

```cpp
#include <imwidgetv4/widgets/TextBlock.h>

// 鍒涘缓鏂囨湰鎺т欢
auto textBlock = std::make_shared<ImWidgetV4::ImTextBlock>();
textBlock->SetText("Hello, ImWidgetV4!");
textBlock->SetVisible(true);

// 璁剧疆鍑犱綍淇℃伅锛堜綅缃拰澶у皬锛?ImWidgetV4::FGeometry geometry;
geometry.Position = ImWidgetV4::FVector2(100.0f, 100.0f);
geometry.Size = ImWidgetV4::FVector2(200.0f, 50.0f);
textBlock->SetGeometry(geometry);

// 娓叉煋
textBlock->Render();
```

### 2. 璁剧疆鏂囨湰棰滆壊

```cpp
// 浣跨敤棰勫畾涔夐鑹?textBlock->SetTextColor(ImWidgetV4::FColor::White);
textBlock->SetTextColor(ImWidgetV4::FColor::Red);
textBlock->SetTextColor(ImWidgetV4::FColor::Yellow);

// 浣跨敤鑷畾涔夐鑹诧紙RGBA锛岃寖鍥?0.0-1.0锛?textBlock->SetTextColor(ImWidgetV4::FColor(1.0f, 0.5f, 0.0f, 1.0f));  // 姗欒壊

// 浠庡瓧鑺傚€煎垱寤洪鑹诧紙0-255锛?textBlock->SetTextColor(ImWidgetV4::FColor::FromBytes(255, 128, 0, 255));  // 姗欒壊
```

### 3. 璁剧疆瀛椾綋澶у皬

```cpp
// 璁剧疆瀛椾綋澶у皬锛堝儚绱狅級
textBlock->SetFontSize(16.0f);  // 榛樿澶у皬
textBlock->SetFontSize(24.0f);  // 澶у瓧浣?textBlock->SetFontSize(12.0f);  // 灏忓瓧浣?```

### 4. 璁剧疆鏂囨湰瀵归綈

```cpp
// 姘村钩瀵归綈
textBlock->SetTextAlignment(ImWidgetV4::ETextAlignment::Left);    // 宸﹀榻?textBlock->SetTextAlignment(ImWidgetV4::ETextAlignment::Center);  // 灞呬腑瀵归綈
textBlock->SetTextAlignment(ImWidgetV4::ETextAlignment::Right);   // 鍙冲榻?
// 鍨傜洿瀵归綈
textBlock->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Top);     // 椤堕儴瀵归綈
textBlock->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Center);  // 灞呬腑瀵归綈
textBlock->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Bottom);  // 搴曢儴瀵归綈
```

### 5. 鑷姩鎹㈣

```cpp
// 鍚敤鑷姩鎹㈣
textBlock->SetWrapText(true);
textBlock->SetText("杩欐槸涓€娈靛緢闀跨殑鏂囨湰锛屽綋鏂囨湰瓒呰繃鎺т欢瀹藉害鏃朵細鑷姩鎹㈣鏄剧ず銆?);

// 绂佺敤鑷姩鎹㈣锛堥粯璁わ級
textBlock->SetWrapText(false);
```

## 瀹屾暣绀轰緥

```cpp
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/Application.h>

void CreateTextBlockExample() {
    // 鍒涘缓鏍囬鏂囨湰
    auto titleText = std::make_shared<ImWidgetV4::ImTextBlock>();
    titleText->SetText("ImWidgetV4 绀轰緥");
    titleText->SetTextColor(ImWidgetV4::FColor::Yellow);
    titleText->SetFontSize(32.0f);
    titleText->SetTextAlignment(ImWidgetV4::ETextAlignment::Center);
    titleText->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Top);
    
    ImWidgetV4::FGeometry titleGeometry;
    titleGeometry.Position = ImWidgetV4::FVector2(0.0f, 20.0f);
    titleGeometry.Size = ImWidgetV4::FVector2(800.0f, 50.0f);
    titleText->SetGeometry(titleGeometry);
    
    // 鍒涘缓鎻忚堪鏂囨湰
    auto descText = std::make_shared<ImWidgetV4::ImTextBlock>();
    descText->SetText("杩欐槸涓€涓熀浜?Dear ImGui 鐨勭幇浠ｅ寲鎺т欢搴?);
    descText->SetTextColor(ImWidgetV4::FColor::White);
    descText->SetFontSize(16.0f);
    descText->SetTextAlignment(ImWidgetV4::ETextAlignment::Center);
    descText->SetVerticalAlignment(ImWidgetV4::EVerticalAlignment::Center);
    
    ImWidgetV4::FGeometry descGeometry;
    descGeometry.Position = ImWidgetV4::FVector2(0.0f, 80.0f);
    descGeometry.Size = ImWidgetV4::FVector2(800.0f, 30.0f);
    descText->SetGeometry(descGeometry);
    
    // 鍒涘缓澶氳鏂囨湰
    auto multilineText = std::make_shared<ImWidgetV4::ImTextBlock>();
    multilineText->SetText(
        "ImTextBlock 鏀寔浠ヤ笅鍔熻兘锛歕n"
        "- 鏂囨湰鍐呭璁剧疆\n"
        "- 鏂囨湰棰滆壊鑷畾涔塡n"
        "- 瀛椾綋澶у皬璋冩暣\n"
        "- 姘村钩鍜屽瀭鐩村榻怽n"
        "- 鑷姩鎹㈣"
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
    
    // 娓叉煋鎵€鏈夋枃鏈帶浠?    titleText->Render();
    descText->Render();
    multilineText->Render();
}
```

## 灞炴€ц鏄?
| 灞炴€?| 绫诲瀷 | 榛樿鍊?| 璇存槑 |
|------|------|--------|------|
| Text | std::string | "" | 鏂囨湰鍐呭 |
| TextColor | FColor | White | 鏂囨湰棰滆壊 |
| FontSize | float | 16.0f | 瀛椾綋澶у皬锛堝儚绱狅級 |
| TextAlignment | ETextAlignment | Left | 姘村钩瀵归綈鏂瑰紡 |
| VerticalAlignment | EVerticalAlignment | Top | 鍨傜洿瀵归綈鏂瑰紡 |
| WrapText | bool | false | 鏄惁鑷姩鎹㈣ |
| Visible | bool | true | 鏄惁鍙 |

## 瀵归綈鏂瑰紡鏋氫妇

### ETextAlignment锛堟按骞冲榻愶級

- `Left` - 宸﹀榻?- `Center` - 灞呬腑瀵归綈
- `Right` - 鍙冲榻?
### EVerticalAlignment锛堝瀭鐩村榻愶級

- `Top` - 椤堕儴瀵归綈
- `Center` - 灞呬腑瀵归綈
- `Bottom` - 搴曢儴瀵归綈

## 娉ㄦ剰浜嬮」

1. **鍑犱綍淇℃伅璁剧疆**锛氬湪璋冪敤 `Render()` 涔嬪墠锛屽繀椤诲厛璁剧疆鎺т欢鐨勫嚑浣曚俊鎭紙浣嶇疆鍜屽ぇ灏忥級銆?
2. **鏂囨湰鎹㈣**锛氬惎鐢?`WrapText` 鏃讹紝鏂囨湰浼氭牴鎹帶浠剁殑瀹藉害鑷姩鎹㈣銆傚鏋滄帶浠跺搴︿负 0锛屽垯涓嶄細鎹㈣銆?
3. **瀛椾綋澶у皬**锛氬瓧浣撳ぇ灏忎互鍍忕礌涓哄崟浣嶃€傞粯璁や娇鐢?ImGui 鐨勯粯璁ゅ瓧浣撱€?
4. **棰滆壊鏍煎紡**锛氶鑹蹭娇鐢?RGBA 鏍煎紡锛屾瘡涓垎閲忕殑鑼冨洿鏄?0.0-1.0銆備篃鍙互浣跨敤 `FromBytes()` 鏂规硶浠?0-255 鐨勫瓧鑺傚€煎垱寤洪鑹层€?
5. **鏈€灏忓昂瀵?*锛歚GetMinSize()` 鏂规硶杩斿洖鏂囨湰鎵€闇€鐨勬渶灏忓昂瀵革紝鍙敤浜庡竷灞€璁＄畻銆?
## 涓嬩竴姝?
- 鏌ョ湅 `ImButton` 鎺т欢绀轰緥锛堝嵆灏嗘帹鍑猴級
- 鏌ョ湅甯冨眬瀹瑰櫒绀轰緥锛堝嵆灏嗘帹鍑猴級
- 鏌ョ湅鏍峰紡绯荤粺鏂囨。锛堝嵆灏嗘帹鍑猴級



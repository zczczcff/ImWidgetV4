#pragma once

#include <imwidgetv4/core/Types.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <imgui.h>

namespace ImWidgetV4 {

struct FSnapshotOptions {
    int Width = 1;
    int Height = 1;
    FColor ClearColor = FColor(0.0f, 0.0f, 0.0f, 0.0f);
};

struct FSnapshotImage {
    int Width = 0;
    int Height = 0;
    std::vector<std::uint8_t> Pixels {};

    void Reset(int width, int height);
};

struct FSnapshotComparison {
    int ExpectedWidth = 0;
    int ExpectedHeight = 0;
    int ActualWidth = 0;
    int ActualHeight = 0;
    std::size_t DifferentPixelCount = 0;
    std::uint8_t MaxChannelDelta = 0;

    bool IsMatch() const;
};

class FSnapshotRenderer {
public:
    static FSnapshotImage Rasterize(const ImDrawData& drawData, const FSnapshotOptions& options);
    static bool SavePng(const std::filesystem::path& filePath, const FSnapshotImage& image);
    static std::uint64_t ComputeHash(const FSnapshotImage& image);
    static FSnapshotComparison Compare(
        const FSnapshotImage& expected,
        const FSnapshotImage& actual,
        std::uint8_t channelTolerance = 0);
};

} // namespace ImWidgetV4

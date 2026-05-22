#pragma once

#include "EnvironmentProbe.h"

#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FEnvironmentProbeFormatOptions {
    std::string TargetLabel = "Target";
    std::string ReadyLabel = "Ready";
    std::string ReadyValue = "Yes";
    std::string NotReadyValue = "No";
    std::string StatusSeparator = " - ";
    bool bIncludeReadyLine = true;
    bool bBlankLineBeforeItems = false;
    bool bIndentDetailsOnSeparateLine = false;
};

std::vector<std::string> FormatEnvironmentProbeReportLines(
    const FEnvironmentProbeReport& report,
    const FEnvironmentProbeFormatOptions& options = {});
std::string FormatEnvironmentProbeItemLine(const FEnvironmentProbeItem& item);
std::string FormatBuildProfileReadinessLabel(
    bool bHasProbeReport,
    const FEnvironmentProbeReport& report,
    bool bRefreshingProbe);

} // namespace ImWidgetV4Editor

#include "EnvironmentProbeFormatter.h"

namespace ImWidgetV4Editor {

std::string FormatEnvironmentProbeItemLine(const FEnvironmentProbeItem& item)
{
    std::string line = item.Label + " [" + ToDisplayString(item.Status) + "]";
    if (!item.Details.empty()) {
        line += " - " + item.Details;
    }
    return line;
}

std::vector<std::string> FormatEnvironmentProbeReportLines(
    const FEnvironmentProbeReport& report,
    const FEnvironmentProbeFormatOptions& options)
{
    std::vector<std::string> lines;
    lines.push_back(options.TargetLabel + ": " + GetTargetPlatformDisplayName(report.TargetPlatform));
    if (options.bIncludeReadyLine) {
        lines.push_back(options.ReadyLabel + ": " + (report.bReady ? options.ReadyValue : options.NotReadyValue));
    }
    if (options.bBlankLineBeforeItems) {
        lines.push_back("");
    }

    for (const FEnvironmentProbeItem& item : report.Items) {
        if (options.bIndentDetailsOnSeparateLine) {
            lines.push_back(item.Label + " [" + ToDisplayString(item.Status) + "]");
            lines.push_back("  " + item.Details);
        } else {
            lines.push_back(item.Label + ": " + ToDisplayString(item.Status) + options.StatusSeparator + item.Details);
        }
    }
    return lines;
}

std::string FormatBuildProfileReadinessLabel(
    bool bHasProbeReport,
    const FEnvironmentProbeReport& report,
    bool bRefreshingProbe)
{
    if (bHasProbeReport) {
        return report.bReady ? "Ready" : "Needs Setup";
    }
    return bRefreshingProbe ? "Refreshing" : "Unknown";
}

} // namespace ImWidgetV4Editor

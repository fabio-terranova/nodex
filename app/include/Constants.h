#ifndef INCLUDE_INCLUDE_CONSTANTS_H_
#define INCLUDE_INCLUDE_CONSTANTS_H_

#include "Filter.h"
#include "imgui.h"
#include <numbers>

/**
 * Constants for the Nodex application.
 */
namespace Nodex::Constants {
// Numbers
constexpr double kTwoPi = 2.0 * std::numbers::pi;

// App settings
constexpr int   kWinWidth{1920};
constexpr int   kWinHeight{1080};
constexpr char  kGlslVersion[] = "#version 330 core";
constexpr float kClearColor[4] = {0.45f, 0.55f, 0.60f, 1.00f};

// Styling
constexpr ImU32 kLinkColor     = IM_COL32(255, 100, 100, 255); // Flashy red
constexpr ImU32 kDragLineColor = IM_COL32(100, 200, 255, 200); // Blue drag line
constexpr float kLinkThickness = 2.0f;
constexpr float kBezierOffset  = 50.0f; // Bezier curve control point offset
constexpr float kPlotWidth     = 200.0f;
constexpr float kPlotHeight    = 150.0f;

// Default parameters (general)
constexpr double kDefaultSamplingFreq = 1000.0;
constexpr int    kDefaultSamples      = 1000;

// Default parameters (MixerNode)
constexpr int    kNumInputs   = 2;
constexpr double kDefaultGain = 1.0;

// Default parameters (SineNode)
constexpr double kDefaultOffset    = 0.0;
constexpr double kDefaultAmplitude = 1.0;
constexpr double kDefaultFrequency = 50.0;
constexpr double kDefaultPhase     = 0.0;

// Default parameters (FilterNode)
constexpr Filter::Mode kDefaultFilterMode  = Filter::Mode::lowpass;
constexpr Filter::Type kDefaultFilterType  = Filter::Type::butter;
constexpr int          kDefaultFilterOrder = 2;
constexpr double       kDefaultCutoffFreq  = 100.0;

} // namespace Nodex::Constants

#endif // INCLUDE_INCLUDE_CONSTANTS_H_

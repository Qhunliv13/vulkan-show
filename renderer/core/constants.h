#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 500;
const int WINDOW_MIN_WIDTH = 400;
const int WINDOW_MIN_HEIGHT = 400;
const int MAX_FRAMES_IN_FLIGHT = 2;

// Aspect ratio modes (like Godot)
enum class AspectRatioMode {
    Keep,        // Keep aspect ratio, add black bars (letterbox/pillarbox)
    Expand,      // Stretch to fill entire window
    KeepWidth,   // Keep width, adapt height
    KeepHeight,  // Keep height, adapt width
    Center       // Center without scaling
};

// Stretch modes (like Godot)
enum class StretchMode {
    Disabled,      // No stretching, keep original size (800x800), fixed pixels ("disabled")
    Scaled,        // Scaled stretch mode: UI scales with window size, uses actual window dimensions
    Fit            // Fit mode: scale to fit, maintain aspect ratio, add black bars if needed ("fit")
};

// Background stretch modes
enum class BackgroundStretchMode {
    Fit,           // Fit mode: keep aspect ratio, fit completely inside window (max size that fits)
    Scaled         // Scaled mode: keep aspect ratio, fill entire window with no gaps (min size that covers)
};


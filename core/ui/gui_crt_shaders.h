#pragma once

#include <string>

// Checks if ReShade DLL and CRT shaders are installed in Flycast directory
bool IsReShadeInstalled();

// Gets the currently active CRT preset key
std::string GetCurrentCRTPreset();

// Sets and saves the active CRT preset
void SetCRTPreset(const std::string& technique_spec);

// Renders the embeddable CRT Shader Suite panel in Video Settings
void gui_settings_crt_shaders();

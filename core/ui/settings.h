/*
	Copyright 2025 flyinghead

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include "game_scanner.h"
#include "gui_util.h"
#include "gui_font.h"
#include "imgui.h"
#include "oslib/i18n.h"
#include <array>
using namespace i18n;

void gui_display_settings();
void gui_settings_general();
void applyCurrentTheme();
void addContentPath(bool start);
void gui_settings_controls(std::array<bool, 4>& mapleDevicesChanges, std::array<std::array<bool, 2>, 4>& expDevicesChanges);
void gui_settings_video();
void gui_settings_audio();
void gui_settings_network();
void gui_settings_about();
void reset_vmus();
void error_popup();

extern GameScanner scanner;
extern bool uiUserScaleUpdated;
extern bool game_started;
extern int insetLeft, insetRight, insetTop, insetBottom;

inline static void header(const char *title)
{
	ImGui::Dummy(ImVec2(0.f, uiScaled(4.f)));
	ImGuiWindow *win = ImGui::GetCurrentWindow();
	const float availW = ImGui::GetContentRegionAvail().x;
	const float barH = ImGui::GetTextLineHeight() + uiScaled(16.f);
	const ImVec2 pMin = ImGui::GetCursorScreenPos();
	const ImVec2 pMax = ImVec2(pMin.x + std::max(availW, uiScaled(120.f)), pMin.y + barH);

	ImDrawList *dl = win->DrawList;
	dl->AddRectFilled(pMin, pMax, IM_COL32(24, 34, 52, 235), uiScaled(7.f));
	dl->AddRect(pMin, pMax, IM_COL32(62, 88, 132, 140), uiScaled(7.f), 0, 1.0f);
	// Left accent bar in studio blue
	dl->AddRectFilled(pMin, ImVec2(pMin.x + uiScaled(5.f), pMax.y), IM_COL32(37, 99, 235, 255), uiScaled(7.f), ImDrawFlags_RoundCornersLeft);

	ImGui::SetCursorScreenPos(ImVec2(pMin.x + uiScaled(16.f), pMin.y + uiScaled(8.f)));
	if (boldFont)
		ImGui::PushFont(boldFont, ImGui::GetStyle().FontSizeBase * 1.06f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.98f, 1.00f, 1.00f));
	ImGui::TextUnformatted(title);
	ImGui::PopStyleColor();
	if (boldFont)
		ImGui::PopFont();

	ImGui::SetCursorScreenPos(ImVec2(pMin.x, pMax.y + uiScaled(8.f)));
}

// ImGui InputText char filter for valid DNS names and IPv4/IPv6 addresses
inline static int dnsCharFilter(ImGuiInputTextCallbackData *data)
{
	ImWchar c = data->EventChar;
	bool good = c == '.' || c == '-' || c == ':' || (c >= '0' && c <= '9')
			|| (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	return static_cast<int>(!good);
}

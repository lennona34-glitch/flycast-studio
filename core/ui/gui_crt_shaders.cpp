#include "gui_crt_shaders.h"
#include "gui.h"
#include "gui_util.h"
#include "IconsFontAwesome6.h"
#include "settings.h"
#include "oslib/i18n.h"

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32) && !defined(TARGET_UWP)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

namespace fs = std::filesystem;
using namespace i18n;

namespace
{
struct CRTPreset
{
	const char* label;
	const char* technique;
	const char* description;
};

static const CRTPreset kCRTPresets[] = {
	{ "CRT-Geom (Arcade Curved Tube & Natural Scanlines)", "GeomCRT@CRTGeom.fx", "Classic curved arcade CRT with subtle phosphors, bloom, and natural scanlines." },
	{ "CRT-Lottes (Bright Trinitron Aperture Grille)", "LottesCRT@CRTLottes.fx", "Bright aperture grille monitor style; optimal for fast arcade fighters and racing." },
	{ "CRT-Easymode (Fast & Crisp Retro Scanlines)", "EasymodeCRT@CRTEasymode.fx", "Zero-latency, ultra-crisp scanline simulation with minimal brightness loss." },
	{ "CRT-Hyllian (Vibrant Dreamcast Glow)", "CRTHyllian@CRTHyllian.fx", "Rich colors, subtle bloom, and authentic 480p Dreamcast VGA / arcade monitor feel." },
	{ "CRTMattias (Curved Retro Arcade Monitor)", "CRTMattias@MattiasCRT.fx", "Authentic curved screen with gentle barrel distortion and phosphor mask." },
	{ "PowerVR2 (Arcade PVR Scanline Mask)", "PowerVR2@PowerVR2.fx", "Custom-tuned for Sega NAOMI and Dreamcast PowerVR2 hardware scanlines." },
	{ "BasicCRT (Subtle Retro TV Lines)", "BasicCRT@BasicCRT.fx", "Lightweight clean scanlines without heavy distortion." },
	{ "RetroCRT (Soft Nostalgic Arcade Glow)", "RetroCRT@RetroCRT.fx", "Warm nostalgic glow and gentle phosphor mask." },
	{ "zfast_crt (Lightweight Pixel-Perfect CRT)", "CRTZFast@zfast_crt.fx", "Lightweight pixel-perfect CRT filter optimized for high frame rates." },
	{ "CRT-NewPixie (Nostalgic Curvature & Trail)", "CRTNewPixie@CRT-NewPixie.fx", "Stylized 90s CRT monitor with subtle phosphor persistence." },
	{ "NTSC Composite (RetroArch NTSC Artifacts)", "NTSC_RetroArch@NTSC_RetroArch.fx", "Composite video signal emulation with classic color fringing and RF artifacts." },
	{ "VHS Distortion (Vintage VHS Camcorder)", "VHS_RA@VHS_RA.fx", "Authentic 90s tape glitch and tracking distortion." },
	{ "Disabled / Raw Digital Pixels", "", "Disables post-processing filters for clean, raw digital rendering." }
};

static const size_t kPresetCount = sizeof(kCRTPresets) / sizeof(kCRTPresets[0]);

static std::atomic<bool> g_is_installing{false};
static std::string g_install_status = "Ready";
static float g_install_progress = 0.0f;
static std::mutex g_status_mutex;

static fs::path GetAppDir()
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	wchar_t path[MAX_PATH];
	if (GetModuleFileNameW(nullptr, path, MAX_PATH) > 0)
	{
		fs::path p(path);
		return p.parent_path();
	}
#endif
	return fs::current_path();
}

static void SetInstallStatus(const std::string& status, float progress = -1.0f)
{
	std::lock_guard<std::mutex> lock(g_status_mutex);
	g_install_status = status;
	if (progress >= 0.0f)
		g_install_progress = progress;
}

static std::string GetInstallStatus(float& out_progress)
{
	std::lock_guard<std::mutex> lock(g_status_mutex);
	out_progress = g_install_progress;
	return g_install_status;
}

} // namespace

bool IsReShadeInstalled()
{
	try
	{
		fs::path appDir = GetAppDir();
		std::error_code ec;
		bool has_dll = fs::exists(appDir / "dxgi.dll", ec) || fs::exists(appDir / "d3d11.dll", ec);
		bool has_shaders = fs::exists(appDir / "reshade-shaders" / "Shaders", ec);
		return has_dll && has_shaders;
	}
	catch (...)
	{
		return false;
	}
}

std::string GetCurrentCRTPreset()
{
	try
	{
		fs::path presetPath = GetAppDir() / "ReShadePreset.ini";
		std::ifstream file(presetPath);
		if (!file.is_open())
			return "";

		std::string line;
		while (std::getline(file, line))
		{
			if (line.rfind("Techniques=", 0) == 0)
			{
				std::string val = line.substr(11);
				while (!val.empty() && (val.back() == '\r' || val.back() == '\n' || val.back() == ' '))
					val.pop_back();
				return val;
			}
		}
	}
	catch (...)
	{
	}
	return "";
}

void SetCRTPreset(const std::string& technique_spec)
{
	try
	{
		fs::path appDir = GetAppDir();
		fs::path presetPath = appDir / "ReShadePreset.ini";

		std::ofstream out(presetPath, std::ios::trunc);
		if (out.is_open())
		{
			out << "Techniques=" << technique_spec << "\n";
			out << "TechniqueSorting=" << technique_spec << "\n";
			out.close();
		}

		// Ensure ReShade.ini exists and references ReShadePreset.ini
		fs::path iniPath = appDir / "ReShade.ini";
		std::error_code ec;
		if (!fs::exists(iniPath, ec))
		{
			std::ofstream iniOut(iniPath, std::ios::trunc);
			if (iniOut.is_open())
			{
				iniOut << "[GENERAL]\n";
				iniOut << "EffectSearchPaths=.\\reshade-shaders\\Shaders\n";
				iniOut << "TextureSearchPaths=.\\reshade-shaders\\Textures\n";
				iniOut << "IntermediateCachePath=.\\reshade-shaders\\Intermediate\n";
				iniOut << "PresetPath=.\\ReShadePreset.ini\n";
				iniOut << "CurrentPresetPath=.\\ReShadePreset.ini\n";
				iniOut << "PerformanceMode=0\n";
				iniOut << "ShowClock=0\n";
				iniOut << "ShowFPS=0\n";
				iniOut << "NoFontScaling=0\n";
				iniOut << "NoReloadOnInit=0\n";
				iniOut << "KeyMenu=36,0,0,0\n";
				iniOut << "KeyOverlay=36,0,0,0\n";
				iniOut << "NoWelcomeScreen=1\n";
				iniOut << "TutorialProgress=4\n";
				iniOut.close();
			}
		}
	}
	catch (...)
	{
	}
}

static void RunDownloadInstallWorker()
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	g_is_installing = true;
	SetInstallStatus("Preparing ReShade CRT Suite download...", 0.05f);

	fs::path appDir = GetAppDir();
	fs::path shadersDir = appDir / "reshade-shaders" / "Shaders";
	fs::path texturesDir = appDir / "reshade-shaders" / "Textures";

	std::error_code ec;
	fs::create_directories(shadersDir, ec);
	fs::create_directories(texturesDir, ec);

	// PowerShell helper script that downloads ReShade 64-bit and RSRetroArch CRT shaders
	std::string script =
		"$ErrorActionPreference = 'Stop'\n"
		"$appDir = '" + appDir.string() + "'\n"
		"Write-Host 'Downloading ReShade...'\n"
		"try {\n"
		"    $rsUrl = 'https://reshade.me/downloads/ReShade_Setup_6.8.0.exe'\n"
		"    $rsExe = Join-Path $appDir 'ReShade_Setup_Temp.exe'\n"
		"    Invoke-WebRequest -Uri $rsUrl -OutFile $rsExe -UserAgent 'Mozilla/5.0'\n"
		"    $sevenZip = 'C:\\Program Files\\7-Zip\\7z.exe'\n"
		"    if (Test-Path $sevenZip) {\n"
		"        & $sevenZip e $rsExe 'ReShade64.dll' -o\"$appDir\" -y\n"
		"        Move-Item -Path (Join-Path $appDir 'ReShade64.dll') -Destination (Join-Path $appDir 'dxgi.dll') -Force\n"
		"    }\n"
		"    Remove-Item $rsExe -Force -ErrorAction SilentlyContinue\n"
		"} catch {\n"
		"    Write-Host 'ReShade binary fetch: ' $_.Exception.Message\n"
		"}\n"
		"try {\n"
		"    $zipUrl = 'https://github.com/Matsilagi/RSRetroArch/archive/refs/heads/main.zip'\n"
		"    $zipFile = Join-Path $appDir 'rs_temp.zip'\n"
		"    Invoke-WebRequest -Uri $zipUrl -OutFile $zipFile -UserAgent 'Mozilla/5.0'\n"
		"    $tempExtract = Join-Path $appDir 'rs_temp_extract'\n"
		"    Expand-Archive -Path $zipFile -DestinationPath $tempExtract -Force\n"
		"    Copy-Item -Path \"$tempExtract\\RSRetroArch-main\\Shaders\\*\" -Destination (Join-Path $appDir 'reshade-shaders\\Shaders') -Recurse -Force\n"
		"    Copy-Item -Path \"$tempExtract\\RSRetroArch-main\\Textures\\*\" -Destination (Join-Path $appDir 'reshade-shaders\\Textures') -Recurse -Force\n"
		"    Remove-Item $tempExtract -Recurse -Force -ErrorAction SilentlyContinue\n"
		"    Remove-Item $zipFile -Force -ErrorAction SilentlyContinue\n"
		"    Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/crosire/reshade-shaders/master/Shaders/ReShade.fxh' -OutFile (Join-Path $appDir 'reshade-shaders\\Shaders\\ReShade.fxh') -UserAgent 'Mozilla/5.0'\n"
		"    Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/crosire/reshade-shaders/master/Shaders/ReShadeUI.fxh' -OutFile (Join-Path $appDir 'reshade-shaders\\Shaders\\ReShadeUI.fxh') -UserAgent 'Mozilla/5.0'\n"
		"} catch {\n"
		"    Write-Host 'CRT Shaders fetch: ' $_.Exception.Message\n"
		"}\n";

	fs::path scriptPath = appDir / "install_reshade_temp.ps1";
	{
		std::ofstream scriptFile(scriptPath);
		if (scriptFile.is_open())
		{
			scriptFile << script;
			scriptFile.close();
		}
	}

	SetInstallStatus("Downloading and extracting ReShade 64-bit and 70+ CRT Shaders...", 0.40f);

	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi = {};
	std::wstring cmd = L"powershell.exe -ExecutionPolicy Bypass -NoProfile -File \"" + scriptPath.wstring() + L"\"";
	std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
	cmdBuf.push_back(0);

	if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, 90000); // Wait up to 90 seconds
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	fs::remove(scriptPath, ec);

	// Ensure default preset is activated if none is currently selected
	if (GetCurrentCRTPreset().empty())
	{
		SetCRTPreset("GeomCRT@CRTGeom.fx");
	}

	if (IsReShadeInstalled())
	{
		SetInstallStatus("Installation Complete! ReShade CRT Suite is active.", 1.0f);
	}
	else
	{
		SetInstallStatus("Installation finished with warnings. Check reshade-shaders folder.", 1.0f);
	}

	g_is_installing = false;
#endif
}

void StartCRTDownloadInstall()
{
	if (g_is_installing)
		return;

	std::thread worker(RunDownloadInstallWorker);
	worker.detach();
}

void gui_settings_crt_shaders()
{
	ImGui::Spacing();
	header(T("CRT & Retro Shaders (ReShade Suite)"));

	const bool is_installed = IsReShadeInstalled();
	float progress = 0.0f;
	std::string status = GetInstallStatus(progress);

	// Description banner
	ImGui::TextWrapped("%s", T("Transform Sega Dreamcast, NAOMI, and Atomiswave games with authentic arcade & CRT television aesthetics. Renders hardware phosphor masks, curved glass tubes, scanlines, bloom, and NTSC color fringing directly over the DirectX 11 / Vulkan swapchain with zero emulation lag."));

	ImGui::Spacing();

	// Status badge row
	if (is_installed)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.85f, 0.40f, 1.0f));
		ImGui::Text("%s %s", ICON_FA_CHECK, T("ReShade CRT Engine: Installed & Active (DirectX 11)"));
		ImGui::PopStyleColor();

		ImGui::SameLine(0, uiScaled(20));
		if (!g_is_installing)
		{
			if (ImGui::Button((std::string(ICON_FA_ROTATE " ") + T("Update / Reinstall Shaders")).c_str(), ScaledVec2(190, 26)))
			{
				StartCRTDownloadInstall();
			}
		}
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.65f, 0.20f, 1.0f));
		ImGui::Text("%s %s", ICON_FA_DOWNLOAD, T("ReShade CRT Engine: Not Installed"));
		ImGui::PopStyleColor();

		ImGui::SameLine(0, uiScaled(20));
		if (!g_is_installing)
		{
			if (ImGui::Button((std::string(ICON_FA_DOWNLOAD " ") + T("Download & Install ReShade + 70+ CRT Shaders")).c_str(), ScaledVec2(320, 28)))
			{
				StartCRTDownloadInstall();
			}
		}
	}

	if (g_is_installing)
	{
		ImGui::Spacing();
		ImGui::ProgressBar(progress, ScaledVec2(-1, 20), status.c_str());
		ImGui::Spacing();
	}
	else if (!status.empty() && status != "Ready")
	{
		ImGui::TextDisabled("%s", status.c_str());
	}

	ImGui::Spacing();

	// Current active preset
	std::string current_tech = GetCurrentCRTPreset();
	int current_idx = 0;
	bool found = false;
	for (size_t i = 0; i < kPresetCount; ++i)
	{
		if (current_tech == kCRTPresets[i].technique)
		{
			current_idx = static_cast<int>(i);
			found = true;
			break;
		}
	}
	if (!found)
	{
		// Default to first preset (CRT-Geom) if matching failed
		current_idx = 0;
	}

	// Preset Selection Combo Box
	const float comboWidth = std::min(uiScaled(460.0f), ImGui::GetContentRegionAvail().x - uiScaled(120.0f));
	ImGui::SetNextItemWidth(comboWidth);
	if (ImGui::BeginCombo(T("Active CRT Preset"), kCRTPresets[current_idx].label))
	{
		for (size_t i = 0; i < kPresetCount; ++i)
		{
			const bool is_selected = (current_idx == static_cast<int>(i));
			if (ImGui::Selectable(kCRTPresets[i].label, is_selected))
			{
				SetCRTPreset(kCRTPresets[i].technique);
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// Description of currently selected preset
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.85f, 1.0f, 1.0f));
	ImGui::TextWrapped("%s %s", ICON_FA_TV, kCRTPresets[current_idx].description);
	ImGui::PopStyleColor();

	ImGui::Spacing();

	// Quick 1-Click Preset Buttons
	ImGui::Text("%s", T("Quick CRT Profiles:"));
	ImGui::SameLine(0, uiScaled(10));

	auto PresetButton = [&](const char* name, const char* tech) {
		bool active = (current_tech == tech);
		if (active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.85f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.55f, 0.95f, 1.0f));
		}
		if (ImGui::Button(name, ScaledVec2(100, 26)))
		{
			SetCRTPreset(tech);
		}
		if (active)
		{
			ImGui::PopStyleColor(2);
		}
		ImGui::SameLine(0, uiScaled(6));
	};

	PresetButton("CRT-Geom", "GeomCRT@CRTGeom.fx");
	PresetButton("CRT-Lottes", "LottesCRT@CRTLottes.fx");
	PresetButton("CRT-Easymode", "EasymodeCRT@CRTEasymode.fx");
	PresetButton("CRT-Hyllian", "CRTHyllian@CRTHyllian.fx");
	PresetButton("PowerVR2", "PowerVR2@PowerVR2.fx");
	PresetButton("Off (Raw)", "");

	ImGui::NewLine();
	ImGui::Spacing();

	// In-game live overlay tip
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.85f, 1.0f));
	ImGui::TextWrapped("%s %s", ICON_FA_KEYBOARD,
		T("Live Shader Fine-Tuning: Press the 'Home' key on your keyboard at any time during gameplay to toggle the ReShade on-screen overlay. You can interactively tweak scanline density, tube curvature, brightness, and mask bloom in real-time!"));
	ImGui::PopStyleColor();
}

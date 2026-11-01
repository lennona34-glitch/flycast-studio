#include "gui_crt_shaders.h"
#include "gui.h"
#include "gui_util.h"
#include "IconsFontAwesome6.h"
#include "settings.h"
#include "cfg/option.h"
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
	{ "Disabled / Raw Digital Pixels", "", "Disables ReShade post-processing for clean, raw digital rendering." },
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
	{ "VHS Distortion (Vintage VHS Camcorder)", "VHS_RA@VHS_RA.fx", "Authentic 90s tape glitch and tracking distortion." }
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
			out << "TechniqueSorting=" << technique_spec << "\n\n";

			// Inject high-resolution parameters so scanlines match modern monitors instead of 320x240
			if (technique_spec.find("GeomCRT") != std::string::npos)
			{
				out << "[CRTGeom.fx]\n";
				out << "texture_sizeX=1440.000000\n";
				out << "texture_sizeY=1080.000000\n";
				out << "video_sizeX=1440.000000\n";
				out << "video_sizeY=1080.000000\n";
				out << "CURVATURE=0.000000\n";
				out << "scanline_weight=0.220000\n";
			}
			else if (technique_spec.find("EasymodeCRT") != std::string::npos)
			{
				out << "[CRTEasymode.fx]\n";
				out << "texture_sizeX=1440.000000\n";
				out << "texture_sizeY=1080.000000\n";
				out << "video_sizeX=1440.000000\n";
				out << "video_sizeY=1080.000000\n";
				out << "SCANLINE_STRENGTH=0.400000\n";
				out << "MASK_STRENGTH=0.200000\n";
			}
			out.close();
		}

		// Ensure ReShade.ini has tutorial disabled in [OVERLAY]
		fs::path iniPath = appDir / "ReShade.ini";
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
			iniOut << "KeyOverlay=36,0,0,0\n\n";
			iniOut << "[OVERLAY]\n";
			iniOut << "TutorialProgress=4\n";
			iniOut << "ShowOverlay=0\n";
			iniOut.close();
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
		WaitForSingleObject(pi.hProcess, 90000);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	fs::remove(scriptPath, ec);

	// Start with empty preset so UI is 100% clean and readable
	SetCRTPreset("");

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

	// SECTION 1: NATIVE DREAMCAST & ARCADE CRT SHADER (Zero UI blur, applies strictly to games)
	header(T("Dreamcast & Arcade CRT Filter (Native In-Engine)"));
	ImGui::TextWrapped("%s", T("Applies authentic retro CRT scanlines and phosphor aperture grille exclusively to the game rendering. All Flycast menus, game library covers, text, and pause buttons remain 100% crisp, sharp, and readable at full resolution!"));

	ImGui::Spacing();
	ImGui::Text("%s", T("Native CRT Profile:"));
	ImGui::SameLine(0, uiScaled(10));

	auto NativeCrtButton = [](const char* label, int filterVal) {
		bool active = (config::CrtFilter == filterVal);
		if (active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.85f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.55f, 0.95f, 1.0f));
		}
		if (ImGui::Button(label, ScaledVec2(120, 28)))
		{
			config::CrtFilter = filterVal;
		}
		if (active)
		{
			ImGui::PopStyleColor(2);
		}
		ImGui::SameLine(0, uiScaled(8));
	};

	NativeCrtButton(T("Off (Crisp 1440p)"), 0);
	NativeCrtButton(T("Arcade CRT"), 1);
	NativeCrtButton(T("Dreamcast VGA"), 2);
	NativeCrtButton(T("Curved Tube"), 3);
	NativeCrtButton(T("Clean Scanlines"), 4);

	ImGui::NewLine();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.75f, 1.0f, 1.0f));
	switch (config::CrtFilter)
	{
	case 1:
		ImGui::TextWrapped("%s %s", ICON_FA_TV, T("Active: Arcade CRT — Trinitron RGB phosphor subpixel mask with natural 480p scanlines."));
		break;
	case 2:
		ImGui::TextWrapped("%s %s", ICON_FA_TV, T("Active: Dreamcast VGA CRT — Clean 480p scanlines with vibrant color saturation and subtle phosphor glow."));
		break;
	case 3:
		ImGui::TextWrapped("%s %s", ICON_FA_TV, T("Active: Curved Tube CRT — Authentic curved arcade cabinet screen with barrel distortion and scanlines."));
		break;
	case 4:
		ImGui::TextWrapped("%s %s", ICON_FA_TV, T("Active: Clean Scanlines — Lightweight 480p line grid without phosphor mask darkening."));
		break;
	default:
		ImGui::TextWrapped("%s %s", ICON_FA_TV, T("Active: Native Raw Rendering — Zero scanlines, razor-sharp internal resolution."));
		break;
	}
	ImGui::PopStyleColor();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// SECTION 2: RESHADE ADVANCED SUITE (Optional external injector)
	header(T("ReShade CRT & Retro Shader Suite (70+ External Shaders)"));

	const bool is_installed = IsReShadeInstalled();
	float progress = 0.0f;
	std::string status = GetInstallStatus(progress);

	if (is_installed)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.85f, 0.40f, 1.0f));
		ImGui::Text("%s %s", ICON_FA_CHECK, T("ReShade Engine: Installed & Active (DirectX 11)"));
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
		ImGui::Text("%s %s", ICON_FA_DOWNLOAD, T("ReShade Engine: Not Installed"));
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
	for (size_t i = 0; i < kPresetCount; ++i)
	{
		if (current_tech == kCRTPresets[i].technique)
		{
			current_idx = static_cast<int>(i);
			break;
		}
	}

	// Preset Selection Combo Box
	const float comboWidth = std::min(uiScaled(460.0f), ImGui::GetContentRegionAvail().x - uiScaled(120.0f));
	ImGui::SetNextItemWidth(comboWidth);
	if (ImGui::BeginCombo(T("ReShade Preset"), kCRTPresets[current_idx].label))
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

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.85f, 1.0f, 1.0f));
	ImGui::TextWrapped("%s %s", ICON_FA_TV, kCRTPresets[current_idx].description);
	ImGui::PopStyleColor();

	ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.85f, 1.0f));
	ImGui::TextWrapped("%s %s", ICON_FA_KEYBOARD,
		T("ReShade Overlay: Press the 'Home' key on your keyboard during gameplay to open the ReShade menu and fine-tune external shaders."));
	ImGui::PopStyleColor();
}

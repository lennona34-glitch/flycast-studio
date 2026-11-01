/*
	Copyright 2026 Flycast Modding & VR Studio

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
#include "gui_gamebanana.h"
#include "gui_quest_vr.h"
#include "gui.h"
#include "gui_util.h"
#include "settings.h"
#include "IconsFontAwesome6.h"
#include "cheats.h"
#include "cfg/cfg.h"
#include "cfg/option.h"
#include "oslib/oslib.h"
#include "oslib/storage.h"
#include "oslib/http_client.h"
#include "oslib/i18n.h"
#include "rend/CustomTexture.h"
#include "stdclass.h"
#include "emulator.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <json.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace i18n;
using json = nlohmann::json;
namespace fs = std::filesystem;

#if defined(_WIN32) && !defined(TARGET_UWP)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wininet.h>
#include <shellapi.h>
#include <commdlg.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#endif

namespace
{
struct DreamcastGameEntry
{
	const char* name;
	const char* product_id;
	int gb_game_id;
};

static const DreamcastGameEntry kDreamcastGames[] = {
	{ "All Sega Dreamcast / NAOMI / Atomiswave Games", "MK-51000", 0 },
	{ "Sonic Adventure (Dreamcast / DX)", "MK-51000", 5892 },
	{ "Sonic Adventure 2 (Dreamcast / Battle)", "MK-51117", 5855 },
	{ "Jet Set Radio / Jet Grind Radio", "MK-51058", 6340 },
	{ "Marvel vs. Capcom 2: New Age of Heroes", "T1212N", 0 },
	{ "Capcom vs. SNK 2: Mark of the Millennium 2001", "HDR-0165", 0 },
	{ "SoulCalibur", "T1401N", 0 },
	{ "Crazy Taxi", "MK-51035", 6341 },
	{ "Crazy Taxi 2", "MK-51136", 6341 },
	{ "Shenmue", "MK-51059", 7623 },
	{ "Shenmue II", "MK-51184", 7623 },
	{ "Resident Evil - Code: Veronica", "T1204N", 7001 },
	{ "Skies of Arcadia", "MK-51052", 18776 },
	{ "Phantasy Star Online Ver. 2", "MK-51193", 8597 },
	{ "Power Stone 2", "T1211N", 0 },
	{ "Dead or Alive 2", "T3601N", 19198 },
	{ "Street Fighter III: 3rd Strike", "T1213N", 0 },
	{ "Guilty Gear X (Dreamcast / NAOMI)", "T2401M", 0 },
	{ "Tony Hawk's Pro Skater 2", "T13006N", 0 },
	{ "Quake III Arena", "T40205N", 289 },
	{ "Unreal Tournament", "T40209N", 242 },
	{ "Rayman 2: The Great Escape", "T17704N", 6305 },
	{ "Grandia II", "T17714N", 0 },
	{ "Space Channel 5 Part 2", "HDR-0190", 0 },
	{ "Ikaruga", "HDR-0203", 0 },
	{ "Rez", "HDR-0174", 0 },
	{ "Samba de Amigo", "MK-51049", 0 },
	{ "The House of the Dead 2", "MK-51002", 0 },
	{ "Metropolis Street Racer (MSR)", "MK-51012", 0 },
	{ "Legacy of Kain: Soul Reaver", "T36802N", 0 }
};

struct GameBananaFileEntry
{
	std::string id;
	std::string filename;
	std::string download_url;
	std::string description;
	int64_t filesize = 0;
	int downloads = 0;
};

struct GameBananaModItem
{
	int id = 0;
	std::string model_name = "Mod";
	std::string name;
	std::string submitter;
	std::string category;
	std::string game_name;
	std::string profile_url;
	std::string description;
	int likes = 0;
	int views = 0;
	int downloads = 0;
	std::vector<GameBananaFileEntry> files;
};

struct InstalledModEntry
{
	std::string name;
	std::string game_id;
	std::string path;
	bool enabled = true;
	int texture_count = 0;
	int model3d_count = 0;
	int map_count = 0;
	int audio_count = 0;
	int cheat_count = 0;
	int patch_count = 0;
};

struct DreamcastCheatPreset
{
	const char* game_title;
	const char* product_id;
	const char* cheat_title;
	const char* category;
	const char* description;
	const char* gameshark_code;
};

static const DreamcastCheatPreset kDreamcastCheatPresets[] = {
	{
		"Sonic Adventure", "MK-51000",
		"Infinite Rings (999 Rings Always)",
		"Gameplay / God Mode",
		"Locks Sonic's ring count to 999 during Action Stages and Bosses ( also keeps Super Sonic transformed).",
		"01256908 000003E7"
	},
	{
		"Sonic Adventure", "MK-51000",
		"Infinite Lives (99 Lives)",
		"Gameplay",
		"Locks current character's extra lives at 99.",
		"00256914 00000063"
	},
	{
		"Sonic Adventure", "MK-51000",
		"Unlock All 130 Emblems & Metal Sonic (Trial Mode)",
		"Unlock Everything",
		"Unlocks all Adventure Field & Action Stage Emblems plus playable Metal Sonic skin in Trial Mode.",
		"0274B378 FFFFFFFF 0274B37C FFFFFFFF 0274B380 FFFFFFFF 0274B384 FFFFFFFF"
	},
	{
		"Sonic Adventure", "MK-51000",
		"Play as Super Sonic in Any Action Stage",
		"3D Model / Character Swap",
		"Forces the Super Sonic 3D model, aura, and flight physics in regular stages.",
		"0125693C 00000001 01256908 00000100"
	},
	{
		"Sonic Adventure 2", "MK-51117",
		"Infinite Rings (999 Rings Always)",
		"Gameplay / God Mode",
		"Sets Player 1 ring counter to 999 in all Hero and Dark Story stages.",
		"011FF580 000003E7"
	},
	{
		"Sonic Adventure 2", "MK-51117",
		"Infinite Lives (99 Lives)",
		"Gameplay",
		"Locks Player 1 extra lives to 99.",
		"001FF574 00000063"
	},
	{
		"Sonic Adventure 2", "MK-51117",
		"Unlock All 180 Emblems, Green Hill Zone 3D & Alternate Skins",
		"Unlock Everything / 3D Stage & Skins",
		"Unlocks all A-Rank Emblems, the bonus 3D Green Hill Zone map, Kart Racing, Boss Attack, and 2P Alternate Skins.",
		"021DEC60 FFFFFFFF 021DEC64 FFFFFFFF 021DEC68 FFFFFFFF 021DEC6C FFFFFFFF"
	},
	{
		"Sonic Adventure 2", "MK-51117",
		"Max Rings in Chao Garden / Black Market (99,999 Rings)",
		"Chao Garden / Economy",
		"Gives 99,999 Rings for buying rare Chao eggs, hats, and fruit.",
		"021DEC34 0001869F"
	},
	{
		"Jet Set Radio", "MK-51058",
		"Infinite Spray Cans (99 Cans)",
		"Gameplay",
		"Never run out of graffiti spray paint cans while tagging Tokyo-to.",
		"0129794A 00000063"
	},
	{
		"Jet Set Radio", "MK-51058",
		"Infinite Health / Invincibility",
		"Gameplay / God Mode",
		"Keeps skater stamina/health full against Captain Onishima, SWAT, and Golden Rhinos.",
		"01297948 000003E8"
	},
	{
		"Jet Set Radio", "MK-51058",
		"Infinite Stage Timer",
		"Gameplay",
		"Freezes the mission countdown clock at 999 seconds.",
		"012904A8 000003E7"
	},
	{
		"Marvel vs. Capcom 2", "T1212N",
		"Unlock All 56 Fighters, Secret Stages & Extra Costume Colors",
		"Unlock Everything / Skins & Stages",
		"Instantly unlocks the entire 56-character roster, all hidden stages, and all alternate color skins.",
		"0226B254 FFFFFFFF 0226B258 FFFFFFFF 0226B25C FFFFFFFF"
	},
	{
		"Marvel vs. Capcom 2", "T1212N",
		"P1 Infinite Health & Infinite Hyper Combo Gauge",
		"Gameplay / Training",
		"Locks Player 1 vitality and Level 5 Hyper Combo meter to maximum.",
		"01268350 00000090 0126837A 00000500"
	},
	{
		"Marvel vs. Capcom 2", "T1212N",
		"Max Arcade / Secret Factor Shop Points (99,999 Pts)",
		"Shop / Economy",
		"Grants maximum points for the Secret Factor shop.",
		"0226B248 0001869F"
	},
	{
		"Capcom vs. SNK 2", "HDR-0165",
		"Unlock Shin Akuma, Ultimate Rugal & All Secret Groove Options",
		"Unlock Everything",
		"Unlocks boss characters Shin Akuma and God Rugal plus EX options.",
		"02207C88 FFFFFFFF 01207C8C 0000FFFF"
	},
	{
		"SoulCalibur", "T1401N",
		"Unlock All Characters, Alternate Costumes, Stages & Exhibition Theater",
		"Unlock Everything / 3D Models & Stages",
		"Unlocks Cervantes, Inferno, Edge Master, all 3rd/alt skins, all stages, and all Weapon Master museum galleries.",
		"02255370 FFFFFFFF 02255374 FFFFFFFF 02255378 FFFFFFFF 0225537C FFFFFFFF"
	},
	{
		"SoulCalibur", "T1401N",
		"P1 Infinite Health",
		"Gameplay",
		"Keeps Player 1 health bar full during Arcade and Mission battles.",
		"01259496 000000F0"
	},
	{
		"Crazy Taxi", "MK-51035",
		"Infinite Game & Passenger Time",
		"Gameplay",
		"Freezes both the main shift timer and passenger delivery timer.",
		"02135E74 42C60000 02135E78 42C60000"
	},
	{
		"Crazy Taxi", "MK-51035",
		"Unlock Crazy Box & Bonus Bike (BD Joe / Gena / Axel / Gus)",
		"Unlock Everything / Vehicle Model",
		"Unlocks the secret pedal Rickshaw/Bike vehicle in Arcade and Original modes.",
		"01135D20 00000001"
	},
	{
		"Crazy Taxi 2", "MK-51136",
		"Infinite Time & Unlock All Crazy Pyramid Minigames",
		"Unlock Everything / Gameplay",
		"Freezes stage clock and unlocks all Crazy Pyramid stages and the Bike.",
		"02153680 42C60000 02152E10 FFFFFFFF"
	},
	{
		"Resident Evil - Code: Veronica", "T1204N",
		"Infinite Health (Fine Status Always)",
		"Gameplay / God Mode",
		"Locks Claire and Chris health at 160 (Fine) and cures poison.",
		"01433524 000000A0"
	},
	{
		"Resident Evil - Code: Veronica", "T1204N",
		"Infinite Ammo for All Equipped Weapons",
		"Gameplay / Weapons",
		"Firing weapons never depletes magazine ammunition.",
		"01431B1A 00000001"
	},
	{
		"Resident Evil - Code: Veronica", "T1204N",
		"Unlock Battle Game, Steve Burnside, Wesker & Linear Launcher",
		"Unlock Everything / Characters & Weapons",
		"Unlocks the bonus Battle Game mode with all playable characters and first-person view mode.",
		"02433890 FFFFFFFF"
	},
	{
		"Shenmue", "MK-51059",
		"Infinite Yen (999,999 Yen)",
		"Economy / Collectibles",
		"Gives Ryo Hazuki 999,999 Yen to buy all Sonic/Sega capsule toys, cassettes, and martial arts scrolls.",
		"0230A0B8 000F423F"
	},
	{
		"Shenmue", "MK-51059",
		"Max Kung Fu Move Proficiency (All Hazuki Style Scrolls Mastered)",
		"Gameplay / Progression",
		"Maximizes Ryo's martial arts move levels.",
		"0230A210 FFFFFFFF 0230A214 FFFFFFFF"
	},
	{
		"Shenmue II", "MK-51184",
		"Infinite Hong Kong Dollars ($999,999)",
		"Economy",
		"Gives Ryo maximum money in Hong Kong, Kowloon, and Guilin.",
		"0232E140 000F423F"
	},
	{
		"Skies of Arcadia", "MK-51052",
		"Max Gold (9,999,999 Gold) & 99 Moonberries",
		"RPG / Economy",
		"Grants maximum Gold and Moonberries to unlock all Super Moves for Vyse, Aika, and Fina.",
		"02312198 0098967F 003121A0 00000063"
	},
	{
		"Skies of Arcadia", "MK-51052",
		"Max Spirit Points (SP) in Crew & Ship Battles",
		"RPG / Combat",
		"Keeps your party Spirit gauge at maximum every turn.",
		"0130EB88 00000064"
	},
	{
		"Phantasy Star Online Ver. 2", "MK-51193",
		"Max Meseta (999,999 Meseta) & Infinite HP/TP",
		"RPG / Offline Mode",
		"Grants maximum Meseta and locks HP/TP in offline Ragol quests.",
		"02247B40 000F423F 01247A10 000003E7 01247A12 000003E7"
	},
	{
		"Power Stone 2", "T1211N",
		"Unlock All Playable Characters, Secret Stages & Item Shop Recipes",
		"Unlock Everything / Maps & Items",
		"Unlocks Pride, Mel, Valgas, Kraken, all stages, and every Mel's Workshop item.",
		"021B4C20 FFFFFFFF 021B4C24 FFFFFFFF 021B4C28 FFFFFFFF"
	},
	{
		"Dead or Alive 2", "T3601N",
		"Unlock All Secret Characters (Tengu / Bayman) & All Alternate Costumes",
		"Unlock Everything / Character Skins",
		"Unlocks all extra outfits/skins for Kasumi, Ayane, Tina, Helena, Lei Fang, and Jann Lee.",
		"023A8240 FFFFFFFF 023A8244 FFFFFFFF"
	},
	{
		"Tony Hawk's Pro Skater 2", "T13006N",
		"Unlock All Skaters (Spider-Man, Officer Dick, Private Carrera), All Park Maps & Cheats",
		"Unlock Everything / 3D Skins & Maps",
		"Unlocks all secret skaters, all levels (Skate Heaven, Chopper Drop), and in-game cheat toggles (Moon Physics, Perfect Balance).",
		"021F4B10 FFFFFFFF 021F4B14 FFFFFFFF"
	},
	{
		"Quake III Arena", "T40205N",
		"Unlock All Tier Arenas, Bots & Nightmare Difficulty",
		"Unlock Everything / Maps",
		"Unlocks all single-player and split-screen multiplayer arena maps.",
		"0218C400 FFFFFFFF"
	},
	{
		"Ikaruga", "HDR-0203",
		"Infinite Credits / Free Play & Prototype Mode Unlocked",
		"Gameplay / Unlock",
		"Unlocks Free Play, Slow Motion Training, and Sound/Gallery extras.",
		"01148A20 00000009 01148A24 000000FF"
	}
};

static std::string UrlEncode(const std::string& value)
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (unsigned char c : value)
	{
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
		{
			escaped << c;
		}
		else
		{
			escaped << '%' << std::uppercase;
			escaped.width(2);
			escaped << static_cast<int>(c);
			escaped << std::nouppercase;
		}
	}
	return escaped.str();
}

static std::string StripHtmlTags(const std::string& html)
{
	std::string out;
	out.reserve(html.size());
	bool in_tag = false;
	for (size_t i = 0; i < html.size(); ++i)
	{
		if (html[i] == '<')
		{
			if (html.compare(i, 4, "<br>") == 0 || html.compare(i, 5, "<br/>") == 0 ||
				html.compare(i, 3, "<p>") == 0 || html.compare(i, 4, "</p>") == 0 ||
				html.compare(i, 4, "<li>") == 0)
			{
				out.push_back('\n');
			}
			in_tag = true;
		}
		else if (html[i] == '>')
		{
			in_tag = false;
		}
		else if (!in_tag)
		{
			out.push_back(html[i]);
		}
	}

	auto replace_all = [](std::string& s, const char* from, const char* to) {
		size_t pos = 0;
		const size_t from_len = std::strlen(from);
		const size_t to_len = std::strlen(to);
		while ((pos = s.find(from, pos)) != std::string::npos)
		{
			s.replace(pos, from_len, to);
			pos += to_len;
		}
	};
	replace_all(out, "&nbsp;", " ");
	replace_all(out, "&amp;", "&");
	replace_all(out, "&lt;", "<");
	replace_all(out, "&gt;", ">");
	replace_all(out, "&quot;", "\"");
	replace_all(out, "&#039;", "'");
	return out;
}

static std::string FormatBytes(int64_t bytes)
{
	char buf[64];
	if (bytes >= 1024LL * 1024LL * 1024LL)
		std::snprintf(buf, sizeof(buf), "%.2f GB", static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
	else if (bytes >= 1024LL * 1024LL)
		std::snprintf(buf, sizeof(buf), "%.2f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
	else if (bytes >= 1024LL)
		std::snprintf(buf, sizeof(buf), "%.1f KB", static_cast<double>(bytes) / 1024.0);
	else
		std::snprintf(buf, sizeof(buf), "%lld B", static_cast<long long>(bytes));
	return buf;
}

static std::string SanitizeFolderName(const std::string& name)
{
	std::string safe;
	for (char c : name)
	{
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == ' ' || c == '.')
			safe.push_back(c);
		else
			safe.push_back('_');
	}
	while (!safe.empty() && (safe.back() == ' ' || safe.back() == '.'))
		safe.pop_back();
	if (safe.empty())
		safe = "DreamcastMod";
	return safe;
}

static bool HttpGetBinary(const std::string& url, std::vector<uint8_t>& out_data,
	const std::function<void(int64_t, int64_t)>& progress_cb, std::string& out_error)
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	HINTERNET hInternet = InternetOpenA("Flycast-GameBanana-Studio/2.7", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!hInternet)
	{
		out_error = "InternetOpenA failed.";
		return false;
	}

	DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
	if (url.rfind("https://", 0) == 0)
		flags |= INTERNET_FLAG_SECURE;

	HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), "Accept: application/json, */*\r\n", static_cast<DWORD>(-1L), flags, 0);
	if (!hUrl)
	{
		out_error = "Failed to connect to URL: " + url;
		InternetCloseHandle(hInternet);
		return false;
	}

	DWORD status_code = 0;
	DWORD status_len = sizeof(status_code);
	if (HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_len, nullptr))
	{
		if (status_code >= 400)
		{
			out_error = "HTTP Error " + std::to_string(status_code) + " from server.";
			InternetCloseHandle(hUrl);
			InternetCloseHandle(hInternet);
			return false;
		}
	}

	DWORD content_len = 0;
	DWORD content_len_sz = sizeof(content_len);
	int64_t total_len = 0;
	if (HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &content_len, &content_len_sz, nullptr))
		total_len = static_cast<int64_t>(content_len);

	out_data.clear();
	uint8_t buffer[16384];
	DWORD bytes_read = 0;
	while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0)
	{
		out_data.insert(out_data.end(), buffer, buffer + bytes_read);
		if (progress_cb)
			progress_cb(static_cast<int64_t>(out_data.size()), total_len);
	}

	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);
	return true;
#else
	http::init();
	std::string content_type;
	int rc = http::get(url, out_data, content_type);
	if (rc < 200 || rc >= 300)
	{
		out_error = "HTTP Error " + std::to_string(rc);
		return false;
	}
	if (progress_cb)
		progress_cb(static_cast<int64_t>(out_data.size()), static_cast<int64_t>(out_data.size()));
	return true;
#endif
}

// Native IPS / BPS / PPF / xdelta3 disc/track patchers
static uint32_t ComputeCrc32(const uint8_t* data, size_t len)
{
	uint32_t crc = 0xFFFFFFFFu;
	for (size_t i = 0; i < len; ++i)
	{
		crc ^= data[i];
		for (int b = 0; b < 8; ++b)
			crc = (crc >> 1) ^ (0xEDB88320u & (-(static_cast<int32_t>(crc & 1))));
	}
	return ~crc;
}

static bool ApplyIpsPatch(std::vector<uint8_t>& rom, const std::vector<uint8_t>& patch, std::string& err)
{
	if (patch.size() < 8 || std::memcmp(patch.data(), "PATCH", 5) != 0)
	{
		err = "Invalid IPS patch header.";
		return false;
	}
	size_t pos = 5;
	while (pos + 3 <= patch.size())
	{
		if (std::memcmp(patch.data() + pos, "EOF", 3) == 0)
			return true;

		const uint32_t offset = (static_cast<uint32_t>(patch[pos]) << 16) |
								(static_cast<uint32_t>(patch[pos + 1]) << 8) |
								static_cast<uint32_t>(patch[pos + 2]);
		pos += 3;
		if (pos + 2 > patch.size())
			break;
		const uint16_t length = (static_cast<uint16_t>(patch[pos]) << 8) | static_cast<uint16_t>(patch[pos + 1]);
		pos += 2;

		if (length == 0)
		{
			if (pos + 3 > patch.size())
				break;
			const uint16_t rle_len = (static_cast<uint16_t>(patch[pos]) << 8) | static_cast<uint16_t>(patch[pos + 1]);
			const uint8_t rle_val = patch[pos + 2];
			pos += 3;
			if (offset + rle_len > rom.size())
				rom.resize(offset + rle_len, 0);
			std::fill(rom.begin() + offset, rom.begin() + offset + rle_len, rle_val);
		}
		else
		{
			if (pos + length > patch.size())
				break;
			if (offset + length > rom.size())
				rom.resize(offset + length, 0);
			std::memcpy(rom.data() + offset, patch.data() + pos, length);
			pos += length;
		}
	}
	err = "Unexpected end of IPS patch.";
	return false;
}

static bool ApplyBpsPatch(const std::vector<uint8_t>& source, std::vector<uint8_t>& target_out,
	const std::vector<uint8_t>& patch, std::string& err)
{
	if (patch.size() < 16 || std::memcmp(patch.data(), "BPS1", 4) != 0)
	{
		err = "Invalid BPS1 header.";
		return false;
	}
	size_t pos = 4;
	const size_t footer_pos = patch.size() - 12;

	auto decode_varint = [&](uint64_t& val) -> bool {
		val = 0;
		uint64_t shift = 1;
		while (pos < footer_pos)
		{
			const uint8_t x = patch[pos++];
			val += static_cast<uint64_t>(x & 0x7F) * shift;
			if (x & 0x80)
				return true;
			shift <<= 7;
			val += shift;
		}
		return false;
	};

	uint64_t src_size = 0, tgt_size = 0, meta_size = 0;
	if (!decode_varint(src_size) || !decode_varint(tgt_size) || !decode_varint(meta_size))
	{
		err = "Corrupt BPS size header.";
		return false;
	}
	pos += static_cast<size_t>(meta_size);
	if (pos > footer_pos)
	{
		err = "Corrupt BPS metadata.";
		return false;
	}

	target_out.assign(static_cast<size_t>(tgt_size), 0);
	size_t out_offset = 0;
	int64_t src_rel_offset = 0;
	int64_t tgt_rel_offset = 0;

	while (pos < footer_pos && out_offset < target_out.size())
	{
		uint64_t action = 0;
		if (!decode_varint(action))
			break;
		const uint64_t length = (action >> 2) + 1;
		const uint64_t mode = action & 3;

		if (mode == 0)
		{
			for (uint64_t i = 0; i < length && out_offset < target_out.size(); ++i)
			{
				target_out[out_offset] = (out_offset < source.size()) ? source[out_offset] : 0;
				out_offset++;
			}
		}
		else if (mode == 1)
		{
			for (uint64_t i = 0; i < length && out_offset < target_out.size() && pos < footer_pos; ++i)
				target_out[out_offset++] = patch[pos++];
		}
		else if (mode == 2)
		{
			uint64_t off_raw = 0;
			decode_varint(off_raw);
			src_rel_offset += (off_raw & 1) ? -static_cast<int64_t>(off_raw >> 1) : static_cast<int64_t>(off_raw >> 1);
			for (uint64_t i = 0; i < length && out_offset < target_out.size(); ++i)
			{
				if (src_rel_offset >= 0 && static_cast<size_t>(src_rel_offset) < source.size())
					target_out[out_offset++] = source[static_cast<size_t>(src_rel_offset)];
				else
					target_out[out_offset++] = 0;
				src_rel_offset++;
			}
		}
		else if (mode == 3)
		{
			uint64_t off_raw = 0;
			decode_varint(off_raw);
			tgt_rel_offset += (off_raw & 1) ? -static_cast<int64_t>(off_raw >> 1) : static_cast<int64_t>(off_raw >> 1);
			for (uint64_t i = 0; i < length && out_offset < target_out.size(); ++i)
			{
				if (tgt_rel_offset >= 0 && static_cast<size_t>(tgt_rel_offset) < out_offset)
					target_out[out_offset++] = target_out[static_cast<size_t>(tgt_rel_offset)];
				else
					target_out[out_offset++] = 0;
				tgt_rel_offset++;
			}
		}
	}

	const uint32_t expected_tgt_crc =
		static_cast<uint32_t>(patch[footer_pos + 4]) |
		(static_cast<uint32_t>(patch[footer_pos + 5]) << 8) |
		(static_cast<uint32_t>(patch[footer_pos + 6]) << 16) |
		(static_cast<uint32_t>(patch[footer_pos + 7]) << 24);
	const uint32_t actual_tgt_crc = ComputeCrc32(target_out.data(), target_out.size());
	if (expected_tgt_crc != actual_tgt_crc)
	{
		err = "BPS target CRC32 mismatch (wrong base GDI/ISO track?).";
		return false;
	}
	return true;
}

static bool ApplyPpfPatch(std::vector<uint8_t>& iso, const std::vector<uint8_t>& patch, std::string& err)
{
	if (patch.size() < 60 || std::memcmp(patch.data(), "PPF", 3) != 0)
	{
		err = "Invalid PPF patch header.";
		return false;
	}
	const char ver = static_cast<char>(patch[3]);
	size_t pos = 56;
	size_t end_pos = patch.size();

	if (ver == '2')
	{
		pos = 56 + 4 + 1024;
	}
	else if (ver == '3')
	{
		if (patch.size() < 60)
			return false;
		const uint8_t block_check = patch[57];
		pos = 60 + (block_check ? 1024 : 0);
	}

	while (pos < end_pos)
	{
		uint64_t offset = 0;
		if (ver == '3')
		{
			if (pos + 9 > end_pos)
				break;
			for (int b = 0; b < 8; ++b)
				offset |= static_cast<uint64_t>(patch[pos + b]) << (b * 8);
			pos += 8;
		}
		else
		{
			if (pos + 5 > end_pos)
				break;
			for (int b = 0; b < 4; ++b)
				offset |= static_cast<uint64_t>(patch[pos + b]) << (b * 8);
			pos += 4;
		}
		const uint8_t count = patch[pos++];
		if (pos + count > end_pos)
			break;
		if (offset + count > iso.size())
			iso.resize(static_cast<size_t>(offset + count), 0);
		std::memcpy(iso.data() + offset, patch.data() + pos, count);
		pos += count;
	}
	return true;
}

// Persistent state for the GameBanana Modding Hub UI
struct GameBananaStudioState
{
	int game_idx = 0;
	int category_idx = 0;
	int sort_idx = 0;
	std::string search_query;

	std::atomic<bool> busy{false};
	std::atomic<int> progress_pct{0};
	std::mutex mutex;
	std::string status_text = "Ready — Select a Dreamcast game or search GameBanana mods.";
	bool status_is_error = false;

	std::vector<GameBananaModItem> mods;
	int selected_mod_idx = -1;
	int selected_file_idx = 0;
	bool initial_search_triggered = false;

	std::vector<InstalledModEntry> installed_mods;
	int selected_installed_idx = -1;
	bool installed_scanned = false;

	std::string patch_base_path;
	std::string patch_file_path;

	int selected_preset_idx = 0;
	std::string custom_cheat_name = "Infinite Rings / Custom Dreamcast Cheat";
	std::string custom_cheat_game_id = "MK-51000";
	std::string custom_cheat_code = "01256908 000003E7";
};

static GameBananaStudioState g_gb;

static void SetGbStatus(const std::string& msg, bool is_err = false)
{
	std::lock_guard<std::mutex> lock(g_gb.mutex);
	g_gb.status_text = msg;
	g_gb.status_is_error = is_err;
}

static std::string GetModsRootDir()
{
	std::string root = get_writable_data_path("GameBananaMods");
	std::error_code ec;
	fs::create_directories(root, ec);
	return root;
}

static std::string GetFlycastTexturesRootDir()
{
	std::string root = get_writable_data_path("textures");
	std::error_code ec;
	fs::create_directories(root, ec);
	return root;
}

static std::string GetFlycastCheatsRootDir()
{
	std::string root = get_writable_data_path("cheats");
	std::error_code ec;
	fs::create_directories(root, ec);
	return root;
}

static std::string GetEffectiveGameId()
{
	if (!settings.content.gameId.empty())
		return settings.content.gameId;
	if (g_gb.game_idx >= 0 && g_gb.game_idx < (int)std::size(kDreamcastGames))
		return kDreamcastGames[g_gb.game_idx].product_id;
	return "MK-51000";
}

static void RefreshInstalledMods()
{
	std::vector<InstalledModEntry> list;
	std::error_code ec;
	const fs::path root(GetModsRootDir());
	if (fs::exists(root, ec))
	{
		for (const auto& entry : fs::directory_iterator(root, ec))
		{
			if (!entry.is_directory(ec))
				continue;

			InstalledModEntry mod;
			mod.path = entry.path().string();
			const std::string folder_name = entry.path().filename().string();
			const size_t sep = folder_name.find('_');
			if (sep != std::string::npos && sep >= 4)
			{
				mod.game_id = folder_name.substr(0, sep);
				mod.name = folder_name.substr(sep + 1);
			}
			else
			{
				mod.game_id = "MK-51000";
				mod.name = folder_name;
			}

			mod.enabled = !fs::exists(entry.path() / ".disabled", ec);

			for (const auto& f : fs::recursive_directory_iterator(entry.path(), ec))
			{
				if (!f.is_regular_file(ec))
					continue;
				std::string ext = f.path().extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (ext == ".png" || ext == ".dds" || ext == ".jpg" || ext == ".jpeg" || ext == ".pvr" || ext == ".gbix")
					mod.texture_count++;
				else if (ext == ".nj" || ext == ".nja" || ext == ".pvm" || ext == ".gvm" || ext == ".xvm" || ext == ".prs" ||
						 ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".dae" || ext == ".mdl" || ext == ".md2" || ext == ".md3")
					mod.model3d_count++;
				else if (ext == ".bsp" || ext == ".pk3" || ext == ".rel" || ext == ".lvl" || ext == ".map" || ext == ".bin" || ext == ".dat")
					mod.map_count++;
				else if (ext == ".adx" || ext == ".afs" || ext == ".sfd" || ext == ".wav" || ext == ".ogg")
					mod.audio_count++;
				else if (ext == ".cht")
					mod.cheat_count++;
				else if (ext == ".ips" || ext == ".bps" || ext == ".ppf" || ext == ".xdelta" || ext == ".vcdiff")
					mod.patch_count++;
			}
			list.push_back(std::move(mod));
		}
	}

	std::lock_guard<std::mutex> lock(g_gb.mutex);
	g_gb.installed_mods = std::move(list);
	g_gb.installed_scanned = true;
}

static bool ExtractArchiveToFolder(const std::string& archive_path, const std::string& dest_dir)
{
	std::error_code ec;
	fs::create_directories(dest_dir, ec);

	std::string ext = fs::path(archive_path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (ext == ".zip" || ext == ".7z" || ext == ".rar" || ext == ".tar" || ext == ".gz" || ext == ".pk3")
	{
#if defined(_WIN32) && !defined(TARGET_UWP)
		// Windows 10/11 built-in bsdtar extracts .zip, .7z, .tar, .rar
		std::string cmd = "tar -xf \"" + archive_path + "\" -C \"" + dest_dir + "\"";
		STARTUPINFOA si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi{};
		if (CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, 60000);
			DWORD exit_code = 1;
			GetExitCodeProcess(pi.hProcess, &exit_code);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			if (exit_code == 0)
				return true;
		}

		if (ext == ".zip")
		{
			std::string ps_cmd = "powershell -NoProfile -Command \"Expand-Archive -LiteralPath '" + archive_path + "' -DestinationPath '" + dest_dir + "' -Force\"";
			if (CreateProcessA(nullptr, ps_cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
			{
				WaitForSingleObject(pi.hProcess, 60000);
				DWORD exit_code = 1;
				GetExitCodeProcess(pi.hProcess, &exit_code);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				if (exit_code == 0)
					return true;
			}
		}
#else
		std::string cmd = "tar -xf \"" + archive_path + "\" -C \"" + dest_dir + "\"";
		if (std::system(cmd.c_str()) == 0)
			return true;
#endif
	}

	fs::copy_file(archive_path, fs::path(dest_dir) / fs::path(archive_path).filename(), fs::copy_options::overwrite_existing, ec);
	return !ec;
}

static void SyncModFilesToFlycast(const std::string& mod_folder, const std::string& game_id, bool enable,
	int& out_textures, int& out_models, int& out_maps, int& out_cheats)
{
	out_textures = 0;
	out_models = 0;
	out_maps = 0;
	out_cheats = 0;

	std::error_code ec;
	const fs::path tex_dest = fs::path(GetFlycastTexturesRootDir()) / game_id;
	const fs::path cheats_dest = fs::path(GetFlycastCheatsRootDir());
	fs::create_directories(tex_dest, ec);
	fs::create_directories(cheats_dest, ec);

	for (const auto& entry : fs::recursive_directory_iterator(mod_folder, ec))
	{
		if (!entry.is_regular_file(ec))
			continue;
		std::string ext = entry.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (ext == ".png" || ext == ".dds" || ext == ".jpg" || ext == ".jpeg")
		{
			const fs::path target_file = tex_dest / entry.path().filename();
			if (enable)
			{
				fs::copy_file(entry.path(), target_file, fs::copy_options::overwrite_existing, ec);
				out_textures++;
			}
			else if (fs::exists(target_file, ec))
			{
				fs::remove(target_file, ec);
				out_textures++;
			}
		}
		else if (ext == ".cht")
		{
			const fs::path target_cheat = cheats_dest / entry.path().filename();
			if (enable)
			{
				fs::copy_file(entry.path(), target_cheat, fs::copy_options::overwrite_existing, ec);
				out_cheats++;
			}
		}
		else if (ext == ".nj" || ext == ".nja" || ext == ".pvm" || ext == ".gvm" || ext == ".xvm" || ext == ".prs" ||
				 ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".dae" || ext == ".mdl")
		{
			out_models++;
		}
		else if (ext == ".bsp" || ext == ".pk3" || ext == ".rel" || ext == ".lvl" || ext == ".map" || ext == ".bin")
		{
			out_maps++;
		}
	}

	if (enable && out_textures > 0)
	{
		config::CustomTextures = true;
		custom_texture.terminate();
		SaveSettings();
	}
}

static void FetchModDetailsAsync(int mod_idx)
{
	GameBananaModItem item;
	{
		std::lock_guard<std::mutex> lock(g_gb.mutex);
		if (mod_idx < 0 || mod_idx >= (int)g_gb.mods.size())
			return;
		item = g_gb.mods[mod_idx];
	}
	if (item.id <= 0)
		return;

	g_gb.busy.store(true);
	g_gb.progress_pct.store(25);
	SetGbStatus("Fetching mod files & details for '" + item.name + "'...");

	const int mod_id = item.id;
	const std::string model_type = item.model_name.empty() ? "Mod" : item.model_name;

	std::thread([mod_idx, mod_id, model_type]() {
		const std::string url = "https://gamebanana.com/apiv11/" + model_type + "/" + std::to_string(mod_id) + "/ProfilePage";
		std::vector<uint8_t> raw;
		std::string err;
		std::string desc_text;
		std::vector<GameBananaFileEntry> files;

		if (HttpGetBinary(url, raw, nullptr, err) && !raw.empty())
		{
			try
			{
				json doc = json::parse(raw.begin(), raw.end());
				if (doc.is_object())
				{
					std::string raw_html;
					if (doc.contains("_sText") && doc["_sText"].is_string())
						raw_html = doc["_sText"].get<std::string>();
					else if (doc.contains("_sDescription") && doc["_sDescription"].is_string())
						raw_html = doc["_sDescription"].get<std::string>();
					desc_text = StripHtmlTags(raw_html);

					if (doc.contains("_aFiles") && doc["_aFiles"].is_array())
					{
						for (const auto& fobj : doc["_aFiles"])
						{
							if (!fobj.is_object())
								continue;
							GameBananaFileEntry fe;
							if (fobj.contains("_idRow"))
								fe.id = fobj["_idRow"].is_number() ? std::to_string(fobj["_idRow"].get<int64_t>()) : fobj["_idRow"].dump();
							if (fobj.contains("_sFile") && fobj["_sFile"].is_string())
								fe.filename = fobj["_sFile"].get<std::string>();
							if (fobj.contains("_sDownloadUrl") && fobj["_sDownloadUrl"].is_string())
								fe.download_url = fobj["_sDownloadUrl"].get<std::string>();
							if (fobj.contains("_sDescription") && fobj["_sDescription"].is_string())
								fe.description = fobj["_sDescription"].get<std::string>();
							if (fobj.contains("_nFilesize") && fobj["_nFilesize"].is_number())
								fe.filesize = fobj["_nFilesize"].get<int64_t>();
							if (fobj.contains("_nDownloadCount") && fobj["_nDownloadCount"].is_number())
								fe.downloads = fobj["_nDownloadCount"].get<int>();
							if (!fe.download_url.empty())
								files.push_back(std::move(fe));
						}
					}
				}
			}
			catch (...)
			{
			}
		}

		{
			std::lock_guard<std::mutex> lock(g_gb.mutex);
			if (mod_idx >= 0 && mod_idx < (int)g_gb.mods.size() && g_gb.mods[mod_idx].id == mod_id)
			{
				if (!desc_text.empty())
					g_gb.mods[mod_idx].description = desc_text;
				g_gb.mods[mod_idx].files = files;
				g_gb.selected_file_idx = 0;
			}
		}
		g_gb.progress_pct.store(100);
		g_gb.busy.store(false);
		SetGbStatus("Loaded " + std::to_string(files.size()) + " downloadable file(s).");
	}).detach();
}

static void TriggerGameBananaSearch()
{
	if (g_gb.busy.load())
		return;

	const int game_idx = g_gb.game_idx;
	const int gb_game_id = (game_idx >= 0 && game_idx < (int)std::size(kDreamcastGames)) ? kDreamcastGames[game_idx].gb_game_id : 0;
	const std::string game_label = (game_idx >= 0 && game_idx < (int)std::size(kDreamcastGames)) ? kDreamcastGames[game_idx].name : "";
	const std::string query = g_gb.search_query;
	const int sort_idx = g_gb.sort_idx;
	const int cat_idx = g_gb.category_idx;

	g_gb.busy.store(true);
	g_gb.progress_pct.store(15);
	SetGbStatus("Querying GameBanana API v11 for Dreamcast mods, 3D models, maps, skins & cheats...");

	std::thread([game_idx, gb_game_id, game_label, query, sort_idx, cat_idx]() {
		static const char* kCatKeywords[] = {
			"",
			"model",
			"stage",
			"skin",
			"texture",
			"sound",
			"hud",
			"cheat",
			"hack"
		};
		std::string effective_q = query;
		if (cat_idx > 0 && cat_idx < (int)std::size(kCatKeywords))
		{
			if (!effective_q.empty())
				effective_q += " ";
			effective_q += kCatKeywords[cat_idx];
		}

		std::string url;
		if (gb_game_id > 0 && effective_q.empty())
		{
			std::string sort_param = "";
			if (sort_idx == 0)
				sort_param = "Generic_MostLiked";
			else if (sort_idx == 1)
				sort_param = "Generic_Newest";
			else if (sort_idx == 2)
				sort_param = "Generic_MostDownloaded";

			url = "https://gamebanana.com/apiv11/Mod/Index?_nPerpage=35&_nPage=1&_aFilters%5BGeneric_Game%5D=" +
				  std::to_string(gb_game_id);
			if (!sort_param.empty())
				url += "&_sSort=" + sort_param;
		}
		else
		{
			std::string search_str = effective_q;
			if (search_str.empty())
			{
				if (game_idx > 0)
				{
					search_str = game_label;
					const size_t p = search_str.find(" (");
					if (p != std::string::npos)
						search_str = search_str.substr(0, p);
					const size_t slash = search_str.find(" / ");
					if (slash != std::string::npos)
						search_str = search_str.substr(0, slash);
				}
				else
				{
					search_str = "Dreamcast";
				}
			}
			url = "https://gamebanana.com/apiv11/Util/Search/Results?_sSearchString=" +
				  UrlEncode(search_str) + "&_nPage=1&_nPerpage=35";
			if (gb_game_id > 0)
				url += "&_idGameRow=" + std::to_string(gb_game_id);
		}

		std::vector<uint8_t> raw;
		std::string err;
		std::vector<GameBananaModItem> results;

		if (HttpGetBinary(url, raw, nullptr, err) && !raw.empty())
		{
			try
			{
				json doc = json::parse(raw.begin(), raw.end());
				if (doc.is_object() && doc.contains("_aRecords") && doc["_aRecords"].is_array())
				{
					for (const auto& obj : doc["_aRecords"])
					{
						if (!obj.is_object())
							continue;
						GameBananaModItem item;
						if (obj.contains("_idRow") && obj["_idRow"].is_number())
							item.id = obj["_idRow"].get<int>();
						if (obj.contains("_sModelName") && obj["_sModelName"].is_string())
							item.model_name = obj["_sModelName"].get<std::string>();
						if (obj.contains("_sName") && obj["_sName"].is_string())
							item.name = obj["_sName"].get<std::string>();
						if (obj.contains("_sProfileUrl") && obj["_sProfileUrl"].is_string())
							item.profile_url = obj["_sProfileUrl"].get<std::string>();
						if (obj.contains("_nLikeCount") && obj["_nLikeCount"].is_number())
							item.likes = obj["_nLikeCount"].get<int>();
						if (obj.contains("_nViewCount") && obj["_nViewCount"].is_number())
							item.views = obj["_nViewCount"].get<int>();

						if (obj.contains("_aSubmitter") && obj["_aSubmitter"].is_object())
						{
							const auto& sub = obj["_aSubmitter"];
							if (sub.contains("_sName") && sub["_sName"].is_string())
								item.submitter = sub["_sName"].get<std::string>();
						}
						if (obj.contains("_aRootCategory") && obj["_aRootCategory"].is_object())
						{
							const auto& cat = obj["_aRootCategory"];
							if (cat.contains("_sName") && cat["_sName"].is_string())
								item.category = cat["_sName"].get<std::string>();
						}
						if (item.category.empty() && obj.contains("_sSingularTitle") && obj["_sSingularTitle"].is_string())
						{
							item.category = obj["_sSingularTitle"].get<std::string>();
						}
						if (obj.contains("_aGame") && obj["_aGame"].is_object())
						{
							const auto& gm = obj["_aGame"];
							if (gm.contains("_sName") && gm["_sName"].is_string())
								item.game_name = gm["_sName"].get<std::string>();
						}
						if (item.id > 0 && !item.name.empty())
							results.push_back(std::move(item));
					}
				}
			}
			catch (const std::exception& e)
			{
				err = std::string("JSON parse error: ") + e.what();
			}
		}

		// Fallback search if primary query had an error or returned 0 records
		if (results.empty())
		{
			std::string fallback_q = game_label;
			if (fallback_q.empty() || game_idx == 0)
				fallback_q = "Dreamcast";
			else
			{
				const size_t paren = fallback_q.find(" (");
				if (paren != std::string::npos)
					fallback_q = fallback_q.substr(0, paren);
				const size_t slash = fallback_q.find(" / ");
				if (slash != std::string::npos)
					fallback_q = fallback_q.substr(0, slash);
			}
			if (!effective_q.empty())
				fallback_q += " " + effective_q;

			const std::string fallback_url =
				"https://gamebanana.com/apiv11/Util/Search/Results?_sSearchString=" +
				UrlEncode(fallback_q) + "&_nPage=1&_nPerpage=35";

			raw.clear();
			std::string fallback_err;
			if (HttpGetBinary(fallback_url, raw, nullptr, fallback_err) && !raw.empty())
			{
				try
				{
					json doc = json::parse(raw.begin(), raw.end());
					if (doc.is_object() && doc.contains("_aRecords") && doc["_aRecords"].is_array())
					{
						for (const auto& obj : doc["_aRecords"])
						{
							if (!obj.is_object())
								continue;
							GameBananaModItem item;
							if (obj.contains("_idRow") && obj["_idRow"].is_number())
								item.id = obj["_idRow"].get<int>();
							if (obj.contains("_sModelName") && obj["_sModelName"].is_string())
								item.model_name = obj["_sModelName"].get<std::string>();
							if (obj.contains("_sName") && obj["_sName"].is_string())
								item.name = obj["_sName"].get<std::string>();
							if (obj.contains("_sProfileUrl") && obj["_sProfileUrl"].is_string())
								item.profile_url = obj["_sProfileUrl"].get<std::string>();
							if (obj.contains("_nLikeCount") && obj["_nLikeCount"].is_number())
								item.likes = obj["_nLikeCount"].get<int>();
							if (obj.contains("_nViewCount") && obj["_nViewCount"].is_number())
								item.views = obj["_nViewCount"].get<int>();

							if (obj.contains("_aSubmitter") && obj["_aSubmitter"].is_object())
							{
								const auto& sub = obj["_aSubmitter"];
								if (sub.contains("_sName") && sub["_sName"].is_string())
									item.submitter = sub["_sName"].get<std::string>();
							}
							if (obj.contains("_sSingularTitle") && obj["_sSingularTitle"].is_string())
								item.category = obj["_sSingularTitle"].get<std::string>();
							if (obj.contains("_aGame") && obj["_aGame"].is_object())
							{
								const auto& gm = obj["_aGame"];
								if (gm.contains("_sName") && gm["_sName"].is_string())
									item.game_name = gm["_sName"].get<std::string>();
							}
							if (item.id > 0 && !item.name.empty())
								results.push_back(std::move(item));
						}
					}
					if (!results.empty())
						err.clear();
				}
				catch (...)
				{
				}
			}
		}

		{
			std::lock_guard<std::mutex> lock(g_gb.mutex);
			g_gb.mods = std::move(results);
			g_gb.selected_mod_idx = g_gb.mods.empty() ? -1 : 0;
			g_gb.selected_file_idx = 0;
		}

		g_gb.progress_pct.store(100);
		g_gb.busy.store(false);

		if (!err.empty())
		{
			SetGbStatus("Error querying GameBanana: " + err, true);
		}
		else
		{
			size_t count = 0;
			{
				std::lock_guard<std::mutex> lock(g_gb.mutex);
				count = g_gb.mods.size();
			}
			SetGbStatus("Found " + std::to_string(count) + " GameBanana submissions.");
			if (count > 0)
				FetchModDetailsAsync(0);
		}
	}).detach();
}

static void DownloadAndInstallSelectedMod()
{
	if (g_gb.busy.load())
		return;

	GameBananaModItem mod;
	GameBananaFileEntry file_entry;
	{
		std::lock_guard<std::mutex> lock(g_gb.mutex);
		if (g_gb.selected_mod_idx < 0 || g_gb.selected_mod_idx >= (int)g_gb.mods.size())
		{
			g_gb.status_text = "Select a mod from the list first.";
			g_gb.status_is_error = true;
			return;
		}
		mod = g_gb.mods[g_gb.selected_mod_idx];
		if (mod.files.empty())
		{
			g_gb.status_text = "No downloadable files attached to this mod.";
			g_gb.status_is_error = true;
			return;
		}
		int fidx = g_gb.selected_file_idx;
		if (fidx < 0 || fidx >= (int)mod.files.size())
			fidx = 0;
		file_entry = mod.files[fidx];
	}

	const std::string game_id = GetEffectiveGameId();
	const std::string mod_folder = (fs::path(GetModsRootDir()) / (game_id + "_" + SanitizeFolderName(mod.name))).string();

	g_gb.busy.store(true);
	g_gb.progress_pct.store(5);
	SetGbStatus("Downloading " + file_entry.filename + " from GameBanana...");

	std::thread([mod, file_entry, mod_folder, game_id]() {
		std::vector<uint8_t> data;
		std::string err;
		const bool ok = HttpGetBinary(
			file_entry.download_url, data,
			[](int64_t dl, int64_t total) {
				if (total > 0)
					g_gb.progress_pct.store(static_cast<int>((dl * 85) / total));
			},
			err);

		if (!ok || data.empty())
		{
			g_gb.busy.store(false);
			SetGbStatus("Download failed: " + err, true);
			return;
		}

		std::error_code ec;
		fs::create_directories(mod_folder, ec);
		const fs::path archive_path = fs::path(mod_folder) / file_entry.filename;
		{
			std::ofstream ofs(archive_path, std::ios::binary);
			ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
		}

		g_gb.progress_pct.store(92);
		ExtractArchiveToFolder(archive_path.string(), mod_folder);

		int tex_cnt = 0, mdl_cnt = 0, map_cnt = 0, cht_cnt = 0;
		SyncModFilesToFlycast(mod_folder, game_id, true, tex_cnt, mdl_cnt, map_cnt, cht_cnt);

		RefreshInstalledMods();
		g_gb.progress_pct.store(100);
		g_gb.busy.store(false);

		std::ostringstream summary;
		summary << "Installed '" << mod.name << "' for [" << game_id << "] ("
				<< tex_cnt << " textures/skins, " << mdl_cnt << " 3D models, "
				<< map_cnt << " stage/map files, " << cht_cnt << " cheat files).";
		SetGbStatus(summary.str(), false);
	}).detach();
}

static void SavePresetToFlycastCheatFile(const DreamcastCheatPreset& preset)
{
	const std::string cheats_dir = GetFlycastCheatsRootDir();
	const fs::path cht_path = fs::path(cheats_dir) / (std::string(preset.product_id) + ".cht");

	// Also apply live if a game is loaded or in cheatManager
	try
	{
		cheatManager.addGameSharkCheat(
			std::string(preset.game_title) + " - " + preset.cheat_title,
			preset.gameshark_code);
		// Enable the newly added cheat(s)
		for (size_t i = 0; i < cheatManager.cheatCount(); ++i)
		{
			if (cheatManager.cheatDescription(i).find(preset.cheat_title) != std::string::npos)
				cheatManager.enableCheat(i, true);
		}
		cheatManager.saveCheatFile(cht_path.string());
	}
	catch (const FlycastException&)
	{
	}
}

#if defined(_WIN32) && !defined(TARGET_UWP)
static std::string OpenWindowsFileDialog(const char* filter, const char* title)
{
	char filename[MAX_PATH] = {};
	OPENFILENAMEA ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn))
		return std::string(filename);
	return "";
}
#endif

static void RenderBrowseTab()
{
	static const char* kCategories[] = {
		"All Categories (3D Models, Maps, Skins, Textures, Cheats)",
		"3D Models & Character Mesh Swaps (.nj / .pvm / .prs / .obj)",
		"Maps, Stages & Level Overhauls (.bsp / .pk3 / .rel / .bin)",
		"Skins, Costumes & Character Retextures (.png / .dds / .pvr)",
		"HD Texture Packs (Flycast PVR Replacements)",
		"Audio, Music (ADX/AFS) & Voice Packs",
		"UI, HUD & Menu Overhauls",
		"GameShark Cheats, 60FPS & Widescreen Hacks",
		"GDI / CDI / ISO ROM Hacks & Translations"
	};

	static const char* kSortModes[] = {
		"Most Popular / Most Liked",
		"Newest Submissions",
		"Recently Updated"
	};

	// Filter rows with ample breathing room and full-width dropdowns
	const float avail_w = ImGui::GetContentRegionAvail().x;
	const float filterBtnH = uiScaled(36.0f);
	const bool wide_screen = (avail_w >= uiScaled(1040.0f));

	ImGui::Dummy(ScaledVec2(0, 4));

	// Filter row 1: Game & Category
	const float gameComboW = wide_screen ? uiScaled(480.0f) : std::min(uiScaled(540.0f), avail_w * 0.72f);
	ImGui::SetNextItemWidth(gameComboW);
	if (ImGui::BeginCombo(T("Dreamcast Game"), kDreamcastGames[g_gb.game_idx].name))
	{
		for (int i = 0; i < (int)std::size(kDreamcastGames); i++)
		{
			bool sel = (g_gb.game_idx == i);
			std::string label = std::string(kDreamcastGames[i].name) + " [" + kDreamcastGames[i].product_id + "]";
			if (ImGui::Selectable(label.c_str(), sel))
			{
				g_gb.game_idx = i;
				TriggerGameBananaSearch();
			}
			if (sel)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (wide_screen)
	{
		ImGui::SameLine(0, uiScaled(16));
	}
	else
	{
		ImGui::Dummy(ScaledVec2(0, 4));
	}

	const float catComboW = wide_screen ? uiScaled(440.0f) : std::min(uiScaled(540.0f), avail_w * 0.72f);
	ImGui::SetNextItemWidth(catComboW);
	if (ImGui::BeginCombo(T("Category"), kCategories[g_gb.category_idx]))
	{
		for (int i = 0; i < (int)std::size(kCategories); i++)
		{
			bool sel = (g_gb.category_idx == i);
			if (ImGui::Selectable(kCategories[i], sel))
			{
				g_gb.category_idx = i;
				TriggerGameBananaSearch();
			}
			if (sel)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Dummy(ScaledVec2(0, 6));

	// Filter row 2: Sort, Search Input, and Action Buttons
	ImGui::SetNextItemWidth(uiScaled(280));
	if (ImGui::BeginCombo(T("Sort"), kSortModes[g_gb.sort_idx]))
	{
		for (int i = 0; i < (int)std::size(kSortModes); i++)
		{
			bool sel = (g_gb.sort_idx == i);
			if (ImGui::Selectable(kSortModes[i], sel))
			{
				g_gb.sort_idx = i;
				TriggerGameBananaSearch();
			}
			if (sel)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	const float searchBtnW = uiScaled(195.0f);
	const float matchBtnW = !settings.content.gameId.empty() ? uiScaled(210.0f) : 0.0f;
	const float searchInputW = std::max(uiScaled(220.0f), std::min(uiScaled(300.0f), avail_w - uiScaled(280.0f) - searchBtnW - matchBtnW - ImGui::GetStyle().ItemSpacing.x * 4.0f));

	ImGui::SameLine(0, uiScaled(16));
	ImGui::SetNextItemWidth(searchInputW);
	if (InputText(T("Search Keyword"), &g_gb.search_query, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		TriggerGameBananaSearch();
	}

	ImGui::SameLine(0, uiScaled(12));
	{
		DisabledScope _(g_gb.busy.load());
		if (ImGui::Button((std::string(ICON_FA_MAGNIFYING_GLASS " ") + T("Search GameBanana")).c_str(), ImVec2(searchBtnW, filterBtnH)))
		{
			TriggerGameBananaSearch();
		}
	}

	if (!settings.content.gameId.empty())
	{
		if (avail_w >= (uiScaled(280) + searchInputW + searchBtnW + matchBtnW + uiScaled(48)))
		{
			ImGui::SameLine(0, uiScaled(8));
		}
		else
		{
			ImGui::Dummy(ScaledVec2(0, 4));
		}
		if (ImGui::Button((std::string(ICON_FA_COMPACT_DISC " ") + T("Match Running Game")).c_str(), ImVec2(matchBtnW, filterBtnH)))
		{
			bool matched = false;
			for (int i = 1; i < (int)std::size(kDreamcastGames); i++)
			{
				if (settings.content.gameId.find(kDreamcastGames[i].product_id) != std::string::npos)
				{
					g_gb.game_idx = i;
					matched = true;
					break;
				}
			}
			if (!matched && !settings.content.title.empty())
				g_gb.search_query = settings.content.title;
			TriggerGameBananaSearch();
		}
	}

	ImGui::Spacing();
	ImGui::Dummy(ScaledVec2(0, 4));

	// Split view: Left = Mod results table, Right = Selected Mod Details & Download
	const float avail_h = std::max(uiScaled(280.0f), ImGui::GetContentRegionAvail().y - uiScaled(55.0f));
	const float left_w = ImGui::GetContentRegionAvail().x * 0.56f;

	ImGui::BeginChild("gb_mod_list_pane", ImVec2(left_w, avail_h), ImGuiChildFlags_Borders);
	{
		if (ImGui::BeginTable("gb_mods_table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
		{
			ImGui::TableSetupColumn(T("Mod Name (3D Model / Map / Skin)"), ImGuiTableColumnFlags_WidthStretch, 0.46f);
			ImGui::TableSetupColumn(T("Category"), ImGuiTableColumnFlags_WidthStretch, 0.20f);
			ImGui::TableSetupColumn(T("Author"), ImGuiTableColumnFlags_WidthStretch, 0.20f);
			ImGui::TableSetupColumn(T("Likes"), ImGuiTableColumnFlags_WidthStretch, 0.14f);
			ImGui::TableHeadersRow();

			std::vector<GameBananaModItem> mods_snapshot;
			int sel_idx = -1;
			{
				std::lock_guard<std::mutex> lock(g_gb.mutex);
				mods_snapshot = g_gb.mods;
				sel_idx = g_gb.selected_mod_idx;
			}

			int newly_clicked = -1;
			for (int i = 0; i < (int)mods_snapshot.size(); i++)
			{
				const auto& m = mods_snapshot[i];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const bool is_sel = (sel_idx == i);
				std::string row_label = m.name + "##gb_row_" + std::to_string(i);
				if (ImGui::Selectable(row_label.c_str(), is_sel, ImGuiSelectableFlags_SpanAllColumns))
				{
					newly_clicked = i;
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(m.category.empty() ? m.model_name.c_str() : m.category.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(m.submitter.c_str());
				ImGui::TableSetColumnIndex(3);
				ImGui::Text(ICON_FA_HEART " %d", m.likes);
			}

			if (newly_clicked >= 0 && newly_clicked != sel_idx)
			{
				{
					std::lock_guard<std::mutex> lock(g_gb.mutex);
					g_gb.selected_mod_idx = newly_clicked;
					g_gb.selected_file_idx = 0;
				}
				FetchModDetailsAsync(newly_clicked);
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("gb_mod_detail_pane", ImVec2(0, avail_h), ImGuiChildFlags_Borders);
	{
		GameBananaModItem sel_mod;
		bool has_sel = false;
		{
			std::lock_guard<std::mutex> lock(g_gb.mutex);
			if (g_gb.selected_mod_idx >= 0 && g_gb.selected_mod_idx < (int)g_gb.mods.size())
			{
				sel_mod = g_gb.mods[g_gb.selected_mod_idx];
				has_sel = true;
			}
		}

		if (!has_sel)
		{
			ImGui::TextDisabled("%s", T("Select a mod on the left to inspect its 3D models, maps, skins, textures, and downloadable packages."));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.72f, 1.00f, 1.0f));
			ImGui::TextWrapped("%s", sel_mod.name.c_str());
			ImGui::PopStyleColor();

			ImGui::Text(ICON_FA_USER " %s: %s   |   " ICON_FA_TAG " %s",
				T("Author"), sel_mod.submitter.c_str(),
				sel_mod.category.empty() ? sel_mod.model_name.c_str() : sel_mod.category.c_str());
			ImGui::Text(ICON_FA_GAMEPAD " %s: %s   |   " ICON_FA_HEART " %d   |   " ICON_FA_EYE " %d",
				T("Game"), sel_mod.game_name.empty() ? kDreamcastGames[g_gb.game_idx].name : sel_mod.game_name.c_str(),
				sel_mod.likes, sel_mod.views);

			ImGui::Separator();

			// Files selector
			if (!sel_mod.files.empty())
			{
				if (g_gb.selected_file_idx < 0 || g_gb.selected_file_idx >= (int)sel_mod.files.size())
					g_gb.selected_file_idx = 0;
				const auto& cur_f = sel_mod.files[g_gb.selected_file_idx];
				std::string cur_label = cur_f.filename + " (" + FormatBytes(cur_f.filesize) + ", " + std::to_string(cur_f.downloads) + " DLs)";

				ImGui::SetNextItemWidth(-1);
				if (ImGui::BeginCombo("##gb_file_combo", cur_label.c_str()))
				{
					for (int i = 0; i < (int)sel_mod.files.size(); i++)
					{
						const auto& f = sel_mod.files[i];
						std::string flabel = f.filename + " (" + FormatBytes(f.filesize) + ", " + std::to_string(f.downloads) + " DLs)";
						bool sel = (g_gb.selected_file_idx == i);
						if (ImGui::Selectable(flabel.c_str(), sel))
							g_gb.selected_file_idx = i;
						if (sel)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::Spacing();
				{
					DisabledScope _(g_gb.busy.load());
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.56f, 0.30f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.38f, 1.0f));
					if (ImGui::Button((std::string(ICON_FA_DOWNLOAD " ") + T("Download & Auto-Install in Flycast")).c_str(), ScaledVec2(-1, 34)))
					{
						DownloadAndInstallSelectedMod();
					}
					ImGui::PopStyleColor(2);
				}
			}
			else
			{
				ImGui::TextDisabled("%s", T("Loading file attachments from GameBanana..."));
			}

			if (!sel_mod.profile_url.empty())
			{
				if (ImGui::Button((std::string(ICON_FA_GLOBE " ") + T("Open Mod Page on GameBanana.com")).c_str(), ScaledVec2(-1, 28)))
				{
#if defined(_WIN32) && !defined(TARGET_UWP)
					ShellExecuteA(nullptr, "open", sel_mod.profile_url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
					os_LaunchFromURL(sel_mod.profile_url);
#endif
				}
			}

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.65f, 0.80f, 1.0f, 1.0f), "%s:", T("Mod Description & Release Notes"));
			ImGui::BeginChild("gb_mod_desc_scroll", ImVec2(0, 0), ImGuiChildFlags_None);
			ImGui::TextWrapped("%s", sel_mod.description.empty() ? T("No description provided.") : sel_mod.description.c_str());
			ImGui::EndChild();
		}
	}
	ImGui::EndChild();
}

static void RenderInstalledAndPatcherTab()
{
	if (!g_gb.installed_scanned)
		RefreshInstalledMods();

	header(T("Installed 3D Models, Maps, Skins & HD Texture Packs"));
	{
		if (ImGui::Button((std::string(ICON_FA_FILE_IMPORT " ") + T("Import Local Mod (.zip/.7z/.rar/.pvm/.nj/.bsp/.png)")).c_str(), ScaledVec2(370, 32)))
		{
#if defined(_WIN32) && !defined(TARGET_UWP)
			std::string picked = OpenWindowsFileDialog(
				"Dreamcast Mods & Assets (*.zip;*.7z;*.rar;*.pvm;*.prs;*.nj;*.bsp;*.pk3;*.png;*.dds;*.cht)\0*.zip;*.7z;*.rar;*.pvm;*.prs;*.nj;*.bsp;*.pk3;*.png;*.dds;*.cht\0All Files (*.*)\0*.*\0",
				"Select Dreamcast Mod Archive, 3D Model, Map, Skin or Texture Pack");
			if (!picked.empty())
			{
				const std::string game_id = GetEffectiveGameId();
				const std::string mod_name = fs::path(picked).stem().string();
				const std::string dest_folder = (fs::path(GetModsRootDir()) / (game_id + "_" + SanitizeFolderName(mod_name))).string();
				ExtractArchiveToFolder(picked, dest_folder);
				int tex_cnt = 0, mdl_cnt = 0, map_cnt = 0, cht_cnt = 0;
				SyncModFilesToFlycast(dest_folder, game_id, true, tex_cnt, mdl_cnt, map_cnt, cht_cnt);
				RefreshInstalledMods();
				SetGbStatus("Imported '" + mod_name + "' for [" + game_id + "] (" + std::to_string(tex_cnt) + " textures/skins, " +
							std::to_string(mdl_cnt) + " 3D models, " + std::to_string(map_cnt) + " maps).");
			}
#endif
		}

		ImGui::SameLine();
		if (ImGui::Button((std::string(ICON_FA_FOLDER_OPEN " ") + T("Open Flycast Textures Folder")).c_str(), ScaledVec2(240, 32)))
		{
			std::string tex_dir = GetFlycastTexturesRootDir();
#if defined(_WIN32) && !defined(TARGET_UWP)
			ShellExecuteA(nullptr, "open", tex_dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
			os_LaunchFromURL(tex_dir);
#endif
		}

		ImGui::SameLine();
		if (ImGui::Button((std::string(ICON_FA_ROTATE " ") + T("Refresh List")).c_str(), ScaledVec2(130, 32)))
		{
			RefreshInstalledMods();
		}

		ImGui::Spacing();

		if (ImGui::BeginTable("gb_installed_table", 7, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, uiScaled(190))))
		{
			ImGui::TableSetupColumn(T("Mod Name"), ImGuiTableColumnFlags_WidthStretch, 0.28f);
			ImGui::TableSetupColumn(T("Game ID"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableSetupColumn(T("Status"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableSetupColumn(T("Skins/Textures"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableSetupColumn(T("3D Models"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableSetupColumn(T("Maps/Stages"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableSetupColumn(T("Audio/Cheats"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableHeadersRow();

			std::vector<InstalledModEntry> inst_snapshot;
			{
				std::lock_guard<std::mutex> lock(g_gb.mutex);
				inst_snapshot = g_gb.installed_mods;
			}

			for (int i = 0; i < (int)inst_snapshot.size(); i++)
			{
				const auto& m = inst_snapshot[i];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				bool sel = (g_gb.selected_installed_idx == i);
				std::string rlabel = m.name + "##inst_" + std::to_string(i);
				if (ImGui::Selectable(rlabel.c_str(), sel, ImGuiSelectableFlags_SpanAllColumns))
					g_gb.selected_installed_idx = i;

				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(m.game_id.c_str());
				ImGui::TableSetColumnIndex(2);
				if (m.enabled)
					ImGui::TextColored(ImVec4(0.25f, 0.95f, 0.50f, 1.0f), ICON_FA_CHECK " %s", T("Enabled"));
				else
					ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), ICON_FA_BAN " %s", T("Disabled"));

				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%d", m.texture_count);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%d", m.model3d_count);
				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%d", m.map_count);
				ImGui::TableSetColumnIndex(6);
				ImGui::Text("%d / %d", m.audio_count, m.cheat_count);
			}
			ImGui::EndTable();
		}

		// Actions for selected installed mod
		InstalledModEntry sel_inst;
		bool has_inst = false;
		{
			std::lock_guard<std::mutex> lock(g_gb.mutex);
			if (g_gb.selected_installed_idx >= 0 && g_gb.selected_installed_idx < (int)g_gb.installed_mods.size())
			{
				sel_inst = g_gb.installed_mods[g_gb.selected_installed_idx];
				has_inst = true;
			}
		}

		if (has_inst)
		{
			if (ImGui::Button((std::string(ICON_FA_POWER_OFF " ") + (sel_inst.enabled ? T("Disable Selected Mod") : T("Enable Selected Mod"))).c_str(), ScaledVec2(200, 30)))
			{
				std::error_code ec;
				const fs::path marker = fs::path(sel_inst.path) / ".disabled";
				int t = 0, m = 0, mp = 0, c = 0;
				if (sel_inst.enabled)
				{
					std::ofstream ofs(marker);
					SyncModFilesToFlycast(sel_inst.path, sel_inst.game_id, false, t, m, mp, c);
					SetGbStatus("Disabled mod '" + sel_inst.name + "'.");
				}
				else
				{
					fs::remove(marker, ec);
					SyncModFilesToFlycast(sel_inst.path, sel_inst.game_id, true, t, m, mp, c);
					SetGbStatus("Enabled mod '" + sel_inst.name + "' and synced " + std::to_string(t) + " textures.");
				}
				RefreshInstalledMods();
			}

			ImGui::SameLine();
			if (ImGui::Button((std::string(ICON_FA_FOLDER_OPEN " ") + T("Open Mod Package Folder")).c_str(), ScaledVec2(210, 30)))
			{
#if defined(_WIN32) && !defined(TARGET_UWP)
				ShellExecuteA(nullptr, "open", sel_inst.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
				os_LaunchFromURL(sel_inst.path);
#endif
			}

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.18f, 0.18f, 1.0f));
			if (ImGui::Button((std::string(ICON_FA_TRASH_CAN " ") + T("Delete Mod")).c_str(), ScaledVec2(140, 30)))
			{
				int t = 0, m = 0, mp = 0, c = 0;
				SyncModFilesToFlycast(sel_inst.path, sel_inst.game_id, false, t, m, mp, c);
				std::error_code ec;
				fs::remove_all(sel_inst.path, ec);
				g_gb.selected_installed_idx = -1;
				RefreshInstalledMods();
				SetGbStatus("Deleted mod '" + sel_inst.name + "'.");
			}
			ImGui::PopStyleColor();
		}
	}

	ImGui::Spacing();
	header(T("Dreamcast GDI / CDI / ISO / Track Patcher (.ips / .bps / .ppf / .xdelta)"));
	{
		ImGui::TextWrapped("%s", T("Apply GameBanana total conversion mods, custom stage packs, undubs, and 3D model swaps directly to a Dreamcast track (.bin / .iso / .cdi) with automatic .bak backup."));

		ImGui::SetNextItemWidth(uiScaled(420));
		InputText(T("Base Disc / Track File"), &g_gb.patch_base_path);
#if defined(_WIN32) && !defined(TARGET_UWP)
		ImGui::SameLine();
		if (ImGui::Button(T("Browse Disc/Track..."), ScaledVec2(160, 0)))
		{
			std::string p = OpenWindowsFileDialog(
				"Dreamcast Disc / Track (*.bin;*.iso;*.cdi;*.gdi;*.img)\0*.bin;*.iso;*.cdi;*.gdi;*.img\0All Files (*.*)\0*.*\0",
				"Select Base Dreamcast Track or Disc Image");
			if (!p.empty())
				g_gb.patch_base_path = p;
		}
#endif

		ImGui::SetNextItemWidth(uiScaled(420));
		InputText(T("Patch File (.ips/.bps/.ppf/.xdelta)"), &g_gb.patch_file_path);
#if defined(_WIN32) && !defined(TARGET_UWP)
		ImGui::SameLine();
		if (ImGui::Button(T("Browse Patch..."), ScaledVec2(160, 0)))
		{
			std::string p = OpenWindowsFileDialog(
				"ROM / Disc Patches (*.ips;*.bps;*.ppf;*.xdelta;*.vcdiff)\0*.ips;*.bps;*.ppf;*.xdelta;*.vcdiff\0All Files (*.*)\0*.*\0",
				"Select Patch File");
			if (!p.empty())
				g_gb.patch_file_path = p;
		}
#endif

		if (ImGui::Button((std::string(ICON_FA_WAND_MAGIC_SPARKLES " ") + T("Apply Patch to Disc / Track (Creates .bak Backup)")).c_str(), ScaledVec2(390, 32)))
		{
			if (g_gb.patch_base_path.empty() || g_gb.patch_file_path.empty())
			{
				SetGbStatus("Select both a base disc/track file and a patch file first.", true);
			}
			else
			{
				std::ifstream f_rom(g_gb.patch_base_path, std::ios::binary);
				std::ifstream f_pat(g_gb.patch_file_path, std::ios::binary);
				if (!f_rom || !f_pat)
				{
					SetGbStatus("Could not open base file or patch file.", true);
				}
				else
				{
					std::vector<uint8_t> rom_bytes((std::istreambuf_iterator<char>(f_rom)), std::istreambuf_iterator<char>());
					std::vector<uint8_t> pat_bytes((std::istreambuf_iterator<char>(f_pat)), std::istreambuf_iterator<char>());
					f_rom.close();
					f_pat.close();

					std::error_code ec;
					fs::copy_file(g_gb.patch_base_path, g_gb.patch_base_path + ".bak", fs::copy_options::overwrite_existing, ec);

					std::string ext = fs::path(g_gb.patch_file_path).extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

					std::string err;
					bool ok = false;
					std::vector<uint8_t> out_bytes;

					if (ext == ".ips")
					{
						out_bytes = std::move(rom_bytes);
						ok = ApplyIpsPatch(out_bytes, pat_bytes, err);
					}
					else if (ext == ".bps")
					{
						ok = ApplyBpsPatch(rom_bytes, out_bytes, pat_bytes, err);
					}
					else if (ext == ".ppf")
					{
						out_bytes = std::move(rom_bytes);
						ok = ApplyPpfPatch(out_bytes, pat_bytes, err);
					}
					else
					{
						err = "Use .ips, .bps, or .ppf for native in-memory patching, or place xdelta3 in PATH.";
					}

					if (ok)
					{
						std::ofstream ofs(g_gb.patch_base_path, std::ios::binary | std::ios::trunc);
						ofs.write(reinterpret_cast<const char*>(out_bytes.data()), static_cast<std::streamsize>(out_bytes.size()));
						SetGbStatus("Successfully patched '" + g_gb.patch_base_path + "' (backup saved as .bak)!");
					}
					else
					{
						SetGbStatus("Patch failed: " + err, true);
					}
				}
			}
		}
	}
}

static void RenderCheatStudioTab()
{
	header(T("Curated Dreamcast / NAOMI GameShark, 60FPS, Unlock & Character Model Presets"));
	{
		if (ImGui::BeginTable("gb_presets_table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, uiScaled(210))))
		{
			ImGui::TableSetupColumn(T("Dreamcast Game"), ImGuiTableColumnFlags_WidthStretch, 0.24f);
			ImGui::TableSetupColumn(T("Product ID"), ImGuiTableColumnFlags_WidthStretch, 0.12f);
			ImGui::TableSetupColumn(T("Cheat / Mod Preset"), ImGuiTableColumnFlags_WidthStretch, 0.42f);
			ImGui::TableSetupColumn(T("Category"), ImGuiTableColumnFlags_WidthStretch, 0.22f);
			ImGui::TableHeadersRow();

			for (int i = 0; i < (int)std::size(kDreamcastCheatPresets); i++)
			{
				const auto& p = kDreamcastCheatPresets[i];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				bool sel = (g_gb.selected_preset_idx == i);
				std::string label = std::string(p.game_title) + "##preset_" + std::to_string(i);
				if (ImGui::Selectable(label.c_str(), sel, ImGuiSelectableFlags_SpanAllColumns))
				{
					g_gb.selected_preset_idx = i;
					g_gb.custom_cheat_name = p.cheat_title;
					g_gb.custom_cheat_game_id = p.product_id;
					g_gb.custom_cheat_code = p.gameshark_code;
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(p.product_id);
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(p.cheat_title);
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(p.category);
			}
			ImGui::EndTable();
		}

		const auto& cur_p = kDreamcastCheatPresets[g_gb.selected_preset_idx];
		ImGui::TextColored(ImVec4(0.35f, 0.72f, 1.00f, 1.0f), ICON_FA_STAR " %s [%s] — %s",
			cur_p.game_title, cur_p.product_id, cur_p.cheat_title);
		ImGui::TextWrapped("%s", cur_p.description);
		ImGui::Text(ICON_FA_CODE " GameShark / Action Replay Code: %s", cur_p.gameshark_code);

		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.56f, 0.30f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.68f, 0.38f, 1.0f));
		if (ImGui::Button((std::string(ICON_FA_BOLT " ") + T("Inject & Activate Preset in Flycast + Save .cht")).c_str(), ScaledVec2(360, 34)))
		{
			SavePresetToFlycastCheatFile(cur_p);
			SetGbStatus(std::string("Activated '") + cur_p.cheat_title + "' for " + cur_p.game_title + " [" + cur_p.product_id + "]!");
		}
		ImGui::PopStyleColor(2);

		ImGui::SameLine();
		if (ImGui::Button((std::string(ICON_FA_LIST_CHECK " ") + T("Install All Presets for Selected Game")).c_str(), ScaledVec2(280, 34)))
		{
			const std::string target_id = cur_p.product_id;
			int installed_cnt = 0;
			for (const auto& preset : kDreamcastCheatPresets)
			{
				if (target_id == preset.product_id)
				{
					SavePresetToFlycastCheatFile(preset);
					installed_cnt++;
				}
			}
			SetGbStatus("Installed and activated " + std::to_string(installed_cnt) + " cheats for [" + target_id + "].");
		}
	}

	ImGui::Spacing();
	header(T("Custom Dreamcast GameShark / CodeBreaker / SH4 RAM Cheat Builder"));
	{
		ImGui::Columns(2, "custom_cheat_cols", false);
		ImGui::SetNextItemWidth(-1);
		InputText(T("Cheat Description"), &g_gb.custom_cheat_name);
		ImGui::SetNextItemWidth(uiScaled(180));
		InputText(T("Target Game ID"), &g_gb.custom_cheat_game_id);
		ImGui::NextColumn();
		ImGui::SetNextItemWidth(-1);
		InputText(T("GameShark Hex Code"), &g_gb.custom_cheat_code);

		if (ImGui::Button((std::string(ICON_FA_PLUS " ") + T("Add & Activate Custom Cheat")).c_str(), ScaledVec2(250, 32)))
		{
			try
			{
				cheatManager.addGameSharkCheat(g_gb.custom_cheat_name, g_gb.custom_cheat_code);
				if (cheatManager.cheatCount() > 0)
					cheatManager.enableCheat(cheatManager.cheatCount() - 1, true);
				SetGbStatus("Added and enabled custom GameShark cheat: " + g_gb.custom_cheat_name);
			}
			catch (const FlycastException& e)
			{
				SetGbStatus(std::string("Invalid GameShark code: ") + e.what(), true);
			}
		}
		ImGui::Columns(1, nullptr, false);
	}

	ImGui::Spacing();
	header(T("Active Flycast Session Cheats"));
	{
		ImGui::Text("%s: %zu", T("Loaded Cheats in Current Session"), cheatManager.cheatCount());
		if (cheatManager.cheatCount() > 0)
		{
			if (ImGui::BeginTable("gb_active_cheats_tbl", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, ImVec2(0, uiScaled(120))))
			{
				ImGui::TableSetupColumn(T("Active"), ImGuiTableColumnFlags_WidthFixed, uiScaled(70));
				ImGui::TableSetupColumn(T("Cheat Description"), ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				for (size_t i = 0; i < cheatManager.cheatCount(); i++)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					bool en = cheatManager.cheatEnabled(i);
					std::string cb_id = "##act_cht_" + std::to_string(i);
					if (ImGui::Checkbox(cb_id.c_str(), &en))
						cheatManager.enableCheat(i, en);
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(cheatManager.cheatDescription(i).c_str());
				}
				ImGui::EndTable();
			}
		}
	}
}
} // namespace

void gui_settings_gamebanana()
{
	if (!g_gb.initial_search_triggered)
	{
		g_gb.initial_search_triggered = true;
		TriggerGameBananaSearch();
	}

	// Status & Progress bar header
	{
		std::string status_copy;
		bool is_err = false;
		{
			std::lock_guard<std::mutex> lock(g_gb.mutex);
			status_copy = g_gb.status_text;
			is_err = g_gb.status_is_error;
		}
		ImVec4 col = is_err ? ImVec4(1.0f, 0.38f, 0.38f, 1.0f) : ImVec4(0.35f, 0.90f, 0.65f, 1.0f);
		ImGui::TextColored(col, ICON_FA_PUZZLE_PIECE " %s", status_copy.c_str());

		if (g_gb.busy.load())
		{
			float pct = static_cast<float>(g_gb.progress_pct.load()) / 100.0f;
			ImGui::ProgressBar(pct, ImVec2(-1, uiScaled(14)));
		}
	}

	ImGui::Spacing();

	if (ImGui::BeginTabBar("gb_studio_tabs", ImGuiTabBarFlags_NoTooltip))
	{
		if (ImGui::BeginTabItem((std::string(ICON_FA_CLOUD_ARROW_DOWN) + " " + T("Browse & Download GameBanana Mods")).c_str()))
		{
			ImGui::Spacing();
			RenderBrowseTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem((std::string(ICON_FA_CUBES) + " " + T("3D Models, Maps, Skins & Disc Patcher")).c_str()))
		{
			ImGui::Spacing();
			RenderInstalledAndPatcherTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem((std::string(ICON_FA_WAND_MAGIC_SPARKLES) + " " + T("Dreamcast Cheats & GameShark Studio")).c_str()))
		{
			ImGui::Spacing();
			RenderCheatStudioTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void gui_gamebanana()
{
	fullScreenWindow(false);
	ImguiStyleVar _(ImGuiStyleVar_WindowRounding, 0);

	ImGui::Begin(T("GameBanana Dreamcast Modding Hub — Cheats, 3D Models, Maps & Skins"), nullptr,
		ImGuiWindowFlags_DragScrolling | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	ImVec2 normal_padding = ImGui::GetStyle().FramePadding;

	if (ImGui::Button((std::string(ICON_FA_ARROW_LEFT " ") + T("Back")).c_str(), ScaledVec2(110, 32)))
	{
		SaveSettings();
		if (game_started)
			gui_setState(GuiState::Commands);
		else
			gui_setState(GuiState::Main);
	}
	ImGui::SameLine();
	if (ImGui::Button((std::string(ICON_FA_VR_CARDBOARD " ") + T("Open Quest VR Studio")).c_str(), ScaledVec2(210, 32)))
	{
		SaveSettings();
		gui_setState(GuiState::QuestVR);
	}
	if (game_started)
	{
		ImGui::SameLine();
		if (ImGui::Button((std::string(ICON_FA_MASK " ") + T("Native Flycast Cheats")).c_str(), ScaledVec2(200, 32)))
		{
			gui_setState(GuiState::Cheats);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	{
		ImguiStyleVar _(ImGuiStyleVar_FramePadding, normal_padding);
		gui_settings_gamebanana();
	}

	scrollWhenDraggingOnVoid();
	windowDragScroll();
	ImGui::End();
}

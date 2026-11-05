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
#pragma once

#include "types.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class QuestBroadcastService
{
public:
	static QuestBroadcastService& Instance();

	bool Start(uint16_t port = 8765, int target_fps = 72, int jpeg_quality = 80);
	void Stop();

	bool IsRunning() const { return m_running.load(); }
	uint16_t GetPort() const { return m_port.load(); }
	std::string GetStreamUrl() const;
	std::string GetLocalIpAddress() const;

	void SetTargetFps(int fps) { m_target_fps.store(fps < 15 ? 15 : (fps > 120 ? 120 : fps)); }
	int GetTargetFps() const { return m_target_fps.load(); }

	void SetJpegQuality(int quality) { m_jpeg_quality.store(quality < 20 ? 20 : (quality > 100 ? 100 : quality)); }
	int GetJpegQuality() const { return m_jpeg_quality.load(); }

	// 0 = Full Window, 1 = Left Half (P1), 2 = Right Half (P2),
	// 3 = Top Half (P1), 4 = Bottom Half (P2),
	// 5 = Top-Left (P1 4P), 6 = Top-Right (P2 4P), 7 = Bottom-Left (P3 4P), 8 = Bottom-Right (P4 4P)
	void SetCropMode(int mode) { m_crop_mode.store(mode); }
	int GetCropMode() const { return m_crop_mode.load(); }

	void SetCaptureTargetWindow(void* hwnd) { m_target_hwnd.store(hwnd); }
	void* GetCaptureTargetWindow() const { return m_target_hwnd.load(); }

	uint64_t GetFramesCaptured() const { return m_frames_captured.load(); }
	uint64_t GetFramesServed() const { return m_frames_served.load(); }
	double GetActualCaptureFps() const { return m_actual_fps.load(); }
	std::string GetLastErrorMsg() const;

	void PushAudioSamples(const void* data, size_t sample_pairs);
	void UpdateControllerInput(uint32_t buttons, float joyx, float joyy, float lt, float rt);

private:
	QuestBroadcastService();
	~QuestBroadcastService();

	void CaptureLoop();
	void ServerLoop();
	bool CaptureFrameToJpeg(std::vector<uint8_t>& out_jpeg);
	void SetError(const std::string& msg);

	std::atomic<bool> m_running{false};
	std::atomic<uint16_t> m_port{8765};
	std::atomic<int> m_target_fps{72};
	std::atomic<int> m_jpeg_quality{80};
	std::atomic<int> m_crop_mode{0};
	std::atomic<void*> m_target_hwnd{nullptr};

	std::atomic<uint64_t> m_frames_captured{0};
	std::atomic<uint64_t> m_frames_served{0};
	std::atomic<double> m_actual_fps{0.0};

	mutable std::mutex m_frame_mutex;
	std::vector<uint8_t> m_latest_jpeg;

	mutable std::mutex m_audio_mutex;
	std::vector<int16_t> m_audio_buffer;

	std::shared_ptr<class GamepadDevice> m_vr_gamepad;

	mutable std::mutex m_error_mutex;
	std::string m_last_error;

	std::thread m_capture_thread;
	std::thread m_server_thread;
	uintptr_t m_listen_socket = ~static_cast<uintptr_t>(0);
};

void QuestBroadcastPushAudio(const void* samples, size_t count);

// Fullscreen Quest VR & Mixed Reality Studio screen (GuiState::QuestVR)
void gui_quest_vr();

// Embeddable Quest VR settings panel for the Settings dialog tab
void gui_settings_quest_vr();

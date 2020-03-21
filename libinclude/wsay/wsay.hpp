#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace wsy {
// helpers
extern bool parse_text_file(
		const std::filesystem::path& path, std::wstring& out_text);

struct voice_impl;
struct voice {
	// Creates a default voice.
	voice();
	~voice();

	// Selection of voices. Use these indexes to select voices.
	size_t available_voices_size() const;
	std::vector<std::wstring> available_voices() const;

	// Selection of playback devices. Use these indexes to select output.
	size_t available_devices_size() const;
	std::vector<std::wstring> available_devices() const;

	void add_file_output(const std::filesystem::path& path);
	void add_device_playback(size_t device_idx);

	// Call this once all outputs have been added.
	void select_voice(size_t voice_idx);

	// Speaks the sentence using selected voice to playback outputs and file.
	// Blocking.
	void speak(const std::wstring& sentence);

	// Speaks the sentence using selected voice to playback outputs and file.
	// Non-blocking.
	void speak_async(const std::wstring& sentence);

	// Interrupts playback speaking.
	// Doesn't interrupt file ouput.
	void stop_speaking();


private:
	const std::unique_ptr<voice_impl> _impl;
};
} // namespace wsy

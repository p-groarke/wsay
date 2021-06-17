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

	voice(const voice&);
	voice(voice&&) noexcept;
	voice& operator=(const voice&);
	voice& operator=(voice&&) noexcept;

	// Number of available voices.
	size_t available_voices_size() const;

	// Selection of voices. Use these indexes to select voices.
	std::vector<std::wstring> available_voices() const;

	// Select the desired voice.
	void select_voice(size_t voice_idx);


	// Number of available devices.
	size_t available_devices_size() const;

	// Selection of playback devices. Use these indexes to select output.
	std::vector<std::wstring> available_devices() const;

	// Voice will speak on the device linked to the provided index.
	void enable_device_playback(size_t device_idx);

	// Stop output on provided device.
	void disable_device_playback(size_t device_idx);


	// Speak commands will output to a wave file at provided path.
	void start_file_output(const std::filesystem::path& path);

	// Stop writing wave file.
	void stop_file_output();


	// Sets the voice volume, from 0 to 100.
	void set_volume(uint16_t volume);


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

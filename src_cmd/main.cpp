#include <filesystem>
#include <iostream>
#include <ns_getopt/ns_getopt.h>
#include <string>
#include <wsay/wsay.hpp>

const std::wstring exit_cmd = L"!exit";
const std::wstring shutup_cmd = L"!stop";

int main(int argc, char** argv) {
	wsy::voice voice;

	bool interactive_mode = false;
	size_t chosen_voice = 0;
	std::wstring speech_text;

	std::array<opt::argument, 8> args = { {
			// clang-format
			{ "\"sentence\"", opt::type::raw_arg,
					[&](std::string_view f) {
						// TEMP HACK, TODO : Support wide strings in ns_getopt.
						speech_text = { f.begin(), f.end() };
						return true;
					},
					"Sentence to say. You can use speech xml." },
			{ "list_voices", opt::type::no_arg,
					[&]() {
						std::vector<std::wstring> names
								= voice.available_voices();
						for (size_t i = 0; i < names.size(); ++i) {
							wprintf(L"%zu : %s\n", i + 1, names[i].c_str());
						}
						return true;
					},
					"Lists available voices.", 'l' },
			{ "voice", opt::type::required_arg,
					[&](std::string_view f) {
						std::string t{ f };
						chosen_voice = std::stoull(t);
						if (chosen_voice == 0
								|| chosen_voice
										> voice.available_voices_size()) {
							fprintf(stderr,
									"beep boop, wrong voice selected.\n\n");
							return false;
						}
						return true;
					},
					"Choose a different voice. Use the voice number printed "
					"using --list_voices.",
					'v' },
			{ "input_text", opt::type::required_arg,
					[&](std::string_view f) {
						std::filesystem::path filepath = { f };
						return wsy::parse_text_file(filepath, speech_text);
					},
					"Play text from '.txt' file. Supports speech xml.", 'i' },
			{ "output_file", opt::type::optional_arg,
					[&](std::string_view f) {
						std::filesystem::path filepath
								= std::filesystem::current_path() / L"out.wav";
						if (!f.empty()) {
							filepath = { f };
						}
						voice.add_file_output(filepath);
						return true;
					},
					"Outputs to wav file. Uses 'out.wav' if no filename is "
					"provided.",
					'o' },
			{ "interactive", opt::type::no_arg,
					[&]() {
						interactive_mode = true;
						return true;
					},
					"Enter interactive mode. Type sentences, they will be "
					"spoken when you press enter.\nUse 'ctrl+c' or "
					"type '!exit' to quit.",
					'I' },
			{ "list_devices", opt::type::no_arg,
					[&]() {
						std::vector<std::wstring> dnames
								= voice.available_devices();

						for (size_t i = 0; i < dnames.size(); ++i) {
							wprintf(L"%zu : %s\n", i + 1, dnames[i].c_str());
						}
						return true;
					},
					"List detected playback devices.", 'd' },
			{ "playback_device", opt::type::multi_arg,
					[&](const opt::multi_array& arr, size_t size) {
						for (size_t i = 0; i < size; ++i) {
							std::string t{ arr[i] };
							unsigned chosen_device = std::stoul(t);
							if (chosen_device == 0
									|| chosen_device
											> voice.available_devices_size()) {
								fprintf(stderr,
										"beep boop, wrong device "
										"selected.\n\n");
								return false;
							}

							voice.add_device_playback(chosen_device - 1);
						}

						return true;
					},
					"Specify a playback device. Use the number "
					"provided by --list_devices.\nYou can provide more than 1 "
					"playback device, seperate the numbers with spaces.\nYou "
					"can also mix output to file + playback.",
					'p' },
	} };

	std::string help_outro = "wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += "\nhttps://github.com/p-groarke/wsay/releases\n";
	help_outro += "Philippe Groarke <philippe.groarke@gmail.com>";

	bool succeeded = opt::parse_arguments(argc, argv, args, { "", help_outro });
	if (!succeeded)
		return -1;

	if (chosen_voice != 0) {
		voice.select_voice(chosen_voice - 1);
	}

	if (interactive_mode) {
		printf("[Info] Type sentences, press enter to speak them.\n");
		wprintf(L"[Command] '%s' : Exit interactive mode.\n", exit_cmd.c_str());
		wprintf(L"[Command] '%s' : Interrupt speaking.\n", shutup_cmd.c_str());
		printf("\n");

		std::wstring wsentence;
		while (std::getline(std::wcin, wsentence)) {
			if (wsentence == exit_cmd) {
				break;
			}

			if (wsentence == shutup_cmd) {
				voice.stop_speaking();
				continue;
			}

			voice.speak_async(wsentence);
		}
	} else {
		voice.speak(speech_text);
	}

	return 0;
}

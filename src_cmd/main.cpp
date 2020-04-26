#include <fcntl.h>
#include <fea_getopt/fea_getopt.hpp>
#include <fea_utils/fea_utils.hpp>
#include <filesystem>
#include <io.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <wsay/wsay.hpp>

const std::wstring exit_cmd = L"!exit";
const std::wstring shutup_cmd = L"!stop";

int wmain(int argc, wchar_t** argv, wchar_t**) {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);

	wsy::voice voice;

	bool interactive_mode = false;
	size_t chosen_voice = 0;
	std::wstring speech_text;

	fea::get_opt<wchar_t> opt;

	opt.add_raw_option(
			L"sentence",
			[&](std::wstring&& str) {
				speech_text = std::move(str);
				return true;
			},
			L"Sentence to say. You can use speech xml.");

	opt.add_flag_option(
			L"list_voices",
			[&]() {
				std::vector<std::wstring> names = voice.available_voices();
				for (size_t i = 0; i < names.size(); ++i) {
					wprintf(L"%zu : %s\n", i + 1, names[i].c_str());
				}
				return true;
			},
			L"Lists available voices.", L'l');

	opt.add_required_arg_option(
			L"voice",
			[&](std::wstring&& str) {
				chosen_voice = std::stoull(str);
				if (chosen_voice == 0
						|| chosen_voice > voice.available_voices_size()) {
					fprintf(stderr, "beep boop, wrong voice selected.\n\n");
					return false;
				}
				return true;
			},
			L"Choose a different voice. Use the voice number printed "
			"using --list_voices.",
			L'v');


	opt.add_required_arg_option(
			L"input_text",
			[&](std::wstring&& f) {
				std::filesystem::path filepath = { std::move(f) };
				return wsy::parse_text_file(filepath, speech_text);
			},
			L"Play text from '.txt' file. Supports speech xml.", L'i');


	opt.add_optional_arg_option(
			L"output_file",
			[&](std::wstring f) {
				std::filesystem::path filepath
						= std::filesystem::current_path() / L"out.wav";
				if (!f.empty()) {
					filepath = { std::move(f) };
				}
				voice.start_file_output(filepath);
				return true;
			},
			L"Outputs to wav file. Uses 'out.wav' if no filename is "
			"provided.",
			L'o'

	);

	opt.add_flag_option(
			L"interactive",
			[&]() {
				interactive_mode = true;
				return true;
			},
			L"Enter interactive mode. Type sentences, they will be "
			"spoken when you press enter.\nUse 'ctrl+c' or "
			"type '!exit' to quit.",
			L'I');

	opt.add_flag_option(
			L"list_devices",
			[&]() {
				std::vector<std::wstring> dnames = voice.available_devices();

				for (size_t i = 0; i < dnames.size(); ++i) {
					wprintf(L"%zu : %s\n", i + 1, dnames[i].c_str());
				}
				return true;
			},
			L"List detected playback devices.", L'd');

	opt.add_multi_arg_option(
			L"playback_device",
			[&](std::vector<std::wstring>&& arr) {
				for (size_t i = 0; i < arr.size(); ++i) {
					unsigned chosen_device = std::stoul(arr[i]);
					if (chosen_device == 0
							|| chosen_device > voice.available_devices_size()) {
						fwprintf(stderr,
								L"beep boop, wrong device "
								"selected.\n\n");
						return false;
					}

					voice.enable_device_playback(chosen_device - 1);
				}

				return true;
			},
			L"Specify a playback device. Use the number "
			"provided by --list_devices.\nYou can provide more than 1 "
			"playback device, seperate the numbers with spaces. You "
			"can also mix output to file + playback.",
			L'p');


	std::wstring help_outro = L"wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += L"\nhttps://github.com/p-groarke/wsay/releases\n";
	help_outro += L"Philippe Groarke <philippe.groarke@gmail.com>";
	opt.add_help_outro(help_outro);

	bool succeeded = opt.parse_options(argc, argv);
	if (!succeeded)
		return -1;

	if (chosen_voice != 0) {
		voice.select_voice(chosen_voice - 1);
	}

	if (interactive_mode) {
		wprintf(L"[Info] Type sentences, press enter to speak them.\n");
		wprintf(L"[Command] '%s' : Exit interactive mode.\n", exit_cmd.c_str());
		wprintf(L"[Command] '%s' : Interrupt speaking.\n", shutup_cmd.c_str());
		wprintf(L"\n");

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

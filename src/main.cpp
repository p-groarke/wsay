#include "util.hpp"

// #include <clocale>
// #include <cstdio>
// #include <fcntl.h>
#include <fea/getopt/getopt.hpp>
// #include <fea/serialize/serializer.hpp>
// #include <fea/string/conversions.hpp>
#include <fea/terminal/utf8.hpp>
#include <filesystem>
// #include <io.h>
#include <format>
#include <iostream>
#include <string>
// #include <wsay/wsay.hpp>
#include <fea/terminal/pipe.hpp>
#include <wsay/engine.hpp>
#include <wsay/voice.hpp>

const std::wstring exit_cmd = L"!exit";
const std::wstring shutup_cmd = L"!stop";

int wmain(int argc, wchar_t** argv, wchar_t**) {
	std::wcout.sync_with_stdio(false);
	std::wcerr.sync_with_stdio(false);
	std::wcin.sync_with_stdio(false);
	auto on_exit_reset_term = fea::utf8_terminal(true);

	wsy::engine engine;
	wsy::voice voice;

	// bool interactive_mode = false;
	std::wstring speech_text = fea::wread_pipe_text();

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
				const std::vector<std::wstring>& names = engine.voices();
				for (size_t i = 0; i < names.size(); ++i) {
					std::wcout << std::format(L"{} : {}\n", i + 1, names[i]);
				}
				std::wcout << L"\n";
				return true;
			},
			L"Lists available voices.", L'l');

	opt.add_required_arg_option(
			L"voice",
			[&](std::wstring&& str) {
				voice.voice_idx = std::stoull(str);
				if (voice.voice_idx == 0
						|| voice.voice_idx > engine.voices().size()) {
					std::wcerr << L"Beep boop, wrong voice selected.\n\n";
					return false;
				}
				--voice.voice_idx;
				return true;
			},
			L"Choose a different voice. Use the voice number printed "
			"using --list_voices.",
			L'v');


	opt.add_required_arg_option(
			L"input_text",
			[&](std::wstring&& f) {
				std::filesystem::path filepath = { std::move(f) };
				return parse_text_file(filepath, speech_text);
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
				voice.add_output_file(filepath);
				return true;
			},
			L"Outputs to wav file. Uses 'out.wav' if no filename is "
			"provided.",
			L'o');

	// opt.add_flag_option(
	//		L"interactive",
	//		[&]() {
	//			interactive_mode = true;
	//			return true;
	//		},
	//		L"Enter interactive mode. Type sentences, they will be "
	//		"spoken when you press enter.\nUse 'ctrl+c' or "
	//		"type '!exit' to quit.",
	//		L'I');

	opt.add_flag_option(
			L"list_devices",
			[&]() {
				const std::vector<std::wstring>& dnames = engine.devices();
				for (size_t i = 0; i < dnames.size(); ++i) {
					std::wcout << std::format(L"{} : {}\n", i + 1, dnames[i]);
				}

				std::wcout << L"\tUse 'all' to select every device.\n\n";
				return true;
			},
			L"List detected playback devices.", L'd');

	opt.add_multi_arg_option(
			L"playback_device",
			[&](std::vector<std::wstring>&& arr) {
				if (arr.size() == 1 && arr.front() == L"all") {
					for (size_t i = 0; i < engine.devices().size(); ++i) {
						voice.add_output_device(i);
					}
					return true;
				}

				for (size_t i = 0; i < arr.size(); ++i) {
					unsigned chosen_device = std::stoul(arr[i]);
					if (chosen_device == 0
							|| chosen_device > engine.devices().size()) {
						std::wcerr << L"Beep boop, wrong device "
									  "selected.\n\n";
						return false;
					}

					voice.add_output_device(chosen_device - 1);
				}

				return true;
			},
			L"Specify a playback device. Use the number "
			"provided by --list_devices.\nYou can provide more than 1 "
			"playback device, seperate the numbers with spaces. You "
			"can also mix output to file + playback.\nUse 'all' to select all "
			"devices.",
			L'p');

	opt.add_required_arg_option(
			L"volume",
			[&](std::wstring&& f) {
				voice.volume = uint8_t(std::stoul(f));
				return true;
			},
			L"Sets the voice volume, from 0 to 100.", L'V');

	opt.add_required_arg_option(
			L"speed",
			[&](std::wstring&& f) {
				voice.speed = uint8_t(std::stoul(f));
				return true;
			},
			L"Sets the voice speed, from 0 to 100. 50 is the default speed.",
			L's');

	opt.add_flag_option(
			L"clipboard",
			[&]() {
				speech_text = get_clipboard_text();
				if (speech_text.empty()) {
					return false;
				}

				std::wcout << std::format(
						L"Playing clipboard text : '{}'\n", speech_text);
				return true;
			},
			L"Speak currently copied text (the text in your clipboard).", L'c');

	opt.add_flag_option(
			L"nospeechxml",
			[&]() {
				voice.xml_parse = false;
				return true;
			},
			L"Disable speech xml detection. Use this if the text contains "
			L"special characters that aren't speech xml.",
			L'X');

	opt.add_flag_option(
			L"fxradio1",
			[&]() {
				voice.compression = wsy::compression_e::alaw;
				voice.bit_depth = wsy::bit_depth_e::_8;
				voice.sampling_rate = wsy::sampling_rate_e::_8;
				return true;
			},
			L"Degrades audio to make it sound a little more like a radio.\n"
			"Note : Radio effects are hit or miss due to obscure reasons.");
	opt.add_flag_option(
			L"fxradio2",
			[&]() {
				voice.compression = wsy::compression_e::gsm610;
				voice.bit_depth = wsy::bit_depth_e::_8;
				voice.sampling_rate = wsy::sampling_rate_e::_8;
				return true;
			},
			L"Degrades audio to make it sound a little more like a radio.\n"
			"Note : Radio effects are hit or miss due to obscure reasons.");
	// opt.add_flag_option(
	//		L"fxradio3",
	//		[&]() {
	//			voice.compression = wsy::compression_e::none;
	//			voice.bit_depth = wsy::bit_depth_e::_8;
	//			voice.sampling_rate = wsy::sampling_rate_e::_11;
	//			return true;
	//		},
	//		L"Degrades audio to make it sound a little more like a radio.\n"
	//		"Note : Radio effects are hit or miss due to obscure reasons.");

	std::wstring help_outro = L"wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += L"\nhttps://github.com/p-groarke/wsay/releases\n";
	help_outro += L"Philippe Groarke <hello@philippegroarke.com>";
	opt.add_help_outro(help_outro);

	if (!speech_text.empty()) {
		// We have some text coming from pipe, or elsewhere.
		opt.no_options_is_ok();
	}

	if (!opt.parse_options(argc, argv)) {
		return -1;
	}

	// if (interactive_mode) {
	//	std::wcout << L"[Info] Type sentences, press enter to speak them.\n";
	//	std::wcout << std::format(
	//			L"[Command] '{}' : Exit interactive mode.\n", exit_cmd);
	//	std::wcout << std::format(
	//			L"[Command] '{}' : Interrupt speaking.\n", shutup_cmd);
	//	std::wcout << L"\n";

	//	std::wstring wsentence;
	//	while (std::getline(std::wcin, wsentence)) {
	//		if (wsentence == exit_cmd) {
	//			break;
	//		}

	//		if (wsentence == shutup_cmd) {
	//			engine.stop_speaking();
	//			continue;
	//		}

	//		voice.speak_async(wsentence);
	//	}
	//	return 0;
	//}

	engine.speak(voice, speech_text);
	return 0;
}

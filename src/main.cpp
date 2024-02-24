#include "private_include/util.hpp"

#include <fea/getopt/getopt.hpp>
#include <fea/terminal/pipe.hpp>
#include <fea/terminal/utf8.hpp>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <wsay/engine.hpp>
#include <wsay/voice.hpp>

const std::wstring exit_cmd = L"!exit";
const std::wstring shutup_cmd = L"!stop";

int wmain(int argc, wchar_t** argv, wchar_t**) {
	fea::fast_iostreams();
	auto on_exit_reset_term = fea::utf8_terminal(true);

	wsy::engine engine;
	wsy::voice voice;

	bool parsetext = true;
	bool interactive_mode = false;
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
				parsetext = false;
				return true;
			},
			L"Disable speech xml detection. Use this if the text contains "
			L"special characters that aren't speech xml.\n"
			L"Also disables text parsing as if calling with '--nosmart'.",
			L'X');

	// opt.add_flag_option(
	//		L"nosmart",
	//		[&]() {
	//			parsetext = false;
	//			return true;
	//		},
	//		L"Disables text parsing and reads text as-is.", L'S');

	opt.add_required_arg_option(
			L"fxradio",
			[&](std::wstring&& f) {
				size_t fx = std::stoul(f) - 1;
				if (fx >= size_t(wsy::effect_e::count)) {
					std::wcerr << std::format(
							L"Selected radio effect out of range. Choose "
							L"between '1' and '{}'.\n",
							wsy::num_radio_fx());
					return false;
				}

				voice.effect(wsy::effect_e(fx));
				return true;
			},
			std::format(L"Degrades audio to make it sound like a "
						L"radio.\nSupported values : 1 to {}.\n",
					wsy::num_radio_fx()));

	std::wstring help_outro = L"wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += L" ALPHA";
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

	if (interactive_mode) {
		wsy::async_token tok = engine.make_async_token(voice);

		std::wcout << L"[Info] Type sentences, press enter to speak them.\n";
		std::wcout << std::format(
				L"[Command] '{}' : Exit interactive mode.\n", exit_cmd);
		std::wcout << std::format(
				L"[Command] '{}' : Interrupt speaking.\n", shutup_cmd);
		std::wcout << L"\n";

		std::wstring wsentence;
		while (std::getline(std::wcin, wsentence)) {
			if (wsentence == exit_cmd) {
				break;
			}

			if (wsentence == shutup_cmd) {
				engine.stop(tok);
				continue;
			}

			// if (parsetext) {
			//	add_speech_xml(wsentence);
			// }
			engine.speak_async(wsentence, tok);
		}
		return 0;
	} else {
		// if (parsetext) {
		//	add_speech_xml(speech_text);
		// }
		engine.speak(voice, speech_text);
	}

	return 0;
}

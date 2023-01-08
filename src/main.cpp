#include <clocale>
#include <cstdio>
#include <fcntl.h>
#include <fea/getopt/getopt.hpp>
#include <fea/serialize/serializer.hpp>
#include <fea/string/conversions.hpp>
#include <fea/terminal/win_term.hpp>
#include <fea/utils/error.hpp>
#include <fea/utils/file.hpp>
#include <fea/utils/scope.hpp>
#include <filesystem>
#include <io.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <wsay/wsay.hpp>

const std::wstring exit_cmd = L"!exit";
const std::wstring shutup_cmd = L"!stop";

std::wstring get_clipboard_text() {
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		fwprintf(stderr, L"Clipboard doesn't contain text.\n");
		return {};
	}

	if (!OpenClipboard(nullptr)) {
		fwprintf(stderr, L"Couldn't open clipboard for reading.\n");
		std::error_code ec = fea::last_os_error();
		std::wstring wmsg = fea::utf8_to_utf16_w(ec.message());
		fwprintf(stderr, L"Error message : '%s'\n", wmsg.c_str());
		return {};
	}
	fea::on_exit e = []() { CloseClipboard(); };

	HANDLE data = GetClipboardData(CF_UNICODETEXT);
	if (data == nullptr) {
		fwprintf(stderr, L"Couldn't read clipboard data.\n");
		std::error_code ec = fea::last_os_error();
		std::wstring wmsg = fea::utf8_to_utf16_w(ec.message());
		fwprintf(stderr, L"Error message : '%s'\n", wmsg.c_str());
		return {};
	}

	const wchar_t* clip_text = static_cast<const wchar_t*>(GlobalLock(data));
	if (clip_text == nullptr) {
		fwprintf(stderr, L"Couldn't convert clipboard data to string.\n");
		return {};
	}
	fea::on_exit e2 = [&]() { GlobalUnlock(data); };

	return std::wstring{ clip_text };
}

std::wstring get_pipe_text() {
	// To fix pipe input, use U8TEXT (and not U16).
	int res = _setmode(_fileno(stdin), _O_U8TEXT);
	res = _setmode(_fileno(stdout), _O_U8TEXT);
	fea::unused(res);

	fea::on_exit e = []() {
		// Reset afterwards.
		std::wcin.clear();

		int res = _setmode(_fileno(stdin), _O_U16TEXT);
		res = _setmode(_fileno(stdout), _O_U16TEXT);
		fea::unused(res);
	};

	// Check if we have anything in cin.
	std::wcin.seekg(0, std::wcin.end);
	std::streamoff cin_count = std::wcin.tellg();
	std::wcin.seekg(0, std::wcin.beg);

	if (cin_count <= 0) {
		return {};
	}

	// wprintf(L"Pipe detected, enabling pipe mode.\n");
	std::wstring ret;
	std::wstring pipe;
	while (std::getline(std::wcin, pipe)) {
		ret.insert(ret.end(), pipe.begin(), pipe.end());
		ret += L'\n';
	}
	// wprintf(L"Pipe text :\n%s\n", ret.c_str());
	return ret;
}

int wmain(int argc, wchar_t** argv, wchar_t**) {
	auto on_exit_reset_term = fea::win_utf8_terminal(true);

	wsy::voice voice;

	bool interactive_mode = false;
	bool clipboard_mode = false;
	size_t chosen_voice = 0;
	std::wstring speech_text = get_pipe_text();

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
				wprintf(L"\n");
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

				wprintf(L"\tUse 'all' to select every device.\n\n");

				return true;
			},
			L"List detected playback devices.", L'd');

	opt.add_multi_arg_option(
			L"playback_device",
			[&](std::vector<std::wstring>&& arr) {
				if (arr.size() == 1 && arr.front() == L"all") {
					for (size_t i = 0; i < voice.available_devices_size();
							++i) {
						voice.enable_device_playback(i);
					}
					return true;
				}

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
			"can also mix output to file + playback.\nUse 'all' to select all "
			"devices.",
			L'p');

	opt.add_required_arg_option(
			L"volume",
			[&](std::wstring&& f) {
				uint16_t vol = uint16_t(std::stoul(f));
				voice.set_volume(vol);
				return true;
			},
			L"Sets the voice volume, from 0 to 100.", L'V');

	opt.add_required_arg_option(
			L"speed",
			[&](std::wstring&& f) {
				uint16_t speed = uint16_t(std::stoul(f));
				voice.set_speed(speed);
				return true;
			},
			L"Sets the voice speed, from 0 to 100. 50 is the default speed.",
			L's');

	opt.add_flag_option(
			L"clipboard",
			[&]() {
				clipboard_mode = true;
				return true;
			},
			L"Speak currently copied text (the text in your clipboard).", L'c');

	std::wstring help_outro = L"wsay\nversion ";
	help_outro += WSAY_VERSION;
	help_outro += L"\nhttps://github.com/p-groarke/wsay/releases\n";
	help_outro += L"Philippe Groarke <hello@philippegroarke.com>";
	opt.add_help_outro(help_outro);

	if (!speech_text.empty()) {
		// We have some text coming from pipe, or elsewhere.
		opt.no_options_is_ok();
	}

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
		return 0;
	}

	if (clipboard_mode) {
		speech_text = get_clipboard_text();
		if (speech_text.empty()) {
			return -1;
		}

		wprintf(L"Playing clipboard text : '%s'\n", speech_text.c_str());
	}

	voice.speak(speech_text);
	return 0;
}

#pragma warning(disable : 4996)
#include "nuklear.hpp"
#include "window.hpp"

#include <fea/string/conversions.hpp>
#include <shlobj.h>
#include <wil/result.h>
#include <wsay/wsay.hpp>

namespace {
constexpr size_t default_text_buf_size = 2000;
} // namespace

int WinMain(HINSTANCE, HINSTANCE, char*, int) {
	wsy::voice voice;

	std::vector<std::string> available_voices;
	std::vector<const char*> voices;

	{
		std::vector<std::wstring> wavailable_voices = voice.available_voices();

		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		for (const std::wstring& str : wavailable_voices) {
			available_voices.push_back(converter.to_bytes(str));
		}
		for (const std::string& str : available_voices) {
			voices.push_back(str.c_str());
		}
	}

	std::vector<std::string> available_devices;
	std::vector<const char*> devices;
	std::vector<int> active_devices;

	{
		std::vector<std::wstring> wavailable_devices
				= voice.available_devices();

		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		for (const std::wstring& str : wavailable_devices) {
			available_devices.push_back(converter.to_bytes(str));
		}
		for (const std::string& str : available_devices) {
			devices.push_back(str.c_str());
			active_devices.push_back(false);
		}
	}


	window w{ 900, 450 };

	bool interactive_mode = false;
	int selected_voice = 0;
	std::string input_text(default_text_buf_size, '\0');

	w.on_filedrop([&](std::filesystem::path&& path) {
		std::wstring str;
		wsy::parse_text_file(path, str);

		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		input_text = converter.to_bytes(str);
	});

	w.on_gui([&](nk_context* ctx, float w_width, float w_height) {
		float left_win_width = w_width / 3.f;
		float right_win_xpos = left_win_width;
		float right_win_width = w_width - left_win_width;

		if (nk_begin(ctx, "Options", nk_rect(0, 0, left_win_width, w_height),
					NK_WINDOW_TITLE | NK_WINDOW_BORDER
							| NK_WINDOW_NO_SCROLLBAR)) {

			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "Voice:", NK_TEXT_LEFT);

			int prev_voice = selected_voice;
			selected_voice = nk_combo(ctx, voices.data(), int(voices.size()),
					selected_voice, 0, nk_vec2(400, 600));

			if (prev_voice != selected_voice) {
				voice.select_voice(size_t(selected_voice));
				if (!input_text.empty() && input_text[0] != '\0') {
					std::wstring text
							= { input_text.begin(), input_text.end() };
					voice.speak_async(text);
				}
			}

			nk_label(ctx, "", NK_TEXT_LEFT);

			nk_label(ctx, "Playback Devices:", NK_TEXT_LEFT);

			for (size_t i = 0; i < devices.size(); ++i) {
				int prev_val = active_devices[i];
				active_devices[i]
						= nk_check_label(ctx, devices[i], active_devices[i]);

				if (prev_val != active_devices[i]) {
					if (active_devices[i]) {
						voice.enable_device_playback(i);
					} else {
						voice.disable_device_playback(i);
					}
				}
			}

			nk_label(ctx, "", NK_TEXT_LEFT);
			nk_label(ctx, "", NK_TEXT_LEFT);
			nk_label(ctx, "", NK_TEXT_LEFT);
			nk_label(ctx, "", NK_TEXT_LEFT);
			nk_labelf(ctx, NK_TEXT_LEFT, "WSay v%s (MEGA SUPER ALPHA)",
					WSAY_VERSION);
			nk_label(ctx, "https://github.com/p-groarke/wsay/releases",
					NK_TEXT_LEFT);
		}
		nk_end(ctx);

		if (nk_begin(ctx, "Main",
					nk_rect(right_win_xpos, 0, right_win_width, w_height),
					NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {

			nk_layout_row_dynamic(ctx, 0, 1);
			nk_label(ctx, "Drag and drop txt file.", NK_TEXT_LEFT);
			nk_label(ctx, "or", NK_TEXT_LEFT);
			nk_label(ctx, "Enter Text:", NK_TEXT_LEFT);

			nk_flags edit_box_flags = NK_EDIT_ALWAYS_INSERT_MODE
					| NK_EDIT_SELECTABLE | NK_EDIT_CLIPBOARD | NK_EDIT_SIG_ENTER
					| NK_EDIT_CTRL_ENTER_NEWLINE | NK_EDIT_MULTILINE
					| NK_EDIT_NO_HORIZONTAL_SCROLL;

			nk_layout_row_dynamic(ctx, 300, 1);
			int edit_event = nk_edit_string_zero_terminated(ctx, edit_box_flags,
					input_text.data(), int(input_text.size()),
					nk_filter_default);

			nk_layout_row_dynamic(ctx, 0, 1);
			struct nk_rect bounds = nk_widget_bounds(ctx);
			interactive_mode
					= nk_check_label(ctx, "Interactive Mode", interactive_mode);
			if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds)) {
				nk_tooltip(ctx,
						"Clears text when pressing 'Speak' or the 'Enter' "
						"key.");
			}

			if (edit_event & NK_EDIT_COMMITED) {
				std::wstring text = { input_text.begin(), input_text.end() };
				voice.speak_async(text);
				if (interactive_mode) {
					input_text.clear();
					input_text.resize(default_text_buf_size, '\0');
				}
			}

			nk_label(ctx, "", NK_TEXT_LEFT);

			nk_layout_row_dynamic(ctx, 0, 4);
			if (nk_button_label(ctx, "Shutup")) {
				voice.stop_speaking();
			}

			nk_label(ctx, "", NK_TEXT_LEFT);

			bounds = nk_widget_bounds(ctx);
			if (nk_button_label(ctx, "Save To File")) {
				std::wstring text = { input_text.begin(), input_text.end() };

				PWSTR path;
				if (SUCCEEDED(SHGetKnownFolderPath(
							FOLDERID_Desktop, 0, nullptr, &path))) {
					std::filesystem::path filepath = path;
					filepath += "/out.wav";
					voice.start_file_output(filepath);
					voice.speak(text);
					voice.stop_file_output();
				}
			}
			if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds)) {
				nk_tooltip(ctx, "Saves to Desktop : out.wav");
			}

			if (nk_button_label(ctx, "Speak")) {
				std::wstring text = { input_text.begin(), input_text.end() };
				voice.speak_async(text);
				if (interactive_mode) {
					input_text.clear();
					input_text.resize(default_text_buf_size, '\0');
				}
			}
		}
		nk_end(ctx);
	});

	w.run();

	return 0;
}

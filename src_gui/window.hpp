#pragma once
#include <filesystem>
#include <functional>
#include <memory>

struct nk_context;
struct nk_font_atlas;
struct GLFWwindow;

struct window {
	window(int width, int height);
	~window();

	void on_gui(std::function<void(nk_context*, float, float)> on_gui_callback);
	void on_filedrop(
			std::function<void(std::filesystem::path)> on_filedrop_callback);

	// Blocking
	void run();

private:
	void render();

	static void on_resize_callback(
			GLFWwindow* glfw_window, int width, int height);
	static void on_filedrop_callback(
			GLFWwindow* glfw_window, int count, const char** paths);

	int _width;
	int _height;

	std::function<void(nk_context*, float, float)> _gui_callback;
	std::function<void(std::filesystem::path)> _filedrop_callback;
	std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>> _window_ptr;

	nk_context* _ctx;
	nk_font_atlas* _atlas;
};

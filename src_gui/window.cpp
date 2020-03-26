#include "window.hpp"

#include <glad/glad.h>

#define NK_IMPLEMENTATION
#include "nuklear.hpp"
#pragma warning(push, 3)
#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear_glfw_gl3.h"
#pragma warning(pop)

namespace {
constexpr int max_vertex_buffers = 512 * 1024;
constexpr int max_element_buffers = 128 * 1024;

inline window* get_my_window(GLFWwindow* w) {
	return reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
}

void on_error_callback(int, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

} // namespace

window::window(int width, int height)
		: _width(width)
		, _height(height) {

	// glfw
	glfwSetErrorCallback(on_error_callback);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to intialize GLFW.\n");
		std::exit(-1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	_window_ptr = { // clangformat
		glfwCreateWindow(_width, _height, "WSay", nullptr, nullptr),
		[this](GLFWwindow* w) { glfwDestroyWindow(w); }
	};

	if (_window_ptr == nullptr) {
		fprintf(stderr, "Couldn't create window.\n");
		std::exit(-1);
	}

	glfwMakeContextCurrent(_window_ptr.get());
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);
	glfwSetWindowUserPointer(_window_ptr.get(), this);
	glfwSetWindowSizeLimits(
			_window_ptr.get(), _width, _height, GLFW_DONT_CARE, GLFW_DONT_CARE);

	// callbacks
	glfwSetFramebufferSizeCallback(_window_ptr.get(), on_resize_callback);
	glfwSetDropCallback(_window_ptr.get(), on_filedrop_callback);

	glfwGetWindowSize(_window_ptr.get(), &_width, &_height);
	glViewport(0, 0, _width, _height);

	if (!glfwGetWindowAttrib(_window_ptr.get(), GLFW_FOCUSED)) {
		glfwRequestWindowAttention(_window_ptr.get());
	}

	// nuklear
	_ctx = nk_glfw3_init(_window_ptr.get(), NK_GLFW3_INSTALL_CALLBACKS);
	nk_glfw3_font_stash_begin(&_atlas);
	nk_glfw3_font_stash_end();

	_ctx->style.window.spacing = nk_vec2(10, 10);
	_ctx->style.window.padding = nk_vec2(10, 2);
	_ctx->style.window.header.label_padding = nk_vec2(2, 10);
	_ctx->style.window.header.padding = nk_vec2(10, 10);

	//_ctx->style.window.combo_padding = nk_vec2(50, 10);
	//_ctx->style.combo.spacing = nk_vec2(50, 10);
}


window::~window() {
	glfwTerminate();
}

void window::on_gui(
		std::function<void(nk_context*, float, float)> on_gui_callback) {
	_gui_callback = on_gui_callback;
}

void window::on_filedrop(
		std::function<void(std::filesystem::path)> on_filedrop_callback) {
	_filedrop_callback = on_filedrop_callback;
}

void window::run() {
	while (!glfwWindowShouldClose(_window_ptr.get())) {
		glfwPollEvents();
		render();
	}
}

void window::render() {
	nk_glfw3_new_frame();

	std::invoke(_gui_callback, _ctx, float(_width), float(_height));

	glfwGetWindowSize(_window_ptr.get(), &_width, &_height);
	glViewport(0, 0, _width, _height);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.f, 0.f, 0.f, 1.f);

	/* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
	 * with blending, scissor, face culling, depth test and viewport and
	 * defaults everything back into a default state.
	 * Make sure to either a.) save and restore or b.) reset your own state
	 * after rendering the UI. */
	nk_glfw3_render(
			NK_ANTI_ALIASING_ON, max_vertex_buffers, max_element_buffers);
	glfwSwapBuffers(_window_ptr.get());
}

void window::on_resize_callback(
		GLFWwindow* glfw_window, int width, int height) {
	window* w = get_my_window(glfw_window);
	w->_width = width;
	w->_height = height;
	w->render();
}

void window::on_filedrop_callback(
		GLFWwindow* glfw_window, int count, const char** paths) {
	window* w = get_my_window(glfw_window);
	if (count == 0) {
		return;
	}

	std::invoke(w->_filedrop_callback, std::filesystem::path{ paths[0] });
}

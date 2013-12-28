#include "../window.h"
#include "../tween.h"

#include "../GL_Header.h"
#include <GL/glfw.h>
#include <stdio.h>
#include <unordered_map>
#include <functional>
//#include <math.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace std;

namespace grappix {
void debug_callback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, void* userParam) {
	LOGD("GLDEBUG:%s", message);
}

Window::Window() : RenderTarget(), winOpen(false), bmCounter(0) {
	NO_CLICK.x = NO_CLICK.y = NO_CLICK.button = -1;
}

void Window::open(bool fs) {
	open(0,0,fs);
}

Window::click Window::NO_CLICK = { -1, -1, -1};

std::deque<int> Window::key_buffer;
std::deque<Window::click> Window::click_buffer;

static void key_fn(int key, int action) {
	if(action == GLFW_PRESS)
		Window::key_buffer.push_back(key);
}

static void mouse_fn(int button, int action) {
	if(action == GLFW_PRESS) {
		Window::click c;
		glfwGetMousePos(&c.x, &c.y);
		c.button = button;
		Window::click_buffer.push_back(c);
	}
}

#ifndef EMSCRIPTEN
static void resize_fn(int w, int h) {
	LOGD("Size now %d %d", w, h);
	screen.resize(w, h);
};
#endif

static function<void()> renderLoopFunction;
static function<void(uint32_t)> renderLoopFunction2;

void Window::open(int w, int h, bool fs) {

	if(winOpen)
		return;

	LOGD("glfwInit");
	glfwInit();
#ifdef EMSCRIPTEN
	if(w < 0) w = 640;
	if(h < 0) h = 480;
	_width = w;
	_height = h;
	fs = false;
	GLFWvidmode mode;
	mode.RedBits = mode.GreenBits = mode.BlueBits = 8;
#else
	_width = w;
	_height = h;
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 2);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 0);
	GLFWvidmode mode;
	glfwGetDesktopMode(&mode);

	LOGD("Desktop is %dx%d", mode.Width, mode.Height);
	//mode.Width = 1600;

	if((float)mode.Width / (float)mode.Height > 2.2)
		mode.Width /= 2;

	if(_width <= 0) {
		_width = mode.Width;
		if(!fs)
			_width /= 2;
	}
	if(_height <= 0) {
		_height = mode.Height;
		if(!fs)
			_height /= 2;
	}
#endif

	int win = glfwOpenWindow(_width, _height, mode.RedBits, mode.GreenBits, mode.BlueBits, 8, 0, 0, fs ? GLFW_FULLSCREEN : GLFW_WINDOW);
	LOGD("%dx%d win -> %d", _width, _height, win);
	if(win) {
	}

#ifndef EMSCRIPTEN
	int rc = glewInit();
	if(rc) {
		LOGE("Glew error: %s", glewGetErrorString(rc));
		exit(0);
	}
	glDebugMessageCallbackARB(debug_callback, nullptr);
#endif

	//initPrograms();

	glfwSwapInterval(1);


	//glDebugMessageInsertARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_ERROR_ARB, 1, 
     //        GL_DEBUG_SEVERITY_HIGH_ARB, 5, "YAY! ");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	lastTime = -1;
	winOpen = true;

	glfwSetKeyCallback(key_fn);
	glfwSetMouseButtonCallback(mouse_fn);
#ifndef EMSCRIPTEN
	glfwSetWindowSizeCallback(resize_fn);
#endif

	startTime = chrono::high_resolution_clock::now();
	frameBuffer = 0;
};

#ifdef EMSCRIPTEN
static uint64_t lastMs = 0;
static void runMainLoop() {
	auto ms = utils::getms();
	uint32_t rate = ms - lastMs;
	lastMs = ms;
	if(renderLoopFunction)
		renderLoopFunction();
	else
		renderLoopFunction2(rate);
}
#endif

void Window::render_loop(function<void()> f, int fps) {
	renderLoopFunction = f;
#ifdef EMSCRIPTEN
	lastMs = utils::getms();
	emscripten_set_main_loop(runMainLoop, fps, false);
#else
	//while(screen.is_open()) {
	//	renderLoopFunction();
	//}
	atexit([](){
		while(screen.is_open()) {
			renderLoopFunction();
		}
	});

#endif
}

void Window::render_loop(function<void(uint32_t)> f, int fps) {
	renderLoopFunction2 = f;
#ifdef EMSCRIPTEN
	emscripten_set_main_loop(runMainLoop, fps, false);
#else
	//while(screen.is_open()) {
	//	renderLoopFunction();
	//}
	atexit([](){
		auto lastMs = utils::getms();
		while(screen.is_open()) {
			auto ms = utils::getms();
			uint32_t rate = ms - lastMs;
			lastMs = ms;
			renderLoopFunction2(rate);
		}
	});

#endif
}

void Window::vsync() {
}

//static uint64_t lastTime;

void Window::flip() {
	auto t = chrono::high_resolution_clock::now();
#ifdef FPS_COUNTER
	auto tm = utils::getms();
	auto d = tm - lastTime;
	if(d > 0)
		fps = fps * 0.8 + (1000 / d) * 0.2;
	lastTime = tm;
	text(utils::format("%d", (int)fps), 0,0);
#endif
	/*if(bmCounter) {
		bmCounter--;
		if(!bmCounter) {
			glfwCloseWindow();
			winOpen = false;
			auto ms = chrono::duration_cast<chrono::microseconds>(t - benchStart).count();
			fprintf(stderr, "TIME: %ldus per frame\n", ms / 100);
		}
		return;
	}*/
	glfwSwapBuffers();
	if(glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED)) {
		glfwCloseWindow();
		winOpen = false;			
	}

	auto ms = chrono::duration_cast<chrono::microseconds>(t - startTime).count();
	tween::Tween::updateTweens(ms / 1000000.0f);
#ifdef EMSCRIPTEN
	int fs;
	emscripten_get_canvas_size(&_width, &_height, &fs);
#endif
}

void Window::benchmark() {
	benchStart = chrono::high_resolution_clock::now();
	bmCounter = 100;
}

unordered_map<int, int> Window::translate = {
	{ F1, GLFW_KEY_F1 },
	{ BACKSPACE, GLFW_KEY_BACKSPACE },
	{ ENTER, GLFW_KEY_ENTER },
	{ ESCAPE, GLFW_KEY_ESC },
	{ SPACE, GLFW_KEY_SPACE },
	{ LEFT, GLFW_KEY_LEFT },
	{ RIGHT, GLFW_KEY_RIGHT },
	{ UP, GLFW_KEY_UP },
	{ DOWN, GLFW_KEY_DOWN },
	{ PAGEUP, GLFW_KEY_PAGEUP },
	{ PAGEDOWN, GLFW_KEY_PAGEDOWN }
};

bool Window::key_pressed(key k) {
	auto glfwKey = translate[k];
	return glfwGetKey(glfwKey) != 0;
}

Window::click Window::get_click() {
	if(click_buffer.size() > 0) {
		auto k = click_buffer.front();
		click_buffer.pop_front();
		return k;
	}
	return NO_CLICK;
}

Window::key Window::get_key() {
	if(key_buffer.size() > 0) {
		auto k = key_buffer.front();
		key_buffer.pop_front();
		for(auto t : translate) {
			if(t.second == k)
				return (key)t.first;
		}
		LOGD("%d", (key)k);
		return (key)k;
	}
	return NO_KEY;
};

Window screen;

}
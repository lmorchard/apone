#include "../window.h"
#include "../resources.h"

#include <tween/tween.h>
#include <coreutils/format.h>
#include <coreutils/utils.h>

#include <linux/input.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unordered_map>
#include <functional>

#include <bcm_host.h>

using namespace std;

//bool initEGL(EGLConfig& eglConfig, EGLContext& eglContext, EGLDisplay& eglDisplay);
bool initEGL(EGLConfig& eglConfig, EGLContext& eglContext, EGLDisplay& eglDisplay, EGLSurface &eglSurface, EGLNativeWindowType nativeWin);

namespace grappix {

Window::Window() : RenderTarget(), winOpen(false), bmCounter(0), lockIt(false) {
	NO_CLICK.x = NO_CLICK.y = NO_CLICK.button = -1;
}

void Window::open(bool fs) {
	open(0,0,fs);
}

std::deque<int> Window::key_buffer;
static uint8_t pressed_keys[512];
//static uint8_t modifiers = 0;

Window::click Window::NO_CLICK = { -1, -1, -1};

static function<void(uint32_t)> renderLoopFunction;

static EGL_DISPMANX_WINDOW_T nativewindow;

static EGLConfig eglConfig;
static EGLContext eglContext;
static EGLDisplay eglDisplay;
static EGLSurface eglSurface;

void Window::open(int w, int h, bool fs) {

	if(winOpen)
		return;

	bcm_host_init();

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	uint32_t display_width;
	uint32_t display_height;

	// create an EGL window surface, passing context width/height
	int success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
	if(success < 0)
		throw display_exception("Cound not get display size");

	// You can hardcode the resolution here:
	//display_width = 640;
	//display_height = 480;
	LOGD("Display %dx%d", display_width, display_height);

	//display_width = 640;
	//display_height = 480;

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = display_width;
	dst_rect.height = display_height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = display_width << 16;
	src_rect.height = display_height << 16;

	dispman_display = vc_dispmanx_display_open(0 /* LCD */);
	dispman_update = vc_dispmanx_update_start(0);

	dispman_element = vc_dispmanx_element_add(dispman_update, 
	dispman_display, 0/*layer*/, &dst_rect, 0/*src*/,
	&src_rect, DISPMANX_PROTECTION_NONE, nullptr /*alpha*/, 
	nullptr/*clamp*/, DISPMANX_NO_ROTATE);
	LOGD("Dispelement %d", dispman_element);

	nativewindow.element = dispman_element;
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync(dispman_update);

	initEGL(eglConfig, eglContext, eglDisplay, eglSurface, &nativewindow);

	//eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, &nativewindow, NULL);
/*
	LOGI("Surface %p", eglSurface);

	if (eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) == EGL_FALSE) {
		LOGI("Unable to eglMakeCurrent");
		return;
	}
*/

	setup(display_width, display_height);
	memset(pressed_keys, 0, sizeof(pressed_keys));
	keyboardThread = thread([=]() {
		vector<uint8_t> buf(64);
		memset(&buf[0], 0x13, 64);
		uint8_t n;
		LOGD("Reading Keyboard");
		int k = ::open("/dev/input/event1", O_RDONLY);
		if(k < 0) {
			LOGW("Could not open keyboard");
			return;
		}
		while(true) {
			int rc = read(k, &buf[0], sizeof(struct input_event) * 4);
			auto *ptr = (struct input_event*)&buf[0];
			if(rc >= sizeof(struct input_event))
				LOGD("[%02x]", buf);
			while(rc >= sizeof(struct input_event)) {

				//int type = ptr[8];
				//int code = ptr[10] | (ptr[11]<<8);
				//int updown = ptr[12];
				LOGD("TYPE %d CODE %d VALUE %d", ptr->type, ptr->code, ptr->value);

				if(ptr->type == EV_KEY) {
					if(ptr->value)
						key_buffer.push_back(ptr->code);
					if(ptr->code < 512)
						pressed_keys[ptr->code] = ptr->value;
				}
/*
				modifiers = ptr[0];
				for(int i=2; i<5; i++) {
					auto k = ptr[i];
					if(k) {
						if(!pressed_keys[k]) {
							fprintf(stderr, utils::format("Got key %02x\n", k).c_str());
							key_buffer.push_back(k);
						}
						pressed_keys[k] = 2;
					}
				}
				for(int i=0; i<256; i++) {
					if(pressed_keys[i])
						pressed_keys[i]--;
				}
*/
				ptr++;
				rc -= sizeof(struct input_event);

			}
			//utils::sleepms(100);
		}
	});
	keyboardThread.detach();

	atexit([](){
		if(!renderLoopFunction) {
			while(screen.is_open()) {
				screen.update_callbacks();
				screen.flip();
			}
		}	
	});
};


void Window::render_loop(function<void(uint32_t)> f, int fps) {
	renderLoopFunction = f;

	//atexit([](){
		auto lastMs = utils::getms();
		while(screen.is_open()) {
			screen.update_callbacks();
			auto ms = utils::getms();
			uint32_t rate = ms - lastMs;
			lastMs = ms;
			renderLoopFunction(rate);
			//while(screen.locked()) {
			//	utils::sleepms(5);
			//}
		}
	//});
}

void Window::flip() {
	if(eglDisplay != EGL_NO_DISPLAY) {
		eglSwapBuffers(eglDisplay, eglSurface);
		//eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &screenWidth);
		//eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &screenHeight);
	}
	//auto t = chrono::high_resolution_clock::now();
	//auto ms = chrono::duration_cast<chrono::microseconds>(t - startTime).count();
	//tween::Tween::updateTweens(ms / 1000000.0f);
	//Resources::getInstance().update();
}

bool Window::mouse_pressed() {
	return false;
}

tuple<int, int> Window::mouse_position() {
	return make_tuple(-1,-1);
}

#define LCTRL 1
#define LSHIFT 2
#define LALT 4
#define LWIN 8
#define RCTRL 0x10
#define RSHIFT 0x20
#define RALT 0x40
#define RWIN 0x80

bool Window::key_pressed(key k) {
	auto rawkey = translate[k];
	return (pressed_keys[rawkey] != 0);
}

bool Window::key_pressed(char k) {
	int rawkey = k - 'A' + 0x4;
	return (pressed_keys[rawkey] != 0);
}

Window::click Window::get_click(bool peek) {
/*	if(click_buffer.size() > 0) {
		auto k = click_buffer.front();
		if(!peek)
			click_buffer.pop_front();
		return k;
	}*/
	return NO_CLICK;
}

unordered_map<int, int> Window::translate = {

	{ 'A', KEY_A },
	{ 'B', KEY_B },
	{ 'C', KEY_C },
	{ 'D', KEY_D },
	{ 'E', KEY_E },
	{ 'F', KEY_F },
	{ 'G', KEY_G },
	{ 'H', KEY_H },
	{ 'I', KEY_I },
	{ 'J', KEY_J },
	{ 'K', KEY_K },
	{ 'L', KEY_L },
	{ 'M', KEY_M },
	{ 'N', KEY_N },
	{ 'O', KEY_O },
	{ 'P', KEY_P },
	{ 'Q', KEY_Q },
	{ 'R', KEY_R },
	{ 'S', KEY_S },
	{ 'T', KEY_T },
	{ 'U', KEY_U },
	{ 'V', KEY_V },
	{ 'W', KEY_W },
	{ 'X', KEY_X },
	{ 'Y', KEY_Y },
	{ 'Z', KEY_Z },
	{ '0', KEY_0 },
	{ ENTER, KEY_ENTER },
	{ SPACE, KEY_SPACE },
	{ PAGEUP, KEY_PAGEUP },
	{ PAGEDOWN, KEY_PAGEDOWN },
	{ RIGHT, KEY_RIGHT },
	{ LEFT, KEY_LEFT },
	{ DOWN, KEY_DOWN },
	{ UP, KEY_UP },
	{ ESCAPE, KEY_ESC },
	{ BACKSPACE, KEY_BACKSPACE },
	{ SHIFT_LEFT, KEY_LEFTSHIFT },
	{ SHIFT_RIGHT, KEY_RIGHTSHIFT },
	{ ALT_LEFT, KEY_LEFTMETA },
	{ ALT_RIGHT, KEY_RIGHTMETA },
	{ CTRL_LEFT, KEY_LEFTCTRL},
	{ CTRL_RIGHT, KEY_RIGHTCTRL }
};

Window::key Window::get_key(bool peek) {
	if(key_buffer.size() > 0) {
		auto k = key_buffer.front();
		if(!peek)
			key_buffer.pop_front();
		if(k >= KEY_1 && k <= KEY_9)
			k += ('1' - KEY_1);
		else if(k >= KEY_F1 && k <= KEY_F10)
			k += (F1-KEY_F1);
		else {
			for(auto t : translate) {
				if(t.second == k)
					return (key)t.first;
			}
		}
		return (key)k;
	}
	return NO_KEY;
};

Window screen;

}
#ifndef FRAME_H
#define FRAME_H

#include "render_target.h"
#include "rectangle.h"

namespace grappix {

class Frame {
public:
	Frame(const Rectangle &r) : r(r) {}
	Frame(float x, float y, float w, float h) : r(Rectangle(x,y,w,h)) {}
	Frame() : r(0,0,0,0) {}

	operator bool() {
		return r.w > 0;
	}

	void set(const RenderTarget &target) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(r.x, target.height()-r.h-r.y, r.w, r.h);
	}

	void unset() {
		glDisable(GL_SCISSOR_TEST);
	}

	int width() { return r.w; }
	int height() { return r.h; }

	const Rectangle &rec() { return r;}

private:
	Rectangle r;
};

} // grappix

#endif // FRAME_H


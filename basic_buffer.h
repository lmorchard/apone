#ifndef GRAPPIX_BASIC_BUFFER_H
#define GRAPPIX_BASIC_BUFFER_H

#include <freetype-gl.h>

#include <vector>
#include <string>
#include <stdint.h>
#include <memory>

struct attribute {
	attribute(GLuint handle, uint16_t offset, uint16_t stride) : handle(handle), offset(offset), stride(stride) {}
	GLuint handle;
	uint16_t offset;
	uint16_t stride;
};

struct uniform {
	uniform(GLuint handle, float f) : handle(handle), type(1), f{f,0,0,0} {}
	GLuint handle;
	int type;
	float f[4];
};

struct gl_object {
	int primitive;
	GLuint vertexBuf;
	GLuint indexBuf;
	short iCount;
	short vCount;
	GLuint texture;

	GLuint program;

	std::vector<attribute> attributes;
	std::vector<uniform> uniforms;

};

class basic_buffer {
public:

	basic_buffer() : frameBuffer(0), _width(0), _height(0), globalScale(1.0) {}
	basic_buffer(unsigned int buffer, int width, int height) : frameBuffer(buffer), _width(width), _height(height), globalScale(1.0) {
	}

	void draw_object(const gl_object &vbo, float x, float y, uint32_t color = 0xffffffff, float scale = 1.0f, float rotation = 0.0f);
	gl_object make_rectangle(float w, float h);

	template <typename T> void line(T p0, T p1, uint32_t color) {
		line(p0[0], p0[1], p1[0], p1[1], color);
	}

	void line(const std::vector<int> &p0, const std::vector<int> &p1, uint32_t color) {
		line(p0[0], p0[1], p1[0], p1[1], color);
	}
	void clear();
	void line(float x0, float y0, float x1, float y1, uint32_t color);

	template <typename T> void circle(T xy, float radius, uint32_t color) {
		circle(xy[0], xy[1], radius, color);
	}

	template <typename T> void rectangle(T pos, T size, uint32_t color, float scale = 1.0) {
		rectangle(pos[0], pos[1], size[0], size[1], color, scale);
	}

	template <typename T> void rectangle(T pos, int w, int h, uint32_t color, float scale = 1.0) {
		rectangle(pos[0], pos[1], w, h, color, scale);
	}

	void rectangle(float x, float y, float w, float h, uint32_t color, float scale = 1.0);

	void circle(int x, int y, float radius, uint32_t color);
	void draw_texture(int texture, float x0, float y0, float w, float h, float *uvs = nullptr, int program = -1);
	int width() { return _width; }
	int height() { return _height; }

	float scale() {  return globalScale; }
	float scale(float s) { globalScale = s; return s; }

	enum {
		DISTANCE_MAP = 1
	};

	void set_font(const std::string &ttfName, int size = 32, int flags = DISTANCE_MAP);
	void text(int x, int y, const std::string &text, uint32_t color, float scale);

protected:
	std::vector<uint> make_text(const std::string &text);
	void render_text(int x, int y, std::vector<uint> vbuf, int tl, uint32_t color, float scale);

	unsigned int frameBuffer;
	int _width;
	int _height;
	float globalScale;

	std::shared_ptr<texture_font_t> font;
	std::shared_ptr<texture_atlas_t> atlas;

};

#endif // GRAPPIX_BASIC_BUFFER_H

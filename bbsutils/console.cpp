#include <coreutils/log.h>
#include "console.h"
//#include "utils.h"

#include <array>
#include <algorithm>

namespace bbs {

using namespace std;

/** PETSCII->ASCII 0x20 -> 0xFF **/

static int petsciiTable[] = {
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,
	0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,0x0030,0x0031,
	0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,
	0x003C,0x003D,0x003E,0x003F,0x0040,0x0061,0x0062,0x0063,0x0064,0x0065,
	0x0066,0x0067,0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,
	0x007A,0x005B,0x00A3,0x005D,0x2191,0x2190,0x2501,0x0041,0x0042,0x0043,
	0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,
	0x004E,0x004F,0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,
	0x0058,0x0059,0x005A,0x253C,0xF12E,0x2502,0x2592,0xF139,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x000A,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x00A0,
	0x258C,0x2584,0x2594,0x2581,0x258F,0x2592,0x2595,0xF12F,0xF13A,0xF130,
	0x251C,0xF134,0x2514,0x2510,0x2582,0x250C,0x2534,0x252C,0x2524,0x258E,
	0x258D,0xF131,0xF132,0xF133,0x2583,0x2713,0xF135,0xF136,0x2518,0xF137,
	0xF138,0x2501,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,
	0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,0x0050,0x0051,0x0052,
	0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005A,0x253C,0xF12E,
	0x2502,0x2592,0xF139,0x00A0,0x258C,0x2584,0x2594,0x2581,0x258F,0x2592,
	0x2595,0xF12F,0xF13A,0xF130,0x251C,0xF134,0x2514,0x2510,0x2582,0x250C,
	0x2534,0x252C,0x2524,0x258E,0x258D,0xF131,0xF132,0xF133,0x2583,0x2713,
	0xF135,0xF136,0x2518,0xF137,0x2592
};



static uint8_t c64pal [] = {
	0xFF, 0xFF, 0xFF, // WHITE
	0x68, 0x37, 0x2B, // RED
	0x58, 0x8D, 0x43, // GREEN
	0x35, 0x28, 0x79, // BLUE
	0x6F, 0x4F, 0x25, // ORANGE
	0x00, 0x00, 0x00, // BLACK
	0x43, 0x39, 0x00, // BROWN
	0x9A, 0x67, 0x59, // LIGHT_READ
	0x44, 0x44, 0x44, // DARK_GREY
	0x6C, 0x6C, 0x6C, // GREY
	0x9A, 0xD2, 0x84, // LIGHT_GREEN
	0x6C, 0x5E, 0xB5, // LIGHT_BLUE
	0x95, 0x95, 0x95, // LIGHT_GREY
	0x6F, 0x3D, 0x86, // PURPLE
	0xB8, 0xC7, 0x6F, // YELLOW
	0x70, 0xA4, 0xB2, // CYAN
};

static vector<uint8_t> petsciiColors = { 5, 28, 30, 31, 129, 144, 149, 150, 151, 152, 153, 154, 155, 156, 158, 159 };
/*
	enum Color {
		WHITE, //15
		RED, //1
		GREEN, //2
		BLUE,//4
		ORANGE,
		BLACK, //0
		BROWN,
		PINK, //9
		DARK_GREY, //8
		GREY,
		LIGHT_GREEN, //10
		LIGHT_BLUE, //12
		LIGHT_GREY, //7
		PURPLE, //5
		YELLOW,//3
		CYAN //6
	};
*/

//static int ansiColors[] = { 15, 1, 2, 4, 11, 0, 13, 9, 8, 14, 10, 12, 7, 5, 3, 6 };
//static int ansiColorsBg[] = {};



LocalTerminal localTerminal;

void Console::clear() {
	for(auto &t : grid) {
		t.c = 0x20;
		t.fg = WHITE;
		t.bg = BLACK;
	}
	for(auto &t : oldGrid) {
		t.c = 0x20;
		t.fg = WHITE;
		t.bg = BLACK;
	}
	impl_clear();
	curX = curY = 0;
}

void Console::refresh() {
	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			Tile &t0 = oldGrid[x+y*width];
			t0.c = -1;
			t0.fg = -1;
			t0.bg = -1;
		}
	}
	impl_clear();
	curX = curY = 0;
	flush();
}

void Console::fill(int bg, int x, int y, int w, int h) {

	if(x < 0) x = width+x;
	if(y < 0) y = height+y;
	if(w <= 0) w = width+w;
	if(h <= 0) h = height+w;

	if(bg == CURRENT_COLOR)
		bg = bgColor;
	for(int yy = y; yy < y + h; yy++)
		for(int xx = x; xx < x + w; xx++) {
			auto &t = grid[xx + width * yy];
			if(fgColor >= 0) t.fg = fgColor;
			if(bg >= 0) t.bg = bg;
			t.c = 0x20;
		}
}

void Console::put(int x, int y, const string &text, int fg, int bg) {

	if(x < 0) x = width+x;
	if(y < 0) y = height+y;

	if(y >= height)
		return;
	for(int i=0; i<(int)text.length(); i++) {

		if(x+i >= width)
			return;

		auto &t = grid[(x+i) + y * width];
		//LOGD("put to %d %d",x+i,y);	
		t.c = (Char)(text[i] & 0xff);
		impl_translate(t.c);

		if(fg == CURRENT_COLOR)
			fg = fgColor;
		if(bg == CURRENT_COLOR)
			bg = bgColor;

		if(fg >= 0)
			t.fg = fg;
		if(bg >= 0)
			t.bg = bg;
	}
}

void Console::resize(int w, int h) {
	width = w;
	height = h;
	LOGD("Resize");
	grid.resize(w*h);
	oldGrid.resize(w*h);
	clear();
}

void Console::flush() {

	auto w = terminal.getWidth();
	auto h = terminal.getHeight();
	if((w > 0 && w != width) || (h > 0 && h != height)) {
		resize(w, h);
	}

	auto saveX = curX;
	auto saveY = curY;

	//int saveFg = fgColor;
	//int saveBg = bgColor;

	//LOGD("flush");
	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			auto &t0 = oldGrid[x+y*width];
			auto &t1 = grid[x+y*width];
			if(t0 != t1) {
				//LOGD("diff in %d %d",x,y);			
				if(curY != y or curX != x) {
					impl_gotoxy(x, y);
					curX = x;
					curY = y;
				}
				if(t1.fg != curFg || t1.bg != curBg) {
					impl_color(t1.fg, t1.bg);
					curFg = t1.fg;
					curBg = t1.bg;
				}
				putChar(t1.c);
				t0 = t1;
			}
		}
	}

	if(curFg != fgColor || curBg != bgColor) {
		if(fgColor >= 0 && bgColor >= 0)
			impl_color(fgColor, bgColor);
		curFg = fgColor;
		curBg = bgColor;
	}

	if(outBuffer.size() > 0) {
		LOGV("OUTBYTES: [%02x]", outBuffer);
		terminal.write(outBuffer, outBuffer.size());
		outBuffer.resize(0);
	}

	//LOGD("Restorting cursor");

	impl_gotoxy(saveX, saveY);
	//curX = saveX;
	//curY = saveY;
}

void Console::putChar(Char c) {
	outBuffer.push_back(c & 0xff);
	curX++;
	if(curX >= width) {
		curX -= width;
		curY++;
	}
}

void Console::moveCursor(int x, int y) {

	if(x < 0) x = width+x;
	if(y < 0) y = height+y;

	impl_gotoxy(x, y);
	if(outBuffer.size() > 0) {
		terminal.write(outBuffer, outBuffer.size());
		outBuffer.resize(0);
	}
	//curX = x;
	//curY = y;
}

void Console::write(const std::string &text) {

	auto x = curX;
	auto y = curY;

	//LOGD("Putting %s to %d,%d", text, x, y);

	while(y >= height) {
		scrollScreen(1);
		y--;
	}
	int spaces = 0;
	for(int i=0; i<(int)text.length(); i++) {
	//for(const auto &c : text) {
		char c;
		if(spaces) {
			spaces--;
			i--;
			c = ' ';
		} else {
			c = text[i];
			if(c == '\t') {
				spaces = 4;
				continue;
			}
		}


		if(x >= width || c == 0xa || c == 0xd) {
			x = 0;
			y++;
			if(y >= height) {
				scrollScreen(1);
				y--;
			}

			if(c == 0xd) {
				if(text[i+1] == 0xa);
					i++;
				c = 0xa;
			}

			if(c == 0xa)
				continue;
		}

		auto &t = grid[x + y * width];
		x++;
		//LOGD("put to %d %d",x+i,y);	
		t.c = (Char)(c & 0xff);
		impl_translate(t.c);
		if(fgColor >= 0)
			t.fg = fgColor;
		if(bgColor >= 0)
			t.bg = bgColor;
		//flush();
		//this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	LOGD("%d/%d", curX, curY);
	flush();
	moveCursor(x, y);
	LOGD("%d/%d", curX, curY);
}

int Console::getKey(int timeout) {

	std::chrono::milliseconds ms { 100 };

	std::vector<uint8_t> temp;
	temp.reserve(16);

	while(true) {
		auto rc = terminal.read(temp, 16);
		if(rc > 0) {
			//LOGD("Got %d bytes", rc);
			for(int i=0; i<rc; i++) {
				//LOGD("Pushing %d", (int)temp[i]);
				inBuffer.push(temp[i]);
			}
		}
		if(inBuffer.size() > 0) {
			//LOGD("Size %d", inBuffer.size());
			return impl_handlekey();
		}

		std::this_thread::sleep_for(ms);
		if(timeout >= 0) {
			timeout -= 100;
			if(timeout < 0)
				return KEY_TIMEOUT;
		}
	}
}

std::string Console::getLine() {
	string line;

	auto startX = curX;
	auto startY = curY;

	auto fg = fgColor;
	auto bg = bgColor;

	//getLineStarted = true;

	int x = 0;
	//bool lineChanged = false;

	while(true) {
		auto c = getKey(500);
		auto lastLen = line.length();
		switch(c) {
		case KEY_ENTER:
			return line;
		case KEY_BACKSPACE:
			if(x > 0) {
				x--;				
				line.erase(x, 1);
			}
			break;
		case KEY_DELETE:
			if(x < (int)line.length()) {
				line.erase(x, 1);
			}
			break;
		case KEY_LEFT:
			if(x > 0)
				x--;
			break;
		case KEY_HOME:
			x = 0;
			break;
		case KEY_END:
			x = line.length();
			break;
		case KEY_RIGHT:
			if(x < (int)line.length())
				x++;
			break;
		default:
			if(c < 256) {
				line.insert(x, 1, c);
				x++;
			}
			break;
		}
		{
			put(startX, startY, line, fg, bg);
			if(lastLen > line.length())
				put(startX+line.length(), startY, " ", fg, bg);
			LOGD("Line now '%s'", line);
			flush();
			moveCursor(startX + x, startY);
		}
	}
}

// Shift all tiles
void Console::shiftTiles(vector<Tile> &tiles, int dx, int dy) {
	auto tempTiles = tiles;
	for(int y = 0; y<height; y++) {
		for(int x = 0; x<width; x++) {
			int tx = x - dx;
			int ty = y - dy;
			if(tx >= 0 && ty >= 0 && tx < width && ty < height)
				tiles[tx+ty*width] = tempTiles[x+y*width];
		}
	}
}

void Console::clearTiles(vector<Tile> &tiles, int x0, int y0, int w, int h) {
	auto x1 = x0 + w;
	auto y1 = y0 + h;
	for(int y = y0; y<y1; y++) {
		for(int x = x0; x<x1; x++) {
			Tile &t = tiles[x+y*width];
			t.c = 0x20;
			t.fg = WHITE;
			t.bg = BLACK;
		}
	}
}

void Console::scrollScreen(int dy) {

	shiftTiles(grid, 0, dy);
	clearTiles(grid, 0, height-dy, width, dy);
	if(impl_scroll_screen(dy)) {
		shiftTiles(oldGrid, 0, dy);
		clearTiles(oldGrid, 0, height-dy, width, dy);
	}
	flush();
}

// ANSIS

AnsiConsole::AnsiConsole(Terminal &terminal) : Console(terminal) {
	//resize(width, height);
	for(int i=0; i<16; i++) {
		auto *p = &c64pal[i*3];
		const string &s = utils::format("\x1b]4;%d;#%02x%02x%02x\x07", 160 + i, p[0], p[1], p[2]);
		//LOGD(s);
		outBuffer.insert(outBuffer.end(), s.begin(), s.end());
	}
	impl_gotoxy(0,0);
	flush();
};


void AnsiConsole::putChar(Char c) {

	if(c <= 0x7f)
		outBuffer.push_back(c);
	else {

		outBuffer.push_back(0xC0 | (c >> 6));
		outBuffer.push_back(0x80 | (c & 0x3F));
		auto l = outBuffer.size();
		LOGD("Translated %02x to %02x %02x", c, (int)outBuffer[l-2], (int)outBuffer[l-1]);
	}

	curX++;
	if(curX >= width) {
		curX -= width;
		curY++;
	}
}

bool AnsiConsole::impl_scroll_screen(int dy) {
	const auto s = dy > 0 ? utils::format("\x1b[%dS",dy) : utils::format("\x1b[%dT", -dy);
	outBuffer.insert(outBuffer.end(), s.begin(), s.end());
	return true;
}

void AnsiConsole::impl_color(int fg, int bg) {

	//int af = ansiColors[fg];
	//int ab = ansiColors[bg];

	//LOGD("## BG %d\n", ab);
	//const string &s = utils::format("\x1b[%d;%d%sm", af + 30, ab + 40, hl ? ";1" : "");
	const auto s = utils::format("\x1b[38;5;%d;48;5;%dm", fg+160, bg+160);

	//uint8_t *fp = &c64pal[fg*3];
	//uint8_t *bp = &c64pal[bg*3];
	//const string &s = utils::format("\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm", fp[0], fp[1], fp[2], bp[0], bp[1], bp[2]);
	outBuffer.insert(outBuffer.end(), s.begin(), s.end());			
};

void AnsiConsole::impl_clear() {
	for(auto x : vector<uint8_t> { '\x1b', '[', '2', 'J' })
		outBuffer.push_back(x);
}


void AnsiConsole::impl_gotoxy(int x, int y) {
	// Not so smart for now
	//LOGD("gotoxy %d,%d", x,y);
	const auto s = utils::format("\x1b[%d;%dH", y+1, x+1);
	outBuffer.insert(outBuffer.end(), s.begin(), s.end());
	curX = x;
	curY = y;
}

int AnsiConsole::impl_handlekey() {
	auto c = inBuffer.front();
	inBuffer.pop();
	if(c != 0x1b) {	
		LOGD("Normal key %d", (int)c);	
		if(c == 13 || c == 10) {
			if(c == 13) {
				auto c2 = inBuffer.front();
				if(c2 == 10) {
					inBuffer.pop();
				}
			}
			return KEY_ENTER;
		} else if(c == 0x7f)
			return KEY_BACKSPACE;
		else if(c == 0x7e)
			return KEY_DELETE;
		return c;
	} else {
		if(inBuffer.size() > 0) {
			auto c2 = inBuffer.front();
			inBuffer.pop();
			auto c3 = inBuffer.front();
			inBuffer.pop();

			LOGD("ESCAPE key %02x %02x %02x", c, c2, c3);

			if(c2 == 0x5b || c2 == 0x4f) {
				switch(c3) {
				case 0x50:
					return KEY_F1;
				case 0x51:
					return KEY_F2;
				case 0x52:
					return KEY_F3;
				case 0x53:
					return KEY_F4;
				case 0x31:
					if(inBuffer.size() >= 2) {
						auto c4 = inBuffer.front();
						inBuffer.pop();
						auto c5 = inBuffer.front();
						inBuffer.pop();
						if(c5 == 126) {
							switch(c4) {
							case '5':
								return KEY_F5;
							case '7':
								return KEY_F6;
							case '8':
								return KEY_F7;
							case '9':
								return KEY_F8;
							}
						}
					}
					break;
				case 0x33:
					if(!inBuffer.empty() && inBuffer.front() == 126)
						inBuffer.pop();
					return KEY_DELETE;
				case 0x48:
					return KEY_HOME;
				case 0x46:
					return KEY_END;
				case 0x44:
					return KEY_LEFT;
				case 0x43:
					return KEY_RIGHT;
				case 0x41:
					return KEY_UP;
				case 0x42:
					return KEY_DOWN;
				case 0x35:
					inBuffer.pop();
					return KEY_PAGEUP;
				case 0x36:
					inBuffer.pop();
					return KEY_PAGEDOWN;
					
				}
			}
		}
		return c;
	}
}


// PETSCII

void PetsciiConsole::putChar(Char c) {
	if(curX == 39) {
		outBuffer.push_back(DEL);
		outBuffer.push_back(c & 0xff);
		outBuffer.push_back(LEFT);
		outBuffer.push_back(INS);
		// NOTE: We don't handle the case where char 38 and 39 have different colors!
		outBuffer.push_back(grid[curY*width+38].c & 0xff);
	} else {
		outBuffer.push_back(c & 0xff);
		curX++;
		if(curX >= width) {
			curX -= width;
			curY++;
	}	}
}

void PetsciiConsole::impl_translate(Char &c) {

	//Char x = c;
	auto *pc = std::find(begin(petsciiTable), end(petsciiTable), c);
	if(pc != end(petsciiTable))
		c = (pc - petsciiTable + 0x20);
	//LOGD("Translated %c (%02x) to %c (%02x)", x, x, c, c);
}

void PetsciiConsole::impl_color(int fg, int bg) {
	if(bg >= 0 && bg != BLACK) {
		outBuffer.push_back(RVS_ON);
		outBuffer.push_back(petsciiColors[bg]);
	} else if(fg >= 0) {
		outBuffer.push_back(RVS_OFF);
		outBuffer.push_back(petsciiColors[fg]);
	}
}

void PetsciiConsole::impl_clear() {
	outBuffer.push_back(147);
}

void PetsciiConsole::impl_gotoxy(int x, int y) {

	//LOGD("Petscii GOTOXY from %d,%d to %d,%d", curX, curY, x, y);

	if(curX - x > x) {
		if(curY == height-1) {
			outBuffer.push_back(UP);
		} else
			curY++;	
		outBuffer.push_back(SHIFT_RETURN);
		if(curBg != BLACK) {
			curFg = curBg;
			curBg = BLACK;
		}
		curX=0;
	}

	while(y > curY) {
		outBuffer.push_back(DOWN);
		curY++;
	}
	while(y < curY) {
		outBuffer.push_back(UP);
		curY--;
	}

	while(x > curX) {
		outBuffer.push_back(RIGHT);
		curX++;
	}

	while(x < curX) {
		outBuffer.push_back(LEFT);
		curX--;
	}
}

int PetsciiConsole::impl_handlekey() {
	auto k = inBuffer.front();
	inBuffer.pop();
	switch(k) {
	case 13 :
		return KEY_ENTER;
	case DEL :
		return KEY_BACKSPACE;
	case DOWN :
		return KEY_DOWN;
	case UP :
		return KEY_UP;
	case LEFT :
		return KEY_LEFT;
	case RIGHT :
		return KEY_RIGHT;
	default:
		if(k >= F1 && k <= F8) {
			k -= F1;
			return KEY_F1 + (k * 2 % 8) + (k / 4); // A little bit of trickery to convert :)
		}
	}
	if(k >= 0x20 && (k <= 0x7f || k >= 0xa0)) {
		auto k2 = k;
		k = petsciiTable[k-0x20];
		LOGD("%02x became %02x (%c)", k2, k, k);
		return k;
	}
	return KEY_UNKNOWN;
}

bool PetsciiConsole::impl_scroll_screen(int dy) {
	//const auto s = dy > 0 ? utils::format("\x1b[%dS",dy) : utils::format("\x1b[%dT", -dy);
	//outBuffer.insert(outBuffer.end(), s.begin(), s.end());
	auto steps = dy + height - curY -1;
	while(steps--)
		outBuffer.push_back(DOWN);
	steps = height - curY - 1;
	while(steps--)
		outBuffer.push_back(UP);
	return true;
}


Console *createLocalConsole() {
	return new AnsiConsole(localTerminal);
}

}


#ifdef UNIT_TEST

#include "catch.hpp"
#include <sys/time.h>

using namespace bbs;

class TestTerminal : public Terminal {
public:
	virtual int write(const std::vector<Char> &source, int len) { 
		for(int i=0; i<len; i++)
			outBuffer.push_back(source[i]);
		return len;
	}
	virtual int read(std::vector<Char> &target, int len) { 
		int rc = -1;//outBuffer.size();
		//target.insert(target.back, outBuffer.begin(), outBuffer.end());
		//outBuffer.resize(0);
		return rc;
	}
	std::vector<Char> outBuffer;


};

TEST_CASE("console::basic", "Console") {

	TestTerminal terminal;
	PetsciiConsole console { terminal };

	//1b 5b 32 4a 1b 5b 32 3b 32 48  74 65 73 74 69 6e 67
	console.setFg(Console::WHITE);
	console.setBg(Console::BLACK);
	console.put(37,1, "abcdefghijk");
	console.put(0, 3, "ABCDEFGH");
	console.flush();
	string s = utils::format("[%02x]\n", terminal.outBuffer);
	printf(s.c_str());


	REQUIRE(true);

}

#endif
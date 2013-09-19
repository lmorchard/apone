#ifndef BBS_EDITOR_H
#define BBS_EDITOR_H

#include "console.h"

namespace bbs {

class Editor {
};

class LineEditor : public Editor {
public:
	LineEditor(Console &console, int width = 0);
	int update(int msec);
	bool isDone();
	std::string getResult();
	void refresh();
protected:
	Console &console;
	std::wstring line;

	int startX;
	int startY;
	int fg;
	int bg;

	int width;
	int maxlen;

	bool password;
	std::string filter;

	int xpos;
	int xoffset;

};

class FullEditor : public Editor {
public:
	FullEditor(Console &console);
	int update(int msec);
	std::string getResult();
	void refresh();
};

}

#endif // BBS_EDITOR_H
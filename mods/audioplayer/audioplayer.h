#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <functional>

class audio_exception : public std::exception {
public:
	audio_exception(const std::string &msg) : msg(msg) {}
	virtual const char *what() const throw() { return msg.c_str(); }
	
	std::string msg;
};

class InternalPlayer;

class AudioPlayer {
public:
	AudioPlayer(int hz = 44100);
	AudioPlayer(std::function<void(int16_t *, int)> cb, int hz = 44100);

	static void play(std::function<void(int16_t *, int)> cb, int hz = 44100);
	static void close();

	void pause();
	void resume();

	static void set_volume(int percent);
	static void pause_audio(); 
	static void resume_audio();

	void touch() const {}

	static int get_delay();

private:
		std::shared_ptr<InternalPlayer> internalPlayer;
		static std::shared_ptr<InternalPlayer> staticInternalPlayer;
};

#endif // AUDIOPLAYER_H

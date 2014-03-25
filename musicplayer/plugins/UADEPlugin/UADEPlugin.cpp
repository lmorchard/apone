
#include "UADEPlugin.h"

#include "../../ChipPlayer.h"

#include <coreutils/utils.h>
#include <coreutils/log.h>
#include <uade/uade.h>

#include <set>
#include <unordered_map>

/*
extern "C" {
	void init_uade();
	int get_samples(uint8_t *target, int bytes);
	int play_song(const char *name);
	void exit_song();
	void set_song(int song);
}*/

namespace chipmachine {

using namespace std;
using namespace utils;

class UADEPlayer : public ChipPlayer {
public:
	UADEPlayer() : valid(false), state(nullptr)  {
	}

	static struct uade_file *amigaloader(const char *name, const char *playerdir, void *context, struct uade_state *state) {
		LOGD("Trying to load '%s' from '%s'", name, playerdir);
		UADEPlayer *up = static_cast<UADEPlayer*>(context);
		string fileName = name;
		if(path_suffix(fileName) == "music") {
			fileName = path_directory(fileName) + "/" + up->baseName + "." + path_prefix(fileName);
			LOGD("Translated back to '%s'", fileName);
		}

		struct uade_file *f = uade_load_amiga_file(fileName.c_str(), playerdir, state);
		return f;
	}

	bool load(string fileName) {

		state = uade_new_state(nullptr);

		if(path_suffix(fileName) == "mdat") {
			uade_set_amiga_loader(UADEPlayer::amigaloader, this, state);
			baseName = path_basename(fileName);
			string uadeFileName = path_directory(fileName) + "/" + path_extention(fileName) + "." + "music";
			LOGD("Translated %s to %s", fileName, uadeFileName);
			File file { uadeFileName };
			File file2 { fileName };
			file.copyFrom(file2);
			file.close();
			fileName = uadeFileName;
		} else
			uade_set_amiga_loader(nullptr, this, state);

		if(uade_play(fileName.c_str(), -1, state) == 1) {
			songInfo = uade_get_song_info(state);
			setMeta(
				"songs", songInfo->subsongs.max - songInfo->subsongs.min + 1,
				"startsong", songInfo->subsongs.def - songInfo->subsongs.min,
				"length", songInfo->duration,
				"title", songInfo->modulename,
				"format", songInfo->playername
			);
			//printf("UADE:%s %s\n", songInfo->playerfname, songInfo->playername);
			valid = true;
		} 
		return valid;

	}



	~UADEPlayer() override {
		//if(valid)
		//	exit_song();
	   uade_stop(state);
	   uade_cleanup_state(state);
	   state = nullptr;
	}

	virtual int getSamples(int16_t *target, int noSamples) override {
		//int rc = get_samples((uint8_t*)target, noSamples * 2);
		ssize_t rc = uade_read(target, noSamples*2, state); 
		if(rc > 0)
			return rc/2;
		return rc;
	}

	virtual void seekTo(int song, int seconds) {
		uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, song + songInfo->subsongs.min, state);
		//set_song(song);	
	}

private:
	bool valid;
	struct uade_state *state;
	const struct uade_song_info *songInfo;
	string baseName;
};

bool UADEPlugin::canHandle(const std::string &name) {
	return true;
}

ChipPlayer *UADEPlugin::fromFile(const std::string &fileName) {

	auto *player = new UADEPlayer();
	if(!player->load(fileName)) {
		delete player;
		player = nullptr;
	}
	return player;
};

}
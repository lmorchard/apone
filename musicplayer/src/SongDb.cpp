#include <coreutils/log.h>

#include "SongDb.h"

#include <coreutils/utils.h>

#include <cstring>
#include <algorithm>
#include <set>

#include <iconv.h>

using namespace std;
using namespace utils;


SongDatabase::SongDatabase(const string &name) : db ( name ) {
	LOGD("DB OPEN");
}

SongDatabase::~SongDatabase() {
	LOGD("DB CLOSE");
}


string SongDatabase::getFullString(int id) const {

	id++;
	LOGD("ID %d", id);

	auto q = db.query<string, string, string, string>("SELECT title, composer, path, metadata FROM songs WHERE _id == ?", id);
	if(q.step()) {
		auto t = q.get_tuple();
		string r = format("%s\t%s\t%s\t%s", get<0>(t), get<1>(t), get<2>(t), get<3>(t));
		LOGD("RESULT %s", r);
		return r;
	}
	throw not_found_exception();
}
enum {
	UNKNOWN = 0,
	C64_SID = 1,
	TRACKER_ANY = 0x10,
	TRACKER_MOD,
	TRACKER_XM,
	TRACKER_S3M,
	TRACKER_FT,
	TRACKER_IT,

	TRACKER_END = 0x2f,
	GM_ANY = 0x30,
	GM_NES,
	GM_SNES,
	GM_GAMEBOY,
	GM_VGM,
	GM_END = 0x4f,

	AUDIO_ANY = 0x50,

	AMI_ANY = 0x80,
	AMI_TFMX,
	AMI_CUSTOM,
	AMI_FC,
	AMI_AHX,
	AMI_END = 0xff
};



unordered_map<string, int> formatMap {
	{ "sid", C64_SID },
	{ "rsid", C64_SID },
	{ "psid", C64_SID },
	{ "mod", TRACKER_MOD },
	{ "xm", TRACKER_XM },
	{ "it", TRACKER_IT },
	{ "s3m", TRACKER_S3M },
	{ "nsf", GM_NES },
	{ "smc", GM_SNES },
	{ "spc", GM_SNES },
	{ "gbs", GM_GAMEBOY },
	{ "vgz", GM_VGM },
	{ "ahx", AMI_AHX },
};	


void SongDatabase::generateIndex() {

	lock_guard<mutex>{dbLock};

	string oldComposer;
	auto query = db.query<string, string, string>("SELECT title, composer, path FROM songs");

	int count = 0;
	//int maxTotal = 3;
	int cindex = 0;
	while(count < 1000000) {
		count++;
		if(!query.step())
			break;

		string title, composer, path;
		tie(title, composer, path) = query.get_tuple();
		string ext = path_extention(path);
		makeLower(ext);
		int fmt = formatMap[ext];
		formats.push_back(fmt);

		// The title index maps one-to-one with the database
		int tindex = titleIndex.add(title);

		if(composer != oldComposer) {
			oldComposer = composer;
			cindex = composerIndex.add(composer);
			// The composer index does not map to the database, but for each composer
			// we save the first index in the database for that composer
			composerToTitle.push_back(tindex);
		}

		// We also need to find the composer for a give title
		titleToComposer.push_back(cindex);
	}
	LOGD("INDEX CREATED");
}

int SongDatabase::search(const string &query, vector<int> &result, unsigned int searchLimit) {

	lock_guard<mutex>{dbLock};
	
	result.resize(0);
	//if(query.size() < 3)
	//	return 0;
	//bool q3 = (q.size() <= 3);

	titleIndex.search(query, result, searchLimit);

	vector<int> cresult;
	composerIndex.search(query, cresult, searchLimit);
	for(int index : cresult) {
		int title_index = composerToTitle[index];

		while(titleToComposer[title_index] == index) {
			result.push_back(title_index++);
		}
	}

	return result.size();
}

#ifdef UNIT_TEST

#include "catch.hpp"
#include <sys/time.h>

TEST_CASE("db::index", "Generate index") {

	SongDatabase db {"hvsc.db"};
	db.generateIndex();

	string search_string = "tune tel fre";
	vector<int> results { 155, 1, 2944, 2694, 2694, 1954, 524, 11, 11, 1, 1, 1 };
	IncrementalQuery q = db.find();
	int i = 0;
	for(char c : search_string) {		
		q.addLetter(c);
		//LOGD("%s %d", q.getString(), q.numHits());
		REQUIRE(q.numHits() == results[i]);
		i++;
	}
	string res = q.getResult(0, 10)[0];
	REQUIRE(res.find("Freaky Tune") != string::npos);
}

/*
TEST_CASE("db::tlcode", "Codes") {

	SongDatabase db {"hvsc.db"};
	unordered_map<uint16_t, std::vector<int>> stringMap;
	logging::setLevel(logging::VERBOSE);
	db.addSubStrings("Testing (4-Mat)", stringMap, 0);
	logging::setLevel(logging::DEBUG);
}*/


TEST_CASE("db::find", "Search Benchmark") {

	timeval tv;
	gettimeofday(&tv, NULL);
	long d0 = tv.tv_sec * 1000000 + tv.tv_usec;

	logging::setLevel(logging::INFO);

	SongDatabase db {"hvsc.db"};
	db.generateIndex();


	vector<IncrementalQuery> iqs;

	for(int i=0; i<20; i++) {
		iqs.push_back(db.find());
	}

	string search_string = "tune tel";
	int i = 0;

	gettimeofday(&tv, NULL);
	long d1 = tv.tv_sec * 1000000 + tv.tv_usec;

	for(char c : search_string) {		
		for(auto &q : iqs) {
			q.addLetter(c);			
			//REQUIRE(q.numHits() == results[i]);
		}
		i++;
	}

	gettimeofday(&tv, NULL);
	long d2 = tv.tv_sec * 1000000 + tv.tv_usec;

	LOGI("Search took %dms", (d2-d1)/1000);
}

#endif

#ifndef WEBGETTER_H
#define WEBGETTER_H

#include <coreutils/file.h>

#include <string>
#include <mutex>
#include <thread>
#include <memory>
#include <stdio.h>

class WebGetter {
public:
	class Job {
	public:
		Job(const std::string &url, const std::string &targetDir);
		~Job();
		bool isDone();
		int getReturnCode();
		std::string getFile();
	private:
		void urlGet(std::string url);
		static size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *userdata);
		static size_t headerFunc(void *ptr, size_t size, size_t nmemb, void *userdata);

		std::mutex m;
		bool loaded;
		int returnCode;
		std::thread jobThread;
		std::string targetDir;
		std::unique_ptr<utils::File> file;
		std::string target;
	};

	WebGetter(const std::string &workDir) ;
	Job* getURL(const std::string &url);
private:
	std::string workDir;
};

#endif // WEBGETTER_H

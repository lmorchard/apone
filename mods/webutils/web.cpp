
#include "web.h"


namespace webutils {

std::vector<std::shared_ptr<Web::Job>> Web::jobs;
int Web::lastCount = 0;
std::mutex Web::sm;
} // namespace webutils


#ifndef ETYMON_CURL_H
#define ETYMON_CURL_H

#include <curl/curl.h>

namespace etymon {

class curl_global {
public:
    curl_global(long flags, CURLcode* code);
    ~curl_global();
};

}

#endif

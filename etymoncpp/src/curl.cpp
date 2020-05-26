#include "../include/curl.h"

namespace etymon {

curl_global::curl_global(long flags, CURLcode* code)
{
    *code = curl_global_init(flags);
}

curl_global::~curl_global()
{
    curl_global_cleanup();
}

}


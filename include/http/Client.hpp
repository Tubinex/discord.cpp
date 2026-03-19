#pragma once

#include <map>
#include <string>

namespace discord {

struct HttpResponse {
    int mStatusCode = 0;
    std::string mBody;
    std::map<std::string, std::string> mHeaders;
    std::string mErrorMessage;
};

class HttpClient {
  public:
    HttpResponse get(const std::string &url,
                     const std::map<std::string, std::string> &headers = {}) const;
    HttpResponse postJson(const std::string &url, const std::string &jsonBody,
                          const std::map<std::string, std::string> &headers = {}) const;
};

}

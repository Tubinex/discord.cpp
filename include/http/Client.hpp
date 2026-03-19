#pragma once

#include <map>
#include <string>

#include "models/ClientType.hpp"

namespace discord {

struct HttpResponse {
    int mStatusCode = 0;
    std::string mBody;
    std::map<std::string, std::string> mHeaders;
    std::string mErrorMessage;
};

class HttpClient {
  public:
    explicit HttpClient(std::string token = {}, ClientType type = ClientType::Bot);
    void setUserAgent(std::string userAgent);

    HttpResponse get(const std::string &url,
                     const std::map<std::string, std::string> &headers = {}) const;
    HttpResponse postJson(const std::string &url, const std::string &jsonBody,
                          const std::map<std::string, std::string> &headers = {}) const;

  private:
    std::map<std::string, std::string> authHeaders() const;

    std::string mToken;
    std::string mUserAgent;
    ClientType mClientType;
};

}

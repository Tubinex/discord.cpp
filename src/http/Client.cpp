#include "http/Client.hpp"

#include <cpr/cpr.h>

namespace discord {

namespace {

HttpResponse toHttpResponse(const cpr::Response &response) {
    HttpResponse result;
    result.mStatusCode = response.status_code;
    result.mBody = response.text;
    result.mErrorMessage = response.error.message;

    for (const auto &[key, value] : response.header) {
        result.mHeaders.emplace(key, value);
    }

    return result;
}

}

HttpResponse HttpClient::get(const std::string &url,
                             const std::map<std::string, std::string> &headers) const {
    cpr::Header requestHeaders;
    for (const auto &[key, value] : headers) {
        requestHeaders[key] = value;
    }

    return toHttpResponse(cpr::Get(cpr::Url{url}, requestHeaders));
}

HttpResponse HttpClient::postJson(const std::string &url, const std::string &jsonBody,
                                  const std::map<std::string, std::string> &headers) const {
    std::map<std::string, std::string> mergedHeaders = headers;
    if (mergedHeaders.find("Content-Type") == mergedHeaders.end()) {
        mergedHeaders["Content-Type"] = "application/json";
    }

    cpr::Header requestHeaders;
    for (const auto &[key, value] : mergedHeaders) {
        requestHeaders[key] = value;
    }

    return toHttpResponse(cpr::Post(cpr::Url{url}, requestHeaders, cpr::Body{jsonBody}));
}

}

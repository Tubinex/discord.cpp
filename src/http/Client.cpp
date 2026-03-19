#include "http/Client.hpp"

#include <cpr/cpr.h>

#include <utility>

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

HttpClient::HttpClient(std::string token, ClientType type)
    : mToken(std::move(token)), mClientType(type) {}

void HttpClient::setUserAgent(std::string userAgent) { mUserAgent = std::move(userAgent); }

std::map<std::string, std::string> HttpClient::authHeaders() const {
    std::map<std::string, std::string> headers;
    if (!mToken.empty()) {
        headers["Authorization"] = mClientType == ClientType::Bot ? "Bot " + mToken : mToken;
    }
    if (!mUserAgent.empty()) {
        headers["User-Agent"] = mUserAgent;
    }
    return headers;
}

HttpResponse HttpClient::get(const std::string &url,
                             const std::map<std::string, std::string> &headers) const {
    cpr::Header requestHeaders;
    for (const auto &[key, value] : authHeaders()) {
        requestHeaders[key] = value;
    }
    for (const auto &[key, value] : headers) {
        requestHeaders[key] = value;
    }

    return toHttpResponse(cpr::Get(cpr::Url{url}, requestHeaders));
}

HttpResponse HttpClient::postJson(const std::string &url, const std::string &jsonBody,
                                  const std::map<std::string, std::string> &headers) const {
    std::map<std::string, std::string> mergedHeaders = authHeaders();
    mergedHeaders["Content-Type"] = "application/json";
    for (const auto &[key, value] : headers) {
        mergedHeaders[key] = value;
    }

    cpr::Header requestHeaders;
    for (const auto &[key, value] : mergedHeaders) {
        requestHeaders[key] = value;
    }

    return toHttpResponse(cpr::Post(cpr::Url{url}, requestHeaders, cpr::Body{jsonBody}));
}

}

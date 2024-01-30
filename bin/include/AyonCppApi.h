#pragma once

#include <string>
class AyonApi {
    public:
        AyonApi();
        ~AyonApi();

        std::string getKey();
        std::string getUrl();
        std::string getSiteId();

    private:
        std::string authKey;
        std::string serverUrl;
        std::string siteId;
};

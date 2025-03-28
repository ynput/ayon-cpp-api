#include "AyonCppApi.h"
#include "nlohmann/json_fwd.hpp"

namespace AyonCppApiTest {

bool load_EnvVariables(
    std::string &envFilePath,
    std::string &AYON_API_KEY, 
    std::string &AYON_SERVER_URL,
    std::string &AYON_SITE_ID,
    std::string &AYON_PROJECT_NAME
);

bool test_SimpleResolve(nlohmann::json &JsonFile, const bool &RunOnlyOnce, const bool &Print, AyonApi &Api);

bool test_BatchResolve(nlohmann::json &JsonFile, const bool &Print, AyonApi &Api);
}   // namespace AyonCppApiTest

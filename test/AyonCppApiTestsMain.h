#include "AyonCppApi.h"
#include "nlohmann/json_fwd.hpp"

namespace AyonCppApiTest {

bool test_SimpleResolve(nlohmann::json &JsonFile, const bool &RunOnlyOnce, const bool &Print, AyonApi &Api);

bool test_BatchResolve(nlohmann::json &JsonFile, const bool &Print, AyonApi &Api);
}   // namespace AyonCppApiTest

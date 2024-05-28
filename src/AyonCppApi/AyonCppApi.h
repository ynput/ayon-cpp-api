#ifndef AYONCPPAPI_H
#define AYONCPPAPI_H
#include "AyonCppApiCrossPlatformMacros.h"
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <regex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>
#include "AyonLogger.h"
#include "appDataFoulder.h"
#include "httplib.h"
#include "nlohmann/json_fwd.hpp"

/**
 * @class AyonApi
 * @brief Central Ayon api class \n
 * Class for exposing Ayon server functions to C++ users. Uses httplib internally for communication with the server
 *
 */
class AyonApi {
    public:
        /**
         * @brief constructor
         */
        AyonApi();
        /**
         * @brief destructor
         */
        ~AyonApi();

        /**
         * @brief returns the stored apikey. Retrieved from the appropriate env variable. (the variable is loaded from
         * loadEnvVars())
         */
        std::string getKey();
        /**
         * @brief returns the stored AYON server url. Retrieved from the appropriate env variable. (the variable is
         * loaded from loadEnvVars())
         */
        std::string getUrl();

        /**
         * @brief runns a get command and returns the response body as std::string
         *
         * @param endPoint reachable http / https endpoint
         * @param headers http headers
         * @param sucsessStatus define what http response code should be considered a success.
         */
        nlohmann::json GET(const std::shared_ptr<std::string> endPoint,
                           const std::shared_ptr<httplib::Headers> headers,
                           uint8_t sucsessStatus);

        /**
         * @brief post Request via a shared httplib client ( serial )
         *
         * @param endPoint the AYON enpoint to hit
         * @param headers the http header that you want to send
         * @param jsonPayload the payload in json format
         * @param sucsessStatus defines what status code is considered a success and brakes the retry loop.
         */
        nlohmann::json SPOST(const std::shared_ptr<std::string> endPoint,
                             const std::shared_ptr<httplib::Headers> headers,
                             nlohmann::json jsonPayload,
                             const std::shared_ptr<uint8_t> sucsessStatus);
        /**
         * @brief http post request utilizing the creation of a new httplib client ( Generative Async )
         *
         * @param endPoint the AYON enpoint to hit
         * @param headers the http header that you want to send
         * @param jsonPayload the payload in json format
         * @param sucsessStatus defines what status code is considered a success and brakes the retry loop.
         */
        nlohmann::json CPOST(const std::shared_ptr<std::string> endPoint,
                             const std::shared_ptr<httplib::Headers> headers,
                             nlohmann::json jsonPayload,
                             const std::shared_ptr<uint8_t> sucsessStatus);

        /**
         * @brief uses the uri resolve endpoint on the AYON server in order to resolve an uri path towards the local
         * path \n gets the siteId from an variable stored in the class
         *
         * @param uriPath
         */
        std::pair<std::string, std::string> resolvePath(const std::string &uriPath);
        /**
         * @brief resolves a vector off paths against the AYON server in an async way uses auto generated batch requests
         *
         * @param uriPaths
         */
        std::unordered_map<std::string, std::string> batchResolvePath(std::vector<std::string> &uriPaths);

        /**
         * @brief this function takes a ayon path uri response(resolved ayon://path) and returns a pair of
         * assetIdentifier(ayon:// path) and the machine local file location
         *
         * @param uriResolverRespone json representation off the resolves the ayon/api/resolve endpoint returns
         */
        std::pair<std::string, std::string> getAssetIdent(const nlohmann::json &uriResolverRespone);

        /**
         * @brief this function loads all needed varible into the class \n
         * this will allso be called by the constructor
         *
         * @return
         */
        bool loadEnvVars();

        /**
         * @brief get function for shared AyonLogger pointer used by this class instance
         */
        std::shared_ptr<AyonLogger> logPointer();

        /**
         * @brief gets the site root overwrites for the current project. Current project is defined via an env variable
         * for now
         */
        std::unordered_map<std::string, std::string>*
        getSiteRoots();   // TODO think about if this should only support current project or multiple projects

        /**
         * @brief replaces {root[var]} for ayon:// paths
         *
         * @param rootLessPath endpoint response for ayon://path with {root[var]} available if no root can be found the
         * path will be returned as is
         */
        std::string rootReplace(const std::string &rootLessPath);

    private:
        /**
         * @brief calls the server in an serial way by sharing the AyonServer pointer
         *
         * @param endPoint endpoint that ayon resolve is loaded on
         * @param headers http headers
         * @param Payload json payload to be resolved by endpoint
         * @param sucsessStatus defines what is considered a success response to break the retry loop
         */
        std::string serialCorePost(const std::string &endPoint,
                                   httplib::Headers headers,
                                   std::string &Payload,
                                   const int &sucsessStatus);
        /**
         * @brief calls the server while creating a new client instance to stay async
         *
         * @param endPoint endpoint that ayon resolve is loaded on
         * @param headers http headers
         * @param Payload json payload to be resolved by endpoint
         * @param sucsessStatus defines what is considered a success response to break the retry loop
         */
        std::string GenerativeCorePost(const std::string &endPoint,
                                       httplib::Headers headers,
                                       std::string &Payload,
                                       const int &sucsessStatus);

        /**
         * @brief converts a vector off uris into an string to serve into CorePost funcs
         *
         * @param uriVec vector off str uris
         */
        std::string convertUriVecToString(const std::vector<std::string> &uriVec);

        // ----- Env Varibles

        std::unique_ptr<httplib::Client> AyonServer;

        std::unordered_map<std::string, std::string> siteRoots;

        const char* authKey;
        const char* serverUrl;
        std::string ayonProjectName;

        std::string ayonAppData = getAppDataDir() + "/AYON";

        // ---- Server Vars
        std::string siteId;
        std::string userName;

        // --- Runtime Dep Vars

        // Async Grp Generation Varibles
        u_int8_t minGrpSizeForAsyncRequests = 10;
        u_int16_t regroupSizeForAsyncRequests = 200;
        u_int16_t maxGroupSizeForAsyncRequests = 300;
        u_int16_t minVecSizeForGroupSplitAsyncRequests = 50;
        u_int8_t maxCallRetrys = 8;
        u_int16_t retryWaight = 800;

        /**
         * @brief maximum number off threads that the cpu can handle at the same time. Will be set via constructor
         */
        const int num_threads;   // set by constructor
        std::shared_ptr<AyonLogger> Log;
        std::string uriResolverEndpoint = "/api/resolve";
        std::string uriResolverEndpointPathOnlyVar = "?pathOnly=true";
        bool pathOnlyReselution = true;

        std::mutex ConcurentRequestAfterffoMutex;
        uint8_t maxConcurentRequestAfterffo = 8;

        uint16_t GenerativeCorePostMaxLoopIterations = 200;

        u_int16_t connectionTimeOutMax = 200;
        u_int8_t readTimeOutMax = 160;
        // bool enableThreadWaithing = false;
        // bool enableBigBlockThreadWaithing = true;
        /**
         * @brief decides if the cpp api removes duplicates from batch request vector default is true
         */
        bool batchResolveOptimizeVector = true;

        uint16_t ServerBusyCode = 503;
        uint16_t RequestDelayWhenServerBusy = 10000;
        // std::mutex allowRequest;

        /**
         * @brief this bool will be set to true if a 503 is encountered
         */
        bool serverBusy = false;

        /**
         * @brief needed for serial resolve operations. to lock acces to AyonServer shared pointer
         */
        std::mutex AyonServerMutex;
};

#endif   // !AYONCPPAPI_H

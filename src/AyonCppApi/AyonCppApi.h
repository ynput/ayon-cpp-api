#ifndef AYONCPPAPI_H
#define AYONCPPAPI_H
#include <sys/types.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

#include <string>
#include <utility>
#include <vector>
#include "lib/ynput/lib/logging/AyonLogger.hpp"
#include "appDataFolder.h"
#include "httplib.h"
#include "nlohmann/json_fwd.hpp"

#ifdef __linux__
// This header provides the dladdr function and Dl_info structure.
#include <dlfcn.h> 
#endif

/**
 * @class AyonApi
 * @brief Central Ayon api class
 * Class for exposing Ayon server functions to C++ users. Uses httplib internally for communication with the server
 */
class AyonApi {
    public:
        /**
         * @brief Constructor
         */
        AyonApi(const std::optional<std::string> &logFilePos,
                const std::string &authKey,
                const std::string &serverUrl,
                const std::string &ayonProjectName,
                const std::string &siteId,
                std::optional<int> concurrency = std::nullopt);
        /**
         * @brief Destructor
         */
        ~AyonApi();

        /**
         * @brief Returns the stored API key. Passed in the constructor.
         */
        std::string getKey();
        /**
         * @brief Returns the stored AYON server URL. Passed in the constructor.
         */
        std::string getUrl();

        /**
         * @brief Runs a GET command
         *
         * @param endPoint Reachable HTTP/HTTPS endpoint
         * @param headers HTTP headers
         * @param successStatus Defines what HTTP response code should be considered a success.
         * @return The response body as nlohmann::json
         */
        nlohmann::json GET(const std::shared_ptr<std::string> endPoint,
                           const std::shared_ptr<httplib::Headers> headers,
                           uint8_t successStatus);

        /**
         * @brief POST request via a shared httplib client (serial)
         *
         * @param endPoint The AYON endpoint to hit
         * @param headers The HTTP headers to send
         * @param jsonPayload The payload in JSON format
         * @param successStatus Defines what status code is considered a success and breaks the retry loop
         * @return The response body as nlohmann::json
         */
        nlohmann::json SPOST(const std::shared_ptr<std::string> endPoint,
                             const std::shared_ptr<httplib::Headers> headers,
                             nlohmann::json jsonPayload,
                             const std::shared_ptr<uint8_t> successStatus);
        /**
         * @brief HTTP POST request utilizing the creation of a new httplib client (Generative Async)
         *
         * @param endPoint The AYON endpoint to hit
         * @param headers The HTTP headers to send
         * @param jsonPayload The payload in JSON format
         * @param successStatus Defines what status code is considered a success and breaks the retry loop.
         * @return The response body as nlohmann::json
         */
        nlohmann::json CPOST(const std::shared_ptr<std::string> endPoint,
                             const std::shared_ptr<httplib::Headers> headers,
                             nlohmann::json jsonPayload,
                             const std::shared_ptr<uint8_t> successStatus);

        /**
         * @brief Uses the URI resolve endpoint on the AYON server to resolve a URI path to the local path.
         * Gets the siteId from a variable stored in the class.
         *
         * @param uriPath The URI path to resolve.
         * @return A pair containing the asset identifier (ayon:// path) and the machine local file location.
         */
        std::pair<std::string, std::string> resolvePath(const std::string &uriPath);

        /**
         * @brief Resolves a vector of paths against the AYON server asynchronously using auto-generated batch requests.
         *
         * @param uriPaths The vector of URI paths to resolve.
         * @return An unordered map containing the resolved paths.
         */
        std::unordered_map<std::string, std::string> batchResolvePath(std::vector<std::string> &uriPaths);

        /**
         * @brief Takes an AYON path URI response (resolved ayon://path) and returns a pair of
         * asset identifier (ayon:// path) and the machine local file location.
         *
         * @param uriResolverResponse JSON representation of the response from the AYON API resolve endpoint.
         * @return A pair containing the asset identifier and the machine local file location.
         */
        std::pair<std::string, std::string> getAssetIdent(const nlohmann::json &uriResolverResponse);

        /**
         * @brief this function loads all needed varible into the class \n
         * this will allso be called by the constructor
         *
         * @return
         */
        bool loadEnvVars();

        /**
         * @brief Get function for shared AyonLogger pointer used by this class instance
         */
        std::shared_ptr<AyonLogger> logPointer();

        /**
         * @brief Gets the site root overwrites for the current project. Current project is defined via an env variable
         * for now
         */
        std::unordered_map<std::string, std::string>*
        getSiteRoots();   // TODO think about if this should only support current project or multiple projects

        /**
         * @brief Replaces {root[var]} for ayon:// paths.
         *
         * @param rootLessPath Endpoint response for ayon://path with {root[var]}. 
         * If no root can be found, the path will be returned as is.
         */
        std::string rootReplace(const std::string &rootLessPath);

    private:
        /**
         * @brief Calls the server in a serial way by sharing the AyonServer pointer.
         *
         * @param endPoint Endpoint that AYON resolve is loaded on.
         * @param headers HTTP headers.
         * @param payload JSON payload to be resolved by endpoint.
         * @param successStatus Defines what is considered a success response to break the retry loop.
         */
        std::string serialCorePost(const std::string &endPoint,
                                   httplib::Headers headers,
                                   std::string &payload,
                                   const int &successStatus);
        /**
         * @brief Calls the server while creating a new client instance to stay async.
         *
         * @param endPoint Endpoint that AYON resolve is loaded on.
         * @param headers HTTP headers.
         * @param payload JSON payload to be resolved by endpoint.
         * @param successStatus Defines what is considered a success response to break the retry loop.
         */
        std::string GenerativeCorePost(const std::string &endPoint,
                                       httplib::Headers headers,
                                       std::string &payload,
                                       const int &successStatus);

        /**
         * @brief Converts a vector of URIs into a string to serve into CorePost functions.
         *
         * @param uriVec Vector of string URIs.
         */
        std::string convertUriVecToString(const std::vector<std::string> &uriVec);

        /**
         * @brief checks if the m_ayonServer is running on ssl based on m_serverUrl
         * dumb implementation but it should work - function from httplib is not working
         * 
         * @return true if m_serverUrl starts with https://
         */
        bool isSSL() const;

        /**
         * @brief sets the ssl cert path for the m_ayonServer httplib client
         */
        void setSSL();
        
        std::unique_ptr<httplib::Client> m_ayonServer;

        std::unordered_map<std::string, std::string> m_siteRoots;
        
        // ----- Env Varibles
        const std::string m_authKey;
        const std::string m_serverUrl;
        std::string m_ayonProjectName;

        // ---- Server Vars
        std::string m_siteId;
        std::string m_userName;

        // --- HTTP Headers
        httplib::Headers m_headers;

        // --- Runtime Dep Vars

        // Async Grp Generation Varaibles
        uint8_t m_minGrpSizeForAsyncRequests = 10;
        uint16_t m_regroupSizeForAsyncRequests = 200;
        uint16_t m_maxGroupSizeForAsyncRequests = 300;
        uint16_t m_minVecSizeForGroupSplitAsyncRequests = 50;
        uint8_t m_maxCallRetries = 8;
        uint16_t m_retryWait = 800;

        /**
         * @brief maximum number of threads that the CPU can handle at the same time. Will be set via constructor
         */
        const int m_num_threads;   // set by constructor
        std::shared_ptr<AyonLogger> m_Log;
        std::string m_uriResolverEndpoint = "/api/resolve";
        std::string m_uriResolverEndpointPathOnlyVar = "?pathOnly=true";
        bool m_pathOnlyResolution = true;

        std::mutex m_ConcurrentRequestAfterffoMutex;
        uint8_t m_maxConcurrentRequestAfterffo = 8;

        uint16_t m_generativeCorePostMaxLoopIterations = 200;

        uint16_t m_connectionTimeoutMax = 200;
        uint8_t m_readTimeoutMax = 160;

        /**
         * @brief Decides if the cpp API removes duplicates from batch request vector. Default is true
         */
        bool m_batchResolveOptimizeVector = true;

        uint16_t m_serverBusyCode = 503;
        uint16_t m_requestDelayWhenServerBusy = 10000;

        /**
         * @brief This bool will be set to true if a 503 is encountered
         */
        bool m_serverBusy = false;

        /**
         * @brief Needed for serial resolve operations. to lock access to AyonServer shared pointer
         */
        std::mutex m_ayonServerMutex;
};

#endif   // !AYONCPPAPI_H

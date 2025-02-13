#include <iostream>
#include <unordered_map>
#include <string>
#include <openssl/crypto.h>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../ext/cpp-httplib/httplib.h" // Include the cpp-httplib header
#include <memory>

// #define BEARER_TOKEN
#define USER
// #define SERVICE
#define CERTIFICATE_VERIFICATION
// #define SITE_ID
#define GET_SITE_ROOTS
// #define GET_PROJECTS


void printRes(const httplib::Result &res) {
    if (res) {
        std::cout << "Status code: " << res->status << std::endl;
        std::cout << "Response body:\n" << res->body << std::endl;
    } else {
        std::cerr << "Error: Unable to connect to the m_AyonServer - " << res.error() << std::endl;
    }
}


void constructorTest(const std::string &authKey,
                     const std::string &serverUrl,
                     const std::string &ayonProjectName,
                     const std::string &siteId) {
    std::string m_authKey = authKey;
    std::string m_serverUrl = serverUrl;
    std::string m_ayonProjectName = ayonProjectName;
    std::string m_siteId = siteId;
    
    std::unique_ptr<httplib::Client> m_AyonServer = std::make_unique<httplib::Client>(m_serverUrl);

    httplib::Headers m_headers = {
        {"X-Api-Key", m_authKey},
        // {"X-ayon-platform", "linux"},
    };

    // X509_STORE* store = X509_STORE_new();
    // if (store) {
    //     std::cout << "X509_STORE created." << std::endl;

    //     // This function loads the default system locations for CA certificates.
    //     if (X509_STORE_set_default_paths(store) != 1) {
    //         std::cout << "X509_STORE_set_default_paths failed." << std::endl;
    //     } else {
    //         m_AyonServer->set_ca_cert_store(store);
    //         std::cout << "Default CA paths loaded and set." << std::endl;
    //     }
    // } else {
    //     std::cout << "Failed to create X509_STORE." << std::endl;
    // }

    // m_AyonServer->enable_server_certificate_verification(true);
    // std::cout << "Server certificate verification enabled." << std::endl;

    // auto res = m_AyonServer->Get("/api/info", m_headers);
    auto res = m_AyonServer->Get("/api/projects/" + m_ayonProjectName + "/siteRoots?platform=linux", m_headers);

    if (res) {
        std::cout << "Response: " << res->status << std::endl;
        std::cout << "Response body: " << res->body << std::endl;
    } else {
        std::cout << "Response is null." << std::endl;
        std::cout << "Response error: " << res.error() << std::endl;
    }
}


std::string parseOutput(std::string& output) {
    // Parse the output to extract the directory path
    std::string::size_type start = output.find('"');
    std::string::size_type end = output.find('"', start + 1);
    if (start != std::string::npos && end != std::string::npos) {
        return output.substr(start + 1, end - start - 1);
    } else {
        throw std::runtime_error("Failed to parse OpenSSL directory from command output.");
    }
}

std::string getOpenSSLDirByCLI() {
    std::array<char, 128> buffer;
    std::string result;
    auto pipeDeleter = [](FILE* pipe) { pclose(pipe); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(popen("openssl version -d", "r"), pipeDeleter);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return parseOutput(result);
}


std::string getOpenSSLDir() {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L  // OpenSSL 1.1.0+
    const char* sslVersion = OpenSSL_version(OPENSSL_DIR);
    std::string sslVersionStr(sslVersion);
    return parseOutput(sslVersionStr);
#else  // OpenSSL 1.0.x
    return parseOutput(SSLeay_version(SSLEAY_DIR));
#endif
}



int main() {
    std::string opensslCliDir = getOpenSSLDirByCLI();
    std::string x509DefaultCertDir = X509_get_default_cert_dir();
    std::string opensslDefaultDir = getOpenSSLDir();

    std::cout << "OpenSSL CLI directory: " << opensslCliDir << " - " << (access(opensslCliDir.c_str(), F_OK) != -1 ? "exists" : "not exists") << std::endl;
    std::cout << "X509_get_default_cert_dir: " << x509DefaultCertDir << " - " << (access(x509DefaultCertDir.c_str(), F_OK) != -1 ? "exists" : "not exists") << std::endl;
    std::cout << "OpenSSL default directory: " << opensslDefaultDir << " - " << (access(opensslDefaultDir.c_str(), F_OK) != -1 ? "exists" : "not exists") << std::endl;

    std::cout << "OpenSSL version: " << OpenSSL_version(OPENSSL_VERSION) << std::endl;

    // return 0;

    // std::string AYON_API_KEY("6268b8b004ce8c7a7645afc548234937a69b6c6095b1c32ca6fa9f8351f8f4f8");
    // std::string AYON_SERVER_URL("https://ayon.dev");
    // std::string AYON_SITE_ID("test-id");
    // std::string AYON_PROJECT_NAME("test_API_project");

    // constructorTest(AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);

    return 0;

    // =============================================
    const std::string auth_key = "884198bdeb1c28a334acd03c3fc6e188b60506af810b0ed2d1c85748fe96e341";

    std::string m_ayonProjectName = "test_API_project";

    std::unique_ptr<httplib::Client> m_AyonServer = std::make_unique<httplib::Client>("https://ayon.dev");
    // =============================================

    httplib::Headers headers = {
            {"X-Api-Key", auth_key},
            // {"X-ayon-platform", "linux"},
        #ifdef _WIN32
            {"X-ayon-platform", "windows"},
        #elif __linux__
            // {"X-ayon-platform", "linux"},
        #endif
        #ifdef SITE_ID
            {"X-ayon-site-id", "test-site-id"}
        #endif
    };

    // Create a new store and let OpenSSL load system default paths
    // X509_STORE* store = X509_STORE_new();
    // if (store) {
    //     std::cout << "X509_STORE created." << std::endl;

    //     // This function loads the default system locations for CA certificates.
    //     if (X509_STORE_set_default_paths(store) != 1) {
    //         std::cout << "X509_STORE_set_default_paths failed." << std::endl;
    //     }
    //     m_AyonServer->set_ca_cert_store(store);
    // } else {
    //     std::cout << "Failed to create X509_STORE." << std::endl;
    // }

    // std::cout << "OpenSSL support not enabled." << std::endl;

    // m_AyonServer->enable_server_certificate_verification(true);

    auto res_0 = m_AyonServer->Get("/api/info", headers);
    std::cout << "Response: " << res_0->status << std::endl;
    // printRes(res_0);

#ifdef GET_SITE_ROOTS
    auto res_1 = m_AyonServer->Get("/api/projects/" + m_ayonProjectName + "/siteRoots?platform=linux", headers);

    printRes(res_1);
#endif

#ifdef GET_PROJECTS
    auto res_2 = m_AyonServer->Get("/api/projects");

    printRes(res_2);
#endif


    return 0;
}
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <fstream>   // for std::ifstream
#include <sstream>   // for std::stringstream
#include <cstdlib> // for system()
#include "ClientProcCommunicator.h"

// Globals or configurable
static const std::string shared_memory_name = "/_shmem1107";
static const int client_id = 1;
static const std::string configuration = "default_config";

int main() {
    using namespace httplib;
    using json = nlohmann::json;

    // Initialize your IPC communicator
    auto master = std::make_unique<ClientProcCommunicator>(shared_memory_name);

    // HTTP server
    Server svr;

    // Serve a simple HTML form at /
svr.Get("/", [](const httplib::Request&, httplib::Response &res) {
    std::ifstream file("index.html"); // open index.html from same directory
    if (!file.is_open()) {
        res.status = 404;
        res.set_content("index.html not found", "text/plain");
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    res.set_content(buffer.str(), "text/html");
});

    // Handle POST /send
   svr.Post("/setconfig", [](const httplib::Request &req, httplib::Response &res) {
    printf("send \n");
    std::string config = req.get_param_value("config"); // works for form submit

    printf("send 1\n");
    auto client = std::make_unique<ClientProcCommunicator>(shared_memory_name);
        printf("send 2\n");
Message msg{client_id, MessageType::HANDSHAKE};
    Message *handshake_resp;
        printf("send 3\n");
    client->sendRequestGetResponse(&msg, &handshake_resp);
        printf("send 3a\n");
    if (handshake_resp->type == MessageType::HANDSHAKE_OK)
    {
        printf("Connected to aquamarine service. Handshake complete %zu.", handshake_resp->id);
    }
    else
    {
        printf("Unexpected msg from aquamarine service. Type:%zu.", handshake_resp->type);
        exit(1);
    }
    bool success = client->sendRequestGetResponse(&msg, &handshake_resp);

    printf("send result %d\n", success);
    printf("send result1 %d %d\n", handshake_resp->id, (int)handshake_resp->type);

    res.set_content(success ? "Request sent!" : "IPC failed", "text/plain");
});

    std::cout << "Server listening on http://localhost:8080\n";
    system("open http://localhost:8080");
    svr.listen("0.0.0.0", 8080);
}

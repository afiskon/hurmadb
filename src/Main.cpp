/* vim: set ai et ts=4 sw=4: */

#include <string>
#include <utility>
#include <map>
#include <iostream>
#include <atomic>
#include <signal.h>
#include <HttpServer.h>
//#include <InMemoryStorage.h>
#include <PersistentStorage.h>

// TODO: plugable porotols, e.g. support Memcached protocol
// TODO: describe REST API in README.md or API.md
// TODO: support rangescans
// TODO: support _rev in ::set, ::delete
// TODO: multithread test with _rev
// TODO: script - run under Valgrind
// TODO: script - run under MemorySanitizer + other kinds of sanitizers
// TODO: see `grep -r TODO ./`

// TODO: Keyspaces class

//InMemoryStorage storage;
PersistentStorage storage;

static std::atomic_bool terminate_flag(false);

static void httpIndexGetHandler(const HttpRequest&, HttpResponse& resp) {
    resp.setStatus(HTTP_STATUS_OK);
    resp.setBody("This is HurmaDB!\n\n");
}

static void httpKVGetHandler(const HttpRequest& req, HttpResponse& resp) {
    bool found;
    const std::string& key = req.getQueryMatch(0);
    const std::string& value = storage.get(key, &found);
    resp.setStatus(found ? HTTP_STATUS_OK : HTTP_STATUS_NOT_FOUND);
    resp.setBody(value);
}

static void httpKVPutHandler(const HttpRequest& req, HttpResponse& resp) {
    const std::string& key = req.getQueryMatch(0);
    const std::string& value = req.getBody();
    storage.set(key, value);
    resp.setStatus(HTTP_STATUS_OK);
}

static void httpKVDeleteHandler(const HttpRequest& req, HttpResponse& resp) {
    bool found;
    const std::string& key = req.getQueryMatch(0);
    storage.del(key, &found);
    resp.setStatus(found ? HTTP_STATUS_OK : HTTP_STATUS_NOT_FOUND);
}

/* Required for generating code coverage report */
static void httpStopPutHandler(const HttpRequest&, HttpResponse& resp) {
    terminate_flag.store(true);
    resp.setStatus(HTTP_STATUS_OK);
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);
    if(port <= 0 || port >= 65536) {
        std::cerr << "Invalid port number!" << std::endl;
        return 2;
    }

    HttpServer server;
    server.addHandler(HTTP_GET, "(?i)^/$", &httpIndexGetHandler);
    server.addHandler(HTTP_PUT, "(?i)^/v1/_stop/?$", &httpStopPutHandler);
    server.addHandler(HTTP_GET, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVGetHandler);
    server.addHandler(HTTP_PUT, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVPutHandler);
    server.addHandler(HTTP_DELETE, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVDeleteHandler);

    server.listen("127.0.0.1", port);
    while(server.accept(terminate_flag)) {}

    return 0;
}

/* vim: set ai et ts=4 sw=4: */

#include <PgsqlServer.h>
#include <TcpServer.h>
#include <HttpServer.h>
#include <PersistentStorage.h>
#include <atomic>
#include <iostream>
#include <map>
#include <signal.h>
#include <sstream>
#include <string>
#include <utility>
#include <unistd.h>

// TODO: support global 64-bit _rev in ::set, ::delete. Useful for snapshots,
// incremental backups and replication. Write
// a multithreaded test
// TODO: add a script that runs clang static analyzer + cppcheck
// TODO: plugable porotols, e.g. support Memcached protocol
// TODO: support rangescans
// TODO: script - run under MemorySanitizer + other kinds of sanitizers
// TODO: see `grep -r TODO ./`

// TODO: Keyspaces class

PersistentStorage storage;

static std::atomic_bool terminate_flag(false);

static void httpIndexGetHandler(const HttpRequest&, HttpResponse& resp) {
    resp.setStatus(HTTP_STATUS_OK);

    std::ostringstream oss;
    oss << "HurmaDB is " << (terminate_flag.load() ? "terminating" : "running") << "!\n\n";
    resp.setBody(oss.str());
}

static void httpKVGetHandler(const HttpRequest& req, HttpResponse& resp) {
    bool found;
    const std::string& key = req.getQueryMatch(0);
    const std::string& value = storage.get(key, &found);
    resp.setStatus(found ? HTTP_STATUS_OK : HTTP_STATUS_NOT_FOUND);
    resp.setBody(value);
}

static void httpKVGetRangeHandler(const HttpRequest& req, HttpResponse& resp) {
    const std::string& key_from = req.getQueryMatch(0);
    const std::string& key_to = req.getQueryMatch(1);
    const std::string& value = storage.getRange(key_from, key_to);
    resp.setStatus(HTTP_STATUS_OK);
    resp.setBody(value);
}

static void httpKVPutHandler(const HttpRequest& req, HttpResponse& resp) {
    const std::string& key = req.getQueryMatch(0);
    const std::string& value = req.getBody();
    bool ok = true;
    try {
        storage.set(key, value);
    } catch(const std::runtime_error& e) {
        // Most likely validation failed
        ok = false;
    }

    resp.setStatus(ok ? HTTP_STATUS_OK : HTTP_STATUS_BAD_REQUEST);
}

static void httpKVDeleteHandler(const HttpRequest& req, HttpResponse& resp) {
    bool found = false;
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

    int c;
    int httpPort = 8080;
    int pgsqlPort = 5432;

    while ((c = getopt (argc, argv, "p:h:")) != -1)
    switch (c) {
        case 'h':
        httpPort = atoi(optarg);
        break;
        case 'p':
        pgsqlPort = atoi(optarg);
        break;
    }


    if(httpPort <= 0 || httpPort >= 65536) {
        std::cerr << "Invalid httpPort number!" << std::endl;
        return 2;
    }

    if(pgsqlPort <= 0 || pgsqlPort >= 65536) {
        std::cerr << "Invalid pgsqlPort number!" << std::endl;
        return 2;
    }

    HttpServer httpServer;

    PgsqlServer pgsqlServer(&terminate_flag);

    httpServer.addHandler(HTTP_GET, "(?i)^/$", &httpIndexGetHandler);
    httpServer.addHandler(HTTP_PUT, "(?i)^/v1/_stop/?$", &httpStopPutHandler);
    httpServer.addHandler(HTTP_GET, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVGetHandler);
    httpServer.addHandler(HTTP_GET, "(?i)^/v1/kv/([\\w-]+)/([\\w-]+)/?$", &httpKVGetRangeHandler);
    httpServer.addHandler(HTTP_PUT, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVPutHandler);
    httpServer.addHandler(HTTP_DELETE, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVDeleteHandler);

    pgsqlServer.listen("127.0.0.1", pgsqlPort);
    httpServer.listen("127.0.0.1", httpPort);

    while(!terminate_flag.load()) {
        //Unlike POSIX accept() procedure none of these .accept methods is blocking
        pgsqlServer.accept(terminate_flag);
        httpServer.accept(terminate_flag);
    }
    
    return 0;
}

/* vim: set ai et ts=4 sw=4: */

#include <HttpServer.h>
#include <PersistentStorage.h>
#include <PgsqlServer.h>
#include <TcpServer.h>
#include <atomic>
#include <iostream>
#include <map>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>

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

/**
 * @todo Rewrite using Storage::getRange instead of deprecated getRangeJson
 */
static void httpKVGetRangeHandler(const HttpRequest& req, HttpResponse& resp) {
    const std::string& key_from = req.getQueryMatch(0);
    const std::string& key_to = req.getQueryMatch(1);
    const std::string& valuesJson = storage.getRangeJson(key_from, key_to);
    resp.setStatus(HTTP_STATUS_OK);
    resp.setBody(valuesJson);
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
    bool httpPortSet = false;
    bool pgsqlPortSet = false;
    int httpPort = 8080;
    int pgsqlPort = 5432;

    while((c = getopt(argc, argv, "p:h:")) != -1)
        switch(c) {
            case 'h':
                httpPort = atoi(optarg);
                httpPortSet = true;
                break;
            case 'p':
                pgsqlPort = atoi(optarg);
                pgsqlPortSet = true;
                break;
        }

    if((!httpPortSet) || (!pgsqlPortSet)) {
        std::cerr << "Usage: " << argv[0] << " -h http_port -p pgsql_port" << std::endl;
        return 2;
    }

    if(httpPort <= 0 || httpPort >= 65536) {
        std::cerr << "Invalid HTTP port number!" << std::endl;
        return 2;
    }

    if(pgsqlPort <= 0 || pgsqlPort >= 65536) {
        std::cerr << "Invalid PgSql port number!" << std::endl;
        return 2;
    }

    HttpServer httpServer;
    PgsqlServer pgsqlServer(&storage);

    httpServer.addHandler(HTTP_GET, "(?i)^/$", &httpIndexGetHandler);
    httpServer.addHandler(HTTP_PUT, "(?i)^/v1/_stop/?$", &httpStopPutHandler);
    httpServer.addHandler(HTTP_GET, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVGetHandler);
    httpServer.addHandler(HTTP_GET, "(?i)^/v1/kv/([\\w-]+)/([\\w-]+)/?$", &httpKVGetRangeHandler);
    httpServer.addHandler(HTTP_PUT, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVPutHandler);
    httpServer.addHandler(HTTP_DELETE, "(?i)^/v1/kv/([\\w-]+)/?$", &httpKVDeleteHandler);

    std::cout << "Starting HTTP server on port " << httpPort << std::endl;
    httpServer.listen("127.0.0.1", httpPort);

    std::cout << "Starting PgSql server on port " << pgsqlPort << std::endl;
    pgsqlServer.listen("127.0.0.1", pgsqlPort);

    // Unlike POSIX accept() procedure none of these .accept methods is blocking
    while(!terminate_flag.load()) {
        // TODO: This seems to be a bottleneck during test execution.
        //       Run every accept() on a separate thread to speed things up.
        pgsqlServer.accept(terminate_flag);
        httpServer.accept(terminate_flag);
    }

    return 0;
}

#!/usr/bin/env python3

# vim: set ai et ts=4 sw=4:

import os
import random
import subprocess
import requests
import logging
import time
import socket
import sys
import psycopg2

START = os.environ.get('HURMADB_PORT') is None
PORT = int(os.getenv('HURMADB_PORT', 8000 + int(random.random()*1000)))
HTTPSERVER_PORT = 8080 #default port of http server
con = None
cur = None
logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
FLAG = '-p' #special flag for pgsql server port

def hurmadb_start(port):
    """
    Start HurmaDB on given port, return an object that corresponds to the created process.
    """
    if START:
        return subprocess.Popen(['../../hurmadb',FLAG, str(port)])
    else:
        return None

def hurmadb_stop(pobj):
    """
    Stop HurmaDB process created by hurmadb_start()
    """
    """
    Close only pgsqlServer without closing of all servers
    """
    if con:
        con.close()

    """
    Close all servers
    """
    requests.put('http://localhost:{}/v1/_stop'.format(HTTPSERVER_PORT))

    if START:
        pobj.wait()

class TestBasic:
    def setup_class(self):
        self.log = logging.getLogger('HurmaDB')
        self.pobj = hurmadb_start(PORT)
        time.sleep(3) # Give RocksDB some time to initialize
        self.log.debug("HurmaDB started on port {}".format(PORT))

    def teardown_class(self):
        hurmadb_stop(self.pobj)
        time.sleep(2) #time for HurmaDB to stop server
        self.log.debug("HurmaDB stopped")

    def test_connection(self):
        try:
            self.log.debug("Running test_connection")
            con = psycopg2.connect("dbname='postgres' user='nikitos' host='localhost' password='pass'")
            cur = con.cursor()
            self.log.debug("Running test_connection")
            cur.execute("UPDATE Cars SET Name='Alfred Schmidt' WHERE Id=1121;")
            cur.execute("DELETE FROM Cars where id = 1120")

        except psycopg2.DatabaseError as e:
            assert (False, ('Error %s' % e))

    def insert_query_without_columns_test(self):
        self.log.debug("Running insert_query_without_test")
        cur.execute("INSERT INTO kv VALUES ('aaa', 'ccc')")
        assert(1==cur.rowcount)

    def insert_query_with_columns_test(self):
        self.log.debug("Running insert_query_with_columns_test")
        cur.execute("INSERT INTO kv (k, v) VALUES ('aaa', 'ccc')")
        assert(1==cur.rowcount)

    def update_query_test(self):
        self.log.debug("Running update_query_test")
        cur.execute("UPDATE kv SET v = 'aaa' WHERE k = 'ccc'")
        assert(1==cur.rowcount)

    def select_by_key_query_test(self):
        self.log.debug("Running update_query_test")
        cur.execute("SELECT v FROM kv WHERE k = 'aaa'")
        assert(1==cur.rowcount)

    def delete_query_test(self):
        self.log.debug("Running update_query_test")
        cur.execute("DELETE FROM kv WHERE k = 'aaa'")
        assert(1==cur.rowcount)
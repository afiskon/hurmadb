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

START = os.environ.get('HURMADB_H_PORT') is None
PORT_PGSQL = int(os.getenv('HURMADB_P_PORT', 8000 + int(random.random()*1000)))
PORT_HTTP = int(os.getenv('HURMADB_H_PORT', 7000 + int(random.random()*1000)))
con = None # TODO: rewrite without these two global variables
cur = None
logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)

def hurmadb_start(http_port, pgsql_port):
    """
    Start HurmaDB on given port, return an object that corresponds to the created process.
    """
    if START:
        return subprocess.Popen(['../../hurmadb','-h', str(http_port), '-p', str(pgsql_port)])
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
    requests.put('http://localhost:{}/v1/_stop'.format(PORT_HTTP))

    if START:
        pobj.wait()

class TestBasic:
    def setup_class(self):
        self.log = logging.getLogger('HurmaDB')
        self.pobj = hurmadb_start(PORT_HTTP, PORT_PGSQL)
        time.sleep(3) # Give RocksDB some time to initialize
        self.log.debug("HurmaDB: http-server started on port {}".format(PORT_HTTP))
        self.log.debug("HurmaDB: pgsql-server started on port {}".format(PORT_PGSQL))

    def teardown_class(self):
        hurmadb_stop(self.pobj)
        time.sleep(2) #time for HurmaDB to stop server
        self.log.debug("HurmaDB stopped")

    def test_connection(self):
        try:
            self.log.debug("Running test_connection")
            con = psycopg2.connect("dbname='postgres', user='user', host='localhost', port=5332, password='pass'")
            cur = con.cursor()
            self.log.debug("Running test_connection")

        except psycopg2.DatabaseError as e:
            assert (False, ('Error %s' % e))

    def insert_query_without_columns_test(self):
        self.log.debug("Running insert_query_without_test")
        cur.execute("INSERT INTO kv VALUES ('aaa', 'ccc1')")
        assert(1==cur.rowcount)

    def insert_query_with_columns_test(self):
        self.log.debug("Running insert_query_with_columns_test")
        cur.execute("INSERT INTO kv (k, v) VALUES ('aaa', 'ccc2')")
        assert(1==cur.rowcount)

    def update_query_test(self):
        self.log.debug("Running update_query_test")
        cur.execute("UPDATE kv SET v = 'aaa' WHERE k = 'ccc'")
        assert(2==cur.rowcount)

    def select_by_key_query_test(self):
        self.log.debug("Running update_query_test")
        cur.execute("SELECT v FROM kv WHERE k = 'aaa'")
        assert(1==cur.rowcount)

    def delete_query_test(self):
        self.log.debug("Running update_query_test")
        cur.execute("DELETE FROM kv WHERE k = 'aaa'")
        assert(1==cur.rowcount)

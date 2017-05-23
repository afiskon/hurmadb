#!/usr/bin/env python3

# vim: set ai et ts=4 sw=4:

import os
import random
import subprocess
import requests
import logging
import time
import socket

START = os.environ.get('HURMADB_PORT') is None
PORT = os.getenv('HURMADB_PORT', 8000 + int(random.random()*1000))
logging.basicConfig(level=logging.WARNING)

def hurmadb_start(port):
    """
    Start HurmaDB on given port, return an object that corresponds to the created process.
    """
    if START:
        return subprocess.Popen(['./hurmadb', str(port)])
    else:
        return None

def hurmadb_stop(pobj):
    """
    Stop HurmaDB process created by hurmadb_start()
    """
    requests.put('http://localhost:{}/v1/_stop'.format(PORT))

    try:
        requests.get('http://localhost:{}/'.format(PORT))
    except:
        pass

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
        self.log.debug("HurmaDB stopped")

    def test_index(self):
        self.log.debug("Running test_index")
        res = requests.get('http://localhost:{}/'.format(PORT))
        assert(res.status_code == 200)

    def test_not_found(self):
        self.log.debug("Running test_not_found")
        res = requests.get('http://localhost:{}/no-such-page/'.format(PORT))
        assert(res.status_code == 404)

    def test_kv(self):
        self.log.debug("Running test_kv")
        url = 'http://localhost:{}/v1/kv/Some_Key-123'.format(PORT)
        doc = {'foo':'bar', 'baz':['qux']}
        # Make sure there is no such document
        res = requests.get(url)
        assert(res.status_code == 404)
        # Create a new document
        res = requests.put(url, json = doc)
        assert(res.status_code == 200)
        # Verify that the document was created
        res = requests.get(url)
        assert(res.status_code == 200)
        assert(res.json() == doc)
        # Delete the document
        res = requests.delete(url)
        assert(res.status_code == 200)
        # Verify that the document was deleted
        res = requests.get(url)
        assert(res.status_code == 404)
        res = requests.delete(url)
        # InMemory backend returns 404, RocksDB backend always returns 200
        assert(res.status_code == 200 or res.status_code == 404)

    # Test multiple message exchange during one connection
    def test_keep_alive(self):
        conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        conn.connect(('localhost', PORT))
        conn.send(b"GET / HTTP/1.1\r\n\r\n")
        data = conn.recv(1024)
        assert(data.startswith(b"HTTP/1.1 200 OK"))
        conn.send(b"GET / HTTP/1.1\r\n\r\n")
        data = conn.recv(1024)
        assert(data.startswith(b"HTTP/1.1 200 OK"))
        conn.close()
        assert(True)

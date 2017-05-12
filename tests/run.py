#!/usr/bin/env python3

# vim: set ai et ts=4 sw=4:

import os
import random
import subprocess
import requests
import logging

PORT = 8000 + int(random.random()*1000)
logging.basicConfig(level=logging.WARNING)

def hurmadb_start(port):
    """
    Start HurmaDB on given port, return an object that corresponds to the created process.
    """
    return subprocess.Popen(['./hurmadb', str(port)])

def hurmadb_stop(pobj):
    """
    Stop HurmaDB process created by hurmadb_start()
    """
    # subprocess.call(f"kill {pobj.pid}", shell = True)
    res = requests.put(f'http://localhost:{PORT}/v1/_stop')
    res = requests.get(f'http://localhost:{PORT}/')
    pobj.wait()

class TestBasic:
    def setup_class(self):
        self.log = logging.getLogger('HurmaDB')
        self.pobj = hurmadb_start(PORT)
        self.log.debug(f"HurmaDB started on port {PORT}")

    def teardown_class(self):
        hurmadb_stop(self.pobj)
        self.log.debug("HurmaDB stopped")

    def test_index(self):
        self.log.debug("Running test_index")
        res = requests.get(f'http://localhost:{PORT}/')
        assert(res.status_code == 200)

    def test_not_found(self):
        self.log.debug("Running test_not_found")
        res = requests.get(f'http://localhost:{PORT}/no-such-page/')
        assert(res.status_code == 404)

    def test_kv(self):
        self.log.debug("Running test_kv")
        url = f'http://localhost:{PORT}/v1/kv/Some_Key-123'
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
        assert(res.status_code == 404)


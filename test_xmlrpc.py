#!/usr/bin/env python3
"""Integration tests for rssani XML-RPC interface.

Starts the rssani binary, waits for the XML-RPC server on port 8080,
runs tests against every RPC method, then shuts down.

Usage: python3 test_xmlrpc.py [path/to/rssani]
"""

import sys
import os
import time
import signal
import subprocess
import unittest
import xmlrpc.client

RSSANI_BIN = sys.argv.pop(1) if len(sys.argv) > 1 else "./build/rssani"
RPC_URL = "http://localhost:8080/RPC2"
PROC = None


def setUpModule():
    global PROC
    env = os.environ.copy()
    env["XDG_CONFIG_HOME"] = "/tmp/rssani_test_config"
    os.makedirs(env["XDG_CONFIG_HOME"] + "/Selu", exist_ok=True)
    # Minimal config so rssani doesn't exit on missing values
    with open(env["XDG_CONFIG_HOME"] + "/Selu/rssani.conf", "w") as f:
        f.write("[principal]\npath=/tmp/rssani_test_torrents\ntimer=9999\n")
    os.makedirs("/tmp/rssani_test_torrents", exist_ok=True)
    PROC = subprocess.Popen(
        [RSSANI_BIN],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    # Wait for XML-RPC server to be ready
    proxy = xmlrpc.client.ServerProxy(RPC_URL)
    for _ in range(30):
        try:
            proxy.rssani.verTimer()
            return
        except (ConnectionRefusedError, OSError):
            time.sleep(0.2)
    raise RuntimeError("rssani did not start in time")


def tearDownModule():
    global PROC
    if PROC and PROC.poll() is None:
        try:
            proxy = xmlrpc.client.ServerProxy(RPC_URL)
            proxy.rssani.shutdown()
            PROC.wait(timeout=5)
        except Exception:
            PROC.send_signal(signal.SIGTERM)
            PROC.wait(timeout=5)


class TestXmlRpc(unittest.TestCase):
    def setUp(self):
        self.rpc = xmlrpc.client.ServerProxy(RPC_URL)

    # --- Read-only methods ---

    def test_ver_ultimo(self):
        result = self.rpc.rssani.verUltimo()
        self.assertIsInstance(result, str)
        # Should contain a date-like string (dd/MM/yyyy hh:mm:ss)
        self.assertRegex(result, r"\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2}")

    def test_ver_timer(self):
        result = self.rpc.rssani.verTimer()
        self.assertIsInstance(result, int)
        self.assertGreater(result, 0)

    def test_ver_log(self):
        result = self.rpc.rssani.verLog(0, 0)
        self.assertIsInstance(result, list)

    def test_lista_expresiones_empty(self):
        result = self.rpc.rssani.listaExpresiones()
        self.assertIsInstance(result, list)

    def test_lista_auths_empty(self):
        result = self.rpc.rssani.listaAuths()
        self.assertIsInstance(result, list)

    def test_ver_opciones(self):
        result = self.rpc.rssani.verOpciones()
        self.assertIsInstance(result, dict)
        self.assertIn("fromMail", result)
        self.assertIn("toMail", result)
        self.assertIn("path", result)

    # --- Regexp CRUD ---

    def test_regexp_crud(self):
        # Add
        self.assertTrue(self.rpc.rssani.anadirRegexp("test.*pattern", "31-12-2030", False, "", 0))

        regexps = self.rpc.rssani.listaExpresiones()
        names = [r["nombre"] for r in regexps]
        self.assertIn("test.*pattern", names)
        idx = names.index("test.*pattern")

        # Edit by index
        self.assertFalse(self.rpc.rssani.editarRegexpI(idx, "edited.*pattern"))
        regexps = self.rpc.rssani.listaExpresiones()
        self.assertEqual(regexps[idx]["nombre"], "edited.*pattern")

        # Edit by name
        self.assertFalse(self.rpc.rssani.editarRegexp("edited.*pattern", "final.*pattern"))
        regexps = self.rpc.rssani.listaExpresiones()
        self.assertEqual(regexps[idx]["nombre"], "final.*pattern")

        # Toggle active
        before = self.rpc.rssani.listaExpresiones()[idx]["activa"]
        self.assertFalse(self.rpc.rssani.activarRegexp(idx))
        after = self.rpc.rssani.listaExpresiones()[idx]["activa"]
        self.assertNotEqual(before, after)

        # Delete by index
        count_before = len(self.rpc.rssani.listaExpresiones())
        self.rpc.rssani.borrarRegexpI(idx)
        count_after = len(self.rpc.rssani.listaExpresiones())
        self.assertEqual(count_after, count_before - 1)

    def test_regexp_add_and_delete_by_name(self):
        self.rpc.rssani.anadirRegexp("deleteme", "", False, "", 0)
        self.rpc.rssani.borrarRegexpS("deleteme")
        names = [r["nombre"] for r in self.rpc.rssani.listaExpresiones()]
        self.assertNotIn("deleteme", names)

    def test_regexp_move(self):
        self.rpc.rssani.anadirRegexp("move_a", "", False, "", 0)
        self.rpc.rssani.anadirRegexp("move_b", "", False, "", 0)
        regexps = self.rpc.rssani.listaExpresiones()
        names = [r["nombre"] for r in regexps]
        idx_a = names.index("move_a")
        idx_b = names.index("move_b")

        self.rpc.rssani.moverRegexp(idx_a, idx_b)
        regexps = self.rpc.rssani.listaExpresiones()
        self.assertEqual(regexps[idx_b]["nombre"], "move_a")

        # Cleanup
        for name in ("move_a", "move_b"):
            self.rpc.rssani.borrarRegexpS(name)

    # --- Auth CRUD ---

    def test_auth_crud(self):
        self.assertTrue(self.rpc.rssani.anadirAuth("https://test.tracker", "uid1", "pass1", "key1"))

        auths = self.rpc.rssani.listaAuths()
        trackers = [a["tracker"] for a in auths]
        self.assertIn("https://test.tracker", trackers)

        self.rpc.rssani.borrarAuth("https://test.tracker")
        auths = self.rpc.rssani.listaAuths()
        trackers = [a["tracker"] for a in auths]
        self.assertNotIn("https://test.tracker", trackers)

    # --- Options ---

    def test_poner_opciones(self):
        self.assertTrue(self.rpc.rssani.ponerOpciones("from@test.com", "to@test.com", "/tmp/test"))
        opts = self.rpc.rssani.verOpciones()
        self.assertEqual(opts["fromMail"], "from@test.com")
        self.assertEqual(opts["toMail"], "to@test.com")
        self.assertEqual(opts["path"], "/tmp/test")

    def test_cambia_timer(self):
        self.assertTrue(self.rpc.rssani.cambiaTimer(5))

    def test_poner_credenciales(self):
        self.assertTrue(self.rpc.rssani.ponerCredenciales("newuser", "newpass"))

    # --- Save ---

    def test_guardar(self):
        self.assertTrue(self.rpc.rssani.guardar())


if __name__ == "__main__":
    unittest.main(verbosity=2)

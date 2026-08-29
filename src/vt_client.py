# src/vt_client.py
import json
import os
import socket
import subprocess
import threading
import time
from queue import Queue

class VTClient:
    def __init__(self, daemon_path="./bin/vt-daemon", socket_path="/tmp/vt.sock"):
        self.daemon_path = daemon_path
        self.socket_path = socket_path
        self.events = Queue()
        self._process = None
        self._socket = None
        self._stop_event = threading.Event()

    def start_daemon(self):
        if os.path.exists(self.socket_path):
            os.remove(self.socket_path)
        self._process = subprocess.Popen([self.daemon_path, "-s", self.socket_path])
        self._wait_for_socket()

    def connect(self):
        self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._socket.connect(self.socket_path)
        self._stop_event.clear()
        threading.Thread(target=self._read_loop, daemon=True).start()

    def write(self, data):
        if not self._socket:
            raise RuntimeError("Not connected to daemon")
        payload = data.encode('utf-8') if isinstance(data, str) else data
        self._socket.sendall(payload)

    def health(self):
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.connect(self.socket_path)
                sock.sendall(b"\x1B_VTD;health\x1B\\")
                sock.settimeout(1.0)
                with sock.makefile('r', encoding='utf-8') as f:
                    for line in f:
                        try:
                            parsed = json.loads(line)
                            if parsed.get("status") == "ok":
                                return True
                        except json.JSONDecodeError:
                            pass
        except Exception:
            pass
        return False

    def shutdown(self):
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.connect(self.socket_path)
                sock.sendall(b"\x1B_VTD;shutdown\x1B\\")
        except Exception:
            pass
        if self._process:
            self._process.wait()

    def stop(self):
        self._stop_event.set()
        if self._socket:
            try:
                self._socket.shutdown(socket.SHUT_RDWR)
                self._socket.close()
            except Exception:
                pass
            self._socket = None

    def _read_loop(self):
        try:
            with self._socket.makefile('r', encoding='utf-8') as f:
                while not self._stop_event.is_set():
                    line = f.readline()
                    if not line:
                        break
                    try:
                        self.events.put(json.loads(line))
                    except json.JSONDecodeError:
                        pass
        except Exception:
            pass

    def _wait_for_socket(self):
        for _ in range(30):
            if os.path.exists(self.socket_path):
                return
            time.sleep(0.1)
        raise RuntimeError(f"Daemon failed to start or bind to socket at {self.socket_path}")


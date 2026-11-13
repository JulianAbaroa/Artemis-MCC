"""
server.py
The HTTP server of the lists builder (LS.3, D291). Only the transport and the security live here; what each endpoint does is in api.py (a table
of routes the orchestrator can add to).

Security (D291 S1-S3, S7, S8): it listens on 127.0.0.1 only, answers 403 to a Host that is not 127.0.0.1:<port> or localhost:<port>, needs the
token of this run (header X-Artemis-Token, embedded in index.html) in every method that is not GET, sends no CORS header, serves the files
of static/ by exact name and nothing else of the disk, and limits the body to 1 MB (400, as an invalid JSON).
"""
import hmac
import json
import os
import re
import secrets
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit

from . import api

HOST = "127.0.0.1"
MAX_BODY = 1024 * 1024
MAX_DRAIN = 8 * MAX_BODY
STATIC_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "static")
STATIC_FILES = ("app.js", "core.js", "lists.js", "maps.js", "presets.js", "style.css")
CONTENT_TYPES = {".js": "text/javascript; charset=utf-8", ".css": "text/css; charset=utf-8", ".html": "text/html; charset=utf-8"}
TOKEN_PLACEHOLDER = "__ARTEMIS_TOKEN__"
STATIC_URL = re.compile(r"^/static/([a-z0-9_]+\.[a-z]+)$")
HEADERS = {"Cache-Control": "no-store", "X-Content-Type-Options": "nosniff"}
PAGE_HEADERS = {"Content-Security-Policy": "default-src 'self'; frame-ancestors 'none'", "Referrer-Policy": "no-referrer"}


class Handler(BaseHTTPRequestHandler):
    server_version = "ArtemisLists"
    protocol_version = "HTTP/1.1"

    def log_message(self, format, *args):
        """No access log: the console of the builder is for its own messages."""

    def _send(self, status, body, content_type, extra=None):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        for key, value in {**HEADERS, **(extra or {})}.items():
            self.send_header(key, value)
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def _json(self, status, payload):
        self._send(status, json.dumps(payload).encode("utf-8"), "application/json; charset=utf-8")

    def _error(self, status, message):
        self._json(status, {"error": message})

    def _host_allowed(self):
        port = self.server.server_address[1]
        return self.headers.get("Host", "").lower() in (f"127.0.0.1:{port}", f"localhost:{port}")

    def _token_valid(self):
        return hmac.compare_digest(self.headers.get("X-Artemis-Token", "").encode("utf-8"), self.server.token.encode("utf-8"))

    def _read_body(self):
        """The JSON of the body: None when there is none. Raises api.ApiError 400 when it is too big, not JSON or chunked."""
        if self.headers.get("Transfer-Encoding"):
            self.close_connection = True
            raise api.ApiError(400, "chunked bodies are not accepted: send a Content-Length")
        try:
            length = int(self.headers.get("Content-Length") or 0)
        except ValueError:
            self.close_connection = True
            raise api.ApiError(400, "Content-Length is not a number") from None
        if length < 0:
            self.close_connection = True
            raise api.ApiError(400, "Content-Length is negative")
        if length > MAX_BODY:
            self._drain(length)
            raise api.ApiError(400, f"the body is over {MAX_BODY} bytes")
        raw = self.rfile.read(length) if length else b""
        if not raw.strip():
            return None
        try:
            return json.loads(raw.decode("utf-8"))
        except ValueError as error:
            raise api.ApiError(400, f"invalid JSON: {error}") from None

    def _drain(self, length):
        """Reads (and drops) an oversized body so the client can read the 400, and closes the connection when it is far too big."""
        if length > MAX_DRAIN:
            self.close_connection = True
            return
        while length > 0:
            chunk = self.rfile.read(min(length, 65536))
            if not chunk:
                break
            length -= len(chunk)

    def _static(self, name):
        if name not in STATIC_FILES:
            return self._error(404, "not found")
        with open(os.path.join(STATIC_FOLDER, name), "rb") as handle:
            self._send(200, handle.read(), CONTENT_TYPES[os.path.splitext(name)[1]])

    def _page(self):
        with open(os.path.join(STATIC_FOLDER, "index.html"), encoding="utf-8") as handle:
            text = handle.read().replace(TOKEN_PLACEHOLDER, self.server.token)
        self._send(200, text.encode("utf-8"), CONTENT_TYPES[".html"], PAGE_HEADERS)

    def _handle(self):
        if not self._host_allowed():
            self.close_connection = True
            return self._error(403, "host not allowed")
        method = self.command
        if method != "GET" and not self._token_valid():
            self.close_connection = True
            return self._error(403, "missing or wrong X-Artemis-Token")
        path = urlsplit(self.path).path
        try:
            if method == "GET" and path == "/":
                return self._page()
            if method == "GET" and not path.startswith("/api/"):
                found = STATIC_URL.match(path)
                return self._static(found.group(1)) if found else self._error(404, "not found")
            if method not in ("GET", "POST", "PUT", "DELETE"):
                self.close_connection = True
                return self._error(405, f"method {method} is not supported")
            body = self._read_body() if method != "GET" else None
            status, payload = api.dispatch(self.server.context, method, path, body)
        except api.ApiError as error:
            return self._error(error.status, error.message)
        except Exception as error:  # a bug of a handler must answer, not drop the connection
            return self._error(500, f"internal error: {error}")
        self._json(status, payload)

    do_GET = do_POST = do_PUT = do_DELETE = do_PATCH = do_OPTIONS = do_HEAD = _handle


class Server(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, port, context):
        super().__init__((HOST, port), Handler)
        self.context = context
        self.token = secrets.token_urlsafe(32)


def make_server(port=0, context=None) -> Server:
    """The server on 127.0.0.1:port (0 = any free port) with the context of the repo (api.default_context()) unless one is given."""
    return Server(port, context or api.default_context())

#!/usr/bin/env python3
"""Server web de test, pentru proxy. Pornire: python3 tools/test_server.py [port]

Rute:
  GET  /            text simplu
  GET  /html        pagina HTML cu link-uri si cuvinte pentru reguli
  GET  /chunked     raspuns cu Transfer-Encoding: chunked
  GET  /gzip        raspuns comprimat cu gzip (indiferent de Accept-Encoding)
  GET  /slow?sec=N  raspunde dupa N secunde
  GET  /secret      pagina cu cuvantul "secret"
  GET  /big         ~200 KB de text
  GET  /img.gif     raspuns cu Content-Type: image/gif
  POST /echo        intoarce body-ul primit (si PUT /echo, DELETE /echo)
"""
import gzip
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *args):
        sys.stderr.write("[test-server] " + (fmt % args) + "\n")

    def send_body(self, body, ctype="text/plain; charset=utf-8", extra=None, code=200):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        for k, v in (extra or {}).items():
            self.send_header(k, v)
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def read_request_body(self):
        if "chunked" in self.headers.get("Transfer-Encoding", "").lower():
            data = b""
            while True:
                size = int(self.rfile.readline().strip().split(b";")[0], 16)
                if size == 0:
                    self.rfile.readline()
                    break
                data += self.rfile.read(size)
                self.rfile.readline()
            return data
        n = int(self.headers.get("Content-Length", 0))
        return self.rfile.read(n) if n else b""

    def do_GET(self):
        url = urlparse(self.path)
        if url.path == "/":
            self.send_body(b"hello from test server\n")
        elif url.path == "/html":
            html = (b"<html><body>\n"
                    b"<p>hello world, acesta este un test</p>\n"
                    b'<a href="http://example.com/pagina">link 1</a>\n'
                    b"<a href='http://example.com/alta'>link 2</a>\n"
                    b'<img src="http://example.com/poza.png">\n'
                    b'<a href="http://alt-site.org/x">link 3 (nu se schimba)</a>\n'
                    b"</body></html>\n")
            self.send_body(html, "text/html; charset=utf-8")
        elif url.path == "/chunked":
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Transfer-Encoding", "chunked")
            self.end_headers()
            for part in (b"hello ", b"chunked ", b"world\n"):
                self.wfile.write(b"%x\r\n%s\r\n" % (len(part), part))
            self.wfile.write(b"0\r\n\r\n")
        elif url.path == "/gzip":
            body = gzip.compress(b"hello gzip world, text comprimat\n")
            self.send_body(body, "text/plain", {"Content-Encoding": "gzip"})
        elif url.path == "/slow":
            sec = float(parse_qs(url.query).get("sec", ["2"])[0])
            time.sleep(sec)
            self.send_body(b"slow done\n")
        elif url.path == "/secret":
            self.send_body(b"this page has a secret inside\n")
        elif url.path == "/big":
            self.send_body((b"0123456789abcdef" * 64 + b"\n") * 200)
        elif url.path == "/img.gif":
            self.send_body(b"GIF89a-fake-image-bytes", "image/gif")
        else:
            self.send_body(b"not found\n", code=404)

    do_HEAD = do_GET

    def do_echo(self):
        if urlparse(self.path).path != "/echo":
            self.send_body(b"not found\n", code=404)
            return
        data = self.read_request_body()
        self.send_body(b"%s got %d bytes: " % (self.command.encode(), len(data)) + data + b"\n",
                       extra={"X-Method": self.command})

    do_POST = do_echo
    do_PUT = do_echo
    do_DELETE = do_echo


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9000
    srv = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"Server de test pe http://127.0.0.1:{port}", flush=True)
    srv.serve_forever()

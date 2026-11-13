"""python -m artemis.web [--port 8765] [--no-browser]: serves the lists builder on 127.0.0.1 and opens it in the browser."""
import argparse
import webbrowser

from .server import make_server


def main():
    parser = argparse.ArgumentParser(description="Local web builder of the training lists (listens on 127.0.0.1 only).")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true", help="do not open the page in the browser")
    arguments = parser.parse_args()
    server = make_server(port=arguments.port)
    url = f"http://127.0.0.1:{server.server_address[1]}/"
    print(f"Artemis lists builder at {url} (Ctrl+C to stop)")
    if not arguments.no_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

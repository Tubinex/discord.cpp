#!/usr/bin/env python3
from __future__ import annotations

import argparse
import asyncio
import json
import signal
from typing import Any

try:
    import websockets
except ImportError as exc: 
    raise SystemExit(
        "Missing dependency: websockets\n"
        "Install it with: pip install websockets"
    ) from exc


async def handle_client(websocket: Any) -> None:
    peer = websocket.remote_address
    print(f"[open] {peer}")

    welcome_payload = {
        "event": "server_ready",
        "message": "connected to local test server",
    }
    await websocket.send(json.dumps(welcome_payload))

    try:
        async for message in websocket:
            print(f"[recv] {peer}: {message}")
            await websocket.send(message)
    except websockets.ConnectionClosed:
        pass
    finally:
        print(f"[close] {peer}")


async def run_server(host: str, port: int) -> None:
    stop_event = asyncio.Event()

    def stop_handler() -> None:
        stop_event.set()

    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop.add_signal_handler(sig, stop_handler)
        except NotImplementedError:
            pass

    async with websockets.serve(handle_client, host, port):
        print(f"WebSocket server listening on ws://{host}:{port}")
        print("Press Ctrl+C to stop")
        await stop_event.wait()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a local WebSocket test server")
    parser.add_argument("--host", default="127.0.0.1", help="host to bind (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8765, help="port to bind (default: 8765)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    asyncio.run(run_server(args.host, args.port))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# vt-daemon

A high-performance, language-agnostic Virtual Terminal (VT) escape sequence parser. 

At its core, this project uses a zero-allocation C implementation of the Paul Williams VT500 state machine. To make this parser universally accessible, it is wrapped in a lightweight Unix Domain Socket (UDS) daemon (`vt-daemon`) that ingests raw terminal bytes and emits Newline-Delimited JSON (NDJSON) events. This allows any high-level language to effortlessly process complex terminal escape sequences.

Client libraries are provided for Crystal, Ruby, and Python.

## Features

* **Zero-Allocation Core**: The C parser (`src/vt.c`) uses a statically allocated 4096-byte lookup table and computed gotos for maximum throughput.
* **Language Agnostic**: Communicates via standard Unix Domain Sockets and NDJSON.
* **Concurrency**: The daemon forks per connection, safely supporting up to 50 concurrent parser streams.
* **UTF-8 Support**: Includes a lightweight, built-in UTF-8 decoder.
* **Lifecycle Management**: Built-in Application Program Command (APC) hooks (`VTD;health` and `VTD;shutdown`) allow clients to manage the daemon safely.

## Installation

Compile the daemon and install it along with its systemd service:

```bash
# Build the binaries
make all

# Install to /usr/local/bin and set up the systemd service
sudo make install
```

Start and enable the daemon via systemd:

```bash
sudo systemctl enable --now vt-daemon
```

Alternatively, run it manually in the background:

```bash
vt-daemon -s /tmp/vt.sock &
```

## Protocol & NDJSON Format

Send raw bytes to the UDS, and receive JSON objects separated by newlines.

**Example via Netcat:**

```bash
echo -n -e '\x1B[38:2:255:0:0m' | nc -U /tmp/vt.sock
```

**Output:**

```json
{"event":"clear"}
{"event":"param","byte":51}
{"event":"param","byte":56}
{"event":"param","byte":58}
{"event":"param","byte":50}
{"event":"param","byte":58}
{"event":"param","byte":50}
{"event":"param","byte":53}
{"event":"param","byte":53}
{"event":"param","byte":58}
{"event":"param","byte":48}
{"event":"param","byte":58}
{"event":"param","byte":48}
{"event":"csi_dispatch","byte":109,"ignore":false,"intermediates":[],"params":[{"v":38,"sub":false},{"v":2,"sub":true},{"v":255,"sub":true},{"v":0,"sub":true},{"v":0,"sub":true}]}
```

## Client Usage

The provided clients abstract the socket lifecycle, daemon process management, and JSON parsing.

### Crystal (`src/vt-client.cr`)

```crystal
require "vt-client"

client = VTClient.new("/usr/local/bin/vt-daemon", "/tmp/vt.sock")
client.start_daemon unless client.health?
client.connect

spawn do
  while event = client.events.receive?
    puts event["event"]
  end
end

client.write("Hello\x1B[1mWorld\x1B[0m")
sleep 1

client.stop
```

### Ruby (`src/vt_client.rb`)

```ruby
require_relative 'vt_client'

client = VTClient.new('/usr/local/bin/vt-daemon', '/tmp/vt.sock')
client.start_daemon unless client.health?
client.connect

consumer = Thread.new do
  while event = client.events.pop
    puts event["event"]
  end
end

client.write("Hello\x1B[1mWorld\x1B[0m")
sleep 1

client.stop
consumer.join
```

### Python (`src/vt_client.py`)

```python
import time
import threading
from vt_client import VTClient

client = VTClient("/usr/local/bin/vt-daemon", "/tmp/vt.sock")
if not client.health():
    client.start_daemon()
    
client.connect()

def consume():
    while True:
        event = client.events.get()
        if event is None:
            break
        print(event["event"])

thread = threading.Thread(target=consume, daemon=True)
thread.start()

client.write("Hello\x1B[1mWorld\x1B[0m")
time.sleep(1)

client.stop()
```

## License

MIT License.
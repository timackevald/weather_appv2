# weather_appv2

A Chas Academy school project: SUVX25 for learning object pattern in C.

## About

A simple HTTP server built in C for embedded systems.

**Key Features:**
- Waits for network activity using `select()` instead of constantly checking
- All memory allocated at startup - no surprises at runtime
- Non-blocking connections - server never freezes
- Handles up to 32 connections at once (configureable)
- Clean layered design: TCP → HTTP → Weather
- Uses almost no CPU when idle (~0.1%)

**How it's organized:**
- **TCP Layer**: Accepts new connections on port 8080 (configureable)
- **HTTP Layer**: Manages connections and handles HTTP requests
- **Weather Layer**: Where the weather logic goes. Under development...
- **Task Scheduler**: Watches all connections and calls their work functions when data arrives

The scheduler uses `select()` to sleep until something happens, then wakes up and processes it.

## Project Structure

```
.
├── include
│   ├── app
│   │   └── weather_app.h
│   ├── http
│   │   ├── http_connection.h
│   │   └── http_server.h
│   ├── task_scheduler
│   │   └── task_scheduler.h
│   ├── tcp
│   │   └── tcp_server.h
│   └── weather
│       ├── weather_connection.h
│       └── weather_server.h
├── main.c
├── Makefile
├── README.md
└── src
    ├── app
    │   └── weather_app.c
    ├── http
    │   ├── http_connection.c
    │   └── http_server.c
    ├── task_scheduler
    │   └── task_scheduler.c
    ├── tcp
    │   └── tcp_server.c
    └── weather
        ├── weather_connection.c
        └── weather_server.c

13 directories, 17 files
```

## Usage

**Build:**
```bash
make
```

**Run:**
```bash
./weather_app
```

**Test:**
```bash
curl http://localhost:8080
```

**Clean:**
```bash
make clean
```

## How It Works

The main loop is simple:
```c
while (1) {
    task_scheduler_run();  // Sleep until something happens
}
```

What happens inside:
1. Scheduler tells `select()` to watch all open connections
2. `select()` sleeps until data arrives on any connection
3. Scheduler wakes up and calls the right work function
4. Work gets done, then back to sleep

This means the server only uses CPU when there's actual work to do.

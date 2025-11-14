# weather_appv2
A Chas Academy school project: SUVX25 for learning object pattern in C.
## About
This is a lightweight, embedded-friendly HTTP weather server built in C.
Key Features:
- Uses a task scheduler to call work in registered tasks
- No allocations done at runtime, memory usage is deterministic
- Non-blocking I/O with cooperative multitasking
- Fixed connection pool (32 max concurrent clients)
- Layered architecture: TCP → HTTP → Weather

TCP Layer: Accepts incoming connections
HTTP Layer: Manages connection pool and HTTP protocol
Weather Layer: Application-specific weather logic
Task Scheduler: Coordinates all async work without threads

All components register with the scheduler and get polled cooperatively.
.
├── include
│   ├── app
│   │   └── weather_app.h
│   ├── http
│   │   ├── http_connection.h
│   │   └── http_server.h
│   ├── task_scheduler
│   │   └── task_scheduler.h
│   ├── tcp
│   │   ├── tcp_connection.h
│   │   └── tcp_server.h
│   └── weather
│       ├── weather_connection.h
│       └── weather_server.h
├── main.c
├── Makefile
├── README.md
└── src
    ├── app
    │   └── weather_app.c
    ├── http
    │   ├── http_connection.c
    │   └── http_server.c
    ├── task_scheduler
    │   └── task_scheduler.c
    ├── tcp
    │   ├── tcp_connection.c
    │   └── tcp_server.c
    └── weather
        ├── weather_connection.c
        └── weather_server.c

13 directories, 19 files

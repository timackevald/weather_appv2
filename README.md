# weather_appv2
A Chas Academy school project: SUVX25 for learning object pattern in C.

## About
A simple HTTP server built in C for embedded systems.

**Key Features:**
- Waits for network activity using `select()` instead of constantly checking
- All memory allocated at startup - no surprises at runtime
- Non-blocking connections - server never freezes
- Handles up to 32 connections at once (configurable)
- Clean layered design: TCP → HTTP → Weather
- Uses almost no CPU when idle (~0.1%)

### Layered Architecture

**App Layer**
- Container that holds and initializes all layers. Coordinates the entire system.

**Weather Layer**
- weather_server_t: Manages weather business logic and connection pool
- weather_connection_t[32]: Fixed pool for API calls and data processing
- Routes requests by endpoint (/weather, /forecast)
- Future: External weather API integration

**HTTP Layer**
- http_server_t: Manages HTTP connection pool and protocol handling
- http_connection_t[32]: Fixed pool for HTTP request/response processing
- Parses HTTP requests and formats responses
- Forwards processed requests to Weather layer via callbacks

**TCP Layer**
- tcp_server_t: Listens for incoming connections on port 8080
- Non-blocking accept loop
- Forwards new connections to HTTP layer via callback

**Task Scheduler**
- Central event loop using select() for I/O monitoring
- Calls work() on registered tasks:
  - tcp_server - accepts new connections
  - http_connection[0..31] - handles HTTP I/O
  - weather_connection[0..31] - processes weather logic

![Design](wa.png)

## Configuration

Settings in `include/config/config.h`:
- `CONNECTION_POOL_SIZE` - Max concurrent connections (default: 32)
- `DEFAULT_PORT` - Server port (default: "8080")
- `SCHEDULER_SELECT_TIMEOUT_MS` - Select timeout (default: 100ms)

Logging level in `main.c`:
```c
logging_init(LOG_LEVEL_INFO);  // Options: DEBUG, INFO, WARN, ERROR
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
curl http://localhost:8080/
curl http://localhost:8080/weather?city=Stockholm
curl http://localhost:8080/forecast?city=Paris
```

**Clean:**
```bash
make clean
```

## Architecture Highlights

- Zero malloc/free - fully pool-based allocation
- Single select() call per iteration (no duplicate polling)
- Proper error handling and resource cleanup on all paths
- Centralized configuration and structured logging

## Future Enhancements
- Integration with real weather APIs
- TLS/HTTPS support
- Connection timeouts
- Response caching

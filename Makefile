# ----------------------------
# Makefile for weather_app (FIXED)
# ----------------------------

CC = gcc
# FIXED: Removed -Wpedantic to allow GNU extension ##__VA_ARGS__
CFLAGS = -Wall -Wextra -std=c11  -D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE
LDFLAGS = 

# Include directories
INCLUDES = -I./include

# Source files (REMOVED tcp_connection.c - unused)
SRCS = \
    main.c \
    src/app/weather_app.c \
    src/task_scheduler/task_scheduler.c \
	src/event_watcher/event_watcher.c \
    src/tcp/tcp_server.c \
    src/http/http_server.c \
    src/http/http_connection.c \
    src/weather/weather_server.c \
    src/weather/weather_connection.c \
    src/logging/logging.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = weather_app

# ----------------------------
# Build rules
# ----------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the application
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Phony targets
.PHONY: all clean run debug

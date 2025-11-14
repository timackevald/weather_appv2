# ----------------------------
# Makefile for weather_app
# ----------------------------

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O0
LDFLAGS = 

# Include directories
INCLUDES = -I./include

# Source files
SRCS = \
    main.c \
    src/app/weather_app.c \
    src/task_scheduler/task_scheduler.c \
    src/tcp/tcp_server.c \
    src/tcp/tcp_connection.c \
    src/http/http_server.c \
    src/http/http_connection.c \
    src/weather/weather_server.c \
    src/weather/weather_connection.c

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

# Phony targets (not files)
.PHONY: all clean

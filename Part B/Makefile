CC = gcc
CFLAGS = -Wall -Wextra -I src
LDFLAGS = -lncurses -lzmq -lpthread -lprotobuf-c -lm

# Directories
ASTRONAUT_CLIENT_DIR = Astronaut-app
ASTRONAUT_DISPLAY_CLIENT_DIR = Astronaut-Display-app
GAME_SERVER_DIR = Game-Server-app
OUTER_SPACE_DISPLAY_DIR = Outer-Space-Display-app
SRC_DIR = src

# Source files
ASTRONAUT_CLIENT_SRCS = $(ASTRONAUT_CLIENT_DIR)/astronaut-client.c
ASTRONAUT_DISPLAY_CLIENT_SRCS = $(ASTRONAUT_DISPLAY_CLIENT_DIR)/astronaut-display-client.c
GAME_SERVER_SRCS = $(GAME_SERVER_DIR)/game-server.c
OUTER_SPACE_DISPLAY_SRCS = $(OUTER_SPACE_DISPLAY_DIR)/outer-space-display.c
COMMON_SRCS = $(SRC_DIR)/game-logic.c $(SRC_DIR)/space-display.c $(SRC_DIR)/client-logic.c $(SRC_DIR)/scores.pb-c.c

# Object files
ASTRONAUT_CLIENT_OBJS = $(ASTRONAUT_CLIENT_SRCS:.c=.o)
ASTRONAUT_DISPLAY_CLIENT_OBJS = $(ASTRONAUT_DISPLAY_CLIENT_SRCS:.c=.o)
GAME_SERVER_OBJS = $(GAME_SERVER_SRCS:.c=.o)
OUTER_SPACE_DISPLAY_OBJS = $(OUTER_SPACE_DISPLAY_SRCS:.c=.o)
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# Targets
all: astronaut-client astronaut-display-client game-server outer-space-display
	rm -f $(ASTRONAUT_CLIENT_OBJS) \
		$(ASTRONAUT_DISPLAY_CLIENT_OBJS) \
		$(GAME_SERVER_OBJS) \
		$(OUTER_SPACE_DISPLAY_OBJS) \
		$(COMMON_OBJS)

astronaut-client: $(ASTRONAUT_CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

astronaut-display-client: $(ASTRONAUT_DISPLAY_CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

game-server: $(GAME_SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

outer-space-display: $(OUTER_SPACE_DISPLAY_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(ASTRONAUT_CLIENT_OBJS) $(GAME_SERVER_OBJS) $(OUTER_SPACE_DISPLAY_OBJS) $(COMMON_OBJS) astronaut-client astronaut-display-client game-server outer-space-display

.PHONY: all clean

# Installation and dependencies
install-deps:
	# Ubuntu/Debian style
	sudo apt-get update
	sudo apt-get install -y \
		build-essential \
		libncurses5-dev \
		libzmq3-dev

# Run all components
run: $(TARGETS)
	@echo "Start game components manually in separate terminals:"
	@echo "1. ./game-server"
	@echo "2. ./outer-space-display"
	@echo "3. Multiple ./astronaut-client or ./astronaut-display-client instances"

# Help target
help:
	@echo "Targets:"
	@echo "  all             - Build all components"
	@echo "  clean           - Remove compiled binaries"
	@echo "  install-deps    - Install development dependencies"
	@echo "  run             - Show instructions to run game"lay-client instances"

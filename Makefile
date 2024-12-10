CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses -lzmq

TARGETS = game-server astronaut-client outer-space-display 

all: $(TARGETS)

game-server: game-server.c game-server.c config.h game-logic.h space-display.h 
	$(CC) $(CFLAGS) -o $@ game-server.c game-logic.c space-display.c $(LDFLAGS)

astronaut-client: astronaut-client.c config.h
	$(CC) $(CFLAGS) -o $@ astronaut-client.c $(LDFLAGS)

outer-space-display: outer-space-display.c space-display.c config.h space-display.h
	$(CC) $(CFLAGS) -o $@ outer-space-display.c space-display.c $(LDFLAGS)

clean:
	rm -f $(TARGETS)

# Phony targets
.PHONY: all clean

# Installation and dependencies
install-deps:
	# Ubuntu/Debian style
	sudo apt-get update
	sudo apt-get install -y \
		build-essential \
		libncurses5-dev \
		libzmq3-dev

	# Alternatively for other distributions, 
	# use their respective package managers

# Run all components
run: $(TARGETS)
	@echo "Start game components manually in separate terminals:"
	@echo "1. ./game-server"
	@echo "2. ./outer-space-display"
	@echo "3. Multiple ./astronaut-client instances"

# Help target
help:
	@echo "Targets:"
	@echo "  all             - Build all components"
	@echo "  clean           - Remove compiled binaries"
	@echo "  install-deps    - Install development dependencies"
	@echo "  run             - Show instructions to run game"
# Directories
BUILD_DIR = build

.PHONY: all clean user tools docker-build docker-up docker-down configure

all: configure user

configure:
	cmake -S . -B $(BUILD_DIR)

user: configure
	cmake --build $(BUILD_DIR) --target user

tools: configure
	cmake --build $(BUILD_DIR) --target random_conn

clean:
	rm -rf $(BUILD_DIR)

docker-build:
	docker-compose -f docker/compose.yaml build

docker-up:
	docker-compose -f docker/compose.yaml up -d

docker-down:
	docker-compose -f docker/compose.yaml down --remove-orphans

docker-logs:
	docker-compose -f docker/compose.yaml logs

# Generate random connections
generate-compose: tools
	./$(BUILD_DIR)/random_conn

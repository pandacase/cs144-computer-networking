CMAKE ?= cmake
BUILD_DIR ?= build

.PHONY: all configure clean

all: configure
	$(CMAKE) --build $(BUILD_DIR)

configure:
	$(CMAKE) -S . -B $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

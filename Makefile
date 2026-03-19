SHELL := /bin/bash

CMAKE ?= cmake
CTEST ?= ctest
PYTHON ?= python3

BUILD_PRESET ?= dev
TIDY_PRESET ?= dev-tidy
WS_URL ?= ws://127.0.0.1:8765

.PHONY: help all configure build test format format-check lint tidy example run-server clean distclean rebuild

help:
	@echo "Available targets:"
	@echo "  make configure     - Configure CMake with $(BUILD_PRESET) preset"
	@echo "  make build         - Build project"
	@echo "  make test          - Run tests"
	@echo "  make format        - Apply clang-format"
	@echo "  make format-check  - Verify clang-format"
	@echo "  make lint          - Run clang-tidy lint target"
	@echo "  make tidy          - Build with clang-tidy enabled preset"
	@echo "  make example       - Run basic consumer example"
	@echo "  make run-server    - Start local Python WebSocket test server"
	@echo "  make clean         - Clean build artifacts (inside build dir)"
	@echo "  make distclean     - Remove build directory"
	@echo "  make rebuild       - distclean + build"

all: build

configure:
	$(CMAKE) --preset $(BUILD_PRESET)

build: configure
	$(CMAKE) --build --preset $(BUILD_PRESET) -j

test: build
	$(CTEST) --preset $(BUILD_PRESET)

format: configure
	$(CMAKE) --build --preset $(BUILD_PRESET) --target format

format-check: configure
	$(CMAKE) --build --preset $(BUILD_PRESET) --target format-check

lint: configure
	$(CMAKE) --build --preset $(BUILD_PRESET) --target lint

tidy:
	$(CMAKE) --preset $(TIDY_PRESET)
	$(CMAKE) --build --preset $(TIDY_PRESET) -j

example: build
	bash -c 'set -a; [ -f .env ] && source .env; set +a; ./build/discordcpp_basic_consumer'

run-server:
	$(PYTHON) scripts/ws_test_server.py

clean: configure
	$(CMAKE) --build --preset $(BUILD_PRESET) --target clean

distclean:
	rm -rf build

rebuild: distclean build

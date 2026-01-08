# Makefile for UnrealLogsReader (Windows)

BUILD_DIR := build
DIST_DIR := dist
BUILD_TYPE := Release
EXECUTABLE := $(DIST_DIR)\UnrealLogsReader.exe

.PHONY: build run clean configure

# Default target
build: configure
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE)
	@if not exist $(DIST_DIR) mkdir $(DIST_DIR)
	copy $(BUILD_DIR)\$(BUILD_TYPE)\UnrealLogsReader.exe $(EXECUTABLE)

configure:
	@if not exist $(BUILD_DIR) cmake -B $(BUILD_DIR)

run: build
	$(EXECUTABLE)

clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
	@if exist $(DIST_DIR) rmdir /s /q $(DIST_DIR)

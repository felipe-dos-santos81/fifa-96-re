# Makefile for fifa96-reversed — DOS FIFA96 loader port (C11 + CMake + CTest)
# Targets by section:
#   build & test → maintenance
SERVICE = fifa96_loader

BUILD = build
CMAKE = cmake
CTEST = ctest
FILE ?= tests/golden/fw1.qfs

.PHONY: help configure build test check run clean rebuild tsr capture trace

# ── Build & test ───────────────────────────────────────────────────────────────

help: ## Print this help message
	@printf '\033[01;32m${SERVICE} — FIFA96 loader port (C11)\033[00;37m\n\n'
	@printf "\033[33mUsage:\033[0m\n  make [target]\n\n\033[33mTargets:\033[0m\n"
	@grep -E '^[-a-zA-Z0-9_\.\/]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; \
		{printf "  \033[36m%-26s\033[0m %s\n", $$1, $$2}'

configure: ## Configure the CMake build tree (C11, -Wall -Wextra -Werror)
	$(CMAKE) -S . -B $(BUILD)

build: configure ## Compile all libraries and test binaries
	$(CMAKE) --build $(BUILD)

test: build ## Build and run the full golden suite via CTest
	$(CTEST) --test-dir $(BUILD) --output-on-failure

check: test ## Exactly the gate set: configure + strict build + full suite

# ── Run ────────────────────────────────────────────────────────────────────────

run: build ## Build and run the port's container dumper (make run FILE=path, default fw1.qfs)
	./$(BUILD)/fifa96_dump $(FILE)

# ── Maintenance ────────────────────────────────────────────────────────────────

clean: ## Remove the CMake build tree
	rm -rf $(BUILD)

rebuild: clean build ## Clean rebuild from scratch

# ── Capture rig ────────────────────────────────────────────────────────────────

TRACE ?= captures/session-latest/trace.bin

tsr: build ## Assemble the capture TSR (requires nasm)
	@command -v nasm >/dev/null || { echo "nasm not installed (apt install nasm)" >&2; exit 1; }
	nasm -f bin tsr/fifa96_capture.asm -o $(BUILD)/FIFACAP.COM
	@printf 'FIFACAP.COM: '; wc -c < $(BUILD)/FIFACAP.COM
	@sha256sum $(BUILD)/FIFACAP.COM

capture: tsr ## Launch the game under the capture rig (needs game/FIFAPCCD96.iso)
	./run-fifa96-capture.sh

trace: build ## Parse a captured trace (make trace TRACE=captures/…/trace.bin)
	./$(BUILD)/fifa96_trace $(TRACE)

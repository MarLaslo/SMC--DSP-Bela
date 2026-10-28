# Builds and runs this project on the Bela, from the Mac terminal.
#
#   make run     copy the project to the Bela, build it there and run it
#                (Ctrl-C stops it)
#   make build   copy and build only
#   make stop    stop whatever is running on the Bela
#   make cpu     show CPU load and mode switches of the running program
#
# The code compiles on the Bela itself. This file is not copied over -
# the Bela uses its own Makefile (in ~/Bela), and this one just calls it.

# Where the board is (192.168.7.2 over USB) and what to call the project
BELA_HOST ?= root@192.168.7.2
PROJECT   ?= smc-dsp
# Extra Bela command line options, e.g. make run CL="--period 32"
CL        ?=

BELA_DIR   = /root/Bela
REMOTE_DIR = $(BELA_DIR)/projects/$(PROJECT)

# No extra compiler flags: every #include in dsp/ and libs/DaisySP is a
# relative path, so the project builds the same here and in the Bela IDE.
BELA_MAKE = make -C $(BELA_DIR) PROJECT=$(PROJECT) CL="$(CL)"

# build/, the program and the IDE's settings.json live only on the
# board, so they are left alone.
RSYNC_EXCLUDES = \
	--exclude=.git --exclude=.vscode --exclude=.DS_Store \
	--exclude=/Makefile --exclude='*.md' \
	--exclude=/build/ --exclude=/$(PROJECT) --exclude=/settings.json

.PHONY: run build stop cpu sync

run: sync
	ssh -t $(BELA_HOST) '$(BELA_MAKE) run'

build: sync
	ssh $(BELA_HOST) '$(BELA_MAKE)'

stop:
	ssh $(BELA_HOST) 'make -C $(BELA_DIR) stop'

cpu:
	ssh $(BELA_HOST) 'cat /proc/xenomai/sched/stat'

sync:
	ssh $(BELA_HOST) 'mkdir -p $(REMOTE_DIR)'
	rsync -az --delete $(RSYNC_EXCLUDES) ./ $(BELA_HOST):$(REMOTE_DIR)/

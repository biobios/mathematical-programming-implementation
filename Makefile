PROJECTS:=$(patsubst src/%, %, $(wildcard src/*))
APP_PROJECTS:=$(filter-out lib%, $(PROJECTS))
LIB_PROJECTS:=$(filter lib%, $(PROJECTS))

APP_TARGETS:=$(addprefix bin/, $(APP_PROJECTS))
DEBUG_APP_TARGETS:=$(addprefix bin/debug/, $(APP_PROJECTS))
PROF_APP_TARGETS:=$(addprefix bin/prof/, $(APP_PROJECTS))

LIB_TARGETS:=$(patsubst %, bin/%.a, $(LIB_PROJECTS))
DEBUG_LIB_TARGETS:=$(patsubst %, bin/debug/%.a, $(LIB_PROJECTS))
PROF_LIB_TARGETS:=$(patsubst %, bin/prof/%.a, $(LIB_PROJECTS))

MAKEFILE_TEMPLATE_FOR_APP=makefiles/Makefile_app.mk
MAKEFILE_TEMPLATE_FOR_LIB=makefiles/Makefile_lib.mk

export ROOT_DIR=$(shell pwd)
export CXXFLAGS:=-std=c++23 -O3 -Wall -Wextra -pedantic -mtune=native -march=native -flto $(CXXFLAGS)

all: $(APP_TARGETS) $(LIB_TARGETS)

$(APP_TARGETS): bin/%: src/%/Makefile
	@$(MAKE) -C src/$*/ build

$(DEBUG_APP_TARGETS): bin/debug/%: src/%/Makefile
	@$(MAKE) -C src/$*/ debug_build

$(PROF_APP_TARGETS): bin/prof/%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof_build

$(LIB_TARGETS): bin/%.a: src/%/Makefile
	@$(MAKE) -C src/$*/ build

$(DEBUG_LIB_TARGETS): bin/debug/%.a: src/%/Makefile
	@$(MAKE) -C src/$*/ debug_build

$(PROF_LIB_TARGETS): bin/prof/%.a: src/%/Makefile
	@$(MAKE) -C src/$*/ prof_build

setup: $(patsubst %, src/%/Makefile, $(PROJECTS))

$(patsubst %, src/%/Makefile, $(APP_PROJECTS)): src/%/Makefile: $(MAKEFILE_TEMPLATE_FOR_APP)
	@cp $< $@

$(patsubst %, src/%/Makefile, $(LIB_PROJECTS)): src/%/Makefile: $(MAKEFILE_TEMPLATE_FOR_LIB)
	@cp $< $@

$(addprefix run/, $(APP_PROJECTS)): run/%: src/%/Makefile
	@$(MAKE) -C src/$*/ run

$(addprefix prof_run/, $(APP_PROJECTS)): prof_run/%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof_run

clean:
	@rm -rf temp
	@rm -rf bin
	@rm -rf debug
	@rm -f src/*/Makefile

.PHONY: all clean setup $(addprefix run/, $(APP_PROJECTS)) $(addprefix prof_run/, $(APP_PROJECTS)) $(APP_TARGETS) $(DEBUG_APP_TARGETS) $(PROF_APP_TARGETS) $(LIB_TARGETS) $(DEBUG_LIB_TARGETS) $(PROF_LIB_TARGETS)
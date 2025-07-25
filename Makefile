LIB_SRCS=$(wildcard src/lib/*.cpp)
LIB_OBJS=$(patsubst src/lib/%.cpp, temp/lib/%.o, $(LIB_SRCS))
PROF_LIB_OBJS=$(patsubst src/lib/%.cpp, temp/prof/lib/%.o, $(LIB_SRCS))

PROJECTS:=$(patsubst src/%, %, $(filter-out src/lib, $(wildcard src/*)))
TARGETS:=$(addprefix bin/, $(PROJECTS))
DEBUG_TARGETS:=$(addprefix bin/debug/, $(PROJECTS))
PROF_TARGETS:=$(addprefix bin/prof/, $(PROJECTS))

export ROOT_DIR=$(shell pwd)
export LIB_NAME=mpilib
export CXXFLAGS=-I$(ROOT_DIR)/src/lib -std=c++23 -O3 -Wall -Wextra -pedantic -mtune=native -march=native

all: $(TARGETS)

$(TARGETS): bin/%: src/%/Makefile
	@$(MAKE) -C src/$*/

$(DEBUG_TARGETS): bin/debug/%: src/%/Makefile
	@$(MAKE) -C src/$*/ debug_build

$(PROF_TARGETS): bin/prof/%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof_build

setup: $(patsubst %, src/%/Makefile, $(PROJECTS))

$(patsubst %, src/%/Makefile, $(PROJECTS)): src/%/Makefile: Makefile_template
	@cp $< $@

debug:
	@echo TARGETS: $(TARGETS)
	@echo LIBS: $(LIBS)
	@echo PROJECTS: $(PROJECTS)
	@echo ROOT_DIR: $(ROOT_DIR)

bin/lib$(LIB_NAME).a: $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $(LIB_OBJS)

bin/prof/lib$(LIB_NAME).a: $(PROF_LIB_OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $(PROF_LIB_OBJS)

$(LIB_OBJS): temp/lib/%.o: src/lib/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(PROF_LIB_OBJS): temp/prof/lib/%.o: src/lib/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) -pg

$(addprefix run/, $(PROJECTS)): run/%: src/%/Makefile
	@$(MAKE) -C src/$*/ run

$(addprefix prof_run/, $(PROJECTS)): prof_run/%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof_run

clean:
	@rm -rf temp
	@rm -rf bin
	@rm -rf debug
	@rm -f src/*/Makefile

.PHONY: all clean setup debug $(TARGETS) $(addprefix run/, $(PROJECTS)) $(DEBUG_TARGETS) $(PROF_TARGETS)
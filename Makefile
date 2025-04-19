LIB_SRCS=$(wildcard src/lib/*.cpp)
LIB_OBJS=$(patsubst src/lib/%.cpp, temp/lib/%.o, $(LIB_SRCS))

PROJECTS:=$(patsubst src/%, %, $(filter-out src/lib, $(wildcard src/*)))
TARGETS:=$(addprefix bin/, $(PROJECTS))

export ROOT_DIR=$(shell pwd)
export LIB_NAME=mpilib
export CXXFLAGS=-I$(ROOT_DIR)/src/lib -L$(ROOT_DIR)/bin -l$(LIB_NAME) -std=c++20

all: $(TARGETS)

$(TARGETS): bin/%: src/%/Makefile
	@$(MAKE) -C src/$*/

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

$(LIB_OBJS): temp/lib/%.o: src/lib/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(addprefix run/, $(PROJECTS)): run/%: src/%/Makefile
	@$(MAKE) -C src/$*/ run

clean:
	@rm -rf temp
	@rm -rf bin
	@rm -rf debug
	@rm -f src/*/Makefile

.PHONY: all clean setup debug $(TARGETS) $(addprefix run/, $(PROJECTS))
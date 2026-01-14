ifndef VERBOSE_MAKEFILE
MAKEFLAGS += --no-print-directory
endif

-include makefiles/common.mk

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
export CXXFLAGS:=-std=c++23 -O3 -Wall -Wextra -pedantic -mtune=native -march=native -flto=auto $(CXXFLAGS)

.PHONY: all
all: $(addprefix build-, $(PROJECTS))

# 通常のmake使用法に対応するためのターゲット群
.PHONY: $(APP_TARGETS)
$(APP_TARGETS): bin/%: src/%/Makefile
	@$(MAKE) -C src/$*/ build

.PHONY: $(DEBUG_APP_TARGETS)
$(DEBUG_APP_TARGETS): bin/debug/%: src/%/Makefile
	@$(MAKE) -C src/$*/ debug-build

.PHONY: $(PROF_APP_TARGETS)
$(PROF_APP_TARGETS): bin/prof/%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof-build

.PHONY: $(LIB_TARGETS)
$(LIB_TARGETS): bin/%.a: src/%/Makefile
	@$(MAKE) -C src/$*/ build

.PHONY: $(DEBUG_LIB_TARGETS)
$(DEBUG_LIB_TARGETS): bin/debug/%.a: src/%/Makefile
	@$(MAKE) -C src/$*/ debug-build

.PHONY: $(PROF_LIB_TARGETS)
$(PROF_LIB_TARGETS): bin/prof/%.a: src/%/Makefile
	@$(MAKE) -C src/$*/ prof-build

# タスクランナー的な使い方に対応するためのターゲット群
# ビルドタスク
.PHONY: $(addprefix build-, $(PROJECTS))
$(addprefix build-, $(PROJECTS)): build-%: src/%/Makefile
	@$(MAKE) -C src/$*/ build

.PHONY: $(addprefix debug-build-, $(PROJECTS))
$(addprefix debug-build-, $(PROJECTS)): debug-build-%: src/%/Makefile
	@$(MAKE) -C src/$*/ debug-build

.PHONY: $(addprefix prof-build-, $(PROJECTS))
$(addprefix prof-build-, $(PROJECTS)): prof-build-%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof-build

# 実行タスク
.PHONY: $(addprefix run-, $(APP_PROJECTS))
$(addprefix run-, $(APP_PROJECTS)): run-%: src/%/Makefile
	@$(MAKE) -C src/$*/ run

.PHONY: $(addprefix prof-run-, $(APP_PROJECTS))
$(addprefix prof-run-, $(APP_PROJECTS)): prof-run-%: src/%/Makefile
	@$(MAKE) -C src/$*/ prof-run

# クリーンタスク
.PHONY: $(addprefix clean-, $(PROJECTS))
$(addprefix clean-, $(PROJECTS)): clean-%: src/%/Makefile
	@$(MAKE) -C src/$*/ clean
	@rm -f src/$*/Makefile

.PHONY: clean
clean: $(addprefix clean-, $(PROJECTS))

# デバッグ用クリーンタスク
.PHONY: $(addprefix debug-clean-, $(APP_PROJECTS))
$(addprefix debug-clean-, $(APP_PROJECTS)): debug-clean-%: src/%/Makefile
	@$(MAKE) -C src/$*/ debug-clean

.PHONY: debug-clean
debug-clean: $(addprefix debug-clean-, $(APP_PROJECTS))

# サブプロジェクトのMakefileをセットアップするタスク
$(patsubst %, src/%/Makefile, $(APP_PROJECTS)): src/%/Makefile: $(MAKEFILE_TEMPLATE_FOR_APP)
	@cp $< $@

$(patsubst %, src/%/Makefile, $(LIB_PROJECTS)): src/%/Makefile: $(MAKEFILE_TEMPLATE_FOR_LIB)
	@cp $< $@

# 強制的にクリーンアップするタスク
.PHONY: fclean
fclean:
	@rm -rf bin
	@rm -rf debug
	@rm -rf temp
	@rm -rf src/*/Makefile
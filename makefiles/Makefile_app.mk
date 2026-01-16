-include project_config.mk
-include dependencies.mk
-include $(ROOT_DIR)/makefiles/recursive_deps.mk
-include $(ROOT_DIR)/makefiles/common.mk

PROJECT_NAME=$(subst $(ROOT_DIR)/src/,,$(shell pwd))

SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,$(ROOT_DIR)/temp/$(PROJECT_NAME)/%.o,$(SRCS))
DEBUG_OBJS=$(patsubst %.cpp,$(ROOT_DIR)/temp/debug/$(PROJECT_NAME)/%.o,$(SRCS))
PROF_OBJS=$(patsubst %.cpp,$(ROOT_DIR)/temp/prof/$(PROJECT_NAME)/%.o,$(SRCS))

TARGET=$(ROOT_DIR)/bin/$(PROJECT_NAME)
DEBUG_TARGET=$(ROOT_DIR)/bin/debug/$(PROJECT_NAME)
PROF_TARGET=$(ROOT_DIR)/bin/prof/$(PROJECT_NAME)
DEPEND_DATA=$(patsubst $(ROOT_DIR)/data/$(PROJECT_NAME)/%,$(ROOT_DIR)/debug/$(PROJECT_NAME)/%,$(wildcard $(ROOT_DIR)/data/$(PROJECT_NAME)/*))

OBJS_DEPEND=$(OBJS:.o=.d)
DEBUG_OBJS_DEPEND=$(DEBUG_OBJS:.o=.d)
PROF_OBJS_DEPEND=$(PROF_OBJS:.o=.d)

LIB_OPTS=-L$(ROOT_DIR)/bin $(addprefix -l, $(DEPEND_LIBS))
DEBUG_LIB_OPTS=-L$(ROOT_DIR)/bin/debug $(addprefix -l, $(DEPEND_LIBS))
PROF_LIB_OPTS=-L$(ROOT_DIR)/bin/prof $(addprefix -l, $(DEPEND_LIBS))
INCLUDE_DIR_OPTS=$(addprefix -I$(ROOT_DIR)/src/lib, $(DEPEND_LIBS))

.PHONY: build
build: $(TARGET)
	@:
	
.PHONY: debug-build
debug-build: $(DEBUG_TARGET)
	@:

.PHONY: prof-build
prof-build: $(PROF_TARGET)
	@:

.PHONY: run
run: build $(DEPEND_DATA)
	@mkdir -p $(ROOT_DIR)/debug/$(PROJECT_NAME)
	$(call log,Running $(TARGET))
	@(cd $(ROOT_DIR)/debug/$(PROJECT_NAME) && $(ROOT_DIR)/bin/$(PROJECT_NAME) $(ARGS))

.PHONY: prof-run
prof-run: prof-build $(DEPEND_DATA)
	@mkdir -p $(ROOT_DIR)/debug/$(PROJECT_NAME)
	$(call log,Running $(PROF_TARGET))
	@(cd $(ROOT_DIR)/debug/$(PROJECT_NAME) && $(ROOT_DIR)/bin/prof/$(PROJECT_NAME) $(ARGS))

$(TARGET): $(OBJS) $(patsubst %, $(ROOT_DIR)/bin/lib%.a, $(DEPEND_LIBS))
	@mkdir -p $(dir $@)
	$(call log,Linking $@)
	@$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LIB_OPTS)
	@echo ""

$(DEBUG_TARGET): $(DEBUG_OBJS) $(patsubst %, $(ROOT_DIR)/bin/debug/lib%.a, $(DEPEND_LIBS))
	@mkdir -p $(dir $@)
	$(call log,Linking $@)
	@$(CXX) -o $@ $(DEBUG_OBJS) $(CXXFLAGS) -O0 -g $(DEBUG_LIB_OPTS)
	@echo ""

$(PROF_TARGET): $(PROF_OBJS) $(patsubst %, $(ROOT_DIR)/bin/prof/lib%.a, $(DEPEND_LIBS))
	@mkdir -p $(dir $@)
	$(call log,Linking $@)
	@$(CXX) -o $@ $(PROF_OBJS) $(CXXFLAGS) -O0 -pg $(PROF_LIB_OPTS)
	@echo ""

$(OBJS): $(ROOT_DIR)/temp/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call log,Compiling $< to $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -MMD -MP

$(DEBUG_OBJS): $(ROOT_DIR)/temp/debug/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call log,Compiling $< to $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -O0 -g -MMD -MP

$(PROF_OBJS): $(ROOT_DIR)/temp/prof/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call log,Compiling $< to $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -O0 -pg -MMD -MP

$(patsubst %, $(ROOT_DIR)/bin/lib%.a, $(DEPEND_LIBS)): $(ROOT_DIR)/bin/%.a: FORCE
	@$(MAKE) -C $(ROOT_DIR) build-$*

$(patsubst %, $(ROOT_DIR)/bin/debug/lib%.a, $(DEPEND_LIBS)): $(ROOT_DIR)/bin/debug/%.a: FORCE
	@$(MAKE) -C $(ROOT_DIR) debug-build-$*

$(patsubst %, $(ROOT_DIR)/bin/prof/lib%.a, $(DEPEND_LIBS)): $(ROOT_DIR)/bin/prof/%.a: FORCE
	@$(MAKE) -C $(ROOT_DIR) prof-build-$*

$(DEPEND_DATA): $(ROOT_DIR)/debug/$(PROJECT_NAME)/%: $(ROOT_DIR)/data/$(PROJECT_NAME)/%
	@mkdir -p $(dir $@)
	@cp $< $@

.PHONY: clean
clean:
	@rm -f $(TARGET)
	@rm -f $(DEBUG_TARGET)
	@rm -f $(PROF_TARGET)
	@rm -rf $(ROOT_DIR)/temp/$(PROJECT_NAME)
	@rm -rf $(ROOT_DIR)/temp/debug/$(PROJECT_NAME)
	@rm -rf $(ROOT_DIR)/temp/prof/$(PROJECT_NAME)

.PHONY: debug-clean
debug-clean:
	@rm -rf $(ROOT_DIR)/debug/$(PROJECT_NAME)

.PHONY: FORCE
FORCE: ;

-include $(OBJS_DEPEND)
-include $(DEBUG_OBJS_DEPEND)
-include $(PROF_OBJS_DEPEND)
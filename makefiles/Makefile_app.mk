-include project_config.mk
-include dependencies.mk
-include $(ROOT_DIR)/makefiles/recursive_deps.mk

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

build: $(TARGET)
	
debug_build: $(DEBUG_TARGET)

prof_build: $(PROF_TARGET)

run: build $(DEPEND_DATA)
	@mkdir -p $(ROOT_DIR)/debug/$(PROJECT_NAME)
	@(cd $(ROOT_DIR)/debug/$(PROJECT_NAME) && $(ROOT_DIR)/bin/$(PROJECT_NAME) $(ARGS))

prof_run: prof_build $(DEPEND_DATA)
	@mkdir -p $(ROOT_DIR)/debug/$(PROJECT_NAME)
	@(cd $(ROOT_DIR)/debug/$(PROJECT_NAME) && $(ROOT_DIR)/bin/prof/$(PROJECT_NAME) $(ARGS))

$(TARGET): $(OBJS) $(patsubst %, $(ROOT_DIR)/bin/lib%.a, $(DEPEND_LIBS))
	@mkdir -p $(dir $@)
	@$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LIB_OPTS)

$(DEBUG_TARGET): $(DEBUG_OBJS) $(patsubst %, $(ROOT_DIR)/bin/debug/lib%.a, $(DEPEND_LIBS))
	@mkdir -p $(dir $@)
	@$(CXX) -o $@ $(DEBUG_OBJS) $(CXXFLAGS) -O0 -g $(DEBUG_LIB_OPTS)

$(PROF_TARGET): $(PROF_OBJS) $(patsubst %, $(ROOT_DIR)/bin/prof/lib%.a, $(DEPEND_LIBS))
	@mkdir -p $(dir $@)
	@$(CXX) -o $@ $(PROF_OBJS) $(CXXFLAGS) -O0 -pg $(PROF_LIB_OPTS)

$(OBJS): $(ROOT_DIR)/temp/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -MMD -MP

$(DEBUG_OBJS): $(ROOT_DIR)/temp/debug/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -O0 -g -MMD -MP

$(PROF_OBJS): $(ROOT_DIR)/temp/prof/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -O0 -pg -MMD -MP

$(patsubst %, $(ROOT_DIR)/bin/lib%.a, $(DEPEND_LIBS)): $(ROOT_DIR)/bin/lib%.a:
	@$(MAKE) -C $(ROOT_DIR) bin/lib$*.a

$(patsubst %, $(ROOT_DIR)/bin/debug/lib%.a, $(DEPEND_LIBS)): $(ROOT_DIR)/bin/debug/lib%.a:
	@$(MAKE) -C $(ROOT_DIR) bin/debug/lib$*.a

$(patsubst %, $(ROOT_DIR)/bin/prof/lib%.a, $(DEPEND_LIBS)): $(ROOT_DIR)/bin/prof/lib%.a:
	@$(MAKE) -C $(ROOT_DIR) bin/prof/lib$*.a

$(DEPEND_DATA): $(ROOT_DIR)/debug/$(PROJECT_NAME)/%: $(ROOT_DIR)/data/$(PROJECT_NAME)/%
	@mkdir -p $(dir $@)
	@cp $< $@

.PHONY: build debug_build prof_build run prof_run $(patsubst %, $(ROOT_DIR)/bin/lib%.a, $(DEPEND_LIBS)) $(patsubst %, $(ROOT_DIR)/bin/debug/lib%.a, $(DEPEND_LIBS)) $(patsubst %, $(ROOT_DIR)/bin/prof/lib%.a, $(DEPEND_LIBS))

-include $(OBJS_DEPEND)
-include $(DEBUG_OBJS_DEPEND)
-include $(PROF_OBJS_DEPEND)
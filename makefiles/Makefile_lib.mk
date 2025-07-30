-include project_config.mk
-include dependencies.mk
-include $(ROOT_DIR)/makefiles/recursive_deps.mk

PROJECT_NAME=$(subst $(ROOT_DIR)/src/,,$(shell pwd))
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,$(ROOT_DIR)/temp/$(PROJECT_NAME)/%.o,$(SRCS))
DEBUG_OBJS=$(patsubst %.cpp,$(ROOT_DIR)/temp/debug/$(PROJECT_NAME)/%.o,$(SRCS))
PROF_OBJS=$(patsubst %.cpp,$(ROOT_DIR)/temp/prof/$(PROJECT_NAME)/%.o,$(SRCS))

TARGET=$(ROOT_DIR)/bin/$(PROJECT_NAME).a
DEBUG_TARGET=$(ROOT_DIR)/bin/debug/$(PROJECT_NAME).a
PROF_TARGET=$(ROOT_DIR)/bin/prof/$(PROJECT_NAME).a

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

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $(OBJS)

$(DEBUG_TARGET): $(DEBUG_OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $(DEBUG_OBJS)

$(PROF_TARGET): $(PROF_OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $(PROF_OBJS)

$(OBJS): $(ROOT_DIR)/temp/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -MMD -MP

$(DEBUG_OBJS): $(ROOT_DIR)/temp/debug/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -O0 -g -MMD -MP

$(PROF_OBJS): $(ROOT_DIR)/temp/prof/$(PROJECT_NAME)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS) $(INCLUDE_DIR_OPTS) -O0 -pg -MMD -MP

.PHONY: build debug_build prof_build

-include $(OBJS_DEPEND)
-include $(DEBUG_OBJS_DEPEND)
-include $(PROF_OBJS_DEPEND)


# Based on the default makefile used in PIN

# If the tool is built out of the kit, PIN_ROOT must be specified in the make invocation and point to the kit root.
ifdef PIN_ROOT
	CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
else
	CONFIG_ROOT := ../Config
endif
include $(CONFIG_ROOT)/makefile.config
include $(TOOLS_ROOT)/Config/makefile.default.rules
#$(info $(TOOL_CXXFLAGS))
#include makefile.rules
#$(info $(TOOLS_ROOT))

#OBJDIR := objs/
#SRC_DIR := .
#SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
#SRC_FILES=branch_histogram.cpp
#OBJ_FILES= $(patsubst %.cpp,$(OBJDIR)%$(OBJ_SUFFIX),$(SRC_FILES))
#$(info $(SRC_FILES))
#$(info $(OBJ_FILES))
#$(info $(OBJDIR))
#$(info $(OBJ_SUFFIX))

TOOL = branch_histogram

TAGE_PREDICTOR = /home/mcopik/projects/ETH/statistics/AndreSeznec/cbp32KB/
CXXFLAGS += -I${TAGE_PREDICTOR}
SRC_FILES += ${TAGE_PREDICTOR}/predictor.cc

all: $(OBJDIR)$(TOOL)$(PINTOOL_SUFFIX)

$(OBJDIR)%$(OBJ_SUFFIX): %.cpp
	$(CXX) -std=c++11 $(TOOL_CXXFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm $(OBJDIR)*
.PHONY: all


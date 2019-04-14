CXXFLAGS := $(LOCAL_CXXFLAGS)
CXXFLAGS += -std=c++11
CXXFLAGS += $(foreach inc, $(INCDIR), -I$(inc) )

LDFLAGS := $(LOCAL_LDFLAGS)


LOCAL_MODULE_STEM := $(subst $(realpath $(TOPDIR))/,,$(realpath $(LOCAL_PATH)))
#$(warning $(LOCAL_MODULE_STEM))

LOCAL_BUILD_MODULE := $(OUTDIR)/$(LOCAL_MODULE_STEM)/$(LOCAL_MODULE)
#$(warning $(LOCAL_BUILD_MODULE))

local_cpp_srcs := $(filter-out *.cpp, $(LOCAL_SRC_FILES))
local_cpp_objs := $(foreach src, $(local_cpp_srcs), \
	$(OUTDIR)/$(LOCAL_MODULE_STEM)/$(basename $(src)).o)
local_depends := $(local_cpp_objs:%.o=%.d)
#$(warning $(local_cpp_objs))


-include $(local_depends)

all: build_prepare $(LOCAL_MODULE)


$(LOCAL_MODULE): $(LOCAL_BUILD_MODULE)


$(LOCAL_BUILD_MODULE): $(local_cpp_objs)
	@$(CXX) $(LDFLAGS) -o $@ $^
	@echo "Build Target: $@ success"


.PHONY build_prepare:
	@mkdir -p $(OUTDIR)/$(LOCAL_MODULE_STEM)

$(OUTDIR)/$(LOCAL_MODULE_STEM)/%.o:$(LOCAL_PATH)/%.cpp
	@echo "CXX $@ <== $<"
	@$(CXX) -c $(CXXFLAGS) -o $@ -MD -MF $(OUTDIR)/$(LOCAL_MODULE_STEM)/$*.d $<


clean:
	@echo "************ Cleanning $(LOCAL_MODULE) ************"
	@$(RM) $(local_depends)
	@$(RM) $(local_cpp_objs)
	@$(RM) $(LOCAL_BUILD_MODULE)



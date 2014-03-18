########################################################################
# TARGET binary

# Everything gets built in here (include BOARD_TAG now)
OBJDIR  	= build-simulator

TARGET  = simulator
ARDUINO_LIBS += Simulator UTFT
CORE_LIB = $(OBJDIR)/libcore.a

########################################################################
# Miscellanea
#
ifndef USER_LIB_PATH
USER_LIB_PATH = $(ARDUINO_SKETCHBOOK)/libraries
endif

########################################################################
# Local sources
#
LOCAL_C_SRCS    = $(wildcard *.c)
LOCAL_CPP_SRCS  = $(wildcard *.cpp)
LOCAL_CC_SRCS   = $(wildcard *.cc)
LOCAL_INO_SRCS  = $(wildcard *.ino)
LOCAL_AS_SRCS   = $(wildcard *.S)
LOCAL_OBJ_FILES = \
	$(LOCAL_C_SRCS:.c=.o)     \
	$(LOCAL_CPP_SRCS:.cpp=.o) \
	$(LOCAL_CC_SRCS:.cc=.o)   \
	$(LOCAL_INO_SRCS:.ino=.o) \
	$(LOCAL_AS_SRCS:.S=.o)
LOCAL_OBJS      = $(patsubst %,$(OBJDIR)/%,$(LOCAL_OBJ_FILES))

########################################################################
# Rules for making stuff
#

# A list of dependencies
DEP_FILE   = $(OBJDIR)/depends.mk

# Names of executables
CAT     = cat
ECHO    = echo

# General arguments
USER_LIBS     = $(patsubst %,$(USER_LIB_PATH)/%,$(ARDUINO_LIBS))
USER_INCLUDES = $(patsubst %,-I%,$(USER_LIBS))
USER_LIB_CPP_SRCS   = $(wildcard $(patsubst %,%/*.cpp,$(USER_LIBS)))
USER_LIB_C_SRCS     = $(wildcard $(patsubst %,%/*.c,$(USER_LIBS)))
USER_LIB_OBJS = $(patsubst $(USER_LIB_PATH)/%.cpp,$(OBJDIR)/libs/%.o,$(USER_LIB_CPP_SRCS)) \
		$(patsubst $(USER_LIB_PATH)/%.c,$(OBJDIR)/libs/%.o,$(USER_LIB_C_SRCS))
CPPFLAGS      = -DSIMULATOR -I. $(USER_INCLUDES) -g -Wall `pkg-config --cflags --libs gtk+-3.0 vte-2.90`
CPPLIBS       = -lc -lm -lstdc++ -lutil `pkg-config --libs gtk+-3.0 vte-2.90`
CFLAGS        = -std=gnu99
CXXFLAGS      = -fno-exceptions
ASFLAGS       = -I. -x assembler-with-cpp
LDFLAGS       = -Wl,--gc-sections

# Dependency files
DEPS          = $(LOCAL_OBJS:.o=.d) $(USER_LIB_OBJS:.o=.d)

# library sources
$(OBJDIR)/libs/%.o: $(USER_LIB_PATH)/%.cpp
	mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/libs/%.o: $(USER_LIB_PATH)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(OBJDIR)/libs/%.d: %.c
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/libs/%.d: %.cc
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/libs/%.d: %.cpp
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/libs/%.d: %.S
	$(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/libs/%.d: %.s
	$(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/%.cpp: %.ino
	$(ECHO) '#include "Ardsim.h"' > $@
	$(CAT)  $< >> $@

# normal local sources
# .o rules are for objects, .d for dependency tracking
# there seems to be an awful lot of duplication here!!!
$(OBJDIR)/%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o: %.S
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@

$(OBJDIR)/%.o: %.s
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@

$(OBJDIR)/%.d: %.c
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/%.d: %.cc
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/%.d: %.cpp
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/%.d: %.S
	$(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/%.d: %.s
	$(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)

$(OBJDIR)/%.o: $(OBJDIR)/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.d: $(OBJDIR)/%.cpp
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)

########################################################################
#
# Explicit targets start here
#

all: 		$(OBJDIR) $(TARGET)

$(OBJDIR):
		mkdir $(OBJDIR)

$(TARGET): 	$(LOCAL_OBJS) $(CORE_LIB)
		$(CC) $(LDFLAGS) -o $@ $(LOCAL_OBJS) $(CORE_LIB) $(CPPLIBS)

$(CORE_LIB):	$(USER_LIB_OBJS)
		$(AR) rcs $@ $(USER_LIB_OBJS)

$(DEP_FILE):	$(OBJDIR) $(DEPS)
		cat $(DEPS) > $(DEP_FILE)

clean:
		rm -f -- $(LOCAL_OBJS) $(CORE_OBJS) $(LIB_OBJS) $(CORE_LIB) $(TARGETS) $(DEP_FILE) $(DEPS) $(USER_LIB_OBJS)

depends:	$(DEPS)
		cat $(DEPS) > $(DEP_FILE)

.PHONY:	all clean depends

include $(DEP_FILE)

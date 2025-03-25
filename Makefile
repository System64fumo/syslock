BINS = syslock
LIBS = libsyslock.so
PKGS = gtkmm-4.0 gtk4-layer-shell-0 pam wayland-client
SRCS = $(filter-out src/tap_to_wake.cpp, $(wildcard src/*.cpp))

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
DATADIR ?= $(PREFIX)/share
BUILDDIR = build

PROTOS = ext-session-lock-v1
PROTO_DIR = /usr/share/wayland-protocols/staging/ext-session-lock

PROTO_HDRS = $(addprefix src/, $(addsuffix .h, $(notdir $(PROTOS))))
PROTO_SRCS = $(addprefix src/, $(addsuffix .c, $(notdir $(PROTOS))))
PROTO_OBJS = $(PROTO_SRCS:.c=.o)

# Features
ifneq (, $(shell grep -E '^#define FEATURE_TAP_TO_WAKE' src/config.hpp))
	SRCS += src/tap_to_wake.cpp
	PKGS += libevdev
	CXXFLAGS += -std=c++20
endif

OBJS = $(patsubst src/%,$(BUILDDIR)/%, $(SRCS:.cpp=.o))

CXXFLAGS += -Oz -s -Wall -flto=auto -fno-exceptions -fPIC
LDFLAGS += -Wl,--as-needed,-z,now,-z,pack-relative-relocs

CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))

$(shell mkdir -p $(BUILDDIR))
JOB_COUNT := $(BINS) $(LIBS) $(PROTO_HDRS) $(PROTO_SRCS) $(PROTO_OBJS) $(OBJS) src/git_info.hpp
JOBS_DONE := $(shell ls -l $(JOB_COUNT) 2> /dev/null | wc -l)

define progress
	$(eval JOBS_DONE := $(shell echo $$(($(JOBS_DONE) + 1))))
	@printf "[$(JOBS_DONE)/$(shell echo $(JOB_COUNT) | wc -w)] %s %s\n" $(1) $(2)
endef

all: $(BINS) $(LIBS)

install: $(all)
	@echo "Installing..."
	@install -D -t $(DESTDIR)$(BINDIR) $(BUILDDIR)/$(BINS)
	@install -D -t $(DESTDIR)$(LIBDIR) $(BUILDDIR)/$(LIBS)
	@install -D -t $(DESTDIR)$(DATADIR)/sys64/lock config.conf style.css

clean:
	@echo "Cleaning up"
	@rm -rf $(BUILDDIR) \
		src/git_info.hpp \
		$(PROTO_OBJS) \
		$(PROTO_SRCS) \
		$(PROTO_HDRS)

$(BINS): src/git_info.hpp $(BUILDDIR)/main.o $(BUILDDIR)/config_parser.o
	$(call progress, Linking $@)
	@$(CXX) -o $(BUILDDIR)/$(BINS) \
	$(BUILDDIR)/main.o \
	$(BUILDDIR)/config_parser.o \
	$(CXXFLAGS) \
	$(shell pkg-config --libs gtkmm-4.0 gtk4-layer-shell-0)

$(LIBS): $(PROTO_HDRS) $(PROTO_SRCS) $(PROTO_OBJS) $(OBJS)
	$(call progress, Linking $@)
	@$(CXX) -o $(BUILDDIR)/$(LIBS) \
	$(filter-out $(BUILDDIR)/main.o, $(OBJS)) \
	$(PROTO_OBJS) \
	$(CXXFLAGS) \
	$(LDFLAGS) \
	-shared

$(BUILDDIR)/%.o: src/%.cpp
	$(call progress, Compiling $@)
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/%.o: src/%.c
	$(call progress, Compiling $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

$(PROTO_HDRS): src/%.h : $(PROTO_DIR)/$(PROTOS).xml
	$(call progress, Creating $@)
	@wayland-scanner client-header $< src/$(notdir $(basename $<)).h

$(PROTO_SRCS): src/%.c : $(PROTO_DIR)/$(PROTOS).xml
	$(call progress, Creating $@)
	@wayland-scanner public-code $< src/$(notdir $(basename $<)).c

src/git_info.hpp:
	$(call progress, Creating $@)
	@commit_hash=$$(git rev-parse HEAD); \
	commit_date=$$(git show -s --format=%cd --date=short $$commit_hash); \
	commit_message=$$(git show -s --format="%s" $$commit_hash | sed 's/"/\\\"/g'); \
	echo "#define GIT_COMMIT_MESSAGE \"$$commit_message\"" > src/git_info.hpp; \
	echo "#define GIT_COMMIT_DATE \"$$commit_date\"" >> src/git_info.hpp

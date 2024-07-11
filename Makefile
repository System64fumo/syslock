EXEC = syslock
LIB = libsyslock.so
PKGS = gtkmm-4.0 gtk4-layer-shell-0 pam wayland-client
SRCS =	$(filter-out src/main.cpp, $(wildcard src/*.cpp))
OBJS = $(SRCS:.cpp=.o)
DESTDIR = $(HOME)/.local

CXXFLAGS = -march=native -mtune=native -Os -s -Wall -flto=auto -fno-exceptions -fPIC
CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS))

PROTOS = ext-session-lock-v1
PROTO_DIR = /usr/share/wayland-protocols/staging/ext-session-lock

PROTO_HDRS = $(addprefix src/, $(addsuffix .h, $(notdir $(PROTOS))))
PROTO_SRCS = $(addprefix src/, $(addsuffix .c, $(notdir $(PROTOS))))
PROTO_OBJS = $(PROTO_SRCS:.c=.o)

all: $(EXEC) $(LIB)

install: $(EXEC) $(LIB)
	mkdir -p $(DESTDIR)/bin $(DESTDIR)/lib
	install $(EXEC) $(DESTDIR)/bin/$(EXEC)
	install $(LIB) $(DESTDIR)/lib/$(LIB)

clean:
	rm	$(EXEC) \
		$(SRCS:.cpp=.o) \
		src/git_info.hpp \
		$(PROTO_OBJS) \
		$(PROTO_SRCS) \
		$(PROTO_HDRS)

$(EXEC): src/main.cpp src/git_info.hpp
	$(CXX) -o $(EXEC) \
	src/main.cpp \
	$(CXXFLAGS) \
	$(LDFLAGS)

$(LIB): $(OBJS) $(PROTO_OBJS)
	$(CXX) -o $(LIB) \
	$(OBJS) \
	$(PROTO_OBJS) \
	$(CXXFLAGS) \
	-shared

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(PROTO_HDRS) $(PROTO_SRCS): $(PROTO_DIR)/$(PROTOS).xml
	wayland-scanner client-header $< src/$(notdir $(basename $<)).h
	wayland-scanner public-code $< src/$(notdir $(basename $<)).c

src/git_info.hpp:
	@commit_hash=$$(git rev-parse HEAD); \
	commit_date=$$(git show -s --format=%cd --date=short $$commit_hash); \
	commit_message=$$(git show -s --format=%s $$commit_hash); \
	echo "#define GIT_COMMIT_MESSAGE \"$$commit_message\"" > src/git_info.hpp; \
	echo "#define GIT_COMMIT_DATE \"$$commit_date\"" >> src/git_info.hpp

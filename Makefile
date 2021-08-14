PROJECT = inet46i

MULTIARCH ?= $(shell $(CC) --print-multiarch)

PREFIX ?= /usr
INCLUDEDIR ?= /include
LIBDIR ?= /lib/$(MULTIARCH)

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	RELEASE ?= 0
else
	RELEASE ?= 1
endif

# default
CPPFLAGS ?= -fdiagnostics-color=always
ifeq ($(RELEASE), 0)
	CFLAGS ?= -Os -fPIC
else
	CFLAGS ?= -O2 -fPIC
endif
LDFLAGS ?=

# debug
ifeq ($(DEBUG), 1)
	CFLAGS += -g
endif
ifeq ($(RELEASE), 0)
	CPPFLAGS += -DDEBUG
endif

# hardening
CPPFLAGS += -D_FORTIFY_SOURCE=2
CFLAGS += -fstack-protector-strong
LDFLAGS += -Wl,-z,relro

# diagnosis
CWARN ?= -Wall -Wextra -Wpedantic -Werror=format-security \
	-Wno-cast-function-type -Wno-missing-field-initializers
ifeq ($(DEBUG), 1)
	CWARN += -fanalyzer
endif
CFLAGS += $(CWARN)

# program flags
CPPFLAGS += -D_DEFAULT_SOURCE
CFLAGS += -std=c2x -fms-extensions
LDFLAGS += -Wl,--gc-sections -flto=auto -fwhole-program

HEADERS := $(wildcard inet46i/*.h)
SOURCES := $(wildcard inet46i/*.c)
OBJS := $(sort $(SOURCES:.c=.o))
PREREQUISITES := $(SOURCES:.c=.d)
THIS_MAKEFILE_LIST := $(MAKEFILE_LIST)

LIB_NAME := lib$(PROJECT).so
SO_NAME := $(LIB_NAME).0
REAL_NAME := $(LIB_NAME).0.0

ARLIB := lib$(PROJECT).a
SHLIB := $(SO_NAME)
PCFILE := $(PROJECT).pc

.PHONY: all
all: $(ARLIB) $(SHLIB) $(PCFILE)

.PHONY: clean
clean:
	rm -f $(ARLIB) $(SHLIB) $(OBJS) $(PCFILE) $(PREREQUISITES)

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< | sed 's,.*\.o *:,$(<:.c=.o) $@: $(THIS_MAKEFILE_LIST),' > $@

-include $(PREREQUISITES)

$(ARLIB): $(OBJS)
	$(AR) rcs $@ $^

$(SHLIB): $(OBJS)
	$(CC) -shared -Wl,-soname,$(SO_NAME) $(LDFLAGS) -o $@ $^

$(PCFILE): $(PCFILE).in
	sed 's|@prefix@|$(PREFIX)|; s|@libdir@|$(LIBDIR)|; s|@includedir@|$(INCLUDEDIR)|' $< > $@

.PHONY: install-shared
install-shared: $(SHLIB)
	install -d $(DESTDIR)$(PREFIX)$(LIBDIR) || true
	install -m 0644 $< $(DESTDIR)$(PREFIX)$(LIBDIR)/$(REAL_NAME)
	rm -f $(DESTDIR)$(PREFIX)$(LIBDIR)/$(SO_NAME)
	ln -s $(REAL_NAME) $(DESTDIR)$(PREFIX)$(LIBDIR)/$(SO_NAME)
	rm -f $(DESTDIR)$(PREFIX)$(LIBDIR)/$(LIB_NAME)
	ln -s $(SO_NAME) $(DESTDIR)$(PREFIX)$(LIBDIR)/$(LIB_NAME)

.PHONY: install-static
install-static: $(ARLIB)
	install -d $(DESTDIR)$(PREFIX)$(LIBDIR) || true
	install -m 0644 $< $(DESTDIR)$(PREFIX)$(LIBDIR)/$(ARLIB)

.PHONY: install-header
install-header: $(HEADERS) $(PCFILE)
	install -d $(DESTDIR)$(PREFIX)$(INCLUDEDIR)/$(PROJECT) || true
	install -m 0644 $(HEADERS) $(DESTDIR)$(PREFIX)$(INCLUDEDIR)/$(PROJECT)/
	install -d $(DESTDIR)$(PREFIX)$(LIBDIR)/pkgconfig || true
	install -m 0644 $(PCFILE) $(DESTDIR)$(PREFIX)$(LIBDIR)/pkgconfig

.PHONY: install
install: install-shared install-static install-header

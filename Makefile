CC?=gcc
XFLAGS=-D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L
CFLAGS?=-std=c11 -O2 -flto -Wall $(XFLAGS)

LIBS:=-lexif
LIBS+=$(shell pkg-config --cflags --libs GraphicsMagickWand)
IDIRS:=$(addprefix -iquote,include roscha roscha/include parcini/include)

BUILDIR?=build/release

ifdef DEBUG
BUILDIR:=build/debug
CFLAGS:=-std=c11 -O0 -DDEBUG $(XFLAGS) -g
endif
ifdef ASAN
CFLAGS+= -fsanitize=address -fno-omit-frame-pointer
endif

OBJDIR=$(BUILDIR)/obj

ROSCHA_SRCS+=$(shell find roscha -name '*.c' -not -path '*/tests/*')
ROSCHA_OBJS:=$(ROSCHA_SRCS:%.c=$(OBJDIR)/%.o)
PARCINI_SRCS:=$(wildcard parcini/src/*.c)
PARCINI_OBJS:=$(PARCINI_SRCS:%.c=$(OBJDIR)/%.o)
REVELA_SRCS:=$(shell find src -name '*.c' -not -path '*/tests/*')
REVELA_OBJS:=$(REVELA_SRCS:%.c=$(OBJDIR)/%.o)
ALL_OBJS:=$(ROSCHA_OBJS) $(PARCINI_OBJS) $(REVELA_OBJS)
TEST_OBJS:=$(filter-out $(OBJDIR)/src/revela.o,$(ALL_OBJS))

PREFIX?=/usr/local/

all: revela docs

test: tests/config tests/fs

tests/%: $(OBJDIR)/src/tests/%.o $(TEST_OBJS)
	mkdir -p $(BUILDIR)/$(@D)
	$(CC) -o $(BUILDIR)/$@ $^ $(IDIRS) $(LIBS) $(CFLAGS)

$(OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(IDIRS) -o $@ $< $(LIBS) $(CFLAGS)

revela: $(ALL_OBJS)
	mkdir -p $(@D)
	$(CC) -o $(BUILDIR)/$@ $^ $(LIBS) $(CFLAGS)

docs:
	mkdir -p build/man/
	scdoc < docs/revela.1.scd > build/man/revela.1
	scdoc < docs/revela.5.scd > build/man/revela.5

install:
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/share/man/man1
	mkdir -p $(PREFIX)/share/man/man5
	mkdir -p $(PREFIX)/share/revela
	install -m755 $(BUILDIR)/revela $(PREFIX)/bin/revela
	install -m755 util/revela-init $(PREFIX)/share/revela/revela-init
	ln -sf $(PREFIX)/share/revela/revela-init $(PREFIX)/bin/
	install -m644 build/man/revela.1 $(PREFIX)/share/man/man1/revela.1
	install -m644 build/man/revela.5 $(PREFIX)/share/man/man5/revela.5
	cp -r assets $(PREFIX)/share/revela/

uninstall:
	rm -rf $(PREFIX)/share/revela
	rm -f $(PREFIX)/bin/revela
	rm -f $(PREFIX)/bin/revela-init
	rm -f $(PREFIX)/share/man/man1/revela.1
	rm -f $(PREFIX)/share/man/man5/revela.5

clean:
	rm -r build

.PHONY: clean all test docs install uninstall

.PRECIOUS: $(OBJDIR)/src/tests/%.o

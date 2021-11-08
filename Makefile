CC?=gcc
XFLAGS=-D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L
CFLAGS?=-std=c99 -O2 -Wall $(XFLAGS)

LIBS:=-lexif
LIBS+=$(shell pkg-config --cflags --libs GraphicsMagickWand)
IDIRS:=$(addprefix -iquote,include unja/src unja parcini/include)

BUILDIR?=build/release

ifdef DEBUG
BUILDIR:=build/debug
CFLAGS:=-std=c99 -O0 -g -DDEBUG $(XFLAGS)
endif
ifdef ASAN
CFLAGS+= -fsanitize=address -fno-omit-frame-pointer
endif

OBJDIR=$(BUILDIR)/obj

UNJA_SRCS+=$(shell find unja -name '*.c' -not -path '*/tests/*')
UNJA_OBJS:=$(UNJA_SRCS:%.c=$(OBJDIR)/%.o)
PARCINI_SRCS:=$(wildcard parcini/src/*.c)
PARCINI_OBJS:=$(PARCINI_SRCS:%.c=$(OBJDIR)/%.o)
REVELA_SRCS:=$(shell find src -name '*.c' -not -path '*/tests/*')
REVELA_OBJS:=$(REVELA_SRCS:%.c=$(OBJDIR)/%.o)
ALL_OBJS:=$(UNJA_OBJS) $(PARCINI_OBJS) $(REVELA_OBJS)
TEST_OBJS:=$(filter-out $(OBJDIR)/src/revela.o,$(ALL_OBJS))

all: revela

test: tests/config tests/fs

tests/%: $(OBJDIR)/src/tests/%.o $(TEST_OBJS)
	mkdir -p $(BUILDIR)/$(@D)
	$(CC) -o $(BUILDIR)/$@ $^ $(LDFLAGS) $(IDIRS) $(LIBS)

$(OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(IDIRS) -o $@ $< $(LIBS)

revela: $(ALL_OBJS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $(BUILDIR)/$@ $^ $(LIBS) $(CFLAGS)

clean:
	rm -r build

.PHONY: clean all test

.PRECIOUS: $(OBJDIR)/src/tests/%.o

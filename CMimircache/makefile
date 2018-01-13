#define DEBUG_MAKEFILE 1

CC=gcc


CFLAGS += -Wall -Winline 
CFLAGS += -I/Library/Frameworks/Python.framework/Versions/3.5/include/python3.5m 
#macpro15 
CFLAGS += -I/usr/local/Cellar/python3/3.6.0/Frameworks/Python.framework/Versions/3.6/include/python3.6m/

CFLAGS += -I/Library/Frameworks/Python.framework/Versions/3.5/lib/python3.5/site-packages/numpy/core/include
CFLAGS += -Iheaders/ -Icache/ -Iheaders/cache -I/usr/lib/python3/dist-packages/numpy/core/include -I/usr/include/python3.4m/
CFLAGS += -Iheaders/ -Icache/ -Iheaders/cache -I/usr/lib/python3/dist-packages/numpy/core/include -I/usr/include/python3.5m/
# ifdef DEBUG_MAKEFILE 
CFLAGS+= -g -O0
#else
# CFLAGS+= -O2
#endif

CFLAGS += $(shell pkg-config --cflags glib-2.0)
LIBS    = $(shell pkg-config --libs glib-2.0 --libs gthread-2.0)
LIBS   += -lm 

# OBJS = splay.o LRUAnalyzer.o 
# SOURCES := $(wildcard cache/*.c) 
# SOURCES += $(wildcard cacheReader/*.c) 
# SOURCES += $(wildcard profiler/generalProfiler/*.c) 
# SOURCES += $(wildcard profiler/*.c) 
# SOURCES += $(wildcard utils/*.c) 
# SOURCES += $(wildcard profiler/LRUProfiler/*.c) 

# SOURCES = $(filter-out *python*, $(SOURCES))

SOURCES = $(shell find . ! -name "*python*" ! -name “*deprecated*” -name "*.c")
# SOURCES += wrapper/python_wrapper.c
# $(info SOURCES="$(SOURCES)")


OBJECTS := $(patsubst %.c, %.o, $(SOURCES))
HEADERS := $(wildcard headers/*.h)
HEADERS += $(wildcard headers/cache/*.h)
# HEADERS = $(shell find . -name "*.h")


# OBJS+= main.o splay.o parda.o parda_print.o narray.o process_args.o seperate.o
# HEADERS= splay.h parda.h narray.h process_args.h seperate.h

# SOURCES=$(subst .o,.c, $(OBJS) )

# $(OBJS):$(HEADERS) makefile
.phony: all clean

all: generalProfiler 

generalProfiler: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $+ $(LIBS)

$(info HEADERS="$(HEADERS)")

$(OBJECTS): %.o:%.c $(HEADERS) makefile
	$(CC) -c $(CFLAGS) $< -o $@ 
# %.d: %.c
# 	set -e; rm -f $@; \
# 	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
#         sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
#         rm -f $@.$$$$
# include $(sources:.c=.d

# $(info SOURCES="$(OBJECTS)")

clean:
	rm -f $(all) $(OBJECTS) generalProfiler
# $(shell find . -name "*.o" -exec rm {} \;)
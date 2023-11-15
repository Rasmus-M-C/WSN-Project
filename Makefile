CONTIKI_PROJECT = A C
all: $(CONTIKI_PROJECT)
CUSTOM_CFLAGS := -WALL CUSTOM_CFLAGS += -Wformat
WERROR := 0

CONTIKI=../..
include $(CONTIKI)/Makefile.include

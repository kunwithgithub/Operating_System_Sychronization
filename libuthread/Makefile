# Target library
lib := libuthread.a
objs := queue.o thread.o sem.o

CC := gcc
CFLAGS := -Wall -Werror
CFLAGS += -g
CCLIB := ar rcs

# Don't print the commands unless explicitely requested with `make V=1`
ifneq ($(V),1)
Q = @
V = 0
endif

all: $(lib) 

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

libuthread.a: $(objs)
	@echo "CCLIB $@"
	$(Q)$(CCLIB) $@ $^

# Generic rule for compiling objects
%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	@echo "clean"
	$(Q)rm -f $(lib) $(filter-out queue.o thread.o, $(objs)) $(deps)
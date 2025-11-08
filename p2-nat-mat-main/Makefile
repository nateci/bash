
SRCS := $(filter-out helpers/%_test.c, $(wildcard *.c helpers/*.c))
OBJS := $(SRCS:.c=.o)
HDRS := $(wildcard *.h) $(wildcard helpers/*.h)

CFLAGS := -g -Wall -I. -Ihelpers $(shell pkg-config fuse --cflags)

LDLIBS := `pkg-config fuse --libs`

nufs: $(OBJS)
	@echo "Linking $@ with: $(LDLIBS)"
	@gcc $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@ls -lh $@

%.o: %.c $(HDRS)
	@echo "Compiling $<"
	@gcc $(CFLAGS) -c -o $@ $<

clean: unmount
	rm -f nufs *.o test.log data.nufs
	rmdir mnt || true

mount: nufs
	mkdir -p mnt || true
	./nufs -s -f mnt data.nufs

unmount:
	fusermount -u mnt || true

test: nufs
	perl test.pl

gdb: nufs
	mkdir -p mnt || true
	gdb --args ./nufs -s -f mnt data.nufs

.PHONY: clean mount unmount gdb

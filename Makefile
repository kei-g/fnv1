AR = ar
ASSEMBLER_SOURCES = fnv1a_1024_avx1.s fnv1a_1024_avx2.s fnv1a_1024_avx512.s
CC = clang
CFLAGS = -D NDEBUG -O3 -Wall -Werror -march=native
CONTROLFILES = control md5sums
DEBPKG = fnv1_1.0.0_amd64.deb
LD = clang
LDFLAGS = -Wl,-s
RM = rm -fr
TAR = tar
TARFLAGS = --group root --owner root -ch
TARGET_BINARY = usr/local/bin/fnv1
TARGET_SOURCES = basis.c dump.c fnv1.c hash.c
TARGET_OBJECTS = $(TARGET_SOURCES:%.c=%.o)
TEST_BINARY = test.dat
TEST_BITS = 32 64 128 256 512 1024

all: $(DEBPKG)

clean:
	$(RM) $(TARGET_BINARY) $(TARGET_OBJECTS)

control.tar.xz: $(CONTROLFILES)
	$(TAR) $(TARFLAGS) -f - $(CONTROLFILES) | xz -cez9 - > $@ && $(RM) md5sums

data.tar.xz: $(TARGET_BINARY)
	$(TAR) $(TARFLAGS) -f - $(shell for name in `ls`; do test -d $$name && echo $$name; done | xargs) | xz -cez9 - > $@

debian-binary:
	@echo 2.0 > $@

disasm: $(TARGET_BINARY)
	@llvm-objdump --disassemble-all $(TARGET_BINARY) | less

md5sums: $(TARGET_BINARY)
	find . -type f | grep -v -e '/\.' | while read name; do echo $$name | cut -d'/' -f2- | grep '/' > /dev/null && echo $$name; done | xargs md5sum > $@

nm: $(TARGET_BINARY)
	@nm -gP --all $^ | less

perf: $(TARGET_BINARY) $(TEST_BINARY)
	@sleep 1
	@sudo dd if=/dev/zero bs=64M count=64 | ./$(TARGET_BINARY) -a -b 1024 --buffer-size 64M --clock || true
	@sleep 1
	@sudo dd if=/dev/zero bs=64M count=64 | ./$(TARGET_BINARY) -a -b 1024 --avx1 --buffer-size 64M --clock || true
	@sleep 1
	@sudo dd if=/dev/zero bs=64M count=64 | ./$(TARGET_BINARY) -a -b 1024 --avx2 --buffer-size 64M --clock || true

test: $(TARGET_BINARY) $(TEST_BINARY)
	@for b in $(TEST_BITS); do \
		temp=`mktemp`; \
		./$(TARGET_BINARY) -a -b $$b --binary < $(TEST_BINARY) > $$temp; \
		name=fnv1a-$$b; \
		{ cmp $$temp test.$$name.dat > /dev/null && echo "$$name success"; } || echo "\033[31m$$name failed\033[m"; \
		./$(TARGET_BINARY) -n -b $$b --binary < $(TEST_BINARY) > $$temp; \
		name=fnv1n-$$b; \
		{ cmp $$temp test.$$name.dat > /dev/null && echo "$$name success"; } || echo "\033[31m$$name failed\033[m"; \
		rm -f $$temp; \
	done
	@for opt in avx1 avx2; do \
		temp=`mktemp`; \
		./$(TARGET_BINARY) -a -b 1024 --$$opt --binary < $(TEST_BINARY) > $$temp && { \
			name=fnv1a-1024-$$opt; \
			{ cmp $$temp test.fnv1a-1024.dat > /dev/null && echo "$$name success"; } || echo "\033[31m$$name failed\033[m"; \
		}; \
		rm -f $$temp; \
	done

.PHONY: clean disasm nm perf test

.c.o:
	$(CC) $(CFLAGS) -c $<

$(DEBPKG): control.tar.xz data.tar.xz debian-binary
	$(AR) cr $@ $^ && $(RM) $^

$(TARGET_BINARY): $(TARGET_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(TARGET_OBJECTS) $(ASSEMBLER_SOURCES)

$(TEST_BINARY): $(TARGET_BINARY)
	@$(BRACKET_OPEN) -f $(TEST_BINARY) $(BRACKET_CLOSE) || { \
		dd if=/dev/urandom of=$(TEST_BINARY) bs=64K count=1 2> /dev/null; \
		for b in $(TEST_BITS); do \
			./$(TARGET_BINARY) -a -b $$b --binary < $(TEST_BINARY) > test.fnv1a-$$b.dat; \
			./$(TARGET_BINARY) -n -b $$b --binary < $(TEST_BINARY) > test.fnv1n-$$b.dat; \
		done; \
	}

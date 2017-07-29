RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

# Clean and compile .so
all: build/libshared-leto.so

create-dirs:
	mkdir -p build/pcb
	mkdir -p build/ipc

build/libshared-leto.so: create-dirs $(OBJS)
	$(CC) -shared -Wall -o "build/libshared-leto.so" $(OBJS) -lcommons

build/ipc/%.o: ipc/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	
build/pcb/%.o: pcb/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC += -DDEBUG -g
debug: all

# Clean build files
clean:
	$(RM) build

install: all
	cp -u build/libshared-leto.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include

uninstall:
	rm -f /usr/lib/libshared-leto.so
	rm -rf /usr/include/shared-leto

.PHONY: all create-dirs clean install uninstall

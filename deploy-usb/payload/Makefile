# Makefile para compilar altscreen_handler.so
# Arquitetura alvo da central MOI3 LG: aarch64 (Linux 64-bit ARM)

CC ?= aarch64-linux-gnu-gcc
CFLAGS ?= -Wall -Wextra -O2 -fPIC -shared
LDFLAGS ?= -ldl

TARGET = altscreen_handler.so
SRCS = altscreen_handler.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

# Build local x86_64 / arm64 macOS (para testes de sintaxe/simulação)
local:
	gcc -Wall -Wextra -O2 -fPIC -shared -o altscreen_handler.local.so $(SRCS) -ldl

clean:
	rm -f $(TARGET) altscreen_handler.local.so

.PHONY: all local clean

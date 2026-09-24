CC      := gcc
CFLAGS  := -Wall -g -O0 -MMD -MP
SRC_DIR := src
BUILD   := build
TARGET  := scanner

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test simple rtest rerun asan check-headers

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 헤더 의존성은 -MMD 가 만드는 build/*.d 를 통해 자동 추적된다.
$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(BUILD) $(TARGET) build-asan output.txt t.*

test: $(TARGET)
	./$(TARGET) grammar.gm test/array03.mod

output: $(TARGET)
	./$(TARGET) grammar.gm test/array01.mod > output.txt

simple: $(TARGET)
	./$(TARGET) simple.gm simple.mod

rtest: $(TARGET)
	python3 gen_pair.py
	./$(TARGET) t.gm t.mod > t.output

rerun: $(TARGET)
	./$(TARGET) t.gm t.mod > t.output

# ASan/UBSan 빌드는 별도 디렉토리에 만들어 일반 빌드와 섞이지 않게 한다.
asan: CFLAGS += -fsanitize=address,undefined
asan: BUILD := build-asan
asan: TARGET := scanner-asan
asan:
	$(MAKE) BUILD=build-asan TARGET=scanner-asan CFLAGS="$(CFLAGS)" scanner-asan

# 각 헤더가 단독으로 컴파일되는지 검사한다 (포함 순서 의존 / 순환 포함 탐지).
check-headers:
	@fail=0; \
	for h in $(SRC_DIR)/*.h; do \
		echo "#include \"$$(basename $$h)\"" > .hdrcheck.c; \
		if $(CC) -fsyntax-only -I$(SRC_DIR) .hdrcheck.c 2>/dev/null; then \
			printf '  %-16s OK\n' "$$(basename $$h)"; \
		else \
			printf '  %-16s FAIL\n' "$$(basename $$h)"; fail=1; \
		fi; \
	done; rm -f .hdrcheck.c; exit $$fail

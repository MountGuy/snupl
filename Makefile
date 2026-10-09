CC      := gcc
CFLAGS  := -Wall -g -O0 -MMD -MP
SRC_DIR := src
BUILD   := build
TARGET  := scanner

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test simple rtest rerun asan check-headers mathlib lexbench

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
	rm -rf $(BUILD) $(TARGET) lexbench build-asan build-bench output.txt t.*

test: $(TARGET)
	./$(TARGET) grammar.gm test/array01.mod

output: $(TARGET)
	./$(TARGET) grammar.gm test/array01.mod > output.txt

simple: $(TARGET)
	./$(TARGET) simple.gm simple.mod

lean: $(TARGET)
	./$(TARGET) test/lean.gm test/lean.lean

rtest: $(TARGET)
	python3 gen_pair.py
	./$(TARGET) t.gm t.mod > t.output

rerun: $(TARGET)
	./$(TARGET) t.gm t.mod > t.output

# 벤치마크: src/ 의 오브젝트에서 main.o 만 빼고 bench/lexbench.c 를 붙인다.
BENCH_OBJS := $(filter-out $(BUILD)/main.o,$(OBJS)) $(BUILD)/lexbench.o

# 벤치마크는 -O2 로 짓는다. -O0 코드의 쓰루풋을 재는 건 의미가 없다.
# asan 과 같은 방식으로 재귀 make 를 써서 -O0 오브젝트와 섞이지 않게 한다.
lexbench:
	$(MAKE) BUILD=build-bench CFLAGS="-Wall -Wextra -O2 -MMD -MP" lexbench-bin

lexbench-bin: $(BENCH_OBJS)
	$(CC) $(CFLAGS) -o lexbench $^

$(BUILD)/%.o: bench/%.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

# mathlib4 코퍼스로 NFA/DFA 쓰루풋 비교. 코퍼스가 없으면 먼저 받아온다.
mathlib: lexbench
	@test -d bench/corpus/mathlib4 || bench/fetch-mathlib.sh
	find bench/corpus/mathlib4/Mathlib -name '*.lean' | \
		./lexbench bench/lean-mathlib.gm --nfa-limit 400

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

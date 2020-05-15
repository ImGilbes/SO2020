BUILD=bin
SRC=src
TEST=test
CC=gcc

help:
	@echo "Farlo secondo le indicazioni del professore"

build: clean
	@mkdir -p $(BUILD)
	@$(CC) -o $(BUILD)/reader -std=gnu90 $(SRC)/reader.c $(SRC)/list.c $(SRC)/file_analysis.c -lm -lpthread
	@$(CC) -o $(BUILD)/slicer -std=gnu90 $(SRC)/slicer.c $(SRC)/list.c $(SRC)/file_analysis.c $(SRC)/itoa.c -lm -lpthread
	@$(CC) -o $(BUILD)/partitioner -std=gnu90 $(SRC)/partitioner.c $(SRC)/list.c $(SRC)/file_analysis.c $(SRC)/itoa.c -lm -lpthread

run: build
	@mkdir -p $(TEST)
	@$(BUILD)/reader -s 1      -m 1   Makefile src/* | sort > $(TEST)/reader
	@$(BUILD)/slicer           -m 5   Makefile src/* | sort > $(TEST)/slicer
	@$(BUILD)/partitioner -n 4 -m 30  Makefile src/* | sort > $(TEST)/partitioner

test: run
	@if diff $(TEST)/slicer $(TEST)/reader > /dev/null; then\
		echo "L'output di slicer coincide con quello di reader";\
	else\
		echo "L'output di slicer NON coincide con quello di reader";\
	fi

	@if diff $(TEST)/partitioner $(TEST)/reader > /dev/null; then\
		echo "L'output di partitioner coincide con quello di reader";\
	else\
		echo "L'output di partitioner NON coincide con quello di reader";\
	fi


clean:
	@rm -R -f $(BUILD) $(TEST)

.PHONY: help build clean run

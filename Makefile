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
	@$(CC) -o $(BUILD)/analyzer -std=gnu90 $(SRC)/analyzer.c $(SRC)/list.c $(SRC)/fs.c $(SRC)/itoa.c -lm -lpthread
	@$(CC) -o $(BUILD)/report -std=gnu90 $(SRC)/report.c $(SRC)/list.c $(SRC)/file_analysis.c -lm -lpthread
	@$(CC) -o $(BUILD)/newreport -std=gnu90 $(SRC)/newreport.c $(SRC)/list.c -lm -lpthread
	@$(CC) -o $(BUILD)/shell -std=gnu90 $(SRC)/shell.c $(SRC)/list.c $(SRC)/file_analysis.c $(SRC)/itoa.c -lm -lpthread

run: build
	@mkdir -p $(TEST)
	@$(BUILD)/reader      assets/* | sort > $(TEST)/reader
	@$(BUILD)/slicer      assets/* | sort > $(TEST)/slicer
	@$(BUILD)/partitioner assets/* | sort > $(TEST)/partitioner

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

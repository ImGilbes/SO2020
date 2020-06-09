BUILD=bin
SRC=src
TEST=test
CC=gcc
LIBS=$(SRC)/file_analysis.c $(SRC)/fs.c $(SRC)/itoa.c $(SRC)/list.c $(SRC)/utilities.c $(SRC)/history.c

help:
	@echo "Ricette disponibili:"
	@echo "- build: compila i programmi"
	@echo "- clean: pulisce le cartelle ed i file creati durante la compilazione (build) ed il testing (test)"
	@echo "- test: compila e verifica che gli output di reader, slicer e partitioner (che sono il "cuore" della lettura dei file) coincidano"
	@echo "- run: compila ed esegue la shell"

build: clean
	@mkdir -p $(BUILD)
	@$(CC) -o $(BUILD)/reader       -std=gnu90 $(SRC)/reader.c      $(LIBS) -lm -lpthread
	@$(CC) -o $(BUILD)/slicer       -std=gnu90 $(SRC)/slicer.c      $(LIBS) -lm -lpthread
	@$(CC) -o $(BUILD)/partitioner  -std=gnu90 $(SRC)/partitioner.c $(LIBS) -lm -lpthread
	@$(CC) -o $(BUILD)/analyzer     -std=gnu90 $(SRC)/analyzer.c    $(LIBS) -lm -lpthread
	@$(CC) -o $(BUILD)/report       -std=gnu90 $(SRC)/report.c      $(LIBS) -lm -lpthread
	@$(CC) -o $(BUILD)/shell        -std=gnu90 $(SRC)/shell.c       $(LIBS) -lm -lpthread

run: build
	@bin/shell

test: build
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

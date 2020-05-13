BUILD=bin
SRC=src
CC=gcc

help:
	@echo Per ora fa solo reader

build: clean
	@mkdir $(BUILD)
	@$(CC) -o $(BUILD)/reader -std=gnu90 $(SRC)/reader.c $(SRC)/list.c $(SRC)/file_analysis.c -lm -lpthread
	@$(CC) -o $(BUILD)/slicer -std=gnu90 $(SRC)/slicer.c $(SRC)/list.c $(SRC)/file_analysis.c $(SRC)/itoa.c -lm -lpthread

run: build
	@$(BUILD)/reader -s 1 -m 3 $(SRC)/reader.c Makefile
	@$(BUILD)/slicer -m 3 $(SRC)/slicer.c Makefile

clean:
	@rm -R -f $(BUILD)

.PHONY: help build clean

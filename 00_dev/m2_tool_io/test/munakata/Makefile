# Extract the base name of the first file in TARGET
FIRST_FILE_BASE := $(basename $(notdir $(firstword $(TARGET))))
# Add an underscore if the base name starts with a digit
SAFE_FIRST_FILE_BASE := $(shell echo $(FIRST_FILE_BASE) | sed 's/^\([0-9]\)/_\1/')

CXXFLAGS := -std=c++17 -Wall `pkg-config --cflags SimpleGtk`
CXXFLAGS += -DDEBUG -DDEBUG_LEVEL=3
CXXFLAGS_RELEASE := -O2 $(CXXFLAGS)
CXXFLAGS_DEBUG := -g -O0 $(CXXFLAGS)
LDFLAGS := `pkg-config --libs SimpleGtk`
CC := g++

.PHONY: all debug clean run

all: 
ifndef TARGET
	@echo "使い方: make TARGET=\"file1.cpp file2.cpp\""
	@echo "使い方: make debug TARGET=\"file1.cpp file2.cpp\""
else
	@$(CC) $(TARGET) $(CXXFLAGS_RELEASE) -D$(SAFE_FIRST_FILE_BASE)_MAIN -o $(FIRST_FILE_BASE) $(LDFLAGS)
	@if [ -f $(FIRST_FILE_BASE) ]; then \
		trap '' INT; \
		./$(FIRST_FILE_BASE); \
		rm -f $(FIRST_FILE_BASE); \
	else \
		echo "コンパイルに失敗しました。実行ファイルが生成されていません。"; \
	fi
endif

debug: 
ifndef TARGET
	@echo "使い方: make debug TARGET=\"file1.cpp file2.cpp\""
else
	@$(CC) $(TARGET) $(CXXFLAGS_DEBUG) -D$(SAFE_FIRST_FILE_BASE)_MAIN -o debug $(LDFLAGS)
	@echo [F5]でデバッグを開始します。
endif

clean:
	rm -f debug $(FIRST_FILE_BASE)
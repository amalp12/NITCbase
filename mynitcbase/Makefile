ifeq ($(mode),debug)
	CFLAGS := -g
	BUILD_DIR = ./build/debug
	TARGET = nitcbase-debug
else
	TARGET = nitcbase
	BUILD_DIR = ./build
endif
SUBDIR = FrontendInterface Frontend Algebra Schema BlockAccess BPlusTree Cache Buffer Disk_Class

ifeq ($(shell test $(stage) -gt 0; echo $$?),0)	
    symlink:
		@rm main.cpp; \
		ln -s stage_$(stage)/main.cpp main.cpp; \

		@for dir in $(SUBDIR); do \
			rm -r $$dir; \
            echo "Creating symbolic link for $$dir in parent directory..."; \
            ln -s stage_$(stage)/$$dir $$dir; \
        done
endif


HEADERS = $(wildcard define/*.h $(foreach fd, $(SUBDIR), $(fd)/*.h))
SRCS = $(wildcard main.cpp $(foreach fd, $(SUBDIR), $(fd)/*.cpp))
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:cpp=o))

$(TARGET): $(OBJS)
	g++ -std=c++11 $(CFLAGS) -o $@ $(OBJS) -lreadline

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	g++ -std=c++11  $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(BUILD_DIR)/*
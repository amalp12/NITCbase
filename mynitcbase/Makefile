ifeq ($(mode),debug)
	CFLAGS := -g
	BUILD_DIR = ./build/debug
	TARGET = nitcbase-debug
else
	TARGET = nitcbase
	BUILD_DIR = ./build
endif

SUBDIR = FrontendInterface Frontend Algebra Schema BlockAccess BPlusTree Cache Buffer Disk_Class

HEADERS = $(wildcard define/*.h $(foreach fd, $(SUBDIR), $(fd)/*.h))
SRCS = $(wildcard main.cpp $(foreach fd, $(SUBDIR), $(fd)/*.cpp))
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:cpp=o))

$(TARGET): $(OBJS)
	g++ -std=c++11 $(CFLAGS) -o $@ $(OBJS) -lreadline

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	g++ -std=c++11 $(CFLAGS) -o $@ -c $<
load:
	@for dir in $(SUBDIR); do \
		echo "Loading $$dir from stage $(stage) in parent directory..."; \
		rm -rf ./$$dir; \
		cp -rf ./stage_$(stage)/$$dir ./$$dir; \
	done
	rm -f ./main.cpp 
	cp ./stage_$(stage)/main.cpp ./main.cpp \

clean:
	rm -rf $(BUILD_DIR)/*
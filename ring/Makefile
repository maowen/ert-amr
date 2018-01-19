CXX ?= g++

CCFLAGS += -ansi -std99 -Wall -Wextra -Wpedantic -Wno-unused-parameter

CXXFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter

# -Wwrite-strings -Wcast-align -Wcast-qual \
# -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs \
# -Wno-unused-parameter -Wuninitialized -Wold-style-definition \
# -Wstrict-prototypes -Wmissing-prototypes

TEST_CXXFLAGS ?= $(CXXFLAGS) -isystem $(GTEST_DIR)/include -pthread

DEPENDS += ringbuf.h


OBJS = ringbuf.o

TEST_OBJS = ringbuftest.o


all: test

test: ringbuftest
	./ringbuftest

clean:
	rm -rf *.o ringbuftest

GTEST_LIB = $(GTEST_DIR)/make/gtest_main.a

$(GTEST_LIB):
	make -C $(GTEST_DIR)/make

%.o : %.c $(DEPENDS)
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

%.o : %.cpp $(DEPENDS)
	$(CXX) $(TEST_CXXFLAGS) -o $@ -c $< 

ringbuftest: $(TEST_OBJS) $(GTEST_LIB)
	$(CXX) $(TEST_CXXFLAGS) $? -o $@

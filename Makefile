APP:=mrpc

CXXFLAGS += -I./
CXXFLAGS += -I./src
CXXFLAGS += -std=c++11 -g -c -o

CXX:=g++

LIB_FILES := -lpthread -lglog -lgflags
LIB_TESTS := $(LIB_FILES) -L/usr/local/lib -lgtest -lgtest_main -lpthread

CPP_SOURCES := ./src/base/mutex.cc \
	./src/base/time.cc \
	./src/base/condition_variable.cc \
	./src/base/semaphore.cc \
	./src/base/ref_counted.cc \
	./src/base/arena.cc \
	./src/base/thread.cc \
	./src/base/pickle.cc \
	./src/base/string_piece.cc \
	\
	./test/opaque_ref_counted.cc \

CPP_OBJECTS := $(CPP_SOURCES:.cc=.o)

TESTS := ref_counted_unittest \


all: $(APP) $(TESTS)

$(APP): main.o $(CPP_OBJECTS)
	$(CXX) -o $(APP) main.o $(CPP_OBJECTS) $(LIB_FILES) -L/usr/local/lib -lgtest

.cc.o:
	$(CXX) $(CXXFLAGS) $@ $<

ref_counted_unittest: ref_counted_unittest.o
	$(CXX) -o $@ $< $(CPP_OBJECTS) $(LIB_TESTS)
ref_counted_unittest.o: ./src/base/ref_counted_unittest.cc
	$(CXX) $(CXXFLAGS) $@ $<


clean:
	rm -fr $(APP)
	rm -fr $(CPP_OBJECTS)

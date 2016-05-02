APP:=mrpc

CXXFLAGS := -I./src
CXXFLAGS += -std=c++11 -g -c -o

CXX:=g++

LIB_FILES := -lpthread -lglog -lgflags

CPP_SOURCES := ./src/base/mutex.cc \
	./src/base/time.cc \
	./src/base/condition_variable.cc \
	./src/base/semaphore.cc \
	./src/base/ref_counted.cc \
	./src/base/arena.cc \
	./src/base/thread.cc \

CPP_OBJECTS := $(CPP_SOURCES:.cc=.o)

all: $(APP)

$(APP): main.o $(CPP_OBJECTS)
	$(CXX) -o $(APP) main.o $(CPP_OBJECTS) $(LIB_FILES)

.cc.o:
	$(CXX) $(CXXFLAGS) $@ $<

clean:
	rm -fr $(APP)
	rm -fr $(CPP_OBJECTS)

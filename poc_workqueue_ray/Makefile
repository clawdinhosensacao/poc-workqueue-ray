CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17

CCTOOLS_DIR ?= ../cctools
WQ_INC = -I$(CCTOOLS_DIR)/work_queue/src -I$(CCTOOLS_DIR)/dttools/src
WQ_LIBS = $(CCTOOLS_DIR)/work_queue/src/libwork_queue.a $(CCTOOLS_DIR)/dttools/src/libdttools.a -lz -lm -pthread -ldl

all: compress_task wq_manager

compress_task: compress_task.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< -lz

wq_manager: wq_manager.cpp
	$(CXX) $(CXXFLAGS) $(WQ_INC) -o $@ $< $(WQ_LIBS)

clean:
	rm -f compress_task wq_manager

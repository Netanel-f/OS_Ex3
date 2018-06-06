CC = g++
CXX = g++

INCS=-I.
CFLAGS = -Wall -lpthread -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -lpthread -std=c++11 -g $(INCS)

TAR = tar
TARFLAGS = -cvf
TARNAME = ex3.tar
BARSRCS = Barrier.cpp Barrier.h
FRAMSRCS = MapReduceFramework.h MapReduceFramework.cpp MapReduceClient.h
CORESRCS =  $(BARSRCS) $(FRAMSRCS)
TARSRCS = MapReduceFramework.cpp $(BARSRCS) Makefile README

default: libMapReduceFramework.a

libMapReduceFramework.a: MapReduceFramework.o Barrier.o
	ar rcs $@ $^

libMapReduceFramework.o: $(FRAMSRCS) Barrier.h

sample: SampleClient

SampleClient: SuppliedFiles/sampleclient/SampleClient.o libMapReduceFramework.a

SuppliedFiles/sampleclient/SampleClient.o: SuppliedFiles/sampleclient/SampleClient.cpp

.PHONY : clean
clean:
	$(RM) *.o libMapReduceFramework.a main $(TARNAME) *~

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
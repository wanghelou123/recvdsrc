CFLAGS=  -Wall
INCS =  -I/usr/apps/pgsql/include/ 
LIBS += -L/usr/apps/pgsql/lib/ -lpq 
INCS += -I /home/klha/src_packages/boost_1_49_0/
LIBS += -L/home/klha/src_packages/boost_1_49_0/stage/lib -lboost_system -lboost_thread-mt  -lboost_date_time-d
LIBS +=   -llog4cplus

SRC := $(shell ls *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRC))
CC = g++

all:recv-data-platform
recv-data-platform:$(OBJS) 
	$(CC) $(CFLAGS) -o  $@ $^   $(INCS) $(LIBS)
%.o: %.cpp
	g++ $(CFLAGS) -c -o  $@ $<  $(INCS) $(LIBS)

clean:
	rm -f *.o recv-data-platform

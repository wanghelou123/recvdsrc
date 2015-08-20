#照明灯系统
#APP=lightsysd

#标准收数软件 
APP := recv-data-platform

CFLAGS :=  -Wall 
CFLAGS +=  -D RECVD_VERSION=\"v2.0.9\"
CFLAGS +=  -D PROGNAME=\"$(APP)\"

ifeq ($(APP),lightsysd)
CFLAGS +=  -D LIGHTSYS
CFLAGS +=  -D RECVD_TYPE=\"照明版\"
else
CFLAGS +=  -D RECVD_TYPE=\"标准版\"
endif

INCS = -I/usr/apps/pgsql/include/ 
LIBS +=  -L/usr/apps/pgsql/lib/ -lpq 
LIBS += -llog4cplus
SRC := $(shell ls *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRC))
CC = g++
OS := $(shell sed -n '1,1p' /etc/issue | cut -d' ' -f1)

ifeq ($(OS), CentOS)
INCS += -I /home/klha/src_packages/boost_1_49_0/
LIBS += -L /home/klha/src_packages/boost_1_49_0/stage/lib -lboost_system -lboost_thread-mt  -lboost_date_time-d
else
INCS += -I /home/whl/boost_1_49_0x86/
LIBS += -L /home/whl/boost_1_49_0x86/boost_1_49_0/stage/lib -lboost_system -lboost_thread-mt  -lboost_date_time-d
endif

all:$(APP)
$(APP):$(OBJS) 
	$(CC) -o  $@ $^  $(CFLAGS)  $(INCS) $(LIBS)
%.o: %.cpp
	$(CC) -c -o  $@ $<  $(CFLAGS) $(INCS) $(LIBS)

clean:
	rm -f *.o $(APP)

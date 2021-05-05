.PHONY:all gude shared clean

#生成目标文件存放目录
$(shell test -d objs || mkdir objs)
#生成日志文件存放目录
$(shell test -d logs || mkdir logs)
#生成种子文件存放目录
$(shell test -d torrent || mkdir torrent)
#生成动态库文件存放目录
$(shell test -d libs || mkdir libs)

CPP = g++
CFLAGS = -std=c++11 -pipe -O1 -W -fPIC
#-g
CLIBS = -lpthread -lm -ldl -lssl -lcrypto
DYNAMIC_PATH = libs/libgude.so

SRC_OBJS = \
	objs/Assert.o \
	objs/thread.o \
	objs/util.o \
	objs/bencode.o \
	objs/process.o \
	objs/log.o \
	objs/address.o \
	objs/socket.o \
	objs/fiber.o \
	objs/scheduler.o \
	objs/fdregister.o \
	objs/timer.o \
	objs/iocontext.o \
	objs/iomanager_epoll.o \
	objs/hook.o \
	objs/environment.o \
	objs/Iconv.o \
	objs/config.o \
	objs/worker.o \
	objs/udpserver.o \
	objs/server.o \
	\
	objs/json_value.o \
	objs/json_reader.o \
	objs/json_writer.o \
	\
	objs/dht_search.o \
	objs/dht_server.o \
	objs/dht_snatch.o

SRC_DEPS = \
	src/version.h \
	src/gude.h \
	src/noncopyable.h \
	src/endian_cpp.h \
	src/singleton.h \
	src/lock.h \
	src/Assert.h \
	src/thread.h \
	src/util.h \
	src/bencode.h \
	src/process.h \
	src/log.h \
	src/address.h \
	src/socket.h \
	src/fiber.h \
	src/scheduler.h \
	src/fdregister.h \
	src/timer.h \
	src/iocontext.h \
	src/iomanager.h \
	src/hook.h \
	src/environment.h \
	src/Iconv.h \
	src/config.h \
	src/worker.h \
	src/udpserver.h \
	src/server.h \
	\
	src/json/json.h \
	\
	src/dht/dht_search.h \
	src/dht/dht_server.h \
	src/dht/dht_snatch.h

SRC_INCS = \
	-I src \
	-I src/json \
	-I src/dht \
	-L libs \
	-L /usr/lib64 \
	-Wl,--copy-dt-needed-entries \
	-Wl,-rpath=libs

all: gude

#生成动态链接库
shared:$(SRC_OBJS)
	$(CPP) $(CFLAGS) $(SRC_INCS) $(CLIBS) -shared -o $(DYNAMIC_PATH) $^

#生成爬虫可执行文件
gude:objs/gude.o shared $(SRC_DEPS)
	$(CPP) $(CFLAGS) $(SRC_INCS) -lgude -o $@ $<

#生成源文件的各个.o文件
objs/%.o:src/%.cpp $(SRC_DEPS)
	$(CPP) -c $(CFLAGS) $(SRC_INCS) $< -o $@

#生成JSON库的.o文件
objs/%.o:src/json/%.cpp $(SRC_DEPS) 
	$(CPP) -c $(CFLAGS) $(SRC_INCS) $< -o $@

#生成DHT库的.o文件
objs/%.o:src/dht/%.cpp $(SRC_DEPS) 
	$(CPP) -c $(CFLAGS) $(SRC_INCS) $< -o $@

clean:
	rm -rf *.o objs/ logs/ libs/ gude

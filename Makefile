.PHONY:all clean

#生成torrent文件存放目录
$(shell test -d torrent || mkdir torrent)
#生成目标文件存放目录
$(shell test -d objs || mkdir objs)
#生成日志文件存放目录
$(shell test -d logs || mkdir logs)
#生成动态库文件存放目录
$(shell test -d libs || mkdir libs)

CPP = g++
CFLAGS = -std=c++11 -pipe -O1 -W -fPIC
#-g
LTCMALLOC = -ltcmalloc_minimal
DYNAMIC_PATH = libs/libbifang.so

SRC_OBJS = \
	objs/dht_download.o \
	objs/dht_crawler.o \
	objs/dht_server.o

SRC_DEPS = \
	../bifang/src/version.h \
	../bifang/src/bifang.h \
	../bifang/src/noncopyable.h \
	../bifang/src/endian_cpp.h \
	../bifang/src/singleton.h \
	../bifang/src/lock.h \
	../bifang/src/Assert.h \
	../bifang/src/thread.h \
	../bifang/src/util.h \
	../bifang/src/bencode.h \
	../bifang/src/process.h \
	../bifang/src/log.h \
	../bifang/src/address.h \
	../bifang/src/socket.h \
	../bifang/src/buffer.h \
	../bifang/src/fiber.h \
	../bifang/src/scheduler.h \
	../bifang/src/fdregister.h \
	../bifang/src/timer.h \
	../bifang/src/iocontext.h \
	../bifang/src/iomanager.h \
	../bifang/src/hook.h \
	../bifang/src/environment.h \
	../bifang/src/Iconv.h \
	../bifang/src/stream.h \
	../bifang/src/config.h \
	../bifang/src/worker.h \
	../bifang/src/uri.h \
	../bifang/src/module.h \
	../bifang/src/authorization.h \
	../bifang/src/tcpserver.h \
	../bifang/src/udpserver.h \
	../bifang/src/server.h \
	\
	../bifang/src/json/json.h \
	\
	../bifang/src/stream/socket_stream.h \
	../bifang/src/stream/zlib_stream.h \
	\
	../bifang/src/http/http_common.h \
	../bifang/src/http/http_server_parse.h \
	../bifang/src/http/http_client_parse.h \
	../bifang/src/http/http_parse.h \
	../bifang/src/http/http_session.h \
	../bifang/src/http/http_client.h \
	../bifang/src/http/servlet.h \
	../bifang/src/http/http_server.h \
	\
	../bifang/src/ws/ws_session.h \
	../bifang/src/ws/ws_client.h \
	../bifang/src/ws/ws_servlet.h \
	../bifang/src/ws/ws_server.h \
	\
	../bifang/src/sql/mysql.h \
	../bifang/src/sql/redis.h \
	\
	src/dht_config.h \
	src/dht_server.h \
	src/dht_download.h \
	src/dht_crawler.h

SRC_INCS = \
	-I src \
	-I ../bifang/src \
	-I ../bifang/src/json \
	-I ../bifang/src/stream \
	-I ../bifang/src/http \
	-I ../bifang/src/ws \
	-I ../bifang/src/sql \
	-L ../bifang/libs \
	-L /usr/lib64/mysql \
	-L /usr/lib64 \
	-Wl,--copy-dt-needed-entries \
	-Wl,-rpath=../bifang/libs

#生成爬虫可执行文件
gude:$(SRC_OBJS) objs/gude.o
	$(CPP) $(CFLAGS) $(SRC_INCS) -lbifang $(LTCMALLOC) -o $@ $^

#生成源文件的各个.o文件
objs/%.o:src/%.cpp $(SRC_DEPS)
	$(CPP) -c $(CFLAGS) $(SRC_INCS) $< -o $@

clean:
	rm -rf *.o objs/ logs/ libs/ gude

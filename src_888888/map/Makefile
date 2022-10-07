
COMMON_H = $(shell ls ../common/*.hpp)
COMMON_AR = ../common/obj/common.a
COMMON_INCLUDE = -I../common/

LIBCONFIG_H = $(shell ls ../../3rdparty/libconfig/*.h)
LIBCONFIG_AR = ../../3rdparty/libconfig/obj/libconfig.a
LIBCONFIG_INCLUDE = -I../../3rdparty/libconfig

RAPIDYAML_OBJ = $(shell find ../../3rdparty/rapidyaml/ -type f -name "*.cpp" | sed -e "s/\.cpp/\.o/g" )
RAPIDYAML_DIR_OBJ = $(RAPIDYAML_OBJ:%=obj/%)
RAPIDYAML_AR = ../../3rdparty/rapidyaml/obj/ryml.a
RAPIDYAML_H = $(shell find ../../3rdparty/rapidyaml/ -type f -name "*.h*")
RAPIDYAML_INCLUDE = -I../../3rdparty/rapidyaml/src -I../../3rdparty/rapidyaml/ext/c4core/src

MAP_OBJ = $(shell ls *.cpp | sed -e "s/\.cpp/\.o/g")
#MAP_OBJ += $(shell ls *.c | sed -e "s/\.c/\.o/g")
MAP_DIR_OBJ = $(MAP_OBJ:%=obj/%)
MAP_H = $(shell ls ../map/*.hpp) \
	$(shell ls ../config/*.hpp) 

HAVE_MYSQL=yes
ifeq ($(HAVE_MYSQL),yes)
	SERVER_DEPENDS=map-server
else
	SERVER_DEPENDS=needs_mysql
endif
ALL_DEPENDS=server

HAVE_PCRE=yes
ifeq ($(HAVE_PCRE),yes)
	PCRE_CFLAGS=-DPCRE_SUPPORT 
else
	PCRE_CFLAGS=
endif



#####################################################################
.PHONY : all server clean help

all: $(ALL_DEPENDS)

server: $(SERVER_DEPENDS)

clean:
	@echo "	CLEAN	map"
	@rm -rf *.o obj ../../map-server

help:
	@echo "possible targets are 'server' 'all' 'clean' 'help'"
	@echo "'server' - map server"
	@echo "'all'    - builds all above targets"
	@echo "'clean'  - cleans builds and objects"
	@echo "'help'   - outputs this message"

#####################################################################

needs_mysql:
	@echo "MySQL not found or disabled by the configure script"
	@exit 1

# object directories

obj:
	@echo "	MKDIR	obj"
	@-mkdir obj

# executables

map-server: obj $(MAP_DIR_OBJ) $(COMMON_AR) $(LIBCONFIG_AR) $(RAPIDYAML_AR)
	@echo "	LD	map-server"
	@g++  -L/usr/lib -o ../../map-server $(MAP_DIR_OBJ) $(COMMON_AR) $(LIBCONFIG_AR) $(RAPIDYAML_AR) -lpcre -lpthread -lz  -ldl -lpcre -L/usr/lib64/mysql -lmysqlclient -lpthread -lm -lrt -lssl -lcrypto -ldl


# map object files
#cause this one failling otherwise
obj/npc.o: npc.cpp $(MAP_H) $(COMMON_H) $(LIBCONFIG_H) $(RAPIDYAML_H)
	@echo "	CXX	$< (custom rule)"
	@g++ -std=c++11  -I../common -DHAS_TLS -fno-strict-aliasing -DHAVE_SETRLIMIT -DHAVE_STRNLEN  -I/usr/include -DHAVE_MONOTONIC_CLOCK $(COMMON_INCLUDE) $(LIBCONFIG_INCLUDE) $(PCRE_CFLAGS) $(RAPIDYAML_INCLUDE) -I/usr/include/mysql  -I../common -DHAS_TLS -fno-strict-aliasing -DHAVE_SETRLIMIT -DHAVE_STRNLEN  -I/usr/include -DHAVE_MONOTONIC_CLOCK -c $(OUTPUT_OPTION) $<
	
obj/%.o: %.cpp $(MAP_H) $(COMMON_H) $(LIBCONFIG_H) $(RAPIDYAML_H)
	@echo "	CXX	$<"
	@g++ -g -O2 -std=c++11 $(COMMON_INCLUDE) $(LIBCONFIG_INCLUDE) $(PCRE_CFLAGS) $(RAPIDYAML_INCLUDE) -I/usr/include/mysql  -I../common -DHAS_TLS -fno-strict-aliasing -DHAVE_SETRLIMIT -DHAVE_STRNLEN  -I/usr/include -DHAVE_MONOTONIC_CLOCK -c $(OUTPUT_OPTION) $<

# missing object files
$(COMMON_AR):
	@$(MAKE) -C ../common server

$(LIBCONFIG_AR):
	@$(MAKE) -C ../../3rdparty/libconfig

$(RAPIDYAML_AR):
	@$(MAKE) -C ../../3rdparty/rapidyaml

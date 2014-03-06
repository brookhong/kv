OBJDIR    =   obj
C_FILES := $(wildcard *.c)
CPP_FILES := $(wildcard *.cpp)
#OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.obj))) $(addprefix obj/,$(notdir $(C_FILES:.c=.obj)))

ifdef SYSTEMROOT
	RM = del /Q
	CC = cl
	LIBS = user32.lib shell32.lib
	OBJSUF = .obj
	LINK_FLAGS = /Fe$(OBJDIR)/kv
	CFLAGS = /Fo$@
	C_FILES := $(wildcard *.c) $(wildcard msw/*.c)
	CPP_FILES := $(wildcard *.cpp) $(wildcard msw/*.cpp)
else
	RM = rm -f
	CC = g++
	CFLAGS = -DHAVE_MMAP -o $@
	#OBJ_FILES := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)
	OBJSUF = .o
	LINK_FLAGS = -o $(OBJDIR)/kv
endif

OBJ_FILES := $(addprefix $(OBJDIR)/,$(CPP_FILES:.cpp=$(OBJSUF))) $(addprefix $(OBJDIR)/,$(C_FILES:.c=$(OBJSUF)))

all: $(OBJDIR) kv.exe

$(OBJDIR):
	mkdir -p $(OBJDIR)/msw

kv.exe: $(OBJ_FILES)
	$(CC) $(LINK_FLAGS) $^ $(LIBS)

$(OBJDIR)/%$(OBJSUF): %.cpp
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

$(OBJDIR)/%$(OBJSUF): %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

clean:
	rm -rf $(OBJDIR)/

test:
	echo $(OBJ_FILES)

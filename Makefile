cc		:=	g++
cflags		:=	-Wall -Wextra -std=c++17 -fPIC -c
lflags		:=	-Wall -Wextra -std=c++17 -shared

src		:=	src
lib		:=	lib
inc		:=	inc
bin		:=	bin

out		:=	engine.so

libraries	:=	-lSDL2 -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lSDL2_mixer

sources		:= 	$(shell find $(src) -name "*.cpp")
objects		:=	$(sources:.cpp=.o)

release: cflags += -DNDEBUG -O2
release: lflags += -DNDEBUG -O2
release: $(bin)/$(out)
debug: cflags += -g
debug: lflags += -g
debug: $(bin)/$(out)

all:	$(bin)/$(out)

%.o : %.cpp
	$(cc) -c $< -o $@ $(cflags)

.PHONY:	clean
clean:
	-$(RM) $(bin)/$(out)
	-$(RM) $(objects)

spritemaker:
	$(cc) -g $(cflags) $(src)/spritemaker/main.c $(src)/spritemaker/spritemaker.c -o $(bin)/spritemaker

run:	all
	./$(bin)/$(out)

$(bin)/$(out):	$(objects)
	$(cc) $(lflags) $^ -o $@ $(libraries)


list: $(shell find src -name "*.cpp")
	echo $^

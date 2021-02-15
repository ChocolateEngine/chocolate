cc		:=	g++
cflags		:=	-Wall -Wextra -g

src		:=	src
lib		:=	lib
inc		:=	inc
bin		:=	bin

out		:=	game

libraries	:=	-lSDL2

sources		:= 	$(shell find $(src) -name "*.cpp")
objects		:=	$(sources:.cpp=.o)

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
	$(cc) $(cflags) $^ -o $@ $(libraries)


list: $(shell find src -name "*.cpp")
	echo $^

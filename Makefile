
default: compiler
all: compiler
compiler: radian-compile

include srctree.mk

SUFFIXES:=c cpp
CXXFLAGS+=-std=c++11 -MD -MP -Werror

-include $(call findtype, $(SUFFIXES), obj/compiler)

radian-compile: $(call objs, $(SUFFIXES), compiler, obj/compiler)
	g++ -o $@ $^

obj/compiler/%.o: compiler/%.c Makefile
	@mkdir -p $(@D)
	-$(CC) -Icompiler $(CXXFLAGS) -c $< -o $@

obj/compiler/%.o: compiler/%.cpp Makefile
	@mkdir -p $(@D)
	-$(CC) -Icompiler $(CXXFLAGS) -c $< -o $@

clean:
	-@rm -f radian
	-@rm -rf $(call findtype, o d, obj)

.PHONY: all clean


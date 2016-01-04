
default: compiler
all: compiler
compiler: radian

include srctree.mk

SUFFIXES:=c cpp
CXXFLAGS+=-Icompiler -std=c++11 -MD -MP -Werror

-include $(call findtype, $(SUFFIXES), obj/compiler)

radian: $(call objs, $(SUFFIXES), compiler, obj/compiler)
	g++ -o $@ $^

obj/compiler/%.o: compiler/%.c Makefile
	@mkdir -p $(@D)
	-$(CC) $(CXXFLAGS) -c $< -o $@

obj/compiler/%.o: compiler/%.cpp Makefile
	@mkdir -p $(@D)
	-$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	-@rm -f radian
	-@rm -rf $(call findtype, o d, obj)

.PHONY: all clean


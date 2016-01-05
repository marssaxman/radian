
default: compiler
all: compiler
compiler: radian-compile

include srctree.mk
-include $(call findtype, d, obj)

CXXFLAGS+=-std=c++11 -MD -MP -Werror

COMPILER_OBJS:=$(call objs, cpp, compiler, obj/compiler)
radian-compile: $(COMPILER_OBJS) libyaml-cpp.a
	g++ -o $@ $^
obj/compiler/%.o: compiler/%.cpp Makefile
	@mkdir -p $(@D)
	-$(CC) -Icompiler -Iyaml-cpp/include $(CXXFLAGS) -c $< -o $@

LIBYAML_OBJS:=$(call objs, cpp, yaml-cpp/src, obj/yaml-cpp)
libyaml-cpp.a: $(LIBYAML_OBJS)
	ar rcs $@ $^
obj/yaml-cpp/%.o: yaml-cpp/src/%.cpp Makefile
	@mkdir -p $(@D)
	-$(CC) -Iyaml-cpp/src -Iyaml-cpp/include $(CXXFLAGS) -c $< -o $@

clean:
	-@rm -f radian libyaml-cpp.a
	-@rm -rf $(call findtype, o d, obj)

.PHONY: all clean


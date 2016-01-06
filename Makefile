
default: compiler
all: compiler linker
compiler: radian-compile
linker: radian-link

include srctree.mk
-include $(call findtype, d, obj)

CXXFLAGS+=-std=c++11 -MD -MP -Werror

COMPILER_OBJS:=$(call objs, cpp, compiler, obj/compiler)
radian-compile: $(COMPILER_OBJS) obj/libyaml-cpp.a
	g++ -o $@ $^
obj/compiler/%.o: compiler/%.cpp
	@mkdir -p $(@D)
	$(CC) -Icompiler -Iyaml-cpp/include $(CXXFLAGS) -c $< -o $@

LIBYAML_OBJS:=$(call objs, cpp, yaml-cpp/src, obj/yaml-cpp)
obj/libyaml-cpp.a: $(LIBYAML_OBJS)
	ar rcs $@ $^
obj/yaml-cpp/%.o: yaml-cpp/src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CC) -Iyaml-cpp/src -Iyaml-cpp/include $(CXXFLAGS) -c $< -o $@

ASMJIT_JOBS:=$(call objs, cpp, asmjit, obj/asmjit)
obj/libasmjit.a: $(ASMJIT_OBJS)
	ar rcs $@ $^
obj/asmjit/%.o: asmjit/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CXXFLAGS) -c $< -o $@

LINKER_OBJS:=$(call objs, cpp, linker, obj/linker)
radian-link: $(LINKER_OBJS) obj/libyaml-cpp.a obj/libasmjit.a
	g++ -o $@ $^
obj/linker/%.o: linker/%.cpp
	@mkdir -p $(@D)
	$(CC) -Ilinker -Iyaml-cpp/include -Iasmjit $(CXXFLAGS) -c $< -o $@

clean:
	-@rm -f radian-compile radian-link
	-@rm -rf $(call findtype, o d a, obj)

.PHONY: all clean


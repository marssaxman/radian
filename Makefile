
default: all
all: compiler linker
compiler: bin/compile
linker: bin/link

include srctree.mk
-include $(call findtype, d, obj)

CXXFLAGS+=-std=c++11 -MD -MP -Werror

COMPILER_OBJS:=$(call objs, cpp, compiler, obj/compiler)
bin/compile: $(COMPILER_OBJS)
	@mkdir -p $(@D)
	g++ -o $@ $^
obj/compiler/%.o: compiler/%.cpp
	@mkdir -p $(@D)
	$(CC) -Icompiler $(CXXFLAGS) -c $< -o $@

LINKER_OBJS:=$(call objs, cpp, linker, obj/linker)
bin/link: $(LINKER_OBJS) asmjit/libasmjit.a
	@mkdir -p $(@D)
	g++ -o $@ $^
obj/linker/%.o: linker/%.cpp
	@mkdir -p $(@D)
	$(CC) -Ilinker -Icpptoml/include -Iasmjit $(CXXFLAGS) -c $< -o $@

asmjit/libasmjit.a:
	$(MAKE) -C asmjit

clean:
	-@rm -rf bin obj
	$(MAKE) -C asmjit clean

.PHONY: all clean



default: all
all: compiler linker essay
compiler: bin/compile
linker: bin/link
essay: bin/essay

include srctree.mk
-include $(call findtype, d, obj)

CXXFLAGS+=-std=c++11 -MD -MP -Werror -g -fvisibility=hidden

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
	$(CC) -Ilinker -Iasmjit $(CXXFLAGS) -c $< -o $@

asmjit/libasmjit.a:
	$(MAKE) -C asmjit


ESSAY_OBJS:=$(call objs, cpp, essay, obj/essay)
bin/essay: $(ESSAY_OBJS)
	@mkdir -p $(@D)
	g++ -o $@ $^
obj/essay/%.o: essay/%.cpp
	@mkdir -p $(@D)
	$(CC) -Iessay $(CXXFLAGS) -c $< -o $@


clean:
	-@rm -rf bin obj
	$(MAKE) -C asmjit clean

.PHONY: all clean


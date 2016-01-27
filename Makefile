
default: all
all: radian

include srctree.mk
-include $(call findtype, d, obj)

CXXFLAGS+=-std=c++11 -MD -MP -Werror -g -fvisibility=hidden

COMPILER_OBJS:=$(call objs, cpp, src, obj)
radian: $(COMPILER_OBJS) asmjit/libasmjit.a
	@mkdir -p $(@D)
	g++ -o $@ $^
obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) -Isrc -Iasmjit $(CXXFLAGS) -c $< -o $@

asmjit/libasmjit.a:
	$(MAKE) -C asmjit

clean:
	-@rm -rf radian obj
	$(MAKE) -C asmjit clean

.PHONY: all clean


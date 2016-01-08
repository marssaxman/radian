
default: libasmjit.a

include ../srctree.mk
-include $(call findtype, d, obj)

CXXFLAGS+=-std=c++11 -MD -MP -Werror
CXXFLAGS+="-DASMJIT_CONFIG_FILE=\"config.h\""

libasmjit.a: $(call objs, cpp, $(CURDIR), obj)
	ar rcs $@ $^

obj/%.o: %.cpp
	@mkdir -p $(@D)
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	-@rm -f libasmjit.a
	-@rm -rf $(call findtype, o d a, obj)

.PHONY: clean


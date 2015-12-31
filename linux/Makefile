# make is not worth the hassle, so we'll do everything inside bash scripts.
# it has several annoying habits. one of them is that if it happens to find a
# directory named "obj", it will cd into that directory before making anything.
# another is that if there happens to be a directory with the name of a target,
# make will refuse to build that target, claiming that it is up to date. grrr.

COMPILERDIRS=./compiler-src ../compiler
RUNTIMEDIRS=./runtime-src ../runtime

all: runtime compiler

compiler:
	@./build-compiler.sh radian ./compiler-obj $(COMPILERDIRS)
	cp radian ../

runtime:
	@./build-lib.sh libstdradian.a ./runtime-obj $(RUNTIMEDIRS)

clean:
	@rm -f radian libstdradian.a ./compiler-obj/* ./runtime-obj/*

install:
	@./install.sh

check:
	@../validation/check.sh ./radian ../validation/suite/














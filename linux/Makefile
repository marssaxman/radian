# make is not worth the hassle, so we'll do everything inside bash scripts.
# it has several annoying habits. one of them is that if it happens to find a
# directory named "obj", it will cd into that directory before making anything.
# another is that if there happens to be a directory with the name of a target,
# make will refuse to build that target, claiming that it is up to date. grrr.

RUNTIMEDIRS=./runtime-src ../runtime

all: runtime

runtime:
	@./build-lib.sh libstdradian.a ./runtime-obj $(RUNTIMEDIRS)

clean:
	@rm -f libstdradian.a

install:
	@./install.sh














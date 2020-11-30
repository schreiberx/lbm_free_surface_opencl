all:	release

debug:
	scons --mode=debug

release:
	scons --mode=release

clean:
	rm -f build/lbm_opencl_*
	rm -rf build/build_*

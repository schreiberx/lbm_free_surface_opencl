#! /bin/sh



# go to root source folder to load shaders
cd ../

#BENCHMARK_NAME="AMD_FirePro_W8000"
BENCHMARK_NAME="GeForce_GTX_470"

LOOPS=1000
#LOOPS=1

# 80 & 196 is broken
#TEST_KERNELS="32 64 80 128 196 256"
TEST_KERNELS="32 64 128 256"

TEST_DOMAIN_SIZES="32 40 48 56 64 80 96 112 128 144 160"

OUTFILE="benchmarks/benchmark_fs_""$BENCHMARK_NAME"".dat"
OUTFILEFPS="benchmarks/benchmark_fs_ssps_""$BENCHMARK_NAME"".dat"

BIN="./build/lbm_opencl_fs_intel_release"

echo -n "Domainsize" > $OUTFILE;
echo -n "Domainsize" > $OUTFILEFPS;
for k in $TEST_KERNELS; do
	echo -n "	$k" >> $OUTFILE
	echo -n "	$k" >> $OUTFILEFPS
done
echo >> $OUTFILE
echo >> $OUTFILEFPS

for r in $TEST_DOMAIN_SIZES; do
	echo "Domainsize: $r"
	echo -n "$r^3" >> $OUTFILE
	echo -n "$r^3" >> $OUTFILEFPS
	for k in $TEST_KERNELS; do
		EXEC_="$BIN -X $r -n -c -k $k -l $LOOPS"
		echo $EXEC_
		OUTPUT=`$EXEC_`
		MLUPS=`echo -n "$OUTPUT" | grep "MLUPS" | sed "s/MLUPS: //"`
		FPS=`echo -n "$OUTPUT" | grep "FPS" | sed "s/FPS: //"`
		CHECKSUM=`echo -n "$OUTPUT" | grep "Checksum" | sed "s/Checksum: //"`
		test -z "$MLUPS" && MLUPS="-"
		test -z "$FPS" && FPS="-"
		test -z "$CHECKSUM" && CHECKSUM="-"
		echo "$r"x"$r"x"$r - $k kernels: $FPS ssps	$MLUPS mlups	$CHECKSUM checksum"
		echo -n "	$MLUPS" >> $OUTFILE
		echo -n "	$FPS" >> $OUTFILEFPS
	done
	echo >> $OUTFILE
	echo >> $OUTFILEFPS
done

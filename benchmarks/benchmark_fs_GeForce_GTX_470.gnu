reset

set terminal pdf
set output "benchmark_fs_GeForce_GTX_470.pdf"

set datafile missing "-"
set xtics nomirror rotate by -45

set xlabel	"Domain size"
set ylabel	"MLUPS"

set key outside title "Work group size"

set style data linespoints
set key autotitle columnhead

set yrange [0:]
plot 'benchmark_fs_GeForce_GTX_470.dat' using 2:xtic(1),\
	'' using 3,	\
	'' using 4,	\
	'' using 5



#set log y
#set output "benchmark_fs_ssps_GeForce_GTX_470.pdf"
#set ylabel	"Simulation steps per second"
#plot 'benchmark_fs_ssps_GeForce_GTX_470.dat' using 2:xtic(1),\
#	'' using 3,	\
#	'' using 4,	\
#	'' using 5

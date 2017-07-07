#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

# generate lab2b_1.png
set title "Lab2b_1 Plot"
set xlabel "Threads"
set logscale x 2
set ylabel "Total operations per second"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-m' lab2b_list.csv | grep '1000,1,'" using ($2):(1000000000/($7)) \
	title 'mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s' lab2b_list.csv | grep '1000,1,'" using ($2):(1000000000/($7)) \
	title 'spin-lock' with linespoints lc rgb 'green'

# generate lab2b_2.png
set title "Lab2b_2 Plot"
set xlabel "Threads"
set logscale x 2
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m' lab2b_list.csv | grep '1000,1,'" using ($2):($8) \
	title 'Wait-for-lock time' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m' lab2b_list.csv | grep '1000,1,'" using ($2):($7) \
	title 'Average time per operation' with linespoints lc rgb 'green'

# generate lab2b_3.png
set title "Lab2b_3 Plot"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
     "< grep 'list-id-none' lab2b_list.csv" using ($2):($3) \
	title 'No lock' with points lc rgb 'blue', \
	"< grep 'list-id-m' lab2b_list.csv" using ($2):($3) \
	title 'Mutex' with points lc rgb 'green', \
	 "< grep 'list-id-s' lab2b_list.csv" using ($2):($3) \
	title 'Spin-lock' with points lc rgb 'red'

# generate lab2b_4.png
set title "Lab2b_4 Plot"
set xlabel "Threads"
set logscale x 2
set ylabel "Total operations per second"
set logscale y 10
set output 'lab2b_4.png'

plot \
     "< grep 'list-none-m' lab2b_list.csv | grep '1000,1,'" using ($2):(1e9/($7)) \
	title 'List number : 1' with linespoints lc rgb 'blue', \
	"< grep 'list-none-m' lab2b_list.csv | grep '1000,4,'" using ($2):(1e9/($7)) \
	title 'List number : 4' with linespoints lc rgb 'green', \
     "< grep 'list-none-m' lab2b_list.csv | grep '1000,8,'" using ($2):(1e9/($7)) \
	title 'List number : 8' with linespoints lc rgb 'red', \
	"< grep 'list-none-m' lab2b_list.csv | grep '1000,16,'" using ($2):(1e9/($7)) \
	title 'List number : 16' with linespoints lc rgb 'yellow'

# generate lab2b_5.png
set title "Lab2b_5 Plot"
set xlabel "Threads"
set logscale x 2
set ylabel "Total operations per second"
set logscale y 10
set output 'lab2b_5.png'

plot \
     "< grep 'list-none-s' lab2b_list.csv | grep '1000,1,'" using ($2):(1e9/($7)) \
	title 'list number : 1' with linespoints lc rgb 'blue', \
	"< grep 'list-none-s' lab2b_list.csv | grep '1000,4,'" using ($2):(1e9/($7)) \
	title 'list number : 4' with linespoints lc rgb 'green', \
     "< grep 'list-none-s' lab2b_list.csv | grep '1000,8,'" using ($2):(1e9/($7)) \
	title 'list number : 8' with linespoints lc rgb 'red', \
	"< grep 'list-none-s' lab2b_list.csv | grep '1000,16,'" using ($2):(1e9/($7)) \
	title 'list number : 16' with linespoints lc rgb 'yellow'
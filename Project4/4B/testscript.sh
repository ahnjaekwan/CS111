#!/bin/bash

echo 'Unrecognizable argument:'
./lab4b --unrecog &> /dev/null
if [ $? -eq 1 ]
then
	echo "..........Passed!!!"
else
	echo "..........Failed!!!"
fi

let errors=0

echo 'Test correct arguments and STDIN input:'
./lab4b --scale=C --period=3 --log="log.txt" <<-EOF
STOP
PERIOD=2
START
SCALE=F
STOP
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "..........Failed with return value=$ret!!!"
	let errors+=1
fi

if [ ! -s log.txt ]
then
	echo "..........Failed: no log file!!!"
	let errors+=1
else
	echo "..........Correct log file!!!"
	for c in PERIOD=1 START SCALE=F STOP OFF SHUTDOWN
	do
		grep $c log.txt > /dev/null
		if [ $? -ne 0 ]
		then
			echo "..........There is no LOG $c command!!!"
			let errors+=1
		else
			echo "..........Passed with $c command!!!"
		fi
	done

	if [ $errors -gt 0 ]
	then
		echo "..........Failed with logging command!!!"
	fi
fi

echo "Test reporting format:"
egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9][0-9].[0-9]' log.txt > /dev/null
if [ $? -eq 0 ] 
then
	echo "..........Passed!!!"
else
	echo "..........No valid log file format!!!"
fi

rm log.txt

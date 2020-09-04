#!/bin/bash
for i in `seq $2`
do
	answ=`host -t txt "d$i.txt.$1"|cut -d ' ' -f 4`
	echo ${answ:2:-1} | xxd -r -p - >> $3
	echo $i ${answ:2:-1}
done
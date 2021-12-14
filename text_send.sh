#!/bin/bash

winid=$(xwininfo | grep id | grep 'xwininfo:' | awk '{print $4}')
xdotool windowfocus $winid

#LANG=C IFS=
#while read -r -d '' -n 1 char
#do
#	echo -n "$char"
#	if [ "$char" = $'\x0a' ]
#	then xdotool key Return
#	else xdotool type "$char"
#	fi
#done

while read line
do
	echo $line
	xdotool type "$line"
	xdotool key Return
done

# apt install xdotool x11-utils
# setxkbmap us
# cat /tmp/test.txt | ./text_send.sh
# cat /tmp/test.bin | base64 | ./text_send.sh

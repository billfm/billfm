#!/bin/bash
tty -s || exec lxterminal -e "$0" "$@"
echo START
cd ~/billfm
git commit -a
git push origin master
echo OK
read -sn1 -p "Press any key to continue..."; echo
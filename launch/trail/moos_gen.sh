#!/bin/bash

tmpfile=/tmp/$1.moos
python3 trail_usv.pb.cfg.py moos > $tmpfile
trap "rm -f $tmpfile" EXIT

while true; do sleep 1; done

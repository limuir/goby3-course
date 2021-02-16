#!/bin/bash

# Generates temporary MOOS file and then deletes it after exiting
# This is required by MOOS applications can't handle process substitution: <(command)

if [ "$#" -lt 1 ]
then
   echo "goby3_course_n_auvs=10 goby3_course_auv_index=1 moos_gen.sh vehicle_type"
fi   

type=$1
if [[ "$type" == "auv" ]]; then
    moos_tmpfile=/tmp/${type}${goby3_course_auv_index}.moos
    bhv_tmpfile=/tmp/${type}${goby3_course_auv_index}.bhv
else
    moos_tmpfile=/tmp/${type}.moos
    bhv_tmpfile=/tmp/${type}.bhv
fi

echo -e "Generating $moos_tmpfile $bhv_tmpfile"
script_dir=$(dirname $0)
python3 ${script_dir}/${type}.pb.cfg.py moos > $moos_tmpfile
python3 ${script_dir}/${type}.pb.cfg.py bhv > $bhv_tmpfile
trap "echo -e \"Deleting $moos_tmpfile $bhv_tmpfile\"; rm -f $moos_tmpfile $bhv_tmpfile" EXIT

while true; do sleep 1; done


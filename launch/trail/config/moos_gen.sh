#!/bin/bash

# Generates temporary MOOS file and then deletes it after exiting
# This is required by MOOS applications can't handle process substitution: <(command)

if [ "$#" -lt 1 ]
then
   echo "goby3_course_n_auvs=10 goby3_course_auv_index=1 moos_gen.sh vehicle_type"
fi   

type=$1
tmpfile=/tmp/${type}${goby3_course_auv_index}.moos
echo -e "Generating $tmpfile"
script_dir=$(dirname $0)
python3 ${script_dir}/${type}.pb.cfg.py moos > $tmpfile
trap "echo -e \"Deleting $tmpfile\"; rm -f $tmpfile" EXIT

while true; do sleep 1; done


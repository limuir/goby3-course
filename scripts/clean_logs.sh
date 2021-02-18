#!/bin/bash

script_dir=$(dirname $0)

log_dir=$(echo "${script_dir}/../logs" | xargs readlink -f)

rm -rf $log_dir
git checkout $log_dir

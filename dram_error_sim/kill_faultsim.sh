#!/bin/bash

# Get the list of processes with name starting with "./faulterrorsim"
process_list=$(pgrep -f "^./faulterrorsim")

# Loop through the processes
for pid in $process_list; do
    # Get the elapsed time of the process in seconds
    elapsed_time=$(ps -o etimes= -p $pid)

    # If the elapsed time is greater than or equal to 180 seconds (3 minutes), kill the process
    if [ $elapsed_time -ge 1200 ]; then
        echo "Killing process $pid with elapsed time $elapsed_time seconds."
        kill $pid
    else
        echo "Process $pid has not yet been running for 3 minutes."
    fi
done


#!/bin/bash

iter=10
command=""

benhmark() {
    # status message
    s="benchmarking ${command} ..."

    # total time
    tot=0

    # go
    for (( i = 1; i <= $iter ; i++ ))
    do
        # run command
        clock=$(${command})
        # add to total
        tot=$(( $tot + $clock ))

        # print status line with percent
        p=$(( $i * 100 / $iter))
        echo -ne "${s} (${p}%) \r" 1>&2
    done;

    # end status line
    echo -ne '\n' 1>&2

    # calculate average
    avg=$(( $tot / $iter ))

    echo "mean: ${avg}"
}

# Option parsing
while getopts n:c: OPT
do
    case "$OPT" in
        n)
            iter=$OPTARG
            ;;
        c)
            command=$OPTARG
            benhmark
            ;;
        \?)
            echo 'no arguments given'
            exit 1
            ;;
    esac
done

shift `expr $OPTIND - 1`

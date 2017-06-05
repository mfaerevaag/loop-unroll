#!/bin/bash

iter=100
prog=""

benchmark() {
    # status message
    s="benchmarking '${prog}' ..."

    prog_base="${prog}-base"
    prog_opt="${prog}-opt"
    prog_best="${prog}-best"

    # TODO
    prog_cur=$prog_opt

    # expected result from all programs
    expected_result=$(./${prog_base}.out | ag -o 'result: [\d]+' | awk '{print $2}')

    # total time
    tot=0

    # go
    for (( i = 1; i <= $iter ; i++ ))
    do
        # run prog
        output=$(./${prog_cur}.out)

        # get result
        result=$(echo ${output} | ag -o 'result: [\d]+' | awk '{print $2}')
        if [ ! "$result" == "$expected_result" ]
        then
            echo -ne "\rexpected result '${expected_result}', but got '${result}'" 1>&2
            exit 1
        fi

        # get time
        time=$(echo ${output} | ag -o 'time: [\d]+' | awk '{print $2}')
        # add to total
        tot=$(( $tot + $time ))

        # print status line with percent
        p=$(( $i * 100 / $iter))
        echo -ne "${s} ${i}/${iter} (${p}%) \r" 1>&2
    done;

    # end status line
    echo -ne '\n' 1>&2

    # calculate average
    avg=$(( $tot / $iter ))

    # calculate code size
    loc=$(wc -l < ${prog_cur}.s)

    echo "prog,mean,loc"
    echo "${prog},${avg},${loc}"
}

# Option parsing
while getopts n:p: OPT
do
    case "$OPT" in
        n)
            iter=$OPTARG
            ;;
        p)
            prog=$OPTARG
            benchmark
            ;;
        \?)
            echo 'no arguments given'
            exit 1
            ;;
    esac
done

shift `expr $OPTIND - 1`

#!/bin/bash

iter=100
count=100
prog=""

benchmark() {
    prog_base="${prog}-base"
    prog_opt="${prog}-opt"
    prog_best="${prog}-best"

    declare -a prog_all=("${prog_base}" "${prog_opt}" "${prog_best}")
    # declare -a prog_all=("${prog_best}")

    # make
    if ! err=$(make cleanprog prog PROG=${prog} PASSCOUNT=${count} 2>&1)
    then
        echo "initial make failed" 1>&2
        echo $err 1>&2
        exit 1
    fi

    # expected result from all programs
    expected_result=$(./${prog_base}.out | ag -o 'result: [\d]+' | awk '{print $2}')

    # total time
    tot=0

    # for each program optimization
    prog_i=1
    for prog_cur in "${prog_all[@]}"
    do

        # set and clear logfile
        logfile="${prog_cur}.csv"
        echo -ne "" > $logfile

        # for each unroll count
        for (( c = 1; c <= $count ; c++ ))
        do

            # make
            if ! err=$(make ${prog_cur}.out PROG=${prog} PASSCOUNT=${c} 2>&1)
            then
                # echo "make failed for count ${c}" 1>&2
                # echo $err 1>&2
                continue;
            fi

            output=""
            tot=0

            # get mean running time
            for (( i = 1; i <= $iter ; i++ ))
            do
                # check if base
                # TODO

                # run prog
                output=$(./${prog_cur}.out)

                # get time
                time=$(echo ${output} | ag -o 'time: [\d]+' | awk '{print $2}')
                # add to total
                tot=$(( $tot + $time ))

                # print status line with percent
                # p=$(( $i * 100 / $iter * $prog_count))
                s="benchmarking '${prog_cur}' ..."
                echo -ne "\r$(tput el)${s} i: $i / $iter count: $c / $count prog: ${prog_i} / ${#prog_all[@]}" 1>&2
            done

            # check for correct result
            result=$(echo ${output} | ag -o 'result: [\d]+' | awk '{print $2}')
            if [ ! "$result" == "$expected_result" ]
            then
                echo -ne "\rexpected result '${expected_result}', but got '${result}'" 1>&2
                exit 1
            fi

            # calculate average
            avg=$(( $tot / $iter ))

            # calculate code size
            loc=$(wc -l < ${prog_cur}.s)

            # write result
            # echo -ne "\n" 1>&2 # end status line
            echo "${c},${avg},${loc}" >> $logfile

        done

        prog_i=$(( prog_i + 1 ))
    done
}

# Option parsing
while getopts i:c:p: OPT
do
    case "$OPT" in
        i)
            iter=$OPTARG
            ;;
        c)
            count=$OPTARG
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

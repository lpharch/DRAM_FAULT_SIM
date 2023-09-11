#!/bin/bash

for error in NE DATA MDC SDC SMDC DE-PAR DE-FSM DE-ADDR DE-WCRC DE-RECC
do
    echo -ne "$error\t"
    for scn in a1 a2 aa w1 w2 wa r1 r2 ra p1 p2 pa
    do
#        for ecc in DDR3 DDR4 Azul AIECC DUMMY
        for ecc in AIECC
        do
            result=`cat cov.$scn.*.$ecc.* 2> /dev/null | tail -12 | grep "\<$error\>" | awk '{print \$2}'`
            echo -ne "$result\t"
        done
    done
    echo ""
done

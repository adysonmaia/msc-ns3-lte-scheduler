#!/bin/bash

path=`pwd`
simulator="time $path/waf --run='m2m-lte"

simTime=3
intTrigger=0.05
minCqi=0
maxCqi=5
nH2H=0
nRbM2M=3
nRbH2H=3
minPerRbM2M=1
bandwidth=15
m2mDelayWeight=0.72

for index in {0..9}
do
    paramsGeneral="--simTime=$simTime --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --bandwidth=$bandwidth --nH2H=$nH2H --nExec=$index --ns3::M2mMacScheduler::M2MDelayWeight=$m2mDelayWeight"
    for nM2M in 290 250 200 150 100 50
    do
        nM2MT=$(($nM2M / 3))
        nM2MR=$(($nM2M - $nM2MT))
        for scheduler in 0 3
        do

#            useClass=0
#            if [ $scheduler -eq 0 ]; then
#                useClass=1
#            fi

            for useClass in 1 0
            do
                if [ $useClass -eq 1 ] && [ $scheduler -eq 3 ]
                then
                    continue
                fi

                echo -e "Scheduler: $scheduler - H2H: $nH2H - M2M T: $nM2MT - M2M R: $nM2MR - useM2MQoSClass: $useClass - index: $index"
                params="--scheduler=$scheduler --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --useM2MQoSClass=$useClass $paramsGeneral"
                command="$simulator $params'"
                eval $command
           done
        done
    done
done

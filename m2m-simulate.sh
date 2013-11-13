#!/bin/bash

path=`pwd`
simulator="time $path/waf --run='m2m-lte"

simTime=3
intTrigger=0.05
minCqi=0
maxCqi=5
nH2H=30
nRbM2M=3
nRbH2H=3
minPerRbM2M=0.48
m2mDelayWeight=0.72

for index in {0..9}
do
    paramsGeneral="--simTime=$simTime --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --nExec=$index"
    for nM2M in 0 50 100 150 200 250
    do
        nM2MT=$(($nM2M / 3))
        nM2MR=$(($nM2M - $nM2MT))
        scheduler=0
        for useClass in 1 0
        do
            echo -e "Scheduler: $scheduler - H2H: $nH2H - M2M T: $nM2MT - M2M R: $nM2MR - useM2MQoSClass: $useClass - index: $index"
            params="--scheduler=$scheduler --nH2H=$nH2H --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --useM2MQoSClass=$useClass --ns3::M2mMacScheduler::M2MDelayWeight=$m2mDelayWeight $paramsGeneral"
            command="$simulator $params'"
            eval $command
        done
        useClass=0
        for scheduler in {1..3}
        do
            echo -e "Scheduler: $scheduler - H2H: $nH2H - M2M T: $nM2MT - M2M R: $nM2MR - useM2MQoSClass: $useClass - index: $index"
            params="--scheduler=$scheduler --nH2H=$nH2H --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --useM2MQoSClass=$useClass $paramsGeneral"
            command="$simulator $params'"
            eval $command
        done
    done
done

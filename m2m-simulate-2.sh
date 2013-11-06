#!/bin/bash

path=`pwd`
simulator="time $path/waf --run='m2m-lte"

simTime=3
intTrigger=0.05
minCqi=0
maxCqi=5
nH2H=30
nM2MT=76
nM2MR=154
nRbH2H=3
bandwidth=25
scheduler=0
useClass=1

nRbM2M=3
minPerRbM2M=0.48

for index in {0..9}
do
    paramsGeneral="--simTime=$simTime --nH2H=$nH2H --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --nExec=$index"
    for delayWeight in 1 0 0.8 0.5 0.7 0.3 0.9
    do
        paramsGeneral="$paramsGeneral --ns3::M2mMacScheduler::M2MDelayWeight=$delayWeight"
        for nRbM2M in {3..5}
        do
            for nMinM2M in 2 3 4
            do
                minPerRbM2M=`python -c "print $nRbM2M*$nMinM2M/$bandwidth.0"`
#for useClass in 1 0
#do
                    fileSuffix="$delayWeight-$nRbM2M-$minPerRbM2M"
                    echo -e "Scheduler: $scheduler - delayWeight: $delayWeight - minRBPerM2M: $nRbM2M - minPercentRBForM2M: $minPerRbM2M - useM2MQoSClass: $useClass - index: $index"
                    params="--scheduler=$scheduler --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --useM2MQoSClass=$useClass --suffixStatsFile=$fileSuffix $paramsGeneral"
                    command="$simulator $params'"
                    eval $command
#done
            done
        done
    done
done

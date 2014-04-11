#!/bin/bash

path=`pwd`
simulator="time $path/waf --run='m2m-lte"

simTime=3
intTrigger=0.05
minCqi=0
maxCqi=6
nH2HVoIP=10
nH2HVideo=10
nH2HFTP=10
nM2MT=83
nM2MR=167
nRbH2H=3
bandwidth=25
scheduler=0
useClass=1
nMinM2M=4
nRbM2M=3
minPerRbM2M=0.48
nM2MT=83
nM2MR=167
delayWeight=0.95

#for index in {0..9}
for index in 0
do
    paramsGeneral="--simTime=$simTime --nH2HVoIP=$nH2HVoIP --nH2HVideo=$nH2HVideo --nH2HFTP=$nH2HFTP --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --nExec=$index"
    for delayWeight in 0.7 0.8 0.9 0.95 1
    do
        paramsGeneral="$paramsGeneral --ns3::M2mMacScheduler::M2MDelayWeight=$delayWeight --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR"
        for nMinM2M in 1 2
        do
            minPerRbM2M=`python -c "print $nRbM2M*$nMinM2M/$bandwidth.0"`
            fileSuffix="$delayWeight-$nRbM2M-$minPerRbM2M"
            params="--scheduler=$scheduler --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --useM2MQoSClass=$useClass --suffixStatsFile=$fileSuffix $paramsGeneral"
            command="$simulator $params'"
            echo -e "Scheduler: $scheduler - delayWeight: $delayWeight - minRBPerM2M: $nRbM2M - minPercentRBForM2M: $minPerRbM2M - useM2MQoSClass: $useClass - index: $index"
            eval $command
        done
    done
done

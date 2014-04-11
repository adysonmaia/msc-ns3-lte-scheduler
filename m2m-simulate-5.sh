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
#delayWeight=0.72
delayWeight=0.95
nRbM2M=3
minPerRbM2M=0.48

for index in {0..9}
#for index in {10..19}
#for index in {20..29}
#for index in {0..29}
#for index in {29..0}
do
    paramsGeneral="--simTime=$simTime --nH2HVoIP=$nH2HVoIP --nH2HVideo=$nH2HVideo --nH2HFTP=$nH2HFTP --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --nExec=$index --ns3::M2mMacScheduler::M2MDelayWeight=$delayWeight"

    for nM2M in 0 50 100 150 200 250
    do
        nM2MT=$(($nM2M / 3))
        nM2MR=$(($nM2M - $nM2MT))

#        for nMinM2M in 0 1 2 3 4 5 6 7 8
        for nMinM2M in 0 2 4 6 8
        do
            minPerRbM2M=`python -c "print $nRbM2M*$nMinM2M/$bandwidth.0"`
            fileSuffix="$delayWeight-$nRbM2M-$minPerRbM2M"
            echo -e "Scheduler: $scheduler - M2M T: $nM2MT - M2M R: $nM2MR - minRBPerM2M: $nRbM2M - minPercentRBForM2M: $minPerRbM2M - useM2MQoSClass: $useClass - index: $index"
            params="--scheduler=$scheduler --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --useM2MQoSClass=$useClass --suffixStatsFile=$fileSuffix $paramsGeneral"
            command="$simulator $params'"
#            echo -e "$command"
            eval $command
        done
    done
done

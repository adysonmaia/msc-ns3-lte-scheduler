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
scheduler=4
useClass=1
nMinM2M=4
nRbM2M=3
minPerRbM2M=0.48
nM2MT=83
nM2MR=167
delayWeight=0.95
minPerRbM2MLow=0.36
minPerRbM2MNormal=0.48
minPerRbM2MHigh=0.6

#for index in {0..9}
for index in {10..29}
do
    paramsGeneral="--simTime=$simTime --nH2HVoIP=$nH2HVoIP --nH2HVideo=$nH2HVideo --nH2HFTP=$nH2HFTP --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --nExec=$index --ns3::M2mMacScheduler::M2MDelayWeight=$delayWeight --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --minPercentRBForM2M=$minPerRbM2M --minRBPerM2M=$nRbM2M  --useM2MQoSClass=$useClass --ns3::M2mMacScheduler2::MinPercentRBForM2MLow=$minPerRbM2MLow --ns3::M2mMacScheduler2::MinPercentRBForM2MNormal=$minPerRbM2MNormal --ns3::M2mMacScheduler2::MinPercentRBForM2MHigh=$minPerRbM2MHigh"
    for congestionLow in 0.1 0.2 0.3 0.4 0.5
    do
        for congestionHigh in 0.5 0.6 0.7 0.8 0.9
        do
            paramsGeneral="$paramsGeneral --ns3::M2mMacScheduler2::CongestionLow=$congestionLow --ns3::M2mMacScheduler2::CongestionHigh=$congestionHigh"
            minPerRbM2M=`python -c "print $nRbM2M*$nMinM2M/$bandwidth.0"`
            fileSuffix="$congestionLow-$congestionHigh"
            params="--scheduler=$scheduler --suffixStatsFile=$fileSuffix $paramsGeneral"
            command="$simulator $params'"
            echo -e "Scheduler: $scheduler - congestionLow: $congestionLow - congestionHigh: $congestionHigh - index: $index"
            eval $command
        done
    done
done

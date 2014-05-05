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

for index in {0..29}
do
    paramsGeneral="--scheduler=$scheduler --simTime=$simTime --nH2HVoIP=$nH2HVoIP --nH2HVideo=$nH2HVideo --nH2HFTP=$nH2HFTP --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --ns3::M2mMacScheduler::M2MDelayWeight=$delayWeight --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --useM2MQoSClass=$useClass --nExec=$index"

    for delaySpread in 0 0.01 0.02 0.04 0.06 0.08 0.1 0.2 0.4 0.6 0.8 1
    do
        fileSuffix="$delaySpread"
        params="--ns3::M2mMacScheduler::M2MDeniedSpread=$delaySpread --suffixStatsFile=$fileSuffix $paramsGeneral"
        command="$simulator $params'"
        echo -e "Scheduler: $scheduler - delaySpread: $delaySpread - index: $index"
        eval $command
    done
done

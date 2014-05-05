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
nRbM2M=3
nRbH2H=3
minPerRbM2M=0.48
m2mDelayWeight=0.95
useClass=0
scheduler=5

#for index in {0..9}
#for index in {10..19}
#for index in {20..29}
for index in {0..29}
#for index in 0
do
    paramsGeneral="--simTime=$simTime --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nH2HVoIP=$nH2HVoIP --nH2HVideo=$nH2HVideo --nH2HFTP=$nH2HFTP --nExec=$index --scheduler=$scheduler --useM2MQoSClass=$useClass "
    for nM2M in 250 200 150 100 50 0
#    for nM2M in 250
    do
        nM2MT=$(($nM2M / 3))
        nM2MR=$(($nM2M - $nM2MT))
        paramsGeneral="$paramsGeneral --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR"

#        for minPerRbM2M in 0.24 0.36 0.6 0.72
        for minPerRbM2M in 0.2 0.4
        do
            fileSuffix="$minPerRbM2M"
            echo -e "Scheduler: $scheduler - H2H VoIP: $nH2HVoIP - H2H Video: $nH2HVideo - H2H FTP: $nH2HFTP - M2M T: $nM2MT - M2M R: $nM2MR - minPerRbM2M: $minPerRbM2M - index: $index"
            params="--minRBPerH2H=$nRbH2H --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --suffixStatsFile=$fileSuffix $paramsGeneral"
            command="$simulator $params'"
    #            echo -e "$params"
            eval $command
        done
    done
done

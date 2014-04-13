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

#for index in {0..9}
#for index in {10..19}
#for index in {20..29}
for index in {0..29}
#for index in 0
do
    paramsGeneral="--simTime=$simTime --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --minRBPerH2H=$nRbH2H --minRBPerM2M=$nRbM2M --minPercentRBForM2M=$minPerRbM2M --nH2HVoIP=$nH2HVoIP --nH2HVideo=$nH2HVideo --nH2HFTP=$nH2HFTP --nExec=$index"
    for nM2M in 250 200 150 100 50 0
    do
        nM2MT=$(($nM2M / 3))
        nM2MR=$(($nM2M - $nM2MT))
        paramsGeneral="$paramsGeneral --nM2MTrigger=$nM2MT --nM2MRegular=$nM2MR"

        scheduler=4
        useClass=1
        congestionLow=0.2
        congestionHigh=0.6
        minPerRbM2MLow=0.36
        minPerRbM2MNormal=0.48
        minPerRbM2MHigh=0.6
#        for useClass in 1 0
#        do
            echo -e "Scheduler: $scheduler - H2H VoIP: $nH2HVoIP - H2H Video: $nH2HVideo - H2H FTP: $nH2HFTP - M2M T: $nM2MT - M2M R: $nM2MR - useM2MQoSClass: $useClass - index: $index"
            params="--scheduler=$scheduler --useM2MQoSClass=$useClass --ns3::M2mMacScheduler::M2MDelayWeight=$m2mDelayWeight --ns3::M2mMacScheduler2::CongestionLow=$congestionLow --ns3::M2mMacScheduler2::CongestionLow=$congestionHigh --ns3::M2mMacScheduler2::MinPercentRBForM2MLow=$minPerRbM2MLow --ns3::M2mMacScheduler2::MinPercentRBForM2MNormal=$minPerRbM2MNormal --ns3::M2mMacScheduler2::MinPercentRBForM2MHigh=$minPerRbM2MHigh $paramsGeneral"
            command="$simulator $params'"
            eval $command
#       done

#        useClass=0
#        scheduler=5
#        for scheduler in {1..3}
#        do
#            echo -e "Scheduler: $scheduler - H2H VoIP: $nH2HVoIP - H2H Video: $nH2HVideo - H2H FTP: $nH2HFTP - M2M T: $nM2MT - M2M R: $nM2MR - useM2MQoSClass: $useClass - index: $index"
#            params="--scheduler=$scheduler --useM2MQoSClass=$useClass $paramsGeneral"
#            command="$simulator $params'"
#            eval $command
#        done
    done
done

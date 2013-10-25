#!/bin/bash

path=`pwd`
simulator="$path/waf --run='m2m-lte"

simTime=2
intTrigger=0.5
minCqi=0
maxCqi=11

for index in {0..5}
do
    # Scheduler: 0 - H2H: 20 - M2M T: 0  - M2M R: 0
    echo -e "Scheduler: 0 - H2H: 20 - M2M T: 0  - M2M R: 0 - index: $index"
    params="--scheduler=0 --simTime=$simTime --nH2H=20 --nM2MTrigger=0 --nM2MRegular=0 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 1 - H2H: 20 - M2M T: 0  - M2M R: 0
    echo -e "Scheduler: 1 - H2H: 20 - M2M T: 0  - M2M R: 0 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=0 --nM2MRegular=0 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 2 - H2H: 20 - M2M T: 0  - M2M R: 0
    echo -e "Scheduler: 2 - H2H: 20 - M2M T: 0  - M2M R: 0 - index: $index"
    params="--scheduler=2 --simTime=$simTime --nH2H=20 --nM2MTrigger=0 --nM2MRegular=0 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    ###############################

    # Scheduler: 0 - H2H: 20 - M2M T: 5  - M2M R: 15
    echo -e "Scheduler: 0 - H2H: 20 - M2M T: 5  - M2M R: 15 - index: $index"
    params="--scheduler=0 --simTime=$simTime --nH2H=20 --nM2MTrigger=5 --nM2MRegular=15 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 1 - H2H: 20 - M2M T: 5  - M2M R: 15
    echo -e "Scheduler: 1 - H2H: 20 - M2M T: 5  - M2M R: 15 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=5 --nM2MRegular=15 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 2 - H2H: 20 - M2M T: 5  - M2M R: 15
    echo -e "Scheduler: 2 - H2H: 20 - M2M T: 5  - M2M R: 15 - index: $index"
    params="--scheduler=2 --simTime=$simTime --nH2H=20 --nM2MTrigger=5 --nM2MRegular=15 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    ###############################

    # Scheduler: 0 - H2H: 20 - M2M T: 13  - M2M R: 37
    echo -e "Scheduler: 0 - H2H: 20 - M2M T: 13  - M2M R: 37 - index: $index"
    params="--scheduler=0 --simTime=$simTime --nH2H=20 --nM2MTrigger=13 --nM2MRegular=37 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 1 - H2H: 20 - M2M T: 13  - M2M R: 37
    echo -e "Scheduler: 1 - H2H: 20 - M2M T: 13  - M2M R: 37 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=13 --nM2MRegular=37 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 2 - H2H: 20 - M2M T: 13  - M2M R: 37
    echo -e "Scheduler: 2 - H2H: 20 - M2M T: 13  - M2M R: 37 - index: $index"
    params="--scheduler=2 --simTime=$simTime --nH2H=20 --nM2MTrigger=13 --nM2MRegular=37 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    ###############################

    # Scheduler: 0 - H2H: 20 - M2M T: 33  - M2M R: 67
    echo -e "Scheduler: 0 - H2H: 20 - M2M T: 33  - M2M R: 67 - index: $index"
    params="--scheduler=0 --simTime=$simTime --nH2H=20 --nM2MTrigger=33 --nM2MRegular=67 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 1 - H2H: 20 - M2M T: 33  - M2M R: 67
    echo -e "Scheduler: 1 - H2H: 20 - M2M T: 33  - M2M R: 67 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=33 --nM2MRegular=67 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 2 - H2H: 20 - M2M T: 33  - M2M R: 67
    echo -e "Scheduler: 2 - H2H: 20 - M2M T: 33  - M2M R: 67 - index: $index"
    params="--scheduler=2 --simTime=$simTime --nH2H=20 --nM2MTrigger=33 --nM2MRegular=67 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    ###############################

    # Scheduler: 0 - H2H: 20 - M2M T: 50  - M2M R: 100
    echo -e "Scheduler: 0 - H2H: 20 - M2M T: 50  - M2M R: 100 - index: $index"
    params="--scheduler=0 --simTime=$simTime --nH2H=20 --nM2MTrigger=50 --nM2MRegular=100 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 1 - H2H: 20 - M2M T: 50  - M2M R: 100
    echo -e "Scheduler: 1 - H2H: 20 - M2M T: 50  - M2M R: 100 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=50 --nM2MRegular=100 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 2 - H2H: 20 - M2M T: 50  - M2M R: 100
    echo -e "Scheduler: 2 - H2H: 20 - M2M T: 50  - M2M R: 100 - index: $index"
    params="--scheduler=2 --simTime=$simTime --nH2H=20 --nM2MTrigger=50 --nM2MRegular=100 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    ###############################

    # Scheduler: 0 - H2H: 20 - M2M T: 66  - M2M R: 134
    echo -e "Scheduler: 0 - H2H: 20 - M2M T: 66  - M2M R: 134 - index: $index"
    params="--scheduler=0 --simTime=$simTime --nH2H=20 --nM2MTrigger=66 --nM2MRegular=134 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 1 - H2H: 20 - M2M T: 66  - M2M R: 134
    echo -e "Scheduler: 1 - H2H: 20 - M2M T: 66  - M2M R: 134 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=66 --nM2MRegular=134 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

    # Scheduler: 2 - H2H: 20 - M2M T: 66  - M2M R: 134
    echo -e "Scheduler: 2 - H2H: 20 - M2M T: 66  - M2M R: 134 - index: $index"
    params="--scheduler=1 --simTime=$simTime --nH2H=20 --nM2MTrigger=66 --nM2MRegular=134 --intervalM2MTrigger=$intTrigger --minM2MRegularCqi=$minCqi --maxM2MRegularCqi=$maxCqi --nExec=$index"
    command="$simulator $params'"
    eval $command

done

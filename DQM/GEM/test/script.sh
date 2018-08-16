#!/bin/bash
run='/eos/cms/store/t0streamer/Minidaq/A/000/319/580'
files=(`/usr/bin/eos ls $run`)
work()
{
    run='/eos/cms/store/t0streamer/Minidaq/A/000/319/580'
    files=(`/usr/bin/eos ls $run`)
    echo "${run}/${files[$1]}"
    lumi=x${files[$1]:10:6}
    mkdir $lumi
    /bin/sed "s@FILENAME_TEMPLATE@${run}/${files[$1]}@g" <./test_counter.py >./$lumi/${lumi}_cfy.py
    cd $lumi
    /cvmfs/cms.cern.ch/slc6_amd64_gcc630/cms/cmssw/CMSSW_10_1_8/bin/slc6_amd64_gcc630/cmsRun ${lumi}_cfy.py
    cd ..
}

nMaxProc=5

for ((i=18; i<${#files[@]}; i++)); do echo $i ; done |
(
    export -f work
    xargs -I{} --max-procs ${nMaxProc} bash -c ' ## work on this...
         {
            work {}
         }'
)
echo "Finished"

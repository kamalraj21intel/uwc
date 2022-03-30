#!/bin/bash
# Copyright (c) 2021 Intel Corporation.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

Current_Dir=$(pwd)
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
MAGENTA=$(tput setaf 5)
NC=$(tput sgr0)
resume_insights="${HOME}/.resume_process"

eii_build_dir="$Current_Dir/../../build"
source uwc_common_lib.sh

#------------------------------------------------------------------
# build_run_UWC
#
# Description:
#        This fun will install all UWC containers
# Return:
#        None
# Usage:
#        build_run_UWC
#-----------------------------------------------------------------
build_run_UWC()
{
    cd "${eii_build_dir}"
    docker-compose up -d
    if [ "$?" -eq "0" ];then
        echo "*****************************************************************"
        echo "${GREEN}Installed UWC containers successfully.${NC}"
    else
        echo "${RED}Failed to install UWC containers.${NC}"
        echo "*****************************************************************"
        exit 1
    fi
    return 0
}

verify_container()
{
    cd "${eii_build_dir}"
    echo "*****************************************************************"
    echo "${GREEN}Below is the containers deployed status.${NC}"
    echo "*****************************************************************"
    docker ps
    return 0
}

function harden()
{
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 mqtt-bridge
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 modbus-tcp-master
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 mqtt_container
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 modbus-rtu-master
	docker ps -q --filter "name=sparkplug-bridge" | grep -q . && docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 sparkplug-bridge
	# Increase pid limit for KPI to 500 for processing larger count of threads.
	docker ps -q --filter "name=kpi-tactic-app" | grep -q . && docker container update --pids-limit=500 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 kpi-tactic-app
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 ia_configmgr_agent
    docker ps -q --filter "name=emb_publisher" | grep -q . && docker container update --pids-limit=100 --cpu-shares 512 -m 1G --memory-swap -1 emb_publisher
    docker ps -q --filter "name=ia_emb_subscriber" | grep -q . && docker container update --pids-limit=100 --cpu-shares 512 -m 1G --memory-swap -1 ia_emb_subscriber
}
function main()
{
    echo "${INFO}Deployment started${NC}"
    check_root_user
    check_internet_connection
    docker_verify
    docker_compose_verify
    RUN_STARTTIME=$(date +%s)
    build_run_UWC
    RUN_ENDTIME=$(date +%s)
    verify_container
    harden
    echo "${INFO}Deployment Completed${NC}"
    RUN_ELAPSEDTIME=$(( ${RUN_ENDTIME} - ${RUN_STARTTIME} ))
    echo "${GREEN}Total Elapsed time for run is : $(( ${RUN_ELAPSEDTIME} / 60 )) minutes ${NC}"
}

export DOCKER_CONTENT_TRUST=1
export DOCKER_BUILDKIT=1
main

cd "${Current_Dir}"
exit 0


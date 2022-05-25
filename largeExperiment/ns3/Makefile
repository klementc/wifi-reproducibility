NS3_PATH=/home/clem/Code/gitlab.inria.fr/lguegan/wifi-flow-level/run/ns-3/ns-3/
#/home/clem/code/gitlab.io/clementcs/ns-3-dev
NS3_VERSION="3.33"
NS3_ARGS= -D NS3_LOG_ENABLE -L ${NS3_PATH}/build/lib -I ${NS3_PATH}/build/
NS3_ARGS+=$(addprefix -l, $(subst lib,,$(subst .so,,$(notdir $(wildcard ${NS3_PATH}/build/lib/libns3*.so)))))


all: wifiSimNS

wifiSimNS: wifisimNS.cpp
	g++ -g -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $^ -o $@

run:  wifiSimNS
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	@export LD_LIBRARY_PATH=${NS3_PATH}/build/lib && ./wifiSimNS ${ARGS}


clean:
	- rm wifiSimNS

.PHONY: clean

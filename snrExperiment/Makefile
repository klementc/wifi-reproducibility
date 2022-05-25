SG_ARGS= -L ./simgrid/build/lib -lsimgrid -I ./simgrid/build/include -I ./simgrid/include -I ./simgrid/ -I ./simgrid/build/
NS3_PATH=/home/clem/Code/gitlab.inria.fr/lguegan/wifi-flow-level/run/ns-3/ns-3/
NS3_VERSION="3.33"
NS3_ARGS= -D NS3_LOG_ENABLE -L ${NS3_PATH}/build/lib -I ${NS3_PATH}/build/
NS3_ARGS+=$(addprefix -l, $(subst lib,,$(subst .so,,$(notdir $(wildcard ${NS3_PATH}/build/lib/libns3*.so)))))


all: experiment_snr experiment_snr_ns simNS

experiment_snr: experiment_snr.cc
	g++ -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

experiment_wifi_nowifi: experiment_wifi_nowifi.cc
	g++ -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

experiment_snr_ns: 80211n-mimo.cc
	g++ -g -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $^ -o $@

simNS: wifisimNS.cc
	g++ -g -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $^ -o $@

run: experiment_snr
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	export LD_LIBRARY_PATH=./simgrid/build/lib:./ns-3/ns-3/build/lib && ./experiment_snr --cfg=network/model:CM02 --cfg=network/crosstraffic:0 ${ARGS}

clean:
	- rm experiment_snr experiment_snr_ns


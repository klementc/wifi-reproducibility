EXEC=simulator
BUILD_DIR=build
SG_ARGS= -L ./simgrid/${BUILD_DIR}/lib -lsimgrid -I ./simgrid/${BUILD_DIR}/include -I ./simgrid/include -I ./simgrid/ -I ./simgrid/${BUILD_DIR}/


all: $(EXEC) pos_test experiment_snr

$(EXEC): src/$(EXEC).cc
	g++ -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

pos_test: src/testPosition.cc
	g++ -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

experiment_snr: src/experiment_snr.cc
	g++ -fPIC $(SG_ARGS) $^ -o $@ $(SG_ARGS)

run: $(EXEC)
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	export LD_LIBRARY_PATH=./simgrid/${BUILD_DIR}/lib:./ns-3/ns-3/build/lib && ./$(EXEC) --cfg=network/model:CM02 --cfg=network/crosstraffic:0 ${ARGS}

clean:
	- rm $(EXEC)
	- rm $(EXEC)_fixed_flows


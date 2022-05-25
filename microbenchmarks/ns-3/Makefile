EXEC=simulator
NS3_PATH=./ns-3
NS3_VERSION="3.30.1"
NS3_ARGS= -D NS3_LOG_ENABLE -L ${NS3_PATH}/build/lib -I ${NS3_PATH}/build/
NS3_ARGS+=$(addprefix -l, $(subst lib,,$(subst .so,,$(notdir $(wildcard ${NS3_PATH}/build/lib/libns3*.so)))))
SRC=$(wildcard src/*)
SRC_FIXED_FLOWS=$(wildcard fixed_flows/*)

all: $(EXEC)

$(EXEC): $(SRC)
	g++ -fPIC -g  -D NS3_VERSION=\"${NS3_VERSION}\" $(NS3_ARGS) $(SRC) -o $@  $(NS3_ARGS)
	@echo -e "\e[32mRun the following command before running $(EXEC):\e[0m"
	@echo -e "\e[32mexport LD_LIBRARY_PATH=${NS3_PATH}/lib\e[0m"

run:  $(EXEC)
ifeq ($(ARGS),)
	$(error Missing ARGS parameter to run the simulator)
endif
	@export LD_LIBRARY_PATH=${NS3_PATH}/build/lib && ./simulator ${ARGS}

clean:
	- rm $(EXEC)

.PHONY: clean

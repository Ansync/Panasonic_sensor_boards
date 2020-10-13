.PHONY: bin clean erase flash gdb gdbserver
include local.config

PROJ_DIR=./panasonic/s140/armgcc
SERIAL?=0

bin:
	cd ${PROJ_DIR} && GNU_INSTALL_ROOT="${GCC_PATH}/" NRFSDK=${NRFSDK} BOARD=${BOARD} make

clean:
	rm -rf ${PROJ_DIR}/build

erase:
	nrfjprog -f nrf52 --eraseall

flash: 
	nrfjprog -f nrf52 --eraseall
	nrfjprog -f nrf52 --program ${NRFSDK}/components/softdevice/s140/hex/s140_nrf52_7.0.1_softdevice.hex --sectorerase
	nrfjprog -f nrf52 --program ${PROJ_DIR}/build/nrf52840_xxaa.hex --sectorerase
	nrfjprog -f nrf52 --memwr 0x10001080 --val ${SERIAL}
	nrfjprog -f nrf52 --memwr 0x10001304 --val 0xfffd
	nrfjprog -f nrf52 --reset

gdb:
	cd ${PROJ_DIR} && ${GCC_PATH}/arm-none-eabi-gdb --command=../../../nrf.gdbinit --tui build/nrf52840_xxaa.out

gdbserver:
	/opt/SEGGER/JLink/JLinkGDBServerExe -device nRF52840_xxAA -if swd &


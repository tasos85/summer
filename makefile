include lpc1768.mk

OBJS = lpc1768_startup.o lpc1768_handlers.o system_LPC17xx.o sbrk.o \
	   core_cm3.o uart/uart.o adc/adc.o EMAC.o tcpip.o easyweb.o\
	   comm/commands.o comm/comm.o utils/utils.o bang/bang.o control/control.o\
	   init/main.o
OUTPUT = firmware

all:$(OUTPUT).bin

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

#lpc1768_startup.o: 
#	$(CC) $(CFLAGS) lpc1768_startup.c -o $@ 

#lpc1768_handlers.o : 
#	$(CC) $(CFLAGS) lpc1768_handlers.c -o $@

#system_LPC17xx.o:
#	$(CC) $(CFLAGS) system_LPC17xx.c -o $@

#core_cm3.o: 
#	$(CC) $(CFLAGS) core_cm3.c -o $@

#sbrk.o:
#	$(CC) $(CFLAGS) sbrk.c -o $@

#uart.o: core_cm3.o
#	$(CC) $(CFLAGS) uart/uart.c -o $@

#commands.o: sbrk.o
#	$(CC) $(CFLAGS) comm/commands.c -o $@

#comm.o: commands.o sbrk.o uart.o
#	$(CC) $(CFLAGS) comm/comm.c -o $@

#utils.o:
#	$(CC) $(CFLAGS) utils/utils.c -o $@	
	
#adc.o: adc/adc.h adc/adc.c core_cm3.o
#	$(CC) $(CFLAGS) adc/adc.c -o $@ 

#bang.o: bang/bang.h bang/bang.c core_cm3.o
#	$(CC) $(CFLAGS) bang/bang.c -o $@ 
	
#control.o: control/control.h control/control.c
#	$(CC) $(CFLAGS) control/control.c -o $@

#main.o: adc.o bang.o comm.o control.o utils.o
#	$(CC) $(CFLAGS) init/main.c -o $@

$(OUTPUT).bin: $(OBJS)
	$(LD) $(LDFLAGS) -T $(LD_SCRIPT) -o $(OUTPUT).elf $(OBJS) -lm 
	$(OBJCOPY) $(OCFLAGS) -O ihex $(OUTPUT).elf $(OUTPUT).hex
	$(OBJCOPY) $(OCFLAGS) -O binary $(OUTPUT).elf $(OUTPUT).bin
	$(LPCRC) $(OUTPUT).elf
	$(LPCRC) $(OUTPUT).hex
	$(LPCRC) $(OUTPUT).bin
clean:
	rm -f *.o */*.o $(OUTPUT).elf $(OUTPUT).bin $(OUTPUT).hex

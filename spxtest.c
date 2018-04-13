// test the SPx-USB driver

#include <stdio.h>
#include "spxdrv.h"


void showerr(unsigned long inst, int retcode, char* funcname) {
	char errmsg[SPX_ERR_DESCR_BUFFER_SIZE];
	SPX_errorMessage(inst, retcode, errmsg);
	printf("%s returned %d: %s\n", funcname, retcode, errmsg);
}

void status(unsigned long inst){
	unsigned char stat;
	int ret;
    ret = SPX_getDeviceStatus(inst, &stat);
    showerr(inst, ret, "getstat");
    
    printf("state %x=\n", stat);
    if (stat == SPX_SCAN_STATE_UNKNOWN){
    	printf("state unknown\n");
    }
    if (stat == SPX_SCAN_STATE_IDLE){
    	printf("state idle\n");
    }
    if (stat & SPX_SCAN_STATE_TRANSFER) {
    	printf("state waiting for transfer\n");
    }
    if (stat & SPX_SCAN_STATE_IN_PROGRES) {
    	printf("state scan in progress (should never see)\n");
    }
    if (stat & SPX_SCAN_STATE_WAIT_TRIGGER) {
    	printf("state idle and armed\n");
    }
}


int main(int argc, char* argv[]){
    unsigned long inst; //instrument handle  (ViPSession = unsigned long*)
    int ret;
    
    ret = SPX_init("bogusresource", 3000, &inst);
    showerr(inst, ret, "init"); // pay atten if nonzero:will segfault on close
    
    char vendor[SPX_BUFFER_SIZE];
    char name[SPX_BUFFER_SIZE];
    char serial[SPX_BUFFER_SIZE];
    char revision[SPX_BUFFER_SIZE];
    ret = SPX_identificationQuery(inst, vendor, name, serial, revision);
    showerr(inst, ret, "ID");
    printf("V=%s N=%s S=%s R=%s\n", vendor, name, serial, revision);
    
    double inttime;
    SPX_getIntTime (inst, &inttime);
    printf("inttime read as %f\n", inttime); 
    
    printf("inttime?\n");
    float newtime;
    scanf("%f", &newtime);
    
    
    SPX_setIntTime (inst, newtime);
    SPX_getIntTime (inst, &inttime);
    printf("inttime read as %f after setting \n", inttime);    
    
    status(inst);

    SPX_startScan(inst);
    showerr(inst, ret, "startscan");
    
    status(inst);
    
    double wave[3000];
    ret = SPX_getWavelengthData(inst, wave, 0,0);
    showerr(inst, ret, "getwavelength");
    
    
    // 3068-68 is number of final pts
    double dat[3000];
    ret = SPX_getScanData(inst, dat);
    showerr(inst, ret, "get");
    
    status(inst);
    
    FILE *outfil;
    outfil = fopen("/tmp/spec", "w");
    int i;
    for (i=0; i<3000; i++) {
    	fprintf(outfil, "%f %f\n", wave[i], dat[i]);
    }
    fclose(outfil);
    
    
    
    
    ret = SPX_close(inst);
    showerr(inst, ret, "close");
    
    return 0;
}

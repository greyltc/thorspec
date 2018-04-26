// test the CCS series USB driver

#include <stdio.h>
#include "CCS_Series_Drv.h"


void showerr(unsigned long inst, int retcode, char* funcname) {
        char errdesc[CCS_SERIES_ERR_DESCR_BUFFER_SIZE];
        CCSseries_errorMessage(inst, retcode, errdesc);
        printf("%s returned %d: %s\n", funcname, retcode, errdesc);
}


void status(unsigned long inst) {
        long stat;
        int ret;

        ret = CCSseries_getDeviceStatus(inst, &stat);
        showerr(inst, ret, "getstat");

        printf("status %x = ", stat);
        if (stat & CCS_SERIES_STATUS_SCAN_IDLE) printf("idle,");
        if (stat & CCS_SERIES_STATUS_SCAN_TRIGGERED) printf("triggered,");
        if (stat & CCS_SERIES_STATUS_SCAN_START_TRANS) printf("starting,");
        if (stat & CCS_SERIES_STATUS_SCAN_TRANSFER) printf("awaiting_xfer,");
        if (stat & CCS_SERIES_STATUS_WAIT_FOR_EXT_TRIG) printf("armed,");
        printf("\n");
}


int main(int argc, char* argv[]) {
    unsigned long inst; //instrument handle  (ViPSession = unsigned long*)
    ViStatus ret;

    ret = CCSseries_init(argv[1],   VI_ON,  VI_ON,      &inst); // read usb vid:pid from command line
    //                   resource name, IDQuery, resetDevice, instrumentHandle
    //ret = CCSseries_init("1313:8081",   VI_OFF,  VI_OFF,      &inst);
    //ret = CCSseries_init("1313:8087",   VI_ON,  VI_ON,      &inst);
    //ret = CCSseries_init("1313:8081",   VI_ON,  VI_ON,      &inst);
    showerr(inst, ret, "init");   // if nonzero must exit, as we'll get a segfault if we proceed

    char vendor[CCS_SERIES_BUFFER_SIZE];
    char device[CCS_SERIES_BUFFER_SIZE];
    char serial[CCS_SERIES_BUFFER_SIZE];
    char revision[CCS_SERIES_BUFFER_SIZE];
    char driver_revision[CCS_SERIES_BUFFER_SIZE];
    ret = CCSseries_identificationQuery(inst, vendor, device, serial, revision, driver_revision);
    showerr(inst, ret, "ID");
    status(inst);
    printf("V=%s D=%s S=%s R=%s DR=%s\n", vendor, device, serial, revision, driver_revision);

    double inttime, newinttime;
    ret = CCSseries_getIntegrationTime(inst, &inttime);
    showerr(inst, ret, "getinttime");
    printf("inttime read as %f\n", inttime);

    printf("inttime?\n");
    scanf("%lf", &newinttime);
    ret = CCSseries_setIntegrationTime(inst, newinttime);
    showerr(inst, ret, "setinttime");

    ret = CCSseries_getIntegrationTime(inst, &inttime);
    showerr(inst, ret, "get2inttime");
    printf("inttime read as %f after setting %f\n", inttime, newinttime);
    status(inst);


    double wavdata[CCS_SERIES_NUM_PIXELS];
    double minimumWavelength, maximumWavelength;
    ret = CCSseries_getWavelengthData(inst, CCS_SERIES_CAL_DATA_SET_FACTORY, wavdata, &minimumWavelength, &maximumWavelength);
    showerr(inst, ret, "getwldata");
    status(inst);

    printf("wavelengths %lf to %lf\n", minimumWavelength, maximumWavelength);
    printf("%lf, %lf,... %lf, %lf\n", wavdata[0], wavdata[1], wavdata[CCS_SERIES_NUM_PIXELS-2],wavdata[CCS_SERIES_NUM_PIXELS-1]);

    CCSseries_startScan(inst);
    //sleep(2);    will time out after 3 s so long integs need something here

    double ampdata[CCS_SERIES_NUM_PIXELS];
    ret = CCSseries_getScanData(inst, ampdata);
    showerr(inst, ret, "getampdata");
    printf("%lf, %lf,... %lf, %lf\n", ampdata[0], ampdata[1], ampdata[CCS_SERIES_NUM_PIXELS-2],ampdata[CCS_SERIES_NUM_PIXELS-1]);

    FILE *outfil;
    outfil = fopen("/tmp/spec", "w");
    int i;
    for (i=0; i<CCS_SERIES_NUM_PIXELS; i++) {
            fprintf(outfil, "%lf %lf\n", wavdata[i], ampdata[i]);
    }
    fclose(outfil);
    system("echo \"plot '/tmp/spec' with lines\" | gnuplot -persist -");


    /// TODO a flush read before starting?

    ret = CCSseries_close(inst);
    showerr(inst, ret, "close");

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "midas.h"
#include "mcstd.h"
#include "frontend.h"
#include <unistd.h>

#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <strings.h>

#include <CAENVMElib.h>
#include <CAENVMEtypes.h>

unsigned int fAddrCTS;
unsigned int fTdcLastChannelNo;
unsigned int fTdcEpochCounter;
unsigned int fCTSExtTrigger;
unsigned int fCTSExtTriggerStatus;
bool fIsIRQEnabled;
bool IsRunStopped;
static const size_t fPacketMaxSize = 65507;
unsigned int fPacket[65507];
int DW;
int DataWaiting;
unsigned int DataToWrite[30000];
unsigned int offsetnow, offsetbefore;


/****************************************************************************
                        INITIALIZATION OF FRONTEND 
*****************************************************************************/
INT frontend_init()
{
    printf("Start 'frontend INIT\n");
    int i, status, csr, aad, bad;
    status = mvme_open(&myvme,0);
    printf("%d\n", status);
    printf("Hello World\n");

    CAENVME_DeviceReset((int)myvme->info);
    CAENVME_SetOutputConf((int)myvme->info,cvOutput0,cvDirect,cvDirect,cvManualSW);
    CAENVME_SetOutputConf((int)myvme->info,cvOutput1,cvDirect,cvDirect,cvManualSW);
    CAENVME_SetOutputRegister((int) myvme->info, cvOut0Bit);
    CAENVME_SetOutputRegister((int) myvme->info, cvOut1Bit);
    CAENVME_SetInputConf((int)myvme->info,cvInput0,cvDirect,cvDirect);
    
    unsigned int InpDat;
    CAENVME_ReadRegister((int)myvme->info,0x0B,&InpDat);
    printf("Input Max Register %4.4x\n", InpDat);

    CAENVME_ReadRegister((int)myvme->info,cvInputReg,&InpDat);
    printf("Input Register %4.4x\n", InpDat);

    // setting addressing and data mode
    mvme_set_am(myvme,MVME_AM_A32);
    mvme_set_dmode(myvme,MVME_DMODE_D32);

    /******************************************************************************
     *
     * INITIALIZE VARIOUS DEVICES
     *
     *****************************************************************************/

    //-----------------CODE To Initialize Digitizer-------------------------------//
    #if defined V1740_CODE
        //check the Digitizer
        printf("V1740\n");

        //status and Firmware version of the Digitizer
        printf("Status of the Digitizer::\n");
        v1740_Status(myvme,V1740_BASE_ADDR);

        aad = v1740_RegisterRead(myvme,V1740_BASE_ADDR, 0xF084);
        bad = v1740_RegisterRead(myvme,V1740_BASE_ADDR, 0xF080);
        printf("Serial Number of Digitizer:: 0x%x%x\n",bad,aad);

        csr = mvme_read_value(myvme,V1740_BASE_ADDR,V1740_ROC_FPGA_FW_REV);
        printf("V1740 ROC(motherboard) Firmware Revision: 0x%8.8x\n",csr);

        // Reset Digitizer's Software
        v1740_Reset(myvme,V1740_BASE_ADDR);
        // setting mode 1. it sets various parameters of the device.
        v1740_Setup(myvme,V1740_BASE_ADDR,1);
        printf("V1740 INITIALIZED\n");
    #endif

/****************Device Initialization Finished *************************/
    int gemlatch = 9;
    printf("End of Frontend Initialization\n");

    aad = v1740_RegisterRead(myvme,V1740_BASE_ADDR,0x811C);
    printf("V1740_ACQUISITION_CONTROL::%x\n", aad);

/*    for (i = 0; i < 9; ++i)
    {
        v1740_ChannelThresholdSet(myvme,V1740_BASE_ADDR,i,0x0868);
    }
*/
    ss_sleep(5000);
    return SUCCESS;
}

/****************************************************************************
                     FRONTEND EXIT FUNCTION
*****************************************************************************/
INT frontend_exit() {return SUCCESS;}

/****************************************************************************
                        BEGINING THE RUN 
*****************************************************************************/
INT begin_of_run(INT run_number, char *error)
{
    printf("Start the Run\n");
    int i, csr, status, evtcnt;
    WORD threshold[32];
    mvme_set_am(myvme,MVME_AM_A32_ND);
    mvme_set_dmode(myvme,MVME_DMODE_D32);
#if defined V1740_CODE
    // check the Digitizer
    printf("V1740\n");
    csr = mvme_read_value(myvme,V1740_BASE_ADDR,V1740_ROC_FPGA_FW_REV);
    printf("V1740 ROC (motherboard) Firmware Revision: 0x%x\n",csr );

    // Reset Digitizer's Software
    v1740_Reset(myvme, V1740_BASE_ADDR);

    // Setting mode 1. It sets various parameters of the device.
    v1740_Setup(myvme, V1740_BASE_ADDR, 0x1);
 /*   for(i=1;i<9;i++)
    {
        v1740_ChannelThresholdSet(myvme, V1740_BASE_ADDR, i, 0x0868);
    } */

    printf("V1740 is Configured\n");
    ss_sleep(5000);
#endif

//******************Device Setup Done*********************************//
    event_counter_for_interface = 0;
    offsetbefore = 0;
    printf("Waiting\n");
    for (i = 0; i < 3; ++i)
    {
        ss_sleep(1000);
        printf(".\n");
    }
    printf("Starting Now\n");
    printf("End 'Begin of Run'\n");
    IsRunStopped=false;

    v1740_AcqCtl(myvme, V1740_BASE_ADDR,V1740_RUN_START);
    csr = v1740_RegisterRead(myvme,V1740_BASE_ADDR,V1740_ACQUISITION_CONTROL);
    printf("V1740_ACQUISITION_CONTROL::%8.8x\n", csr);
    ss_sleep(5000);
    return SUCCESS;
}

/****************************************************************************
                        END OF RUN 
*****************************************************************************/

INT end_of_run(INT run_number, char *error)
{
    IsRunStopped=true;
    fIsIRQEnabled=false;       //disabling triggers
    v1740_AcqCtl(myvme,V1740_BASE_ADDR,V1740_RUN_STOP);
    printf("End of Run\n");
    return SUCCESS;
}

/****************************************************************************
                        PAUSE RUN 
*****************************************************************************/
INT pause_run (INT run_number, char *error) {return SUCCESS;}

/****************************************************************************
                        RESUME RUN 
*****************************************************************************/
INT resume_run (INT run_number, char *error) {return SUCCESS;}

/****************************************************************************
                        FRONTEND LOOP 
*****************************************************************************/
INT frontend_loop () {return SUCCESS;}


/****************************************************************************
 *
 *
 * DATA READOUT ROUTINES
 *
 *
****************************************************************************/

/****************************************************************************
                    Triggered event Data Availability Check
****************************************************************************/

//Checking if the events are available to be stored in the data bank
//*****************POLLING EVENT********************************************/

//MIDAS asks vme device if the data is avaliable
INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event is available.
If the test equals TRUE, don't return. The test flag is used to
time the polling */
{
    int latch = 0;
    int InpDat = 0;
    for (int i = 0; i < count+2; ++i)
    {
        InpDat = v1740_RegisterRead(myvme,V1740_BASE_ADDR,V1740_VME_STATUS);
        InpDat = InpDat & 0x00000001;
        if (InpDat==1){return 1;}
    }
    return 0;
}

//*****************INTERRUPT EVENT********************************************/ 
// not sure it's needed

INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

/****************************************************************************
 *                      DATA READOUT
****************************************************************************/

INT read_trigger_event(char *pevent, INT off)
{
    // DWORD *tdata, *ddata, *pdata,*pdata1, *pdata2, *pdata3,*pdata4, *evdata;
    DWORD *d1740data, *evdata;
    int nentry, nextra, dummyevent, fevtcnt, temp, dtemp, dentry, dextra, devtcnt;
    int size_of_evt, nu_of_evt;

    // Generating a pulse
    CAENVME_SetOutputRegister((int)myvme->info,cvOut0Bit);
    CAENVME_SetOutputRegister((int)myvme->info,cvOut1Bit);
    bk_init32(pevent);

//-----------------Take Data from Digitizer----------------------------//
#if defined V1740_CODE
    v1740_AcqCtl(myvme,V1740_BASE_ADDR,V1740_RUN_STOP);
    dentry = 0;
    dextra = 0;
    devtcnt = 0;
    size_of_evt = 0;
    nu_of_evt = 0;
    int readmsg = 0;
    int cycle = 0,  maxcycles = 6;     // This varialble is important to read just the first event and not let ODB overflow

    int bytes_totransfer = 0, bytes_remaining = 0 ;
    int bytes_transferred =  0;
    int bytes_max =  4096;

    bk_create(pevent,"DG01",TID_DWORD, &d1740data);

    int InpDat = v1740_RegisterRead(myvme, V1740_BASE_ADDR, V1740_VME_STATUS);
    InpDat = InpDat & 0x00000001;

    mvme_set_am(myvme, MVME_AM_A32_ND); //set addressing mode to A32 non-privileged data
    mvme_set_dmode(myvme, MVME_DMODE_D32);
    mvme_set_blt(myvme, MVME_BLT_BLT32);

    if (InpDat == 1)
    {
        size_of_evt =  mvme_read_value(myvme, V1740_BASE_ADDR, V1740_EVENT_SIZE);
        nu_of_evt =  mvme_read_value(myvme, V1740_BASE_ADDR, V1740_EVENT_STORED);

        bytes_remaining = size_of_evt * nu_of_evt * 4;     //chaning from 32bit to byte
        int toread = bytes_remaining;

        printf("\n\n\n\nNew Event:: \t size of Event :: %d \t Nu of event :: %d \t  Bytes Remaining:: %d\n", size_of_evt, nu_of_evt, bytes_remaining);
        if (bytes_remaining > 1 )
        {
//            printf("\n\n\n\nNew Event:: \t Bytes Remaining:: %d\n", bytes_remaining);
            *d1740data = (DWORD *) malloc(bytes_remaining);
            cycle = 0;
            int buffer_address = V1740_EVENT_READOUT_BUFFER;

            while(bytes_remaining > 1 && cycle < maxcycles)   // the cycle setting is important untill we have smart 
            {                                                  // trigger oterwise it will read many cylcles 
                                                               // and return odb fragmentation  error 
               printf("Bytes Remaining:: %d Cycle :: %d\n", bytes_remaining, cycle);
                if (bytes_remaining > 4096)
                {
                    bytes_totransfer = bytes_max;
//                    printf("Bytes Remaining:: %d\n", bytes_remaining);
                }
               else
                    bytes_totransfer = bytes_remaining;

                //        to_transfer = bytes_remaining > bytes_max ? bytes_max : bytes_remaining;
                //V1720_EVENT_READOUT_BUFFER
                readmsg = CAENVME_BLTReadCycle((long *)myvme->info, V1740_BASE_ADDR + V1740_EVENT_READOUT_BUFFER , d1740data, bytes_totransfer, cvA32_U_BLT, 0x04, &bytes_transferred);
                bytes_remaining = bytes_remaining - bytes_transferred;
                if (readmsg == 0)
                {
//                    printf("BLT success::%d\n",readmsg);
                    d1740data += bytes_transferred/4;
//                  printf("Cycles Requ:: %d \t Cycles READ:: %d \n", bytes_totransfer, bytes_transferred);
//                  printf("To REad:: %d \t ACt. READ:: %d \t Rem. to Read:: %d \n", bytes_totransfer, bytes_transferred, bytes_remaining);
//                    printf("First Data Word:: %d \n", ddata[0]);
                }
                if (readmsg != 0)
                {
                    printf("\n\n\n\nBLT REad Message::%d\n", readmsg);
                    printf("Cycles Requ:: %d \t Cycles READ:: %d \n", bytes_totransfer, bytes_transferred);
                }
                cycle++;
            }
        }
    }

bk_close(pevent,d1740data);
#endif

//-----------------Bank for DAQ event counter----------------------------------//
    bk_create(pevent,"EVNT",TID_DWORD,&evdata);
    *evdata++ = event_counter_for_interface;
    bk_close(pevent,evdata);
    event_counter_for_interface++;
    //Generating a Pulse
    CAENVME_SetOutputRegister((int) myvme->info,cvOut0Bit);
    CAENVME_SetOutputRegister((int) myvme->info,cvOut1Bit);
    v1740_AcqCtl(myvme,V1740_BASE_ADDR,V1740_RUN_START);
    return bk_size(pevent);
}

int eventcounteroffset (int event_counter_for_interface, int gemlatch)
{return ((event_counter_for_interface & 0x7) - ((gemlatch & 0xf)>>1))& 0x7;}



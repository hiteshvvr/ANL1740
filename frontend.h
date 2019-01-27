#include "vmicvme.h"
#include "v1740.h"
//#include "v1740drv.h"
#include "v812.h"

#define V1740_ROC_FPGA_FW_REV    0x8124
#define V1740_CODE

const int evtdebug = 1;

#ifdef __cplusplus
extern "C" {
#endif

/**********Globals*******************/

/* The frontend name (client name ) as seen by other MIDAS clients */    
char *frontend_name = "NEAT_40";

/* The frontend file name, don't change it*/
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 1000;

/* maximum event size produced by this frontend */
INT max_event_size = 200000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 100000;

/* Hardware */
MVME_INTERFACE *myvme;



/* GLOBALS */

#define N_DG01 128
#define N_QDC 32
#define N_TDC1 16
#define N_TDC2 128
#define N_EVNT 2

extern HNDLE hDB;
HNDLE hSet;

/* VME BASE ADRESSES */
DWORD V1740_BASE_ADDR = 0xCCCC0000;
DWORD V812_BASE_ADDR = 0xFFFF0000;


int pvmeRemoteVmeAcessF (int verbose,
                        int unit_number,
                        unsigned int remote_vme_address,
                        unsigned int remote_vme_am,
                        unsigned int pvic_page_number,
                        unsigned int pvme_page_number,
                        int data_bytes,
                        int data_type,
                        int write_acess,
                        int write_data,
                        int repeat_count);

int pvmePioAcessExecute (int vme_address,
                        char *pointer,
                        int data_bytes,
                        int data_type,
                        int write_acess,
                        int write_data,
                        int repeat_count);


/* new event counter for interfacing with GEM DAQ */
INT event_counter_for_interface;

/* FUNCTION DECLERATIONS */
INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_of_run(INT run_number, char *error);
INT resume_of_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);
INT read_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off);
void register_cnaf_callback(int debug);
int pvmeLocalSetup(int localNodeId);
char *GetVMEPtr(int remote_vme_address,int remote_vme_am,int bsize, int pvmenum,int pvicnum);
char *GetVMEPtr32(int remote_vme_adress, int remote_vme_am, int bsize, int pvmenum, int pvicnum);
int pvmeRemoteSetup(int remoteNodeId);

BANK_LIST trigger_bank_list[] = {
  // online data banks
 {"DG01", TID_WORD, N_DG01, NULL} // V1740
 ,
 // {"QDC1", TID_WORD, N_QDC, NULL} // V965
 // ,
};

/* EQUIPMENT LIST */

EQUIPMENT equipment[] = {
    {"Trigger",             /* Equipment Name */
        {1,0,               /* Event ID, Trigger Mask */
        "SYSTEM",           /* Event Buffer */
        EQ_POLLED,          /* Equipment Type */
        LAM_SOURCE(0,0x0),  /* Event Source Crate 0, All Stations */
        "MIDAS",            /* FORMAT */
        TRUE,               /* Enabled */
        RO_RUNNING,         /* Read Only When Running */
        500,                /* Poll For 500 ms */
        0,                  /* stop run after this event limit */
        0,                  /* number of sub events */
        0,                  /* don't log history */
        "","","",},
        read_trigger_event, /* Readout Routine */
        NULL, NULL,
        trigger_bank_list,
        },
    {""}
};

#ifdef __cplusplus
}
#endif

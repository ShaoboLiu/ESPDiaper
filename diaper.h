  

#include <ESP8266WiFi.h>
#include "ESP.h"
#include "IPAddress.h"
#include <Ticker.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>          
#include <vector>
//#include <algorithm>
#include "FS.h"
#include <ArduinoJson.h>
//#include <string.h>

#include <ESP8266HTTPClient.h>
extern "C" {
#include "user_interface.h"
}


#define VERSION   1000109

#define DEBUG_ESP_UPDATER    	1
#define DEBUG_ESP_HTTP_UPDATE  	1
#define DEBUG_ESP_PORT      	Serial




//debug
#define MYDEBUG   				1
#define NO_SLEEP_SAMPLING 		1


//=========================================
#define DIAPER_SUCCESS         		0
#define DIAPER_FAILURE         		1
#define DIAPER_MOUNT_FS_FAILURE   	2
#define DIAPER_SENDFILE_ERROR       3
#define DIAPER_SENDALERT_ERROR      4
#define DIAPER_FILE_NOT_EXIST       5



//===========default value for parameters==================
#define  DEFAULT_WIFISTATE          2
#define  DEFAULT_LIFESPAN           0
#define  DEFAULT_ALLACC             0
#define  DEFAULT_NUMEVENT       	0
#define  DEFAULT_DIAPERTYPE       	0
#define  DEFAULT_VOLTAGE        	0
#define  DEFAULT_THRESHOLD        	1000000000
#define  DEFAULT_MAXNUMEVENT      	10
#define  DEFAULT_MAXHOURS       	1000
#define  DEFAULT_RUNMODE        	1               //attention!
#define  DEFAULT_BEGINTIMEOFEVENT   0
#define  DEFAULT_ENDTIMEOFEVENT     0
#define  DEFAULT_ONLINEMODE       	1
#define  DEFAULT_THRESHOLDFLAG    	0
#define  DEFAULT_NUMEVENTFLAG    	0
#define  DEFAULT_MOVINGAVERAGE   	100
#define  DEFAULT_MAXDIFF         	0

//rtc mem
#define STARTOFRTCMEM           64
#define ADDR_WIFISTATE          64
#define ADDR_LIFESPAN           65
#define ADDR_LIFESPAN_US        66
#define ADDR_ALLACC             67     // 4 in a row
#define ADDR_NUMEVENT           71      
#define ADDR_DIAPERTYPE         72
#define ADDR_VOLTAGE            73
#define ADDR_THRESHOLD          74     //4 //in a row
#define ADDR_MAXNUMEVENT        78
#define ADDR_MAXHOURS           79     //save lower timestamp from server
#define ADDR_RUNMODE            80     //1=test mode    ;  0 =manufacture mode 
#define ADDR_BEGINTIMEOFEVENT   81
#define ADDR_ENDTIMEOFEVENT     82
#define ADDR_ONLINEMODE         83     //1=online mode   ;  0=offline mode 
#define ADDR_THRESHOLDFLAG      84     //0  no pass   2: happened
#define ADDR_NUMEVENTFLAG       85     //save upper timestamp from server


#define ADDR_MOVINGAVERAGE      86      //4 //in a row  
#define ADDR_DIFF               90      //4 //in a row
#define ADDR_MAXDIFF            94      //4 //in a row 
#define ADDR_MAXAVER            98      //4 //in a row 
#define ADDR_EMBEDDEDEVENT      102      //1: in the event  ; 
#define ADDR_PENDEDEVENTTIME    103

#define ADDR_TRIGERCHS          104

#define ADDR_TRIGERCHS_AVER     105

// pre-sample buffer in rtc mem  
#define ADDR_CURRENTPOINT       106    //  points to first space available
#define BEGINADDR               107                
#define BUFFERLENGTH            5  
#define ENDADDR                 (BEGINADDR+BUFFERLENGTH*4)  //post end             


#define ENDOFRTCMEM             (ENDADDR+1)
#define LEN_RTCMEM              (ENDOFRTCMEM-STARTOFRTCMEM+1)  


/* the interval(us) the pod will sleep */
#define POWERONDELAY            2000        //ms
#define DEEPSLEEPTIME           1000000     //us
#define CHARGEMODEDEEPSLEEPTIME 0           //forever
#define DEEPSLEEPSHORTTIME      5           //us
#define DEEPSLEEPSAMPLE         1          //second



#define EVENTTHRESHOLD               20        //to determine the end of event in terms of diff value
#define DIFF_EVENTTHRESHOLD         100 
#define THRESHOLD_OF_END_EVENT      200       //to determine the end of event in terms of value
#define DIFF_THRESHOLD_END_EVENT    30       //to determine the end of event in terms of diff value
#define TIME_WINDOW_WIDTH           16

//general file header
#define OFFSETNUMEVENT          20
#define LEN_HEADSECT            32
#define LEN_DATAFILESECTION     48

#define MAXERRORTRY            15
#define MAXERRORTRYCHMODE      50
#define ERRORDELAYMS           500 
enum TWifiState {twsCycleNoWifi=0,twsEventSession,twsPowerOn,twsSendingNow};

/* the container of ADC values*/
#define DATASIZE                64                  //container size
#define DATASIZEINBYTE          (DATASIZE*sizeof(uint32_t))
#define TRANSFERBUFF            1460 
std::vector<uint32_t> VALUES{DATASIZE};


/* ADC gpio config*/
const uint16_t CHS[]={5,4,13,12};
#define LEDPIN        14


uint32_t INTEGRATION[4]={0};     //sum within one event,saved in file at the end of event
uint16_t MAX[4]={0};            
//since last power on ,the sum of integrations of all cycles
uint32_t SUM[4]={0};             //saved in ADDR_ALLACC

 
 
const char   GENERALFILENAME[]="/general";
const char   FAILUREFILENAME[]="/failure";


#ifdef REDIRECT
    
// get paramter
    //const char* SERVERADDR_GET1 = "http://10.0.42.120:8080/RestServelet/RestServelet";
    //const char* GET_CONTENT_TYPE= "application/JSON";
    //const char* SERVERADDR_GET1 = "http://10.0.42.120:8080/RestServelet/RestServelet";
    //const char* GET_CONTENT_TYPE= "application/JSON";
    
// Post mac addr
    const char* SERVERADDR_POST1="http://10.0.42.120:8080/RestServelet/RestServelet?mac=";     //?mac=
    const char* POST1_CONTENT_TYPE= "application/text";


    const char* SERVERADDR_POST2="http://10.0.42.120:8080/RestServelet/RestServelet?data=";    //x&mac=addr
    const char* SERVERADDR_POST3="http://10.0.42.120:8080/RestServelet/RestServelet?failure=";    
    const char* POST2_CONTENT_TYPE= "multipart/form-data; boundary=--AaB03x--";
#else
    
    // get paramter
    const char* SERVERADDR_GET1 = "http://www1.datawind-s.com:8888/RestServelet/RestServelet";
    const char* GET_CONTENT_TYPE= "application/JSON";
	  //const char* SERVERADDR_GET1 = "http://10.0.42.119:8080/RestServelet/RestServelet";
    //const char* GET_CONTENT_TYPE= "application/JSON";

   // Post mac addr
    const char* SERVERADDR_POST1="http://www1.datawind-s.com:8888/RestServelet/RestServelet?mac=";     //?mac=
    const char* POST1_CONTENT_TYPE= "application/text";


    const char* SERVERADDR_POST2="http://www1.datawind-s.com:8888/RestServelet/RestServelet?";    //x&mac=addr

    const char* POST2_CONTENT_TYPE= "multipart/form-data; boundary=--AaB03x--";
    
    
  //SERVERADDR_GET1  const char* UPDATEOTAADDR="http://10.0.42.119:8080/RestServelet/updateota";
  //   const char* UPDATEOTAADDR="http://www1.datawind-s.com:8888"; 
    
    
#endif



//uint32_t WIFISTATE=0 ;
//WiFiManager wifiManager;

/* a timer */
Ticker timer;
 
/*timer for flashing the led*/
Ticker ledtimer;
  
uint32_t STAMP_OF_BEGIN=0;    //
//uint32_t STAMP_OF_END=0;
  
  
uint32_t count = 0;       // counter in a single event
uint32_t countDiff=0;
bool     diffFlag=false;
uint32_t countAver=0;
bool     averFlag=false; 

/* the state whether sending the data to server*/
uint8_t flag;

 

/* the main loop work flag*/
bool CANWORK=false;

bool SENDINGNOW=false;

uint8_t COMMAND =0;
uint8_t TrigerCHs=0;
         
/* the Counter of error when fail to connect to AP or server*/
//uint16_t ErrCounter;
 
 
FSInfo fs_info;

struct TransferInfo
{
  uint32_t TimeStamp;
  uint32_t TimeStampH;
  uint16_t  AlterType;           // 0:yellow alert     1:red alert   2: new 3 :old  4:end
  uint16_t  FinalStatus;         // 0: success         1: failure

}TransferInfo;


static void samplecallback();
static bool isChargingMode(void);
static void ChargingMode();
static uint8_t isNewEvent(void);
static void UpdateLifeSpan(uint32_t newdelta,uint32_t deepleep_us);

static void DumpFailureFile();


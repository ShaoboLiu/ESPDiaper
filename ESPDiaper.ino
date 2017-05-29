

#include "diaper.h"


static uint32_t getVersion()
{
	return VERSION;
}


static void WriteRTCMem(uint32_t addr, uint32_t val)
{
	uint32_t		placeholder = val;

	system_rtc_mem_write(addr, &placeholder, 4);

}


static uint32_t ReadRTCMem(uint32_t addr)
{
	uint32_t		placeholder;

	system_rtc_mem_read(addr, &placeholder, 4);
	return placeholder;
}


static void Mark_Start(void)
{
	STAMP_OF_BEGIN		= system_get_rtc_time();


}


/*
static void TimeZero(void)
{
	uint32_t val=system_get_rtc_time();

	WriteRTCMem(ADDR_OLDRTCTIME,val);
	UpdateLifeSpan(val,0);

}

static void Mark_Start_New(void)
{
	uint32_t oldTime=ReadRTCMem(ADDR_OLDRTCTIME);

	uint32_t val=system_get_rtc_time();

	uint32_t  delta;
	if (val<oldTime)
		delta=0xffffffff-oldTime+val;
	else
		delta=val-oldTime;

	WriteRTCMem(ADDR_OLDRTCTIME,val);

	UpdateLifeSpan(delta,0);

}
*/
static uint32_t Mark_End(void)
{
	return (system_get_rtc_time() -STAMP_OF_BEGIN);
}


static void UpdateLifeSpan(uint32_t newdelta, uint32_t deepleep_us)
{
	uint32_t		old;
	uint32_t		newinterval, temp;
	uint32_t		deltaS;

	float			unit = 5.75;					//us

	system_rtc_mem_read(ADDR_LIFESPAN_US, &old, 4);

	//temp=system_rtc_clock_cali_proc();
	// unit=((temp*1000)>>12)/1000+ ((temp*1000)>>12)%1000;
	newinterval 		= old + round((newdelta * unit) +deepleep_us); //us

	if (newinterval > 1000000)
		{
		deltaS				= newinterval / 1000000;
		system_rtc_mem_read(ADDR_LIFESPAN, &temp, 4);
		temp				+= deltaS;
		system_rtc_mem_write(ADDR_LIFESPAN, &temp, 4);

		temp				= newinterval % 1000000;
		system_rtc_mem_write(ADDR_LIFESPAN_US, &temp, 4);

		}
	else 
		system_rtc_mem_write(ADDR_LIFESPAN_US, &newinterval, 4);


}




static void Clear_RTCMem(int len)
{
	uint32_t		placeholder = 0;

	for (int i = 0; i < len; i++)
		{
		system_rtc_mem_write(STARTOFRTCMEM + i, &placeholder, 4);
		}

}


static void Clear_RTCPreSampleBuffer(void)
{
	uint32_t		placeholder = 0;

	for (int i = 0; i < BUFFERLENGTH; i++)
		{
		system_rtc_mem_write(BEGINADDR + i, &placeholder, 4);
		}

	placeholder 		= BEGINADDR;
	system_rtc_mem_write(ADDR_CURRENTPOINT, &placeholder, 4);

}


template <typename Generic> void DEBUG_LOG(Generic text)
{
#ifdef MYDEBUG
	Serial.print("*LOG:    ");
	Serial.println(text);
#endif

}


template <typename Generic,typename Generic2> void DEBUG_LOG(Generic Display, Generic2 Content)
{
#ifdef MYDEBUG
	Serial.print("*LOG:    ");
	Serial.print(Display);
	Serial.println(Content);
#endif

}


template <typename Generic> void DEBUG_ERROR(Generic text)
{
#ifdef MYDEBUG
	Serial.print("*ERROR:	 ");
	Serial.println(text);
#endif
}


template <typename Generic,typename Generic2> void DEBUG_ERROR(Generic Display, Generic2 Content)
{
#ifdef MYDEBUG
	Serial.print("*ERROR:	 ");
	Serial.print(Display);
	Serial.println(Content);
#endif

}



/*0 = manufacture mode : no saving data in flash
   1 =test mode    : saving data in flash
*/
static uint32_t getRunMode(void)
{
	uint32_t		val;

	system_rtc_mem_read(ADDR_RUNMODE, &val, 4);
	return val;
}


static uint32_t getFreeFlashSize()
{

	SPIFFS.begin();
	SPIFFS.info(fs_info);
	SPIFFS.end();

	int 			left = fs_info.totalBytes -fs_info.usedBytes;

	return left > 0 ? left: 0;

}


static void Clear_Flashfile(void)
{
	uint8_t 		offset = 20;
	char			filename[30];
	char			numevent = 0;


	union holder
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;


	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		}
	else 
		{
		// open the general file

		/* if (SPIFFS.exists(GENERALFILENAME))
			 {

				 DEBUG_LOG("General Number exists:");

				 File f = SPIFFS.open(GENERALFILENAME, "r+");


				 f.seek(OFFSETNUMEVENT,SeekSet);
				 f.readBytes((char*)holder.ch,4);
				 numevent=holder.val;

				 f.seek(OFFSETNUMEVENT,SeekSet);
				 holder.val=0;
				 f.write(holder.ch,4);


				 DEBUG_LOG("General Number is:",numevent);

				 f.close();

				 SPIFFS.remove(GENERALFILENAME);
			 }


		 if (getRunMode()==1)
			 {

				 if (numevent>0)
					 {
						 for (int i =1; i<=numevent; i++)
							 {

								 sprintf(filename,"/data%d",i)	;
								 if (SPIFFS.exists(filename))
									 {
										 //if (SPIFFS.remove(filename))
										 //   {
										 // int temp=0;
										 // system_rtc_mem_read(ADDR_NUMEVENT,&temp,4);
										 // if (temp>0)  temp--;
										 // system_rtc_mem_write(ADDR_NUMEVENT,&temp,4);

										 //  }

										 SPIFFS.remove(filename);
										 DEBUG_LOG("Deleted the file:",filename);


									 }

							 }



					 }
				*/
		SPIFFS.format();


		//TODO
		WriteRTCMem(ADDR_NUMEVENT, 0);

		SPIFFS.end();

		DEBUG_LOG("leaving Clear_Flashfile ");

		}


}


static void InitRTCBuffer(void)
{
	WriteRTCMem(ADDR_CURRENTPOINT, BEGINADDR);

	for (uint32_t i = BEGINADDR; i < ENDOFRTCMEM; i++)
		{
		WriteRTCMem(i, 0);
		}
}


static void Init_RTCMem(void)
{

	WriteRTCMem(ADDR_LIFESPAN, DEFAULT_LIFESPAN);

	uint32_t		temp = system_get_rtc_time();

	WriteRTCMem(ADDR_LIFESPAN_US, temp);
	WriteRTCMem(ADDR_WIFISTATE, DEFAULT_WIFISTATE);

	for (int i = 0; i < 4; i++)
		WriteRTCMem(ADDR_ALLACC + i, DEFAULT_ALLACC);

	WriteRTCMem(ADDR_VOLTAGE, DEFAULT_VOLTAGE);
	WriteRTCMem(ADDR_NUMEVENT, DEFAULT_NUMEVENT);
	WriteRTCMem(ADDR_DIAPERTYPE, DEFAULT_DIAPERTYPE);

	for (int i = 0; i < 4; i++)
		WriteRTCMem(ADDR_THRESHOLD + i, DEFAULT_THRESHOLD);

	WriteRTCMem(ADDR_MAXNUMEVENT, DEFAULT_MAXNUMEVENT);
	WriteRTCMem(ADDR_MAXHOURS, DEFAULT_MAXHOURS);
	WriteRTCMem(ADDR_RUNMODE, DEFAULT_RUNMODE);
	WriteRTCMem(ADDR_CURRENTPOINT, BEGINADDR);
	WriteRTCMem(ADDR_ONLINEMODE, DEFAULT_ONLINEMODE);
	WriteRTCMem(ADDR_THRESHOLDFLAG, DEFAULT_THRESHOLDFLAG);
	WriteRTCMem(ADDR_NUMEVENTFLAG, DEFAULT_NUMEVENTFLAG);

	for (int i = 0; i < 4; i++)
		WriteRTCMem(ADDR_MOVINGAVERAGE + i, DEFAULT_MOVINGAVERAGE);

}


static void InitChs(void)
{
	for (int i = 0; i < (sizeof(CHS) / sizeof(CHS[0])); i++)
		pinMode(CHS[i], INPUT);

}


static uint32_t ReadSingleCh(unsigned int ch)
{
	uint16_t		v1, v2;
	uint32_t		val;

	/* reset all chs to input */
	InitChs();

	/* change the select ch to output */
	pinMode(ch, OUTPUT);
	pinMode(0, OUTPUT);

	digitalWrite(ch, HIGH);
	digitalWrite(0, HIGH);


#if 0
	v1					= analogRead(A0);
	digitalWrite(ch, LOW);
	v2					= analogRead(A0);

#else

	/*just for test*/
	v1					= analogRead(A0);

	if (ch == 12)
		pinMode(ch, INPUT);
	else 
		digitalWrite(ch, LOW);

	v2					= analogRead(A0);

#endif

	// val=((v1<<16)+v2);
	val 				= ((v2 << 16) +v1);
	return val;

}


static bool CheckThreshold(void)
{
	uint32_t		temp;

	for (int i = 0; i < sizeof(SUM) / sizeof(SUM[0]); i++)
		{
		SUM[i]				= ReadRTCMem(ADDR_ALLACC + i);
		temp				= ReadRTCMem(ADDR_THRESHOLD + i);

		if ((SUM[i] > temp) && (0 == ReadRTCMem(ADDR_THRESHOLDFLAG)))
			{
			return true;
			}
		}

	return false;
}


// 2: new event that embedded in a event that is progressing
// 1: old fashion end
// 0: no end of event
static uint8_t isEndofEvent()
{
	std::vector < uint32_t >::size_type pos = VALUES.size() - 1;

	if (isNewEvent() > 0)
		{
		if (ReadRTCMem(ADDR_PENDEDEVENTTIME) == 0)
			{
			uint32_t		lspan = ReadRTCMem(ADDR_LIFESPAN);

			WriteRTCMem(ADDR_PENDEDEVENTTIME, lspan);

			return 2;
			}

		else 
			{
			uint32_t		lspan = ReadRTCMem(ADDR_LIFESPAN);
			uint32_t		startTime = ReadRTCMem(ADDR_PENDEDEVENTTIME);

			if (lspan - startTime < 16)
				return 0;

			else 
				WriteRTCMem(ADDR_PENDEDEVENTTIME, 0);
			}

		}

	//if ((isNewEvent()>0)&&(count>TIME_WINDOW_WIDTH-1)) return 2;
	if (pos >= 40)
		{
		for (int i = pos; i > pos - 40; i--)
			if ((VALUES[i] &0xffff) > THRESHOLD_OF_END_EVENT)
				return 0;

		return 1;
		}

	return 0;


}


static uint32_t ReadChs(void)
{
	uint32_t		temp[4], oldAve[4], newAve[4], oldMaxAver[4], timestamp;
	uint32_t		numevent;
	char			filename[30];
	int 			diff[4], oldMax[4];

	/*	old solution*/
	Mark_Start();
	UpdateLifeSpan(Mark_End(), 1000000);
	system_rtc_mem_read(ADDR_LIFESPAN, &timestamp, 4);

#ifdef MYDEBUG
	Serial.print(timestamp);
	Serial.print("\t");
	Serial.print(count);
	Serial.print("\t");
#endif

	/* read all channels  and calc current event acc*/
	for (int i = 0; i < sizeof(CHS) / sizeof(CHS[0]); i++)
		{
		temp[i] 			= ReadSingleCh(CHS[i]);
		VALUES.push_back(temp[i]);

		//calulate the new diff
		oldAve[i]			= ReadRTCMem(ADDR_MOVINGAVERAGE + i);
		oldMaxAver[i]		= ReadRTCMem(ADDR_MAXAVER + i);
		oldMax[i]			= ReadRTCMem(ADDR_MAXDIFF + i);

		newAve[i]			= ((oldAve[i] << 4) -oldAve[i] + (temp[i] &0xffff)) >> 4;
		diff[i] 			= (temp[i] &0xffff) -newAve[i];

		if (diff[i] > oldMax[i])
			WriteRTCMem((ADDR_MAXDIFF + i), diff[i]);

		if (newAve[i] > oldMaxAver[i])
			WriteRTCMem((ADDR_MAXAVER + i), newAve[i]);

		WriteRTCMem((ADDR_MOVINGAVERAGE + i), newAve[i]);
		WriteRTCMem((ADDR_DIFF + i), diff[i]);

#ifdef MYDEBUG
		Serial.print(diff[i]);
		Serial.print("\t");
		Serial.print((temp[i] >> 16) & 0xffff);
		Serial.print("\t");
		Serial.print((temp[i]) & 0xffff);
		Serial.print("\t");
#endif

		INTEGRATION[i % 4]	+= (temp[i] &0xffff);
		SUM[i]				+= (temp[i] &0xffff);

		if ((temp[i] &0xffff) > MAX[i])
			MAX[i] = temp[i] &0xffff;

		}


	/* update the ALLACC*/
	for (int i = 0; i < sizeof(SUM) / sizeof(SUM[0]); i++)
		{
		WriteRTCMem(ADDR_ALLACC + i, SUM[i]);
		}


#ifdef MYDEBUG
	Serial.println(" ");
#endif

	//save this page to flash
	//todo:add	check the remaining space
	uint32_t		freespace = getFreeFlashSize();
	uint32_t size=VALUES.size();

    DEBUG_LOG( "Vector size:",size);
	if (size>= DATASIZE)
		{

		if (1 == getRunMode() && freespace > 5120)
			{
			// timer.detach();
			bool			result = SPIFFS.begin();
			if (false == result)
				{
				DEBUG_ERROR("SPIFFS.begin failed! ");
				}
			else 
				{

				system_rtc_mem_read(ADDR_NUMEVENT, &numevent, 4);
				sprintf(filename, "/data%d", numevent);

				File			f	= SPIFFS.open(filename, "a");

				DEBUG_LOG("Saving data into FileName: ", filename);
				DEBUG_LOG("Free Flash Space:", freespace);

				for (auto item: VALUES)
					{
					f.write(item & 0xff);
					f.write((item >> 8) & 0xff);
					f.write((item >> 16) & 0xff);
					f.write((item >> 24) & 0xff);
					}

				f.close();

				SPIFFS.end();
				}

			//timer.attach(DEEPSLEEPSAMPLE,samplecallback);
			}

		VALUES.clear();

		}

  
}


static void UpdataWifiStateInRtcMem(int val)
{

	WriteRTCMem(ADDR_WIFISTATE, val);

}



static uint32_t getThreshold(uint8_t i) //i=0-3
{

	return ReadRTCMem(ADDR_THRESHOLD + i);

}


#ifdef MYDEBUG


static void PrintPreSampleBuffer(void)
{
	uint32_t		addr, val, distance;

	system_rtc_mem_read(ADDR_CURRENTPOINT, &addr, 4);

	distance			= addr - BEGINADDR;

	if (distance % 4 != 0)
		Serial.println("ERROR: Presample buffer alignment error!");

	Serial.println("======Presample buffer	Output=======");


	for (int i = addr; i < ENDADDR; i++)
		{


		system_rtc_mem_read(i, &val, 4);
		uint16_t		high = (val >> 16) & 0xffff;
		uint16_t		low = (val) & 0xffff;

		Serial.println("====== ");
		Serial.print(high);
		Serial.print("	  ");
		Serial.print(low);
		Serial.print("		");

		}

	Serial.println(" ");

	for (int i = BEGINADDR; i < addr; i++)
		{
		system_rtc_mem_read(i, &val, 4);
		uint16_t		high = (val >> 16);
		uint16_t		low = (val) & 0xffff;

		Serial.print(high);
		Serial.print("	  ");
		Serial.print(low);
		Serial.print("		");

		Serial.println("====== ");

		}

	Serial.println("======Presample buffer end =======");
}


#endif


static int Recordfailure(struct TransferInfo *info)
{
	char			filename[30];

	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		return (-1);

		}
	else 
		{

		File			f	= SPIFFS.open(FAILUREFILENAME, "a");

	    f.write(info->TimeStamp & 0xff);
		f.write((info->TimeStamp >> 8) & 0xff);
		f.write((info->TimeStamp >> 16) & 0xff);
		f.write((info->TimeStamp >> 24) & 0xff);

	    f.write(info->TimeStampH & 0xff);
		f.write((info->TimeStampH >> 8) & 0xff);
		f.write((info->TimeStampH >> 16) & 0xff);
		f.write((info->TimeStampH >> 24) & 0xff);

		f.write(info->AlterType & 0xff);
		f.write((info->AlterType >> 8) & 0xff);

		f.write(info->FinalStatus & 0xff);
		f.write((info->FinalStatus >> 8) & 0xff);
		

		f.close();

		SPIFFS.end();


#ifdef MYDEBUG
		DumpFailureFile();
#endif


		return 0;
		}


}

/* process the data and make a conclusion ,return a flag if sensing the event*/
static uint8_t ReadAndProcessData(void)
{
	int 			flag;
	uint32_t		val[4], newAve[4], oldAve[4];
	uint32_t		addr, temp;
	int 			diff[4];

	flag				= 0;

	for (int i = 0; i < sizeof(CHS) / sizeof(CHS[0]); i++)
		{
		val[i]				= ReadSingleCh(CHS[i]);

		oldAve[i]			= ReadRTCMem(ADDR_MOVINGAVERAGE + i);
		newAve[i]			= ((oldAve[i] << 4) -oldAve[i] + (val[i] &0xffff)) >> 4;
		diff[i] 			= (val[i] &0xffff) -newAve[i];

		WriteRTCMem((ADDR_MOVINGAVERAGE + i), newAve[i]);
		WriteRTCMem((ADDR_DIFF + i), diff[i]);

		addr				= ReadRTCMem(ADDR_CURRENTPOINT);
		WriteRTCMem(addr, val[i]);
		addr++;

		if (addr == ENDADDR)
			addr = BEGINADDR;

		WriteRTCMem(ADDR_CURRENTPOINT, addr);

#ifdef MYDEBUG
		Serial.print((val[i] >> 16) & 0xffff);
		Serial.print("	 ");
		Serial.print(val[i] &0xffff);
		Serial.print("	 ");
		Serial.println(diff[i]);
#endif

		// if (val[i]>=getThreshold(i)) flag++;
		if ((val[i] &0xffff) >= EVENTTHRESHOLD)
			flag++;

		//if (diff[i]>=EVENTTHRESHOLD) flag++;
		}

	DEBUG_LOG(" ");
	return flag;


}



static uint32_t getNumberEvent(void)
{
	union holder
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;


	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");

		}
	else 
		{

		holder.val			= 0;

		if (SPIFFS.exists(GENERALFILENAME))
			{
			File			f	= SPIFFS.open(GENERALFILENAME, "r+");

			holder.val			= 0;
			f.seek(OFFSETNUMEVENT, SeekSet);
			f.readBytes((char *) holder.ch, 4);

			DEBUG_LOG("The NUMEVENT field in GeneralFile =", holder.val);

			f.close();

			}

		SPIFFS.end();



		}

	return holder.val > 0 ? holder.val: 0;



}



static int RecordFilesize()
{
  
	    uint32_t		numevent;
		char			filename[30];
	
		numevent			= getNumberEvent();
	
		bool			result = SPIFFS.begin();
		if (false == result)
			{
			DEBUG_ERROR("SPIFFS.begin failed! ");
			return DIAPER_MOUNT_FS_FAILURE;
			}
		else 
			{
			
					for (uint32_t i = 1; i <= numevent; i++)
						{
						sprintf(filename, "/data%d", i);
						if (SPIFFS.exists(filename))
							{
							   File f=SPIFFS.open(filename,"r+");
							   
							   TransferInfo.AlterType=7;
							   TransferInfo.FinalStatus=int(f.size()/1024);
							   UpdateLifeSpan(Mark_End(), 0);
							   TransferInfo.TimeStamp=ReadRTCMem(ADDR_LIFESPAN);
							   TransferInfo.TimeStampH=i;
							   f.close();

							   
								f	= SPIFFS.open(FAILUREFILENAME, "a");

							    f.write(TransferInfo.TimeStamp & 0xff);
								f.write((TransferInfo.TimeStamp >> 8) & 0xff);
								f.write((TransferInfo.TimeStamp >> 16) & 0xff);
								f.write((TransferInfo.TimeStamp >> 24) & 0xff);

							    f.write(TransferInfo.TimeStampH & 0xff);
								f.write((TransferInfo.TimeStampH >> 8) & 0xff);
								f.write((TransferInfo.TimeStampH >> 16) & 0xff);
								f.write((TransferInfo.TimeStampH >> 24) & 0xff);

								f.write(TransferInfo.AlterType & 0xff);
								f.write((TransferInfo.AlterType >> 8) & 0xff);

								f.write(TransferInfo.FinalStatus & 0xff);
								f.write((TransferInfo.FinalStatus >> 8) & 0xff);
								

								f.close();
							   
							}
						else 
							{
							DEBUG_ERROR(" Data File NOT Exists");
							DEBUG_LOG(filename);
							return	 DIAPER_FILE_NOT_EXIST;
							}
	
	
						}
	
		   }
	
        
		SPIFFS.end(); 

}



// 0:   YEL   1:RED      2: new 3 :old  4:end    5:CHR     
static char * Message(uint16_t flagtype)
{
    if (0==flagtype)
		return "YEL";
	
	else if	(1==flagtype)
		return "RED";
	
	else if (2==flagtype)
		return "NEW";
	
    else if (3==flagtype)
		return "OLD";
	
	else if (4==flagtype)
		return "END";
	
	else if (5==flagtype )
		return "CHR"; 
	
	else if (8==flagtype)
		return "UDT";

}

/*
   return value:
   0: success
   1: failure
*/
//static int SendNewFlag(String str, uint16_t chs)
static int SendNewFlag(uint16_t flagtype , uint32_t chs)

{

	HTTPClient		http;
	uint8_t 		mac[6];
	char			macStr[28];
	struct  TransferInfo   ti;

    UpdateLifeSpan(Mark_End(), 0);
	ti.TimeStamp=ReadRTCMem(ADDR_LIFESPAN);
	ti.TimeStampH=0;
	ti.AlterType=flagtype;

	String str=Message(flagtype);
    DEBUG_LOG("type",str);
	
	wifi_get_macaddr(STATION_IF, mac);
	sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

   

	String addr(SERVERADDR_POST1);

	addr				= addr + String(macStr) + "&chip_id=" + String(ESP.getChipId()) + "&chs=" + String(chs)+"&ss="+String(WiFi.RSSI());

	http.setReuse(true);
	http.begin(addr);

	DEBUG_LOG(addr);

	http.addHeader("Content-Type", POST1_CONTENT_TYPE);

	int 			postcode = http.POST(str);

	yield();

	int 			ErrorCount = 0;
	int 			result = DIAPER_SUCCESS;

	while (postcode != 200)
		{
		uint32_t		maxtry;

		delay(ERRORDELAYMS);
		postcode			= http.POST(str);
		yield();
		DEBUG_LOG("POSTCODE=", postcode);

		ErrorCount++;

		if (isChargingMode())
			maxtry = MAXERRORTRYCHMODE;
		else 
			maxtry = MAXERRORTRY;


		if (maxtry == ErrorCount)
			{
			result				= DIAPER_FAILURE;
			WriteRTCMem(ADDR_ONLINEMODE, 0);
			http.end();

			ti.FinalStatus=result;
			Recordfailure(&ti);
			RecordFilesize();
			return result;
			}

		}

	http.end();
	ti.FinalStatus=DIAPER_SUCCESS;
	Recordfailure(&ti);

    RecordFilesize();
	
	return DIAPER_SUCCESS;


}



static void ApplyDefaultParams()
{
	uint32_t		temp;

	/*
	"parameter_id":"1",
	"diaper_type":1,
	"voltage":0,
	"max_events":2,
	"max_hours":0,
	"threshold":1,
	"test_mode":1,
	"last_update":"2017-01-23 10:54:42.0"}
	*/
	//set diaper default paramter
	temp				= 0;
	WriteRTCMem(ADDR_DIAPERTYPE, temp);

	temp				= 0;
	WriteRTCMem(ADDR_VOLTAGE, temp);

	temp				= 10000000;

	for (int i = 0; i < 4; i++)
		WriteRTCMem(ADDR_THRESHOLD + i, temp);

	//76
	temp				= 10;
	WriteRTCMem(ADDR_MAXNUMEVENT, temp);

	//77
	temp				= 500000;
	WriteRTCMem(ADDR_MAXHOURS, temp);

	//79
	temp				= 1;						/*test mode*/
	WriteRTCMem(ADDR_RUNMODE, temp);

}


static void do_UpdateDiaperType(uint32_t diapertype)
{
	uint32_t		val = 0;

	if (diapertype > 6)
		val = 0;
	else 
		val = diapertype;

	WriteRTCMem(ADDR_DIAPERTYPE, val);

	DEBUG_LOG("Update the ADDR_DIAPERTYPE:", val);

}


static void do_UpdateVoltage(uint32_t voltage)
{
	uint32_t		val;

	if (voltage > 4)
		val = 0;
	else 
		val = voltage;

	WriteRTCMem(ADDR_VOLTAGE, val);

	DEBUG_LOG("Update the ADDR_VOLTAGE:", val);


}


static void do_UpdateTestMode(uint32_t test_mode)
{
	uint32_t		val;

	if (test_mode > 1)
		val = 1;
	else 
		val = test_mode;

	WriteRTCMem(ADDR_RUNMODE, val);

	DEBUG_LOG("Update the ADDR_RUNMODE:", val);


}


static void do_UpdateThreshold(uint32_t threshold)
{
	uint32_t		val;


	val 				= threshold;

	for (int i = 0; i < 4; i++)
		{
		WriteRTCMem(ADDR_THRESHOLD + i, val);

		}


	DEBUG_LOG("Update the ADDR_THRESHOLD:", val);



}


static void do_UpdateMaxevent(uint32_t maxevent)
{
	uint32_t		val;

	if (maxevent > 20)
		val = 10;
	else 
		val = maxevent;


	WriteRTCMem(ADDR_MAXNUMEVENT, val);

	DEBUG_LOG("Update the ADDR_MAXNUMEVENT:", val);

}


static void do_UpdateMaxhours(uint32_t maxhour)
{
	uint32_t		val = 50000;

	/*
	   if ((maxhour>10) or (maxhour<1))
		   val=1;
	   else
		   val=maxhour;
	 */
	WriteRTCMem(ADDR_MAXHOURS, val);

	DEBUG_LOG("Update the ADDR_MAXHOURS:", val);

}

static bool parseUserData(char * content)
{
	//TODO verify each params must be in a reasonable range

	/*
	"parameter_id":"1",
	"diaper_type":1,
	"voltage":0,
	"max_events":2,
	"max_hours":0,
	"threshold":1,
	"test_mode":1,
	"last_update":"2017-01-23 10:54:42.0"}
	*/

	
	const size_t	BUFFER_SIZE = JSON_OBJECT_SIZE(8);

	// Allocate a temporary memory pool on the stack
	StaticJsonBuffer < BUFFER_SIZE > jsonBuffer;

	JsonObject &	root = jsonBuffer.parseObject(content);

	if (!root.success())
		{
		Serial.println("JSON parsing failed!");
		return false;
		}

	//diapertype
	uint32_t		diapertype = root["diaper_type"];

	do_UpdateDiaperType(diapertype);

	//voltage
	uint32_t		voltage = root["voltage"];

	do_UpdateVoltage(voltage);

	//test_mode
	uint32_t		test_mode = root["test_mode"];

	do_UpdateTestMode(test_mode);


	//threshold
	uint32_t		threshold = root["threshold"];

	do_UpdateThreshold(threshold);


	//max_events
	uint32_t		maxevent = root["max_events"];

	do_UpdateMaxevent(maxevent);

	//max_hours
	uint32_t		maxhours = root["max_hours"];
    WriteRTCMem(ADDR_MAXHOURS, maxhours) ;        //save lower timestamp from server   
	do_UpdateMaxhours(maxhours);


	
    uint32_t		lastupdate = root["last_update"];
	WriteRTCMem(ADDR_NUMEVENTFLAG, lastupdate);  //save upper timestamp from server

	TransferInfo.TimeStamp=maxhours;
	TransferInfo.TimeStampH=lastupdate;
	TransferInfo.AlterType=6;

 	UpdateLifeSpan(Mark_End(), 0);
	TransferInfo.FinalStatus= ReadRTCMem(ADDR_LIFESPAN);
	
	Recordfailure(&TransferInfo);
  
	return true;
}



static bool ReceiveConfig(void)
{
	uint32_t		paramter = 0;
	HTTPClient		http;


	if (ReadRTCMem(ADDR_ONLINEMODE) == 0)
		return false;

	http.begin(SERVERADDR_GET1);

	int 			httpCode = http.GET();

	while (httpCode <= 0)
		{
		delay(100);
		httpCode			= http.GET();
		DEBUG_LOG(httpCode);
		}

	bool			result = true;

	if (httpCode > 0)
		{
		// HTTP header has been send and Server response header has been handled
		DEBUG_LOG("[HTTP] GET... code:	", httpCode);

		// file found at server
		if (httpCode == HTTP_CODE_OK)
			{
			String			payload = http.getString();

			DEBUG_LOG(payload);
			result				= parseUserData(const_cast < char * > (payload.c_str()));

			}
		}
	else 
		{

		DEBUG_LOG("[HTTP] GET... failed, error: ", http.errorToString(httpCode).c_str());

		result				= false;
		}

	http.end();
	return result;

}


static int sendSingleDataFile(char * filename, uint32_t num, uint8_t filetype)
{
	uint32_t		len = 0;
	char			container[TRANSFERBUFF];
	int 			bulk = 0, left;
	uint32_t		i	= 0;
	uint32_t		ErrorCount = 0;

    int maxtry ;
	


	File			f	= SPIFFS.open(filename, "r");

	len 				= f.size();
	DEBUG_LOG("FILE SIZE=");
	DEBUG_LOG(len);


	bulk				= len / (TRANSFERBUFF);
	left				= len % (TRANSFERBUFF);


	HTTPClient		http;
	uint8_t 		mac[6];
	char			macStr[28] ;
		

	wifi_get_macaddr(STATION_IF, mac);
	sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	String addr(SERVERADDR_POST2);

	if (filetype == 0)
		addr = addr + "data=";
	else 
		addr = addr + "failure=";

	addr				= addr + String(num) + "&mac=" + String(macStr) + "&chip_id=" + String(ESP.getChipId())+"&len="+String(len);
    DEBUG_LOG("addr of Datafile=",addr);
	http.setReuse(true);
	http.begin(addr);
	http.addHeader("content-type", POST2_CONTENT_TYPE);
	
	

    int			result = DIAPER_SUCCESS;
	for (int i = 0; i < bulk; i++)
		{
		f.seek(i * TRANSFERBUFF, SeekSet);
		f.readBytes(container, TRANSFERBUFF);

		int 			postcode = http.POST((uint8_t *) container, TRANSFERBUFF);
		yield();
		ErrorCount			= 0;

		while (postcode != 200)
			{
			uint32_t		maxtry;

			delay(100);
			postcode			= http.POST((uint8_t *) container, TRANSFERBUFF);
			yield();
			DEBUG_LOG(postcode);

			ErrorCount++;

			if (isChargingMode())
				maxtry = MAXERRORTRYCHMODE;
			else 
				maxtry = MAXERRORTRY;


			if (maxtry == ErrorCount)
				{
				result				= DIAPER_FAILURE;
				f.close();
				http.end();
				WriteRTCMem(ADDR_ONLINEMODE, 0);
				return result;
				}

			}


		}


	if (left > 0)
		{
		f.readBytes(container, left);
		int 			postcode = http.POST((uint8_t *) container, left);

		yield();
		
		ErrorCount			= 0;
		while (postcode != 200)
			{
			delay(100);
			postcode			= http.POST((uint8_t *) container, left);
			yield();
			DEBUG_LOG(postcode);

			ErrorCount++;

			if (isChargingMode())
				maxtry = MAXERRORTRYCHMODE;
			else 
				maxtry = MAXERRORTRY;

			if (maxtry == ErrorCount)
				{
				result				= DIAPER_FAILURE;
				f.close();
				http.end();
				WriteRTCMem(ADDR_ONLINEMODE, 0);
				return result;
				}

			}
		}

	f.close();

	http.end();

	return result;


}


static int SendFailureFile()
{

	HTTPClient		http;
	uint32_t		len;
	uint8_t 		num = 0;
	char *			p	= NULL;

	
	uint8_t 		mac[6];
	char			macStr[28] ;

	if (SPIFFS.exists(FAILUREFILENAME))
	{
	File			f	= SPIFFS.open(FAILUREFILENAME, "r+");
	wifi_get_macaddr(STATION_IF, mac);
	sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	
	
	String addr(SERVERADDR_POST2);
	
	addr = addr + String("data=") +String("00&mac=") +String(macStr) + "&chip_id=" + String(ESP.getChipId())+"&len="+String(len);
    DEBUG_LOG("addr of failure=",addr);
	http.setReuse(true);
	http.begin(addr);
	http.addHeader("content-type", "multipart/form-data; boundary=--AaB03x--");

    
	len=f.size();
	p=(char*)malloc(len);
	f.seek(0, SeekSet);
	f.readBytes(p, len);
	DEBUG_LOG("File size=",len);
	
	int 			postcode = http.POST((uint8_t *) p, len);
	
	yield();
	DEBUG_LOG("POST CODE:");
	DEBUG_LOG(postcode);
	
	uint8_t 		ErrorCount = 0;
	
	int 			result = DIAPER_SUCCESS;
	
	
	while (postcode != 200)
		{
		uint32_t		maxtry;
	
		delay(ERRORDELAYMS);
		postcode			= http.POST((uint8_t *) p, len);
		yield();
		DEBUG_LOG(postcode);
	
		ErrorCount++;
	
		if (isChargingMode())
			maxtry = MAXERRORTRYCHMODE;
		else 
			maxtry = MAXERRORTRY;
	
		if (ErrorCount == maxtry)
			{
			result				= DIAPER_FAILURE;
			free(p);
			f.close();
			http.end();
			return result;
	
			}

		}
	
	
	free(p);
	p = NULL;
	
	f.close();
	http.end();
	
	return result;
	
	}



}

static int SendGeneralFile()
{
	uint32_t		len;
	uint8_t 		num = 0;
	char *			p	= NULL;


	union holder
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;


	if (SPIFFS.exists(GENERALFILENAME))
		{
		File			f	= SPIFFS.open(GENERALFILENAME, "r+");

		f.seek(OFFSETNUMEVENT, SeekSet);
		f.readBytes((char *) holder.ch, 4);

		len 				= LEN_HEADSECT + (holder.val) *LEN_DATAFILESECTION;
		p					= (char *)malloc(len);
		f.seek(0, SeekSet);
		f.readBytes(p, len);

		HTTPClient		http;

		uint8_t 		mac[6];
		char			macStr[28] ;
			
		wifi_get_macaddr(STATION_IF, mac);
		sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


		String addr(SERVERADDR_POST2);

		addr				= addr + String("data=") +String("0&mac=") +String(macStr) + "&chip_id=" + String(ESP.getChipId())+"&len="+String(len);
        DEBUG_LOG("addr of gen=",addr);
		http.setReuse(true);
		http.begin(addr);
		http.addHeader("content-type", "multipart/form-data; boundary=--AaB03x--");


		int 			postcode = http.POST((uint8_t *) p, len);
		yield();
		DEBUG_LOG("POST CODE:");
		DEBUG_LOG(postcode);

		uint16_t 		ErrorCount = 0;
		int 			result = DIAPER_SUCCESS;


		while (postcode != 200)
			{
			uint32_t		maxtry;
			delay(ERRORDELAYMS);
			postcode	= http.POST((uint8_t *) p, len);
			yield();
			DEBUG_LOG(postcode);

			ErrorCount++;
			DEBUG_LOG("count=",ErrorCount);

			if (isChargingMode())
				maxtry = MAXERRORTRYCHMODE;
			else 
				maxtry = MAXERRORTRY;

			if (ErrorCount == maxtry)
				{
				result				= DIAPER_FAILURE;
				free(p);
				f.close();
				http.end();
				return result;

				}






			}

		free(p);
		p					= NULL;

		f.close();
		http.end();

		return result;

		}



}



/*
   return value:
   0: success
   1: failure
*/
static int SendData()
{
	uint32_t		numevent;
	char			filename[30];

	numevent			= getNumberEvent();

	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		return DIAPER_MOUNT_FS_FAILURE;
		}
	else 
		{
		if (DIAPER_FAILURE == SendGeneralFile())
			return DIAPER_SENDFILE_ERROR;

		if (DIAPER_FAILURE == SendFailureFile())
			return DIAPER_SENDFILE_ERROR;
		


			DEBUG_LOG("Numevent=", numevent);

			if (1 == getRunMode())
				{
				for (uint32_t i = 1; i <= numevent; i++)
					{
					sprintf(filename, "/data%d", i);
					if (SPIFFS.exists(filename))
						{
						if (DIAPER_FAILURE == sendSingleDataFile(filename, i, 0))
							return DIAPER_SENDFILE_ERROR;
						}
					else 
						{
						DEBUG_ERROR(" Data File NOT Exists");
						DEBUG_LOG(filename);
						//return   DIAPER_FILE_NOT_EXIST;
						}


					}





			}



		if (DIAPER_FAILURE== SendNewFlag(4, VERSION))   //end=4 message    
			 return DIAPER_SENDALERT_ERROR;

		WriteRTCMem(ADDR_NUMEVENT, 0);

		SPIFFS.end();

		return DIAPER_SUCCESS;


		}



}


static void ledCallback()
{
	if (count % 2 == 0)
		digitalWrite(LEDPIN, HIGH);
	else 
		digitalWrite(LEDPIN, LOW);

	count++;
}


static void APModeCallback(WiFiManager * wm)
{
	count				= 0;
	ledtimer.attach(1.5, ledCallback);
}


static void SetConnCallback(void)
{

	count				= 0;
	ledtimer.attach(0.2, ledCallback);
}


static void ConnFailCallback(void)
{
	count				= 0;
	ledtimer.attach(1.5, ledCallback);

}



static boolean EstablishConn(bool flashled)
{
	WiFiManager 	wifiManager;

	/* enable wifi */
	WiFi.forceSleepWake();
	delay(500);

	/* work as a station*/
	WiFi.mode(WIFI_STA);


	DEBUG_LOG("We are here in EstablishConn  ");

	if (flashled)
		{
		pinMode(LEDPIN, OUTPUT);					/*	   MTMS=GPIO14 */
		count				= 0;
		ledtimer.attach(0.2, ledCallback);
		}

	if (isChargingMode())
		{
		//wifiManager.resetSettings();
		wifiManager.setConfigPortalTimeout(1000000000);
		wifiManager.setAPCallback(APModeCallback);
		wifiManager.setConnFailConfigCallback(ConnFailCallback);
		wifiManager.setPreConnCallback(SetConnCallback);
		}
	else 
		{
		//wifiManager.resetSettings();
		//wifiManager.setConnectTimeout(8);
		wifiManager.setConfigPortalTimeout(1);
		}

	bool			result = wifiManager.autoConnect();

	UpdateLifeSpan(Mark_End(), 0);


	if (isChargingMode())
		{
		int 			i	= 1;
		bool			result;

		while (!result )
			{
			delay(5000);
			i++;
			result				= wifiManager.autoConnect();
			delay(5000);
			UpdateLifeSpan(Mark_End(), 0);
			if (2 == i)
				break;
			}

		}


	if (!result)
		{
		DEBUG_LOG("failed to connect the AP or hit timeout ,enter into offline mode!");

		// if (flashled&&(!isChargingMode()))
		if (flashled)
			{
			ledtimer.detach();
			digitalWrite(LEDPIN, LOW);

			//pinMode(LEDPIN, INPUT);
			count				= 0;
			}

		if (isChargingMode())
			{
			count				= 0;

			//ledtimer.attach(0.1,ledCallback);
			while (1)
				{
				delay(10);

				if (count == 6)
					{
					count				= 0;
					digitalWrite(LEDPIN, LOW);
					ledtimer.detach();
					break;
					}

				}

			}


		WriteRTCMem(ADDR_ONLINEMODE, 0);
		return false; // offline mode


		}
	else 
		{

		DEBUG_LOG("Enter into online mode");

		if (flashled)
			{

			ledtimer.detach();
			digitalWrite(LEDPIN, LOW);

			// pinMode(LEDPIN, INPUT);
			delay(100);
			count				= 0;
			}

		WriteRTCMem(ADDR_ONLINEMODE, 1);

		return true; //online mode

		}



}


static uint8_t InitGeneralFile(void)
{
	uint32_t		temp;

	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		}
	else 
		{

		File			f	= SPIFFS.open(GENERALFILENAME, "w");

		f.seek(0, SeekSet);

		//chip id
		temp				= ESP.getChipId();
		f.write(temp & 0xff);
		f.write((temp >> 8) & 0xff);
		f.write((temp >> 16) & 0xff);
		f.write((temp >> 24) & 0xff);

		//ALLACC
		for (int i = 0; i < 4; i++)
			{
			temp				= 0;
			f.write(temp & 0xff);
			f.write((temp >> 8) & 0xff);
			f.write((temp >> 16) & 0xff);
			f.write((temp >> 24) & 0xff);

			}

		//num event
		temp				= 0;
		f.write((temp) & 0xff);
		f.write(((temp) >> 8) & 0xff);
		f.write(((temp) >> 16) & 0xff);
		f.write(((temp) >> 24) & 0xff);



		temp				= ReadRTCMem(ADDR_RUNMODE); //test mode
		f.write((temp) & 0xff);
		f.write(((temp) >> 8) & 0xff);
		f.write(((temp) >> 16) & 0xff);
		f.write(((temp) >> 24) & 0xff);

		temp				= LEN_HEADSECT; 		//data	offset in general file
		f.write((temp) & 0xff);
		f.write(((temp) >> 8) & 0xff);
		f.write(((temp) >> 16) & 0xff);
		f.write(((temp) >> 24) & 0xff);

		f.close();

		SPIFFS.end();

		}

}


static void DumpFailureFile()
{
    
	union 
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;

		union 
		{
		uint8_t 		ch[2];
		uint16_t		val;
		} holder2;

	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		}
	else 
		{
		 

		if (SPIFFS.exists(FAILUREFILENAME))
			{
			File	f	= SPIFFS.open(FAILUREFILENAME, "r+");

			uint32_t filesize= f.size();

			uint32_t round=filesize/sizeof(struct TransferInfo);

            DEBUG_LOG("filesize= ",filesize);
			DEBUG_LOG("struct size= ",sizeof(struct TransferInfo));

			DEBUG_LOG("=========DUMP Failure========");
			f.seek(0, SeekSet); 
			
			for (int i=0 ;i<round ;i++)
			{
			  DEBUG_LOG("[");
			  //time stamp1 
			  f.readBytes((char *) holder.ch, 4);
			  DEBUG_LOG("TS1:", holder.val);
			  
			  
			  //time stamp2 
			  f.readBytes((char *) holder.ch, 4);
			  DEBUG_LOG("TS2:", holder.val);


              
			  f.readBytes((char *) holder2.ch, 2);
			  DEBUG_LOG("Type:",holder2.val);


			  f.readBytes((char *) holder2.ch, 2);
			  DEBUG_LOG("result:",holder2.val);
			  /*
			  if (val==0)
			      DEBUG_LOG("result:", "0");
			  else
			  	  DEBUG_LOG("result:", "1");
			  	  */
              DEBUG_LOG("]");
			  }
		
		
			f.close();
			
		}
		else 
				{
				DEBUG_LOG("GeneralFile nonexist.");
				}
			
	

			
	}
		SPIFFS.end();

}


static void DumpgeneralFile()
{
	union 
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;


	uint32_t		numevent;

	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		}
	else 
		{
		if (SPIFFS.exists(GENERALFILENAME))
			{
			File			f	= SPIFFS.open(GENERALFILENAME, "r+");

			DEBUG_LOG("=========DUMP General========");
			f.seek(0, SeekSet);

			//id
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("ID:", holder.val);


			//ACC0-3
			f.seek(4, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("ACC0:", holder.val);


			f.seek(8, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("ACC1:", holder.val);


			f.seek(12, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("ACC2:", holder.val);


			f.seek(16, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("ACC3:", holder.val);


			//num event
			f.seek(20, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("NUM Event:", holder.val);

			numevent			= holder.val;

			//Test Mode
			f.seek(24, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("Test Mode:", holder.val);


			//offset data summary
			f.seek(28, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("data Summary offset:", holder.val);

			for (int i = 1; i <= numevent; i++)
				{
				f.seek((LEN_HEADSECT + 48 * (i - 1)), SeekSet);

				//MAX
				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("MAX0:", holder.val);

				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("MAX1:", holder.val);

				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("MAX2:", holder.val);

				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("MAX3:", holder.val);


				//INTEGRATION
				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("INTEGRATION0:", holder.val);

				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("INTEGRATION1:", holder.val);


				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("INTEGRATION2:", holder.val);


				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("INTEGRATION3:", holder.val);


				//10% duration
				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("10% duration:", holder.val);

				//50% duration
				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("50% duration:", holder.val);


				//begin stamp
				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("Begin Stamp:", holder.val);

				//end stamp
				f.readBytes((char *) holder.ch, 4);
				DEBUG_LOG("End Stamp:", holder.val);
				}

			Serial.println("=========End DUMP General========");

			f.close();

			}
		else 
			{
			DEBUG_LOG("GeneralFile nonexist.");
			}

		SPIFFS.end();

		}

}



static uint8_t UpdateGeneralFile(uint32_t number)
{


	union holder
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;


	uint32_t		temp;
	uint8_t 		numevent;

	bool			result = SPIFFS.begin();

	if (false == result)
		{
		DEBUG_ERROR("SPIFFS.begin failed! ");
		}
	else 
		{
		if (SPIFFS.exists(GENERALFILENAME))
			{
			File			f	= SPIFFS.open(GENERALFILENAME, "r+");

			f.seek(OFFSETNUMEVENT, SeekSet);
			holder.val			= number;
			f.write(holder.ch, 4);

#ifdef MYDEBUG
			f.seek(OFFSETNUMEVENT, SeekSet);
			f.readBytes((char *) holder.ch, 4);
			DEBUG_LOG("UpdateGeneralFile :check the numevent is:", holder.val);
#endif

			//update ACCx in general file
			temp				= 0;

			for (int i = 0; i < 4; i++)
				{
				f.seek((4 + i * 4), SeekSet);
				system_rtc_mem_read(ADDR_ALLACC + i, &temp, 4);
				f.write((temp) & 0xff);
				f.write(((temp) >> 8) & 0xff);
				f.write(((temp) >> 16) & 0xff);
				f.write(((temp) >> 24) & 0xff);
				}

			f.seek((LEN_HEADSECT + (number - 1) * 48), SeekSet);

			//MAX
			for (int i = 0; i < 4; i++)
				{
				f.write(MAX[i] &0xff);
				f.write((MAX[i] >> 8) & 0xff);
				f.write((MAX[i] >> 16) & 0xff);
				f.write((MAX[i] >> 24) & 0xff);
				}

			for (int i = 0; i < 4; i++)
				{
				f.write(INTEGRATION[i] &0xff);
				f.write((INTEGRATION[i] >> 8) & 0xff);
				f.write((INTEGRATION[i] >> 16) & 0xff);
				f.write((INTEGRATION[i] >> 24) & 0xff);
				}

			temp				= 101010;
			f.write(temp & 0xff);
			f.write((temp >> 8) & 0xff);
			f.write((temp >> 16) & 0xff);
			f.write((temp >> 24) & 0xff);

			temp				= 505050;
			f.write(temp & 0xff);
			f.write((temp >> 8) & 0xff);
			f.write((temp >> 16) & 0xff);
			f.write((temp >> 24) & 0xff);

			//start event stamp
			system_rtc_mem_read(ADDR_BEGINTIMEOFEVENT, &temp, 4);
			f.write(temp & 0xff);
			f.write((temp >> 8) & 0xff);
			f.write((temp >> 16) & 0xff);
			f.write((temp >> 24) & 0xff);

			//stop event stamp
			system_rtc_mem_read(ADDR_ENDTIMEOFEVENT, &temp, 4);

			if (temp == 0)
				temp = 0xffffffff;

			f.write(temp & 0xff);
			f.write((temp >> 8) & 0xff);
			f.write((temp >> 16) & 0xff);
			f.write((temp >> 24) & 0xff);

			f.close();

			}
		else 
			{
#ifdef MYDEBUG
			DEBUG_ERROR("UpdateGeneralFile failed.");
#endif
			}

		SPIFFS.end();

		}

}


static uint8_t isNewEvent(void)
{
	uint8_t 		flag = 0;



	for (int i = 0; i < 4; i++)
		{
		int 			val = ReadRTCMem(ADDR_DIFF + i);

		if (val > DIFF_EVENTTHRESHOLD)
			flag = flag + (1 << i);

		}



	return flag;


}


static uint8_t isFalseAlarm(void)
{

	int 			flag = 0;

	/*
		if (count==TIME_WINDOW_WIDTH)
			{
				for (int i=0 ; i<4; i++)
					{
						int val=ReadRTCMem(ADDR_DIFF+i);
						if (val<DIFF_THRESHOLD_END_EVENT)	  flag++   ;

					}


			}

		return flag;
	*/
	uint32_t		lspan = ReadRTCMem(ADDR_LIFESPAN);
	uint32_t		beginTimeEvent = ReadRTCMem(ADDR_PENDEDEVENTTIME);

	if (lspan - beginTimeEvent > 16)
		return false;

	else if (lspan - beginTimeEvent == 16)
		{
		for (int i = 0; i < 4; i++)
			{
			int 			max = ReadRTCMem(ADDR_MAXDIFF + i);
			int 			cur = ReadRTCMem(ADDR_DIFF + i);

			if (cur < (max >> 3))
				return true;

			}

		}
	else 
		{
		for (int i = 0; i < 4; i++)
			{
			int 			val = ReadRTCMem(ADDR_DIFF + i);

			if (val < 0)
				return true;

			}

		}

	return false;


}


static void appendFile(char * oldfilename, char * currfilename)
{



}


void SendAlert(uint16_t comm)
{
   /*
	TransferInfo.AlterType = comm;
	
	UpdateLifeSpan(Mark_End(), 0);
	TransferInfo.TimeStamp = ReadRTCMem(ADDR_LIFESPAN);
	TransferInfo.TimeStampH =0;
	
	TransferInfo.FinalStatus = 500;
   */
    COMMAND   			= comm;
	CANWORK 			= true;

}


static uint8_t NewEventArrive()
{
	int 			val = isNewEvent();

	DEBUG_LOG("val", val);
	DEBUG_LOG("diffFlag", diffFlag);
	DEBUG_LOG("countDiff=", countDiff);

	if ((val > 0) && (!diffFlag))
		{

		//TrigerCHs=val;
		WriteRTCMem(ADDR_TRIGERCHS, val);
		WriteRTCMem(ADDR_TRIGERCHS_AVER, val);

		diffFlag			= true;
		averFlag			= true;

		for (int i = 0; i < 4; i++)
			WriteRTCMem(ADDR_MAXDIFF + i, 0);
		}

	if (diffFlag)
		countDiff++;

	for (int i = 0; i < 4; i++)
		{

		int 			cur = ReadRTCMem(ADDR_DIFF + i);

		/*	if (cur<=0)
			  {
				  diffFlag=false;
				  averFlag=false;
				  countDiff=0;
				  countAver=0;
				  TrigerCHs=0;
				  return 0;

			  }
		 */
		}

	/*
	 for (int i=0; i<4; i++)
				 {
					 int max=ReadRTCMem(ADDR_MAXDIFF+i);
					 DEBUG_LOG("MAXDIFF=",max)	;
				 }
	 */
	uint32_t		lastval = ReadRTCMem(ADDR_TRIGERCHS_AVER);

	if (lastval & 0x0f00)
		return 0;

	if (countDiff == 16)
		{
		for (int i = 0; i < 4; i++)
			{
			int 			max = ReadRTCMem(ADDR_MAXDIFF + i);

			DEBUG_LOG("MAXDIFF=", max);
			int 			cur = ReadRTCMem(ADDR_DIFF + i);

			uint32_t		shortlist = ReadRTCMem(ADDR_TRIGERCHS);

			if ((cur > (max >> 3)) && ((shortlist & 0xf) & (1 << i)))
				{
				// diffFlag=false;
				// countDiff=0;
				shortlist			|= ((1 << i) << 4);
				WriteRTCMem(ADDR_TRIGERCHS, shortlist);
				return 2;

				}
			else 
				continue;

			}

		//false alarm
		diffFlag			= false;
		countDiff			= 0;
		WriteRTCMem(ADDR_TRIGERCHS, 0);

		//TrigerCHs=0;
		//handle false alarm
		uint32_t		num = ReadRTCMem(ADDR_NUMEVENT);

		if (num >= 1)
			{
			num--;
			WriteRTCMem(ADDR_NUMEVENT, num);
			}

		return 0;
		}
	else if (countDiff >= 64)
		{
		diffFlag			= false;
		countDiff			= 0;

		//TrigerCHs=0;
		WriteRTCMem(ADDR_TRIGERCHS, 0);
		return 0;

		}
	else 
		return 0;


}


static uint8_t isSatured()
{
	if (averFlag)
		countAver++;

	DEBUG_LOG("countAver", countAver);
	DEBUG_LOG("averFlag", averFlag);
	uint32_t		shortlist = ReadRTCMem(ADDR_TRIGERCHS_AVER);

	DEBUG_LOG("shortlist_aver", shortlist);

	if (countAver == 600)
		{
		for (int i = 0; i < 4; i++)
			{
			int 			max = ReadRTCMem(ADDR_MAXAVER + i);
			int 			curr = ReadRTCMem(ADDR_MOVINGAVERAGE + i);

			uint32_t		shortlist = ReadRTCMem(ADDR_TRIGERCHS_AVER);

			if (((max - curr) <= (max >> 5)) && ((shortlist & 0xf) & (1 << i)))
				{

				countAver			= 0;
				averFlag			= false;
				shortlist			|= ((1 << i) << 8);
				WriteRTCMem(ADDR_TRIGERCHS_AVER, shortlist);
				return 2;

				}
			else 
				continue;


			}

		countAver			= 0;
		averFlag			= false;
		WriteRTCMem(ADDR_TRIGERCHS_AVER, 0);
		return 0;


		}
	else 
		return 0;

}


static void samplecallback_new()
{
	uint32_t		temp;
	uint32_t		currNumEvent, numevent;
	char			filename[10], oldfilename[10];


	union holder
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;
   
   uint32_t old=system_get_time();
	count++;

	/* read all channels*/
	ReadChs();


	int 	rval = NewEventArrive();
    uint32_t delta=system_get_time()-old;
	DEBUG_LOG("it take samplecallback_new :",delta);

	if (rval == 2)
		{
		DEBUG_LOG("it is going to send yel!");
		timer.detach();
		SendAlert(0);                         //0   :yellow
		}

	else if (isSatured() == 2)
		{
		DEBUG_LOG("it is going to send red!");
		timer.detach();
		SendAlert(1);                         //1  :red
		}


}


static void samplecallback()
{
	uint32_t		temp;
	uint32_t		currNumEvent, numevent;
	char			filename[10], oldfilename[10];


	union holder
		{
		uint8_t 		ch[4];
		uint32_t		val;
		} holder;


	count++;

	/* read all channels*/
	ReadChs();


	if (isFalseAlarm())
		{
		DEBUG_LOG("False alarm\n ");
		uint32_t		currEventNum = ReadRTCMem(ADDR_NUMEVENT);

		bool			result = SPIFFS.begin();

		if (false == result)
			{
			DEBUG_ERROR("SPIFFS.begin failed! ");
			}
		else 
			{

			//update general file
			if (SPIFFS.exists(GENERALFILENAME))
				{
				File			f	= SPIFFS.open(GENERALFILENAME, "r+");

				f.seek(OFFSETNUMEVENT, SeekSet);
				f.readBytes((char *) holder.ch, 4);

				if (holder.val > 0)
					{
					holder.val--;
					f.seek(OFFSETNUMEVENT, SeekSet);
					f.write(holder.ch, 4);
					}

				if (ReadRTCMem(ADDR_EMBEDDEDEVENT) == 0)
					{
					//delete current data file
					sprintf(filename, "/data%d", currEventNum);

					if (SPIFFS.exists(filename))
						{
						SPIFFS.remove(filename);
						DEBUG_LOG("Deleted the file:", filename);
						}
					}
				else // in the event
					{
					sprintf(oldfilename, "/data%d", currEventNum - 1);
					sprintf(filename, "/data%d", currEventNum);

					if (SPIFFS.exists(oldfilename) && SPIFFS.exists(filename))
						{
						appendFile(oldfilename, filename);

						//SPIFFS.remove(filename);
						DEBUG_LOG("Append currfile to oldfile ");
						}


					}

				SPIFFS.end();

				//descrease the numevent
				if (currEventNum > 0)
					currEventNum--;

				WriteRTCMem(ADDR_NUMEVENT, currEventNum);

				if (ReadRTCMem(ADDR_EMBEDDEDEVENT) == 0)
					{
					UpdataWifiStateInRtcMem(0);
					UpdateLifeSpan(Mark_End(), DEEPSLEEPTIME);
					ESP.deepSleep(DEEPSLEEPTIME, WAKE_RF_DISABLED);

					}

				}



			}


		//
		}
	else 
		{
		bool			isover = CheckThreshold();

		uint8_t 		endtype = isEndofEvent();

		if (isover || (endtype > 0)) // reach the end of event
			{
			DEBUG_LOG("End of event ! ");

			timer.detach();

			/*tag the end time of this event*/
			UpdateLifeSpan(Mark_End(), 0);
			temp				= ReadRTCMem(ADDR_LIFESPAN);
			WriteRTCMem(ADDR_ENDTIMEOFEVENT, temp);

			if ((1 == getRunMode()) && (getFreeFlashSize() > DATASIZEINBYTE))
				{
				//save the existing data into file
				bool			result = SPIFFS.begin();

				if (false == result)
					{
					DEBUG_ERROR("SPIFFS.begin failed! ");
					}
				else 
					{

					numevent			= ReadRTCMem(ADDR_NUMEVENT);
					sprintf(filename, "/data%d", numevent);

					DEBUG_LOG("The Data is saved into file: ", filename);

					File			f	= SPIFFS.open(filename, "a");

					for (auto item: VALUES)
						{
						f.write(item & 0xff);
						f.write((item >> 8) & 0xff);
						f.write((item >> 16) & 0xff);
						f.write((item >> 24) & 0xff);
						}

					/*
						uint16_t s= f.write((uint8_t *)&VALUES[0],DATASIZEINBYTE);

						if (s!=DATASIZEINBYTE)
						  DEBUG_ERROR("Writing file Error!!!");

						  f.close();
					*/
					SPIFFS.end();

					}

				DEBUG_LOG("end of adding remaining data");

				}

			//TODO
			InitRTCBuffer();

			VALUES.clear();


			temp				= ReadRTCMem(ADDR_NUMEVENT);
			UpdateGeneralFile(temp);

			if (endtype == 1)
				WriteRTCMem(ADDR_EMBEDDEDEVENT, 0);

#ifdef MYDEBUG
			DEBUG_LOG("end of Updating General File");
			DumpgeneralFile();
#endif

			SENDINGNOW			= false;

			uint32_t		flag1 = ReadRTCMem(ADDR_THRESHOLDFLAG);

			if (isover && (flag1 == 0))
				{
				SENDINGNOW			= true;
				WriteRTCMem(ADDR_THRESHOLDFLAG, 1);
				}

			uint32_t		flag2 = ReadRTCMem(ADDR_NUMEVENTFLAG);

			currNumEvent		= ReadRTCMem(ADDR_MAXNUMEVENT);

			if ((temp >= currNumEvent) && (flag2 == 0))
				{
				SENDINGNOW			= true;
				WriteRTCMem(ADDR_NUMEVENTFLAG, 1);
				}

			if (SENDINGNOW)
				{
				DEBUG_LOG("begin to send the data ");

				if (ReadRTCMem(ADDR_ONLINEMODE))
					CANWORK = true;
				else 
					{
					CANWORK 			= false;

					UpdataWifiStateInRtcMem(0);
					UpdateLifeSpan(Mark_End(), DEEPSLEEPTIME);
					ESP.deepSleep(DEEPSLEEPTIME, WAKE_RF_DISABLED);
					}
				}
			else 
				{
				if (endtype == 2)
					{
					UpdataWifiStateInRtcMem(1);
					UpdateLifeSpan(Mark_End(), DEEPSLEEPSHORTTIME);
					ESP.deepSleep(DEEPSLEEPSHORTTIME);

					}
				else 
					{
					DEBUG_LOG("not reach the threshold to send data  ");
					UpdataWifiStateInRtcMem(0);
					UpdateLifeSpan(Mark_End(), DEEPSLEEPTIME);
					ESP.deepSleep(DEEPSLEEPTIME, WAKE_RF_DISABLED);

					}

				}
			}

		}

}


// tranfer buf in rtc mem to vector
static void InitVectorWithRTCBuffer(void)
{

	uint32_t		addr, val, distance;

	system_rtc_mem_read(ADDR_CURRENTPOINT, &addr, 4);

	distance			= addr - BEGINADDR;

#ifdef MYDEBUG

	if (distance % 4 != 0)
		Serial.println("ERROR: Presample buffer alignment error!");

#endif


#ifdef MYDEBUG
	Serial.println("Dump  RTCBuffer:");
#endif

	for (int i = addr; i < ENDADDR; i++)
		{

		system_rtc_mem_read(i, &val, 4);

#ifdef MYDEBUG
		Serial.print((val >> 16) & 0xffff);
		Serial.print("	 ");
		Serial.println(val & 0xffff);
		Serial.print("	");
#endif

		VALUES.push_back(val);
		}

	for (int i = BEGINADDR; i < addr; i++)
		{
		system_rtc_mem_read(i, &val, 4);

#ifdef MYDEBUG
		Serial.print((val >> 16) & 0xffff);
		Serial.print("	 ");
		Serial.println(val & 0xffff);
		Serial.print("	");
#endif

		VALUES.push_back(val);
		}

#ifdef MYDEBUG
	Serial.println();
#endif

}


static void UpdateRTCBufferWithVector(void)
{
	uint32_t		addr = 0, temp;

	system_rtc_mem_read(ADDR_CURRENTPOINT, &addr, 4);

	for (auto i = VALUES.size() - 23; i != VALUES.size(); i++)
		{
		temp				= VALUES[i];
		system_rtc_mem_write(addr, &temp, 4);
		addr++;

		if (addr == ENDADDR)
			addr = BEGINADDR;

		}
}


static void State0(void)
{
	//disable wifi
	WiFi.mode(WIFI_OFF);
	WiFi.forceSleepBegin();

	count				= 0;
	countDiff			= 0;
	diffFlag			= false;
	countAver			= 0;
	averFlag			= false;
	CANWORK 			= false;
	SENDINGNOW			= false;


	/*read 4 channels once and detect if there is a event*/
	int 			flag = ReadAndProcessData();

	/*there is a event*/
	if (flag > 0)
		{
		/* after a deepsleep to collect data every a second  */
		UpdataWifiStateInRtcMem(1);
		UpdateLifeSpan(Mark_End(), DEEPSLEEPSHORTTIME);
		ESP.deepSleep(DEEPSLEEPSHORTTIME);
		}

	else //flag<=0
		{
		UpdataWifiStateInRtcMem(0);
		UpdateLifeSpan(Mark_End(), DEEPSLEEPTIME);
		ESP.deepSleep(DEEPSLEEPTIME, WAKE_RF_DISABLED);

		}

}


static void State1(void)
{

	char			filename[20];

	DEBUG_LOG("A event happened and begin to collect data");

	WiFi.mode(WIFI_OFF);

	//record the timestamp at the beginning
	UpdateLifeSpan(Mark_End(), 0);

	uint32_t		lspan = ReadRTCMem(ADDR_LIFESPAN);

	WriteRTCMem(ADDR_BEGINTIMEOFEVENT, lspan);
	WriteRTCMem(ADDR_PENDEDEVENTTIME, lspan);

	//reset the counter
	count				= 0;
	countDiff			= 0;
	diffFlag			= false;
	countAver			= 0;
	averFlag			= false;
	CANWORK 			= false;
	SENDINGNOW			= false;

	//inc and record the NUMEVENT beforehand
	uint32_t		val = ReadRTCMem(ADDR_NUMEVENT);

	++val;
	WriteRTCMem(ADDR_NUMEVENT, val);

#if 0

	//handle the special case
	if (val == 0)
		{
		++val;
		WriteRTCMem(ADDR_NUMEVENT, val);
		}
	else 
		{
		bool			result = SPIFFS.begin();

		if (false == result)
			{
			DEBUG_ERROR("SPIFFS.begin failed! ");
			}
		else 
			{
			sprintf(filename, "/data%d", val);

			if (!SPIFFS.exists(filename))
				WriteRTCMem(ADDR_NUMEVENT, val);
			else 
				WriteRTCMem(ADDR_NUMEVENT, ++val);
			}

		SPIFFS.end();

		}

#endif


	// in the event
	if (ReadRTCMem(ADDR_EMBEDDEDEVENT) == 0)
		WriteRTCMem(ADDR_EMBEDDEDEVENT, 1);

	//reinit the ADDR_MOVINGAVERAGE variable
	for (int i = 0; i < 4; i++)
		{
		WriteRTCMem(ADDR_MOVINGAVERAGE + i, DEFAULT_MOVINGAVERAGE);
		WriteRTCMem(ADDR_MAXDIFF + i, DEFAULT_MAXDIFF);
		}

	//update the info in the general file
	//system_rtc_mem_read(ADDR_NUMEVENT, &temp, 4);
	UpdateGeneralFile(val);

	DEBUG_LOG("General File at the beginning of event: ");

#ifdef MYDEBUG
	DumpgeneralFile();
#endif

	//init the container
	VALUES.clear();
	VALUES.reserve(DATASIZE);

	InitVectorWithRTCBuffer();

	//init the local vars
	for (int i = 0; i < sizeof(CHS) / sizeof(CHS[0]); ++i)
		{
		INTEGRATION[i]		= 0;
		}

	//arm the timer
#ifdef NO_SLEEP_SAMPLING //depend on parameter

	timer.attach(DEEPSLEEPSAMPLE, samplecallback_new); //for test mode

#else

	UpdataWifiStateInRtcMem(3); 					//for release mode	   TODO
	UpdateLifeSpan(Mark_End(), DEEPSLEEPSHORTTIME);
	ESP.deepSleep(DEEPSLEEPSHORTTIME);
#endif

}


static void State2(void)
{
	//check if there are data that unsent
	uint32_t	val = getNumberEvent();

	//false=offline mode
	boolean 	apstatus = EstablishConn(true);

   //record the size of datafile that will be sent
    RecordFilesize();
	
	if (isChargingMode())
		SendNewFlag(5, VERSION);     //CHR message

	if (val > 0)
		{
#ifdef MYDEBUG
		DumpgeneralFile();
		DEBUG_LOG("resending the data");
#endif

		if (apstatus && (DIAPER_SUCCESS == SendNewFlag(3, VERSION)))   // old message
			{
            

			if (DIAPER_SUCCESS == SendData())
				{
				 Clear_Flashfile();
				 InitGeneralFile();
                 WriteRTCMem(ADDR_NUMEVENT, 0);
				}
			}
		else 
			{
			//offline mode
			WriteRTCMem(ADDR_NUMEVENT, val);

			}
		
		}
	else
		{
           Clear_Flashfile();
		   InitGeneralFile();
           WriteRTCMem(ADDR_NUMEVENT, 0);
	}
     
	DEBUG_LOG(" end of doing some init works");

	// run into ChargingMode ,never come back
	if (isChargingMode())
		{
		ChargingMode();
		}

#ifdef MYDEBUG
	DumpgeneralFile();
	DEBUG_LOG("Sending new flag");
#endif

	if (1 == ReadRTCMem(ADDR_ONLINEMODE))
		{
		// send new flag
		SendNewFlag(2, VERSION);                       //New message
		DEBUG_LOG("receiving the parameters:");

		//get configuration parameters
		if (ReceiveConfig() == false)
			ApplyDefaultParams();
		}
	else 
		ApplyDefaultParams();


	//init the local vars
	for (int i = 0; i < sizeof(CHS) / sizeof(CHS[0]); ++i)
		{
		SUM[i]				= 0;
		}

	UpdataWifiStateInRtcMem(0);
	UpdateLifeSpan(Mark_End(), DEEPSLEEPTIME);
	ESP.deepSleep(DEEPSLEEPTIME, WAKE_RF_DISABLED);

}


//
static void State3(void)
{

	uint32_t		temp;
	uint32_t		currNumEvent, numevent;
	char			filename[30];

	//samplecallback();

	/* read all channels*/
	/*
	ReadChs();

	if (isEndofEvent( ))   // reach the end of event
		{

			PostEventprocess( );
		}

	*/
}


static bool UpdateOTA(void)
{

	//show the beginning of update
	pinMode(LEDPIN, OUTPUT);						/*	   MTMS=GPIO14 */
	digitalWrite(LEDPIN, LOW);

	ESPhttpUpdate.rebootOnUpdate(false);

	t_httpUpdate_return ret = ESPhttpUpdate.update("http://www1.datawind-s.com:8888/quercus/ESPDiaper.10002.bin");
	//t_httpUpdate_return ret=ESPhttpUpdate.update("http://www1.datawind-s.com", 8888, "/quercus/esp/update/arduino.php", String(VERSION));

	  while(ret!=HTTP_UPDATE_OK)
		{

		delay(10000);
		DEBUG_LOG("OTA Failed ,try again...");
		t_httpUpdate_return ret = ESPhttpUpdate.update("http://www1.datawind-s.com:8888/quercus/ESPDiaper.10002.bin");

		}

	DEBUG_LOG("OTA is Done!");

	return true;

}


static void State4(void)
{

	UpdataWifiStateInRtcMem(4);
	ESP.deepSleep(CHARGEMODEDEEPSLEEPTIME, WAKE_RF_DISABLED);


}





static void Handle_Sleepwake_Rst(void)
{

	Mark_Start();

#ifdef MYDEBUG
	uint32_t		temp = ReadRTCMem(ADDR_LIFESPAN);
	DEBUG_LOG("LifeSpan:", temp);
#endif

	uint32_t		state = ReadRTCMem(ADDR_WIFISTATE);
	DEBUG_LOG("WIFISTATE:", state);
	switch (state)
		{
		case 0: //normal cycle without event
			State0();
			break;

		case 1: //transtion state to timer handler
			State1();
			break;

		case 2: //the first cycle since the power on
			State2();
			break;

		case 3: //same as state1,but not in timer handler
			State3();
			break;

		case 4: //charing mode
			State4();
			break;


		default:
				{
				DEBUG_ERROR("Unknown Wifi State!");
				ESP.deepSleep(0, WAKE_RF_DISABLED); //sleep forever
				}
		}
}


//what should be done the moment it powers on
static void Handle_Poweron_Rst(void)
{

	Mark_Start();
	DEBUG_LOG("Power ON ");

	//clear variable in rtc memory
	Clear_RTCMem(LEN_RTCMEM);

	//set default value
	Init_RTCMem();

	UpdataWifiStateInRtcMem(2);
	UpdateLifeSpan(Mark_End(), DEEPSLEEPSHORTTIME);
	ESP.deepSleep(DEEPSLEEPSHORTTIME);

}

static bool isChargingMode(void)
{
	int 			flag = 0;
	uint32_t		val[4];

	for (int i = 0; i < sizeof(CHS) / sizeof(CHS[0]); i++)
		{
		val[i]				= ReadSingleCh(CHS[i]);

		if ((val[i] &0xffff) < 20)
			flag++;
		}

	if (flag == 4)
		return true;

	else 
		return false;

}


static void ChargingMode()
{

	//2.calibration
	DEBUG_LOG("Charging Mode .... ");

	//update on the air
	if (UpdateOTA())

	{

	 	DEBUG_LOG("finishing OTA,then restart.... ");
	
    	delay(600000);          //10 mins
		ESP.restart();
	}
   
    

	//ESP.deepSleep(DEEPSLEEPTIME, WAKE_RF_DISABLED);
	


}


/* Arduino framework*/
void setup()
{

	/*init serial */
	Serial.begin(115200);

	//delay(50);
	Serial.println();
	DEBUG_LOG("Current Version:", getVersion());

	DEBUG_LOG("SSID:", WiFi.SSID());
	DEBUG_LOG("Password:", WiFi.psk());
	
	//indicate that update is successful
	pinMode(LEDPIN, OUTPUT);						/*	   MTMS=GPIO14 */
	digitalWrite(LEDPIN, HIGH);


   #if 0
	  SPIFFS.begin();
	  SPIFFS.format();
	  SPIFFS.end();
   #endif

	/*branch based on the reset info */
	const rst_info * resetInfo = system_get_rst_info();
	DEBUG_LOG("ResetReason: ", resetInfo->reason);
	DEBUG_LOG("Free Flash Space: ", getFreeFlashSize());


	//power on
	if ((REASON_DEFAULT_RST 	== resetInfo->reason) 	//0
	|| (REASON_WDT_RST 			== resetInfo->reason) 	//1
	|| (REASON_EXCEPTION_RST 	== resetInfo->reason) 	//2
	|| (REASON_SOFT_WDT_RST 	== resetInfo->reason) 	//3  software watch dog reset, GPIO status won? change
	|| (REASON_SOFT_RESTART 	== resetInfo->reason) 	//4
	|| (REASON_EXT_SYS_RST 		== resetInfo->reason)) 	//6
		{

			//filter the power switch bounce, this code should stay here
			delay(POWERONDELAY);
			Handle_Poweron_Rst();
		}

	//wake up
	else if (REASON_DEEP_SLEEP_AWAKE == resetInfo->reason) //5
		{
		Handle_Sleepwake_Rst();
		}

	else 
		{

		DEBUG_ERROR("Unknown Start State  ");
		system_restart();
		delay(1000);
		}


}

static void SaveTimeDelayInfo(uint32_t diff)
{
	 
		uint32_t		numevent;
		char			filename[30];
		 
		
		VALUES.push_back(0);
	    VALUES.push_back(0);
		VALUES.push_back(0);
		VALUES.push_back(diff);

	   if (VALUES.size() >= DATASIZE)
			{
			uint32_t		freespace = getFreeFlashSize();
	
			if (1 == getRunMode() && freespace > 5120)
				{

				bool			result = SPIFFS.begin();
	
				if (false == result)
					{
					DEBUG_ERROR("SPIFFS.begin failed! ");
					}
				else 
					{
	
					system_rtc_mem_read(ADDR_NUMEVENT, &numevent, 4);
					sprintf(filename, "/data%d", numevent);
	
	
	
					File			f	= SPIFFS.open(filename, "a");
	
					DEBUG_LOG("Saving data into FileName: ", filename);
					DEBUG_LOG("Free Flash Space:", freespace);
	
					for (auto item: VALUES)
						{
						f.write(item & 0xff);
						f.write((item >> 8) & 0xff);
						f.write((item >> 16) & 0xff);
						f.write((item >> 24) & 0xff);
						}
	
					f.close();
	
					SPIFFS.end();
					}

				}
	
			VALUES.clear();
	
			}


}


/* Arduino framework*/
void loop()
{


	if (CANWORK == true)
		{

		   UpdateLifeSpan(Mark_End(), 0);
		   uint32_t prev=ReadRTCMem(ADDR_LIFESPAN);
		   
		   EstablishConn(false);

		if (0==COMMAND)    			//YEL 
			{
			uint16_t		chs = ReadRTCMem(ADDR_TRIGERCHS);
			SendNewFlag(0, chs) ;   
			}
		else if (1==COMMAND )		//red
			{
			uint16_t		chs = ReadRTCMem(ADDR_TRIGERCHS_AVER);
            SendNewFlag(1, chs) ; 
			}

		CANWORK 		= false;
		UpdateLifeSpan(Mark_End(), 0);
		uint32_t diff=ReadRTCMem(ADDR_LIFESPAN)-prev;
		DEBUG_LOG("Time Missed:",diff);
		SaveTimeDelayInfo(diff);
		timer.attach(DEEPSLEEPSAMPLE, samplecallback_new);
	     

		}

}






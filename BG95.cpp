#include "BG95.h"

bool getBG95response(char command[], char exp_response[], char response[], uint32_t timeout)
{
  uint8_t count = 0;
  bool resp_OK = false;

  response[0] = '\0';
  
  //DEBUG_STREAM.print(command);  
  NBIOT_STREAM.print(command);

  uint32_t t0 = millis();
  while ((millis() - t0) < timeout)
  {
    if (NBIOT_STREAM.available())
    {
      response[count] = NBIOT_STREAM.read();
      DEBUG_STREAM.write(response[count]);
      response[++count] = '\0';
    }
    if (strstr(response, exp_response))
    {
      resp_OK = true;
      break;
    }
  }

  delay(10);
  
  while (NBIOT_STREAM.available())
  { 
    response[count] = NBIOT_STREAM.read();
    DEBUG_STREAM.write(response[count]);
    response[++count] = '\0';
  }
  DEBUG_STREAM.print("\r\n");
  
  return resp_OK;
}

bool BG95_turnOn()
{
  //turn on BG95
  DEBUG_STREAM.print("BG95 reset...");
  pinMode(BG95_PWRKEY, OUTPUT);
  digitalWrite(BG95_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(BG95_PWRKEY, LOW);
  DEBUG_STREAM.println("DONE!");
 
  char response[256];
  //  check FW version
  if (getBG95response("", "APP RDY", response, 8000))
    return true;

  return false;
}

bool BG95_testIfAlive()
{
  char response[256];
  if (getBG95response("AT\r\n", "OK", response, 3000)) {
    DEBUG_STREAM.println("BG95 is alive!");
    return true;
  } else {
    DEBUG_STREAM.println("BG95 is not responding.");
    return false;
  }
}

bool BG95_nwkRegister()
{
  char response[256];
  if (!getBG95response("AT\r\n", "OK", response, 1000))
    return false;

  if (!getBG95response("AT+GMR\r\n", "OK", response, 1000))
    return false;
   
  //Switch the modem to max functionality (bilo min)
  if (!getBG95response("AT+CFUN=1\r\n", "OK", response, 5000))  // bilo 0,0
    return false;

  // Verbose Error Reporting to get understandable error reporting (optional)
  if (!getBG95response("AT+CMEE=2\r\n", "OK", response, 5000))
    return false;

  //scan sequence: first Cat-M1, then NB-IoT, then GSM
  if (!getBG95response("AT+QCFG=\"nwscanseq\",020301,1\r\n", "OK", response, 1000)) // 02 -> CatM1; 03 -> NMIoT; 01 -> GSM
    return false;
  //Automatic (GSM and LTE)
  if (!getBG95response("AT+QCFG=\"nwscanmode\",0,1\r\n", "OK", response, 1000))
    return false;
  //Network category to be searched under LTE RAT: Cat-M1
  if (!getBG95response("AT+QCFG=\"iotopmode\",2,1\r\n", "OK", response, 1000))  // 0 -> CatM1; 2 -> CatM1&NBIoT
    return false;

  //  turn on full module functionality
  if (!getBG95response("AT+CFUN=1,0\r\n", "OK", response, 5000))
    return false;

  // check ICCID
  if (!getBG95response("AT+QCCID\r\n", "OK", response, 2000))
    return false;
  
  //  check IMSI
  char IMSI[20];
  if (!getBG95response("AT+CIMI\r\n", "OK", response, 2000))
    return false;


//  //set APN
//  if (!getBG95response("AT+CGDCONT=1,\"IP\",\"VIP.IOT\"\r\n", "OK", response, 3000))
//    return false;

  
  //  error reporting
  if (!getBG95response("AT+CMEE=1\r\n", "OK", response, 10000))
    return false;
  
  //  automatically report network registration status
  if (!getBG95response("AT+CEREG=1\r\n", "OK", response, 3000))
    return false;
  if (!getBG95response("AT+CEREG?\r\n", "OK", response, 3000))
    return false;

// check available operators and their technologies
//  if (!getBG95response("AT+COPS=?\r\n", "OK", response, 180000))  
//    return false; 

  // connect
  if (!getBG95response("AT+COPS=0,,,9\r\n", "OK", response, 50000))  // 8 -> LTE-CatM1; 9 -> NB-IoT; 0 -> GSM
    return false;
  if (!getBG95response("AT+COPS?\r\n", "OK", response, 600000))  
    return false; 

  //polling the network registration status
  bool reg_ok;
  uint8_t attempt_cnt = 0; 
  do 
  {
    reg_ok = getBG95response("AT+CGATT?\r\n", "+CGATT: 1", response, 5000);
    if (!reg_ok)
    {
      if (++attempt_cnt == 30)
        return false; 
    }
  } while (!reg_ok);

  for (int i = 0; i < 3; i++) {
    getBG95response("AT+QCSQ\r\n", "OK", response, 3000);
    delay(1000);
  }
  getBG95response("AT+CCLK?\r\n", "OK", response, 3000);

    char cmd[128];
  sprintf(cmd, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n", APN, APN_USER, APN_PASS);
  if (!getBG95response(cmd, "OK", response, 3000))
    return false;

  if (!getBG95response("AT+QIACT=1\r\n", "OK", response, 3000))
    return false;

  if (!getBG95response("AT+QIACT?\r\n", "OK", response, 5000))
    return false;
  
  return true;
}

bool BG95_TxRxUDP(char payload[], char server_IP[], uint16_t port)
{
  char response[256], cmd[128];

  if (!getBG95response("AT+QIOPEN=1,2,\"UDP SERVICE\",\"127.0.0.1\",0,3030,0\r\n", "+QIOPEN: 2,0", response, 5000))
    return false;

  if (!getBG95response("AT+QISTATE=0,1\r\n", "OK", response, 3000))
    return false;

  sprintf(cmd, "AT+QISEND=2,%d,\"%s\",%d\r\n", strlen(payload), server_IP, port);
  getBG95response(cmd, ">", response, 3000);
  getBG95response(payload, "+QIURC: \"recv\",2", response, 5000);

  if (!getBG95response("AT+QIRD=2\r\n", "OK", response, 5000))
    return false;

  if (!getBG95response("AT+QICLOSE=2\r\n", "OK", response, 3000))
    return false;
    
  return true;
}

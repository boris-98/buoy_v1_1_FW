#include "Ezo_i2c.h"
#include "Ezo_i2c_util.h"
#include "BG95.h"
#include "LoRaWan_APP.h"

//============================================================================//
// LoRaWAN related stuff
//----------------------------------------------------------------------------//
/* LoraWan OTAA para*/
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xA1, 0x23, 0x45, 0xB6 };
uint8_t appEui[] = { 0x00, 0x1A, 0x79, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F };
uint8_t appKey[] = { 0x1A, 0xCF, 0x45, 0x78, 0xB9, 0xE2, 0x33, 0xD4, 0x99, 0xAA, 0xCC, 0xFF, 0x10, 0x34, 0x67, 0x89 };

/* LoraWan ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
    appDataSize = 4;
    appData[0] = 0x00;
    appData[1] = 0x01;
    appData[2] = 0x02;
    appData[3] = 0x03;
}
//----------------------------------------------------------------------------//
// End of LoRaWAN related stuff
//============================================================================//

#define I2C_ADDRESS_DO  97
#define I2C_ADDRESS_ORP 98
#define I2C_ADDRESS_PH  99
#define I2C_ADDRESS_EC  100
#define I2C_ADDRESS_RTD 102

Ezo_board DO  = Ezo_board(I2C_ADDRESS_DO,  "DO");     //create a RTD circuit object who's address is I2C_ADDRESS_DO and name is "DO"
Ezo_board ORP = Ezo_board(I2C_ADDRESS_ORP, "ORP");    //create a RTD circuit object who's address is I2C_ADDRESS_ORP and name is "ORP"
Ezo_board PH  = Ezo_board(I2C_ADDRESS_PH,  "PH");     //create a PH circuit object, who's address is I2C_ADDRESS_PH and name is "PH"
Ezo_board EC  = Ezo_board(I2C_ADDRESS_EC,  "EC");     //create an EC circuit object who's address is I2C_ADDRESS_EC and name is "EC"
Ezo_board RTD = Ezo_board(I2C_ADDRESS_RTD, "RTD");    //create a RTD circuit object who's address is I2C_ADDRESS_RTD and name is "RTD"

void DO_calibration();
void ORP_calibration();
void PH_calibration();
void EC_calibration();
void RTD_calibration();
void calibrate_sensors();
void calibration_call_delay(unsigned long timeout_millis);


void setup() 
{
  Wire.begin();           // Sensors
  Serial.begin(115200);   // Terminal
  Serial2.begin(115200);  // BG95

  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);  // From LoRa example

  // BG95 test - comment out the following 2 lines if nothing works
  //while(!BG95_turnOn());  // SET BG95 RESET PIN NUMBER (BG95_PWRKEY) IN BG95.h
  BG95_testIfAlive();
  //BG95_nwkRegister();

  calibration_call_delay(5000);   // fenseraj koji ceka proizvoljan broj sekundi da korisnik unese 'c' ako hoce kalibraciju (moze se izmeniti na taster)
}

void loop()
{       
  // U loop zasad samo pojedinacna citanja senzora i njihov ispis.
  // Ako sve bude radilo, to bi se objedinilo u jednu f-ju
  // koja bi ocitala sve senzore i spojila sve rezultate u jedan string,
  // pa bi on dalje mogao da se salje na server i usput sve da se cuva na SD karticu

  ORP.send_read_cmd();  // sensor needs 900ms for the reading, according to the datasheet 
  PH.send_read_cmd();   // 900ms
  DO.send_read_cmd();   // 600ms                  
  EC.send_read_cmd();   // 600ms
  RTD.send_read_cmd();  // 600ms
  delay(1000);          // wait for sensors readings (900ms is the slowest)

  // get the readings, and print them
  receive_and_print_reading(ORP);
  receive_and_print_reading(PH);
  receive_and_print_reading(DO);
  receive_and_print_reading(EC);
  receive_and_print_reading(RTD);

  // save the readings on a SD card
  // dodati sta treba za ovaj deo//

  // send the readings to a server
  // via NB-IoT (BG95)
  // ovde ce se pozvati BG95_TxRxUDP za slanje poruke, zasad je dovoljan test u setup() da se vidi da li BG95 registruje komande

  // via LoraWAN, and go to sleep
  // ovaj deo isto treba prilagoditi, zasad samo salje random poruku da se vidi da li je LoRa ziva
  while (1)   // staviti ovde neki normalan uslov i logiku, da ne zakuca u nekom slucaju
  {           // switch se moze iskoristiti i za celu logiku loop() dela, tj. da svi koraci budu strpani u njega
    switch( deviceState )
    {
      case DEVICE_STATE_INIT:
      {
        Serial.println("INIT state");
  #if(LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
  #endif
        LoRaWAN.init(loraWanClass,loraWanRegion);
        //both set join DR and DR when ADR off 
        LoRaWAN.setDefaultDR(3);

        break;
      }
      case DEVICE_STATE_JOIN:
      {
        Serial.println("JOIN state");
        LoRaWAN.join();
        break;
      }
      case DEVICE_STATE_SEND:
      {
        Serial.println("SEND state");
        prepareTxFrame( appPort );
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
      case DEVICE_STATE_CYCLE:
      {
        Serial.println("CYCLE state");
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        Serial.println("Going to SLEEP state");
        break;
      }
      case DEVICE_STATE_SLEEP:
      {
        // dodati ovde prvo sleep za ostale periferije
        LoRaWAN.sleep(loraWanClass);
        break;
      }
      default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
    }
  }
}

void calibration_call_delay(unsigned long timeout_millis)
{
  Serial.print("Please input character 'c' if calibration is needed...");
  unsigned long currMillis = millis();
  unsigned long prevMillis = currMillis;
  int counter = 0;

  while (currMillis - prevMillis < timeout_millis) {
    if (Serial.available()) {
      char recvChar = Serial.read();
      while (Serial.available()) Serial.read(); // brisem eventualni visak, ako terminal dodaje newline i sl.

      if (recvChar == 'c') {
        Serial.println("CALIBRATING");
        calibrate_sensors();
        return;
      }
    }

    currMillis = millis();
    if (currMillis - prevMillis > counter * 1000) {
      Serial.print(5 - counter);
      Serial.print("...");
      counter++;
    }
  }
  Serial.println("0... skipping calibration.");
}

void calibrate_sensors()
{
  int userChoice = 0;

  while (userChoice != 6) {
    Serial.println("\nChoose which sensor to calibrate (enter num. 1 to 6): \n\t1. DO\n\t2. ORP\n\t3. PH\n\t4. EC\n\t5. RTD\n\t6. Finish calibrating.");
    while (!Serial.available());
    userChoice = Serial.parseInt();
    while (Serial.available()) Serial.read(); 

    switch(userChoice) {
      case 1:
        DO_calibration();
        break;

      case 2:
        ORP_calibration();
        break;

      case 3:
        PH_calibration();
        break;

      case 4:
        EC_calibration();
        break;

      case 5:
        RTD_calibration();
        break;

      case 6:
        break;

      default:
        Serial.println("Invalid choice. Skipping calibration.");
        break;
    }
  }
}

void DO_calibration() 
{
  char userChoice;
  char recv_buf[32];

  Serial.println("\nCalibrating DO sensor.\nEnter:\n\ta - to calibrate to atmospheric oxygen levels\n\tz - to calibrate to 0 dissolved oxygen\n\tc - to clear calibration data");

  while (!Serial.available()) {
    // print the sensor readings to the user while waiting
    DO.send_read_cmd();
    delay(1000);
    receive_and_print_reading(DO);
  }
  userChoice = Serial.read();
  while (Serial.available()) Serial.read(); 
  Serial.println("\n\n");
  
  switch (userChoice) {
    case 'a':
      DO.send_cmd("Cal");
      delay(1300);
      DO.receive_cmd(recv_buf, 32);
      print_success_or_error(DO, "Executed \"Cal\" command successfully.");
      DO.send_cmd("Cal,?");
      delay(300);
      receive_and_print_response(DO);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                        // primer odgovora: ?Cal,0  <- neuspesna kalibracija (inace 1 za single point, 2 za two point calib.)
      break;

    case 'z':
      DO.send_cmd("Cal,0");
      delay(1300);
      DO.receive_cmd(recv_buf, 32);
      print_success_or_error(DO, "Executed \"Cal,0\" command successfully.\n");
      DO.send_cmd("Cal,?");
      delay(300);
      receive_and_print_response(DO);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                        // primer odgovora: ?Cal,0  <- neuspesna kalibracija (inace 1 za single point, 2 za two point calib.)
      break;

    case 'c':
      DO.send_cmd("Cal,clear");
      delay(300);
      DO.receive_cmd(recv_buf, 32);
      print_success_or_error(DO, "Executed \"Cal,clear\" command successfully.\n");
      break;
    
    default:
      Serial.println("Invalid option. Skipping DO calibration.");
      break;
  }
}

void ORP_calibration()
{
  int userValue;
  char recv_buf[32];
  char send_buf[32];

  Serial.println("\nCalibrating ORP sensor.\nEnter a reference value, or input 'c' to clear calibration data:");
  while(!Serial.available()) {
    // print the sensor readings to the user while waiting
    ORP.send_read_cmd();
    delay(1000);
    receive_and_print_reading(ORP);
  }
  Serial.println("\n\n");

  if (Serial.peek() == 'c')    // ako korisnik unese 'c' ide clear komanda, u suprotnom smatram da je broj i ide kalibracija
  {   
    while (Serial.available()) Serial.read(); 

    ORP.send_cmd("Cal,clear");
    delay(300);

    ORP.receive_cmd(recv_buf, 32);
    print_success_or_error(ORP, "Executed \"Cal,clear\" command successfully.\n");
  } 
  else 
  {
    userValue = Serial.parseInt();
    while (Serial.available()) Serial.read(); 
    
    sprintf(send_buf, "Cal,%d", userValue);
    ORP.send_cmd(send_buf);
    delay(900);

    ORP.receive_cmd(recv_buf, 32);
    print_success_or_error(ORP, "Executed \"Cal,N\" command successfully.\n");

    ORP.send_cmd("Cal,?");
    delay(300);
    receive_and_print_response(ORP);    // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                        // primer odgovora: ?Cal,0  <- neuspesna kalibracija 
  }
}

void PH_calibration()
{
  char userChoice;
  char recv_buf[32];
  float userVal;

  Serial.println("\nCalibrating pH sensor.\nEnter:\n\tm - to calibrate at midpoint\n\tl - to calibrate at lowpoint\n\th - to calibrate at highpoint\n\tc - to clear calibration data");

  while (!Serial.available()) {
    // print the sensor readings to the user while waiting
    PH.send_read_cmd();
    delay(1000);
    receive_and_print_reading(PH);
  }
  userChoice = Serial.read();
  while (Serial.available()) Serial.read(); 
  Serial.println("\n\n");
  
  if (userChoice == 'c')  // ako korisnik unese 'c' ide clear komanda, u suprotnom smatram da je neka od drugih validnih opcija
  {    
    PH.send_cmd("Cal,clear");
    delay(300);
    PH.receive_cmd(recv_buf, 32);
    print_success_or_error(PH, "Executed \"Cal,clear\" command successfully.\n");
  }
  else
  {
    Serial.println("Enter a reference value (float, eg. 7.000): ");
    while(!Serial.available()) {
      // print the sensor readings to the user while waiting
      PH.send_read_cmd();
      delay(1000);
      receive_and_print_reading(PH);
    }
    userVal = Serial.parseFloat();
    while (Serial.available()) Serial.read(); 
    Serial.println("\n\n");
    
    switch (userChoice) {
      case 'm':
        PH.send_cmd_with_num("Cal,mid,", userVal);
        delay(900);
        PH.receive_cmd(recv_buf, 32);
        print_success_or_error(PH, "Executed \"Cal,mid,N\" command successfully.");
        PH.send_cmd("Cal,?");
        delay(300);
        receive_and_print_response(PH);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                          // primer odgovora: ?Cal,0  <- neuspesna kalibracija (inace 1 za single point, 2 za two point calib.)
        break;
      
      case 'l':
        PH.send_cmd_with_num("Cal,low,", userVal);
        delay(900);
        PH.receive_cmd(recv_buf, 32);
        print_success_or_error(PH, "Executed \"Cal,low,N\" command successfully.");
        PH.send_cmd("Cal,?");
        delay(300);
        receive_and_print_response(PH);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                          // primer odgovora: ?Cal,0  <- neuspesna kalibracija (inace 1 za single point, 2 za two point calib.)
        break;

      case 'h':
        PH.send_cmd_with_num("Cal,high,", userVal);
        delay(900);
        PH.receive_cmd(recv_buf, 32);
        print_success_or_error(PH, "Executed \"Cal,high,N\" command successfully.");
        PH.send_cmd("Cal,?");
        delay(300);
        receive_and_print_response(PH);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                          // primer odgovora: ?Cal,0  <- neuspesna kalibracija (inace 1 za single point, 2 za two point calib.)
        break;

      default:
        Serial.println("Invalid option. Skipping pH calibration.");
        break;
    }
  }
}

void EC_calibration()
{
  char userChoice;
  char recv_buf[32];
  float userVal;

  Serial.println("\nCalibrating EC sensor.\nEnter:\n\td - for dry calibration\n\ts - for single point calibration\n\tl - to calibrate at lowpoint\n\th - to calibrate at highpoint\n\tc - to clear calibration data");

  while (!Serial.available()) {
    // print the sensor readings to the user while waiting
    EC.send_read_cmd();
    delay(1000);
    receive_and_print_reading(EC);
  }
  userChoice = Serial.read();
  while (Serial.available()) Serial.read(); 
  Serial.println("\n\n");
  
  if (userChoice == 'c')  // ako korisnik unese 'c' ide clear komanda, u suprotnom smatram da je neka od drugih validnih opcija
  {    
    EC.send_cmd("Cal,clear");
    delay(300);
    EC.receive_cmd(recv_buf, 32);
    print_success_or_error(EC, "Executed \"Cal,clear\" command successfully.\n");
  }
  else if (userChoice == 'd')
  {
    EC.send_cmd("Cal,dry");
    delay(600);
    EC.receive_cmd(recv_buf, 32);
    print_success_or_error(EC, "Executed \"Cal,dry\" command successfully.\n");
  }
  else
  {
    Serial.println("Enter a reference value (float, eg. 1413.000): ");
    while(!Serial.available()) {
      // print the sensor readings to the user while waiting
      EC.send_read_cmd();
      delay(1000);
      receive_and_print_reading(EC);
    }
    userVal = Serial.parseFloat();
    while (Serial.available()) Serial.read(); 
    Serial.println("\n\n");
    
    switch (userChoice) {
      case 's':
        EC.send_cmd_with_num("Cal,", userVal);
        delay(600);
        EC.receive_cmd(recv_buf, 32);
        print_success_or_error(EC, "Executed \"Cal,N\" command successfully.");
        EC.send_cmd("Cal,?");
        delay(300);
        receive_and_print_response(EC);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                          // primer odgovora: ?Cal,0  <- neuspesna kalibracija 
        break;
      
      case 'l':
        EC.send_cmd_with_num("Cal,low,", userVal);
        delay(600);
        EC.receive_cmd(recv_buf, 32);
        print_success_or_error(EC, "Executed \"Cal,low,N\" command successfully.");
        EC.send_cmd("Cal,?");
        delay(300);
        receive_and_print_response(EC);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                          // primer odgovora: ?Cal,0  <- neuspesna kalibracija
        break;

      case 'h':
        EC.send_cmd_with_num("Cal,high,", userVal);
        delay(600);
        EC.receive_cmd(recv_buf, 32);
        print_success_or_error(EC, "Executed \"Cal,high,N\" command successfully.");
        EC.send_cmd("Cal,?");
        delay(300);
        receive_and_print_response(EC);   // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                          // primer odgovora: ?Cal,0  <- neuspesna kalibracija
        break;

      default:
        Serial.println("Invalid option. Skipping EC calibration.");
        break;
    }
  }
}

void RTD_calibration()
{
  float userValue;
  char recv_buf[32];
  char send_buf[32];

  Serial.println("\nCalibrating RTD sensor.\nEnter a reference value, or input 'c' to clear calibration data:");
  while(!Serial.available()) {
    // print the sensor readings to the user while waiting
    RTD.send_read_cmd();
    delay(1000);
    receive_and_print_reading(RTD);
  }
  Serial.println("\n\n");

  if (Serial.peek() == 'c')    // ako korisnik unese 'c' ide clear komanda, u suprotnom smatram da je broj i ide kalibracija
  {   
    while (Serial.available()) Serial.read(); 

    RTD.send_cmd("Cal,clear");
    delay(300);

    RTD.receive_cmd(recv_buf, 32);
    print_success_or_error(RTD, "Executed \"Cal,clear\" command successfully.\n");
  } 
  else 
  {
    userValue = Serial.parseFloat();
    while (Serial.available()) Serial.read(); 
    
    sprintf(send_buf, "Cal,%.3f", userValue);
    RTD.send_cmd(send_buf);
    delay(600);

    RTD.receive_cmd(recv_buf, 32);
    print_success_or_error(RTD, "Executed \"Cal,N\" command successfully.\n");

    RTD.send_cmd("Cal,?");
    delay(300);
    receive_and_print_response(RTD);    // ovde samo printam response, moze se to ulepsati i izmeniti izvrsavanje u skladu sa odgovorom
                                        // primer odgovora: ?Cal,0  <- neuspesna kalibracija 
  }
}
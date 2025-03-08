#include "LoRa_util.h"
#include "Ezo_i2c.h"
#include "Ezo_i2c_util.h"
#include "BG95.h"
#include "SD_util.h"
#include "DS3231M_util.h"

float orp_val;
float ph_val;
float do_val;
float ec_val;
float rtd_val;
uint32_t timestamp;
char newCsvRow[128];
char payload[128];

void setup() 
{
  // Initialize I2C & UARTs
  Wire.begin();           // Sensors
  Serial.begin(115200);   // Terminal
  BG95_Serial.begin(115200, SERIAL_8N1, BG95_RX_PIN, BG95_TX_PIN);  // BG95 serial 

  // Init MCU, taken from Heltec LoRa example
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);  

  // Initialize DS3231M module
  DS3231M_init();

  // Initialize SD card module
  SD_module_init();

  // BG95 test - comment out the following 2 lines if nothing works
  //while(!BG95_turnOn());  // SET BG95 RESET PIN NUMBER (BG95_PWRKEY) IN BG95.h
  //BG95_testIfAlive();
  //BG95_nwkRegister(); // if BG95 is alive, use this to attach to the network

  calibration_call_delay(5000);   // fenseraj koji ceka proizvoljan broj sekundi da korisnik unese 'c' ako hoce kalibraciju (moze se izmeniti na taster)
}

void loop()
{     
  /*
    U INIT stanju se ispise vreme i kreira fajl za merenja na SD kartici, pa inicijalizuje LoRa
    U JOIN stanju se LoRa povezuje na mrezu
    U SEND stanju se citaju senzori, upisuju merenja na SD i inicijalizuje slanje preko mreze
    Nakon SLEEP stanja se automatski vraca u SEND gde ponovo ide slanje preko mreze i rad sa SD 
  */  
  switch (deviceState) {
    case DEVICE_STATE_INIT:
      {
        Serial.println("INIT state");

        // print current time with DS3231M
        DS3231M_get_and_print_time();

        // create a new file in the root directory for the sensors' readings (format: YY-MM-DD.csv)
        sprintf(filePath, "/%04d-%02d-%02d.csv", currentTime.year(), currentTime.month(), currentTime.day());

        if (!SD.exists(filePath)) {
          Serial.println("Creating a new CSV file for today's date.");
          writeFile(SD, filePath, "Date,Time,ORP,PH,DO,EC,RTD");      // create a new .csv file with columns defined as the last call argument
        } else {
          Serial.println("File for current date already exists.");
        }

        // list the root directory
        listDir(SD, "/", 0);

        // initialize LoRa
#if (LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
        LoRaWAN.init(loraWanClass, loraWanRegion);
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
        // read all senzors
        // DODATI ZA I2C SWITCHER PALJENJE POJEDINACNIH MAGISTRALA PRE GADJANJA SVAKOG SENZORA
        ORP.send_read_cmd();  // sensor needs 900ms for the reading, according to the datasheet 
        delay(800);
        receive_and_print_reading(ORP);

        delay(50);

        PH.send_read_cmd();   // 900ms
        delay(800);
        receive_and_print_reading(PH);

        delay(50);

        DO.send_read_cmd();   // 600ms 
        delay(600);
        receive_and_print_reading(DO);    

        delay(50);

        RTD.send_read_cmd();  // 600ms
        delay(1000);          
        receive_and_print_reading(RTD);

        delay(50);

        EC.send_read_cmd();   // 600ms
        delay(600);
        receive_and_print_reading(EC);
        
        // prepare the readings for local/remove saving
        DateTime now = DS3231M.now();  // get the current time
        orp_val = ORP.get_last_received_reading();
        ph_val = PH.get_last_received_reading();
        do_val = DO.get_last_received_reading();
        ec_val = EC.get_last_received_reading();
        rtd_val = RTD.get_last_received_reading();
        // Bolje ovako kao ispod pa da se salje UTC u sekundama kao polje (format poruke: <sto_si_vec_imao>,sent_time=timestamp)
        timestamp = now.unixtime();
        
        // prepare a new csv row with the readings:
        // UNIX_UTC | ORP | PH | DO | EC | RTD  
        snprintf(newCsvRow, sizeof(newCsvRow), "%d,%.3f,%.3f,%.3f,%.3f,%.3f\n",
             timestamp,
             orp_val,
             ph_val,
             do_val,
             ec_val,
             rtd_val);

        // prepare a payload for LoRa & NB-IoT with the readings
        snprintf(payload, sizeof(payload), "testpacket,board=buoyV1_1,packet=udp ORP=%.3f,PH=%.3f,DO=%.3f,EC=%.3f,RTD=%.3f,UNIXUTC=%d",
            orp_val, ph_val, do_val, ec_val, rtd_val, timestamp);

        // save the new readings to the SD card
        appendFile(SD, filePath, newCsvRow); 

        // send the readings via LoRa
        prepareTxFrame(appPort, payload, strlen(payload));
        LoRaWAN.send();  

        // send the readings via NB-IoT
        //BG95_TxRxUDP(payload, SERVER_IP, UDP_PORT); 

        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        Serial.println("CYCLE state");
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP; 
        Serial.println("Going to SLEEP state");
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        // dodati sleep za ostale periferije

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


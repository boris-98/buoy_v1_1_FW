#include "LoRa_util.h"
#include "Ezo_i2c.h"
#include "Ezo_i2c_util.h"
#include "BG95.h"
#include "SD_util.h"
#include "DS3231M_util.h"


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
  BG95_testIfAlive();
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
        delay(900);
        receive_and_print_reading(ORP);

        PH.send_read_cmd();   // 900ms
        delay(900);
        receive_and_print_reading(PH);

        DO.send_read_cmd();   // 600ms 
        delay(600);
        receive_and_print_reading(DO);    

        EC.send_read_cmd();   // 600ms
        delay(600);
        receive_and_print_reading(EC);

        RTD.send_read_cmd();  // 600ms
        delay(600);          
        receive_and_print_reading(RTD);

        // prepare a new csv row for the readings:
        // YY-MM-DD | HH-MM-SS | ORP | PH | DO | EC | RTD  
        char newCsvRow[128];
        char timestamp[19];
        DateTime now = DS3231M.now();  // get the current time
        float orp_val = ORP.get_last_received_reading();
        float ph_val = PH.get_last_received_reading();
        float do_val = DO.get_last_received_reading();
        float ec_val = EC.get_last_received_reading();
        float rtd_val = RTD.get_last_received_reading();
        
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d,%02d:%02d:%02d");
        snprintf(newCsvRow, sizeof(newCsvRow), "%s,%.3f,%.3f,%.3f,%.3f,%.3f\n",
             timestamp,
             orp_val,
             ph_val,
             do_val,
             ec_val,
             rtd_val);

        // save the new readings to the SD card
        appendFile(SD, filePath, newCsvRow); 

        // send the readings via LoRa
        prepareTxFrame(appPort, timestamp, orp_val, ph_val, do_val, ec_val, rtd_val);
        LoRaWAN.send();    

        // send the readings via NB-IoT
        // DODAJ SLANJE PREKO BG95 (samo da se napravi payload i posalje)  

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


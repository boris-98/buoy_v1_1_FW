#include <Ezo_i2c_util.h>

Ezo_board DO  = Ezo_board(I2C_ADDRESS_DO,  "DO");     //create a RTD circuit object who's address is I2C_ADDRESS_DO and name is "DO"
Ezo_board ORP = Ezo_board(I2C_ADDRESS_ORP, "ORP");    //create a RTD circuit object who's address is I2C_ADDRESS_ORP and name is "ORP"
Ezo_board PH  = Ezo_board(I2C_ADDRESS_PH,  "PH");     //create a PH circuit object, who's address is I2C_ADDRESS_PH and name is "PH"
Ezo_board EC  = Ezo_board(I2C_ADDRESS_EC,  "EC");     //create an EC circuit object who's address is I2C_ADDRESS_EC and name is "EC"
Ezo_board RTD = Ezo_board(I2C_ADDRESS_RTD, "RTD");    //create a RTD circuit object who's address is I2C_ADDRESS_RTD and name is "RTD"

// prints the boards name and I2C address
void print_device_info(Ezo_board &Device) {
  Serial.print(Device.get_name());
  Serial.print(" ");
  Serial.print(Device.get_address());
}

// used for printing either a success_string message if a command was successful or the error type if it wasnt
void print_success_or_error(Ezo_board &Device, const char* success_string) {
  switch (Device.get_error()) {             //switch case based on what the response code is.
    case Ezo_board::SUCCESS:
      Serial.print(success_string);   //the command was successful, print the success string
      break;

    case Ezo_board::FAIL:
      Serial.print("Failed ");        //means the command has failed.
      break;

    case Ezo_board::NOT_READY:
      Serial.print("Pending ");       //the command has not yet been finished calculating.
      break;

    case Ezo_board::NO_DATA:
      Serial.print("No Data ");       //the sensor has no data to send.
      break;
    case Ezo_board::NOT_READ_CMD:
      Serial.print("Not Read Cmd ");  //the sensor has not received a read command before user requested reading.
      break;
  }
}

void receive_and_print_response(Ezo_board &Device) {
  char receive_buffer[32];                  //buffer used to hold each boards response
  Device.receive_cmd(receive_buffer, 32);   //put the response into the buffer

  print_success_or_error(Device, " - ");          //print if our response is an error or not
  print_device_info(Device);                //print our boards name and address
  Serial.print(": ");
  Serial.println(receive_buffer);           //print the boards response
}

void receive_and_print_reading(Ezo_board &Device) {              // function to decode the reading after the read command was issued
  Serial.print(Device.get_name()); Serial.print(": "); // print the name of the circuit getting the reading
  Device.receive_read_cmd();              //get the response data and put it into the [Device].reading variable if successful
  print_success_or_error(Device, String(Device.get_last_received_reading(), 2).c_str());  //print either the reading or an error message
}


//============================================================================//
// My Ezo_i2c_util functions
//----------------------------------------------------------------------------//
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
//----------------------------------------------------------------------------//
//============================================================================//



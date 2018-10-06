//LAB3
#include <Wire.h>
#include <Adafruit_MPL3115A2.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_GPS.h>
#include <RH_RF95.h>
#include <TeensyThreads.h>


//LoRA-#####################################################################
//Mission Control caller ID
const uint8_t MC_ID = 8;

//Flight Computer caller ID
const uint8_t FC_ID = 9;

//two types of commands: set state & set throttle
uint8_t CMD_SEND_BACK = 0x01; // B 0000 0001;
uint8_t CMD_SET_NAME = 0x02; // B 0000 0010;

//the command I will be sending: nr.1-command type, nr.2-value and nr.3-error check.
const int MESSAGE_SIZE = 3;
uint8_t MESSAGE[MESSAGE_SIZE];
uint8_t CMD_TYPE;
uint8_t CMD_VALUE;
uint8_t CMD_CHECK;
//the data I will be recieving from the Flight Computer
uint8_t DATA_TYPE;
uint8_t DATA_VALUE;
uint8_t DATA_ERROR;


//GPS-#####################################################################
// what's the name of the hardware serial port?
#define GPSSerial Serial5
// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false
//#####################################################################

uint32_t timer = millis();

//hitamaelir er a pinna 14 sem er A0!!!!!!!!!!!!!!

//Fyrir ANALOG hitanema:
#define TMP 14
int VALUE;
float TEMP;

//Fyrir h��arnema
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
//BREYTURR----------------------------------------------
float pascals;
float altm;
float tempC;


//fyrir radio
RH_RF95 rf95(10,24);


//fyrir SD kort
// change this to match your SD shield or module;
 // Teensy 3.5 & 3.6 on-board: BUILTIN_SDCARD
const int chipSelect = BUILTIN_SDCARD;

//---------------------------------------------

void setup()
{
   Serial.begin(115200);
	
   analogReadResolution(12);
   pinMode(TMP, INPUT);

	pinMode(PIN_A17, OUTPUT);

	//SD KORT ___________________________________________________________
	Serial.print("Initializing SD card...");

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
		return;
	}
	Serial.println("card initialized.");
	//H��ARNEMI ___________________________________________________________
	//Chekka hvort h��arnemi s� tengdur
	while (!baro.begin()) {
		Serial.println("Couldnt find Altidude sensor...");
		delay(100);
	}

	//GPS ___________________________________________________________
	// 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
	GPS.begin(9600);
	// uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
	GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
	// uncomment this line to turn on only the "minimum recommended" data
	//GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
	// For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
	// the parser doesn't care about other sentences at this time
	// Set the update rate
	GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
	// For the parsing code to work nicely and have time to sort thru the data, and
	// print it out we don't suggest using anything higher than 1 Hz

	// Request updates on antenna status, comment out to keep quiet
	GPS.sendCommand(PGCMD_ANTENNA);

	delay(1000);

	// Ask for firmware version
	GPSSerial.println(PMTK_Q_RELEASE);

	Serial.println("GPS DONE");

	
	//RADIO ___________________________________________________________
	if (!rf95.init()) {
		Serial.println("init failed");
	}
	else { Serial.println("RF95 Success"); }
	

	Serial.println("Done setup");
	// Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

	//�r��ir
	threads.addThread(updateWheater);
	

	Serial.println("Thread set");

	digitalWrite(PIN_A17, HIGH);
}

void readGPS() {
		
	// read data from the GPS in the 'main loop'
	GPS.read();
	// if you want to debug, this is a good time to do it!
	//if (GPSECHO)
	//	if (c) Serial.print(c);
	// if a sentence is received, we can check the checksum, parse it...
	if (GPS.newNMEAreceived()) {
		// a tricky thing here is if we print the NMEA sentence, or data
		// we end up not listening and catching other sentences!
		// so be very wary if using OUTPUT_ALLDATA and trytng to print out data
		//Serial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
		if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
			return; // we can fail to parse a sentence in which case we should just wait for another
	}
	
	
	
}

//�etta fall er � �r��i og er kalla� s�felt � �a�. 
void updateWheater() {

	while(1){
		//Serial.println("----WHEATER UPDATE-----");
		pascals = baro.getPressure();
		threads.delay(50);

		altm = baro.getAltitude();
		threads.delay(50);
		
		tempC = baro.getTemperature();
		
		threads.yield();
	}

}

bool logToSDCard(const char *name) {
	
	//Serial.println("----LOG UPDATE-----");
	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	File dataFile = SD.open(name, FILE_WRITE);
	   
	// if the file is available, write to it:
	if (dataFile) {

		dataFile.print(GPS.day, DEC); dataFile.print('/');
		dataFile.print(GPS.month, DEC); dataFile.print("/20");
		dataFile.print(GPS.year, DEC);
		dataFile.print(",");

		dataFile.print(GPS.hour, DEC); dataFile.print(':');
		dataFile.print(GPS.minute, DEC); dataFile.print(':');
		dataFile.print(GPS.seconds, DEC);
		dataFile.print(",");

		dataFile.print(GPS.latitude, 4); dataFile.print(GPS.lat);
		dataFile.print(",");
		
		dataFile.print(GPS.longitude, 4); dataFile.print(GPS.lon);
		dataFile.print(",");

		dataFile.print(GPS.altitude);
		dataFile.print(",");

		dataFile.print(tempC);
		dataFile.print(",");
		
		dataFile.print(altm);
		dataFile.print(",");

		dataFile.print((int)GPS.fix);
		dataFile.print(",");
		
		dataFile.print((int)GPS.fixquality);
		dataFile.print(",");
		
		dataFile.print((int)GPS.satellites);
		dataFile.println(";");

		dataFile.close();
		// print to the serial port too:
		Serial.println("#Log Done.");
		return true;
	}
	// if the file isn't open, pop up an error:
	else {
		Serial.println("-error opening datalog.txt");
		return false;
	}
	Serial.println();



}

void printToSerial() {
	
	Serial.print("Alt:"); Serial.print(altm); Serial.println(" meters");
	Serial.print("Temp:"); Serial.print(tempC); Serial.println("*C");
	Serial.print("Pressure:"); Serial.print(pascals / 1000); Serial.println(" mBar");

	Serial.print("\nTime: ");
	Serial.print(GPS.hour, DEC); Serial.print(':');
	Serial.print(GPS.minute, DEC); Serial.print(':');
	Serial.print(GPS.seconds, DEC); Serial.print('.');
	Serial.println(GPS.milliseconds);
	Serial.print("Date: ");
	Serial.print(GPS.day, DEC); Serial.print('/');
	Serial.print(GPS.month, DEC); Serial.print("/20");
	Serial.println(GPS.year, DEC);
	Serial.print("Fix: "); Serial.print((int)GPS.fix);
	Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
	if (GPS.fix) {
		Serial.print("Location: ");
		Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
		Serial.print(", ");
		Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
		Serial.print("Speed (knots): "); Serial.println(GPS.speed);
		Serial.print("Angle: "); Serial.println(GPS.angle);
		Serial.print("Altitude: "); Serial.println(GPS.altitude);
		Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
	}


}

void sendMessage()
{
	
	CMD_TYPE = 0x01;
	CMD_VALUE = TEMP;


	//check is the XOR of TYPE and VALUE
	CMD_CHECK = CMD_TYPE ^ CMD_VALUE;
	MESSAGE[0] = CMD_TYPE;
	MESSAGE[1] = CMD_VALUE;
	MESSAGE[2] = CMD_CHECK;

	
	rf95.setHeaderId(MC_ID);
	Serial.print("Transmitting ");
	//Serial.println(MC_ID);
	rf95.send((uint8_t *)MESSAGE, MESSAGE_SIZE);
	rf95.waitPacketSent();
	Serial.println("Done");
	//Serial.print("#Message Sent");
	//Serial.print(CMD_TYPE, HEX);
	//Serial.print(" Data: ");
	//Serial.println(CMD_VALUE, DEC);

	
	//recieveDATA();

}

void recieveDATA()
{
	uint8_t buf[MESSAGE_SIZE];
	uint8_t len = sizeof(buf);

	//Serial.println("Waiting for reply...");
	//MAX wait for DATA is set 1 sec.
	if (rf95.waitAvailableTimeout(100))
	{
		if (rf95.recv(buf, &len))
		{
			if (rf95.headerId() == FC_ID) {
				if (buf[2] == (buf[0] ^ buf[1]))
				{
					DATA_TYPE = buf[0];
					DATA_VALUE = buf[1];
					DATA_ERROR = buf[2];

					switch (DATA_TYPE)
					{
					case 0x01:
						Serial.print("#ACK COMMAND: ");
						Serial.println(buf[1]);
						break;

					case 0x02:
						Serial.print("#BATT VOLTAGE: ");
						Serial.println(buf[1]);
						break;
					case 0x03:
						Serial.println("#Command FAIL");
						break;
					default:
						Serial.println("#FAILED");
						break;
					}
				}
				else
				{
					Serial.println("CheackSum ERROR");
					

				}
				//Serial.println((char*)buf);
				//Serial.print("RSSI: ");
				//Serial.println(rf95.lastRssi(), DEC);
			}
			else {
				Serial.println("Not my message, ID:");
				Serial.println(rf95.headerId());
			}

		}
		else
		{
			Serial.println("Receive failed");
			
		}
	}
	else
	{
		Serial.println("ACK not received");
		
	}

}

float modifiedMap(float x, float in_min, float in_max, float out_min, float out_max)
{
	float temp = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	temp = (float)(4 * temp);
	return (float)temp / 4;
}

void readTEMP()
{
	VALUE = analogRead(TMP);
	TEMP = modifiedMap(VALUE, 663, 930, 0.00, 25.00);
}


void loop()
{
	
	readGPS(); //GPS er ekki � �r��i �v� �a� kom einvher bugg
	// if millis() or timer wraps around, we'll just reset it
	if (timer > millis()) timer = millis();

	// approximately every 2 seconds or so, print out the current stats
	if (millis() - timer > 1000) {
		timer = millis(); // reset the timer
		readTEMP();			// B�ta vi� a� �essi loggast l�ka � SD-korti�

		printToSerial();
		logToSDCard("GPS.txt");

		sendMessage();
		Serial.println("_________________________________");

	}

	

}









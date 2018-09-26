//LAB3
#include <Wire.h>
#include <Adafruit_MPL3115A2.h>
#include <SD.h>
#include <SPI.h>
#include "TinyGPS++.h"


//Fyrir hæðarnema
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();

//Fyrir GPS, notum TinyGPS++
TinyGPSPlus gps;


 // change this to match your SD shield or module;
 // Teensy 3.5 & 3.6 on-board: BUILTIN_SDCARD
const int chipSelect = BUILTIN_SDCARD;
//BREYTURR----------------------------------------------
float pascals;
float altm;
float tempC;



//---------------------------------------------

void setup()
{

	//Wire1.begin();
	

   // Open serial communications and wait for port to open:
	Serial.begin(9600);

	//GPS Serial
	Serial5.begin(9600);

	pinMode(PIN_A17, OUTPUT);

	Serial.print("Initializing SD card...");

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
		return;
	}
	Serial.println("card initialized.");


	//Chekka hvort hæðarnemi sé tengdur
	while (!baro.begin()) {
		Serial.println("Couldnt find Altidude sensor...");
		delay(100);
		
	}

}






void updateWheater() {
	
	pascals = baro.getPressure();
	Serial.print(pascals / 1000); Serial.print(" mBar#  ");

	altm = baro.getAltitude();
	Serial.print(altm); Serial.print(" meters#  ");

	tempC = baro.getTemperature();
	Serial.print(tempC); Serial.println("*C#");
}





bool logData(const char *name) {

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	File dataFile = SD.open(name, FILE_WRITE);
	   
	// if the file is available, write to it:
	if (dataFile) {
		dataFile.print("DATA;");
		dataFile.print(tempC);
		dataFile.print(";");
		dataFile.print(altm);
		dataFile.print(";");
		dataFile.print(gps.location.lat(), 6);
		dataFile.print(";");
		dataFile.print(gps.location.lng(), 6);
		dataFile.println(";");

		
		dataFile.close();
		// print to the serial port too:
		Serial.println("Log Done.");
		return true;
	}
	// if the file isn't open, pop up an error:
	else {
		Serial.println("error opening datalog.txt");
		return false;
	}



}

void loop()
{

	updateWheater();
	
	delay(250);
	while (Serial5.available() > 0){
		gps.encode(Serial5.read());
		//Serial.write(Serial5.read());
	}

	logData("Loggari.txt");

	Serial.print(" #LAT=");  Serial.print(gps.location.lat(), 6);
	Serial.print(" #LONG="); Serial.print(gps.location.lng(), 6);
	Serial.print(" #ALT=");  Serial.println(gps.altitude.meters());

	digitalWrite(PIN_A17, HIGH);

	Serial.print(" #SAT=");  Serial.println(gps.satellites.value());



}









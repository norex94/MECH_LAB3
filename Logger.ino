//LAB3
#include <Wire.h>
#include <Adafruit_MPL3115A2.h>
#include <SD.h>
#include <SPI.h>
#include "TinyGPS++.h"


Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();


TinyGPSPlus gps;


 // change this to match your SD shield or module;
 // Teensy 3.5 & 3.6 on-board: BUILTIN_SDCARD

const int chipSelect = BUILTIN_SDCARD;

void setup()
{

	Wire.setSDA(PIN_A19);
	Wire.setSCL(PIN_A18);

	Wire.begin();
	

   // Open serial communications and wait for port to open:
	Serial.begin(9600);

	while (!Serial) {
		; // wait for serial port to connect. Needed for Leonardo only
	}
	//GPS Serial
	Serial5.begin(9600);

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

void loop()
{

	float pascals = baro.getPressure();
	// Our weather page presents pressure in Inches (Hg)
	// Use http://www.onlineconversion.com/pressure.htm for other units
	Serial.print(pascals / 3377); Serial.println(" Inches (Hg)");

	float altm = baro.getAltitude();
	Serial.print(altm); Serial.println(" meters");

	float tempC = baro.getTemperature();
	Serial.print(tempC); Serial.println("*C");

	// make a string for assembling the data to log:
	String dataString = "";




	// read three sensors and append to the string:
	for (int analogPin = 0; analogPin < 3; analogPin++) {
		int sensor = analogRead(analogPin);
		dataString += String(sensor);
		if (analogPin < 2) {
			dataString += ",";
		}
	}
	delay(250);
	while (Serial5.available() > 0){
		//gps.encode(Serial5.read());
		Serial.write(Serial5.read());
	}

	//Serial.print("LAT=");  Serial.println(gps.location.lat(), 6);
	//Serial.print("LONG="); Serial.println(gps.location.lng(), 6);
	//Serial.print("ALT=");  Serial.println(gps.altitude.meters());


// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	File dataFile = SD.open("datalog.txt", FILE_WRITE);

	// if the file is available, write to it:
	if (dataFile) {
		dataFile.println(dataString);
		dataFile.close();
		// print to the serial port too:
		//Serial.println(dataString);
	}
	// if the file isn't open, pop up an error:
	else {
		//Serial.println("error opening datalog.txt");
	}
	delay(10);
}









/*
 Name:		FlightController.ino
 Created:	09/16/2018 11:24:47
 Author:	Arnór
*/



#include <SPI.h>
#include <RH_RF95.h>


// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 430.0

#define RFM95_CS		5
#define RFM95_RST		6
#define ERRORLED		9
#define RFM95_INT		10

#define FUELPUMP		15
#define ENGINE			0
#define PARACHUTE		1
#define BATTERY			14

#define REPLY_ACK 0x01
#define REPLY_BATT 0x02
#define REPLY_ERROR 0x03	

//Mission Control caller ID
const uint8_t MC_ID = 9;

//Flight Computer caller ID
const uint8_t FC_ID = 8;

//Data I will be sending to Mission Control
uint8_t DATA_;
uint8_t DATA_TEMP;
uint8_t DATA_CHECK;

//Commands I will be recieving from Mission Control
uint8_t CMD_TYPE;

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// packet counter, we increment per transmission
int16_t packetnum = 0;

void setup()
{
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);
	pinMode(ERRORLED, OUTPUT);
	digitalWrite(ERRORLED, LOW);
	
	pinMode(ENGINE, OUTPUT);
	pinMode(PARACHUTE, OUTPUT);
	pinMode(FUELPUMP, OUTPUT);

	pinMode(BATTERY, INPUT);


	//Serial.begin(115200);
	//while (!Serial) {
	//  delay(1);
	//}

	delay(100);

	Serial.println("Initializing Flight Controller");

	// manual reset
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!rf95.init()) {
		Serial.println("Connection failed!");
		while (1);
	}
	Serial.println("Connection achieved!");

	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
	if (!rf95.setFrequency(RF95_FREQ)) {
		Serial.println("setFrequency failed");
		while (1);
	}
	Serial.print("Frequency set to: "); Serial.println(RF95_FREQ);

	// Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

	// The default transmitter power is 13dBm, using PA_BOOST.
	// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
	// you can set transmitter powers from 5 to 23 dBm:
	rf95.setTxPower(23, false);
}

void sendBack(uint8_t CMD_VALUE)
{
	//check is the XOR of TYPE and VALUE
	CMD_CHECK = CMD_VALUE ^ CMD_TYPE;
	COMMAND[0] = CMD_TYPE;
	COMMAND[1] = CMD_VALUE;
	COMMAND[2] = CMD_CHECK;
	digitalWrite(ERRORLED, LOW);
	rf95.setHeaderId(FC_ID);
	//Serial.print("Transmitting on: ");
	//Serial.println(FC_ID);
	rf95.send((uint8_t *)COMMAND, COMMAND_size);
	delay(10);
	rf95.waitPacketSent();
	Serial.print("Reply Sent: ");
	Serial.print(CMD_TYPE, HEX);
	Serial.print(" Data: ");
	Serial.println(CMD_VALUE, DEC);
}

void recieveDATA()
{
	uint8_t buf[COMMAND_size];
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

					Serial.print("Temperature: ");
					Serial.println(DATA_VALUE);
				}
				else
				{
					Serial.println("CheckSum ERROR");
					digitalWrite(ERRORLED, HIGH);

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
			digitalWrite(ERRORLED, HIGH);							//DATA_TYPE: 1-data, 2-ack, 3-error

		}
	}
	else
	{
		Serial.println("ACK not received");
		digitalWrite(ERRORLED, HIGH);
	}

}

void loop()
{

}

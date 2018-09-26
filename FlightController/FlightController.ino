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

//two types of commands: set state & set throttle
uint8_t CMD_SET_STATE = 0x01; // B 0000 0001;
uint8_t CMD_SET_THROTTLE = 0x02; // B 0000 0010;


//the command I will be sending: nr.1-command type, nr.2-value and nr.3-error check.
const int COMMAND_size = 3;
uint8_t COMMAND[COMMAND_size];
uint8_t CMD_TYPE;
uint8_t CMD_VALUE;
uint8_t CMD_CHECK;

//the data I will be recieving from the Flight Computer
uint8_t DATA_TYPE;
uint8_t DATA_VALUE;
uint8_t DATA_ERROR;


//Data
uint8_t DATA_BATTERY;
uint8_t DATA_THROTTLE;

uint8_t Current_Stage = 0;

bool KILL_SWITCH = false;
bool FAIL_SAFE = false;

enum STATE_NAMES { START, TEST, READY, IGNITION, ASCENT, COASTING, APOGEE, DESCENT, LAND, DISABLE, RESET, SELFDESTRUCT };



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

uint32_t readBattery()
{
	//The entire voltage range is read with 32 bits and converted to 8 bits (1 byte) for transmission..
	uint32_t temp_VALUE = map(analogRead(BATTERY), 0, 1023, 0, 33);
	return temp_VALUE;

}


void setState(uint8_t state)
{
	switch (state)
	{
	case 0:
		//CMD_VALUE = START;
		//igitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	case 1:
		//CMD_VALUE = TEST;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, HIGH);
		analogWrite(FUELPUMP, 255);
		delay(2000);
		digitalWrite(ENGINE, HIGH);
		analogWrite(FUELPUMP, 100);
		delay(3000);
		analogWrite(FUELPUMP, 0);
		digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		Serial.println("Test Done.");
		break;
	case 2:
		//CMD_VALUE = READY;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	case 3:
		//CMD_VALUE = IGNITION;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, HIGH);
		break;
	case 4:
		//CMD_VALUE = ASCENT;
		//digitalWrite(ENGINE, HIGH);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, HIGH);
		break;
	case 5:
		//CMD_VALUE = COASTING;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	case 6:
		//CMD_VALUE = APOGEE;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, HIGH);
		digitalWrite(ENGINE, LOW);
		break;
	case 7:
		//CMD_VALUE = DESCENT;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, HIGH);
		digitalWrite(ENGINE, LOW);
		break;
	case 8:
		//CMD_VALUE = LAND;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	case 9:
		//CMD_VALUE = DISABLE;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	case 10:
		//CMD_VALUE = RESET;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	case 11:
		//CMD_VALUE = SELFDESTRUCT;
		//digitalWrite(ENGINE, LOW);
		digitalWrite(PARACHUTE, LOW);
		digitalWrite(ENGINE, LOW);
		break;
	default:
		break;
	}
}

void sendBack(uint8_t CMD_TYPE, uint8_t CMD_VALUE)
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
	if (rf95.waitAvailableTimeout(1000))
	{
		if (rf95.recv(buf, &len))
		{
			if (rf95.headerId() == MC_ID) 
			{
				if (buf[2] == (buf[0] ^ buf[1]))
				{
					DATA_TYPE = buf[0];
					DATA_VALUE = buf[1];
					DATA_ERROR = buf[2];
					
					
					switch (DATA_TYPE)
					{
					case 0x01:
						Serial.print("#Stage recived: ");
						Serial.println(buf[1], HEX);
						Current_Stage = buf[1];
						setState(Current_Stage);
						sendBack(REPLY_ACK, DATA_VALUE);
						break;
					case 0x02:
						Serial.print("#Throttle recived: ");
						Serial.println(buf[1], DEC);
						DATA_THROTTLE = buf[1];
						
						sendBack(REPLY_BATT, DATA_BATTERY);
						break;

					default:
						Serial.print("#Error on Stage: ");
						Serial.println(DATA_TYPE, HEX);
						sendBack(REPLY_ERROR, DATA_VALUE);
						break;
					}



				}
				else
				{
					Serial.println("CheackSum ERROR");
					digitalWrite(ERRORLED, HIGH);

				}
			}
			else { 
				Serial.println("Not my message, ID:"); 
				Serial.println(rf95.headerId());
			}
			//Serial.println((char*)buf);
			//Serial.print("RSSI: ");
			//Serial.println(rf95.lastRssi(), DEC);
		}
		else
		{
			Serial.println("Receive failed");
			digitalWrite(ERRORLED, HIGH);							//DATA_TYPE: 1-data, 2-ack, 3-error
		}
	}
	else
	{
		Serial.println("MC not sending..");
		digitalWrite(ERRORLED, HIGH);
	}

}




void loop()
{
	//sound();

	Serial.println("____________________");
	DATA_BATTERY = readBattery();
	recieveDATA();
	analogWrite(FUELPUMP, map(DATA_THROTTLE, 0, 100, 0, 255));
	//delay(50);
}

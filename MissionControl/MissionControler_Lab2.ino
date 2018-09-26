//Mission Controller


#include <SPI.h>
#include <RH_RF95.h>


// include the library code:
#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//(12, 13, 18, 17, 16, 15);
const int rs = 12, en = 13, d4 = 18, d5 = 17, d6 = 16, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 430.0
#define RFM95_CS		5
#define RFM95_RST		6
#define RFM95_INT		10

#define ERRORLED			1





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
	pinMode(PIN_THROTTLE, INPUT);
	pinMode(20, INPUT);	

	Serial.println("Initializing");

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

	rf95.setTxPower(23, false);

	//testRun();
	lcd.begin(16, 2);
	delay(10);
	lcd.print("This is working");
}

void sendCOMMAND(uint8_t CMD_TYPE, uint8_t CMD_VALUE)
{
	//check is the XOR of TYPE and VALUE
	CMD_CHECK = CMD_TYPE ^ CMD_VALUE;
	COMMAND[0] = CMD_TYPE;
	COMMAND[1] = CMD_VALUE;
	COMMAND[2] = CMD_CHECK;
	
	rf95.setHeaderId(MC_ID);
	//Serial.print("Transmitting on: ");
	//Serial.println(MC_ID);
	rf95.send((uint8_t *)COMMAND, COMMAND_size);
	delay(10);
	rf95.waitPacketSent();
	Serial.print("Command Sent: ");
	Serial.print(CMD_TYPE, HEX);
	//Serial.print(" Data: ");
	//Serial.println(CMD_VALUE, DEC);

	recieveDATA();
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

					switch (DATA_TYPE)
					{
					case 0x01:
						Serial.print("#ACK COMMAND: ");
						Serial.println(buf[1]);
						digitalWrite(ERRORLED, LOW);
						break;
						
					case 0x02:
						Serial.print("#BATT VOLTAGE: ");
						BATTERY = buf[1];
						Serial.println(buf[1]);
						digitalWrite(ERRORLED, LOW);
						break;
					case 0x03:
						Serial.println("#Command FAIL");
						digitalWrite(ERRORLED, HIGH);
						break;
					default:
						Serial.println("#FAILED");
						digitalWrite(ERRORLED, HIGH);
						break;
					}
				}
				else
				{
					Serial.println("CheackSum ERROR");
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
	printLCD();

	currentState();
	
	Serial.println("Current stage:" + (String)CurrentStateNow);

	Serial.println("____________________");
	delay(100);
}
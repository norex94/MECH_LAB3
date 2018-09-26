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
#define PIN_THROTTLE		0
#define PIN_KILL_ENGINE		22





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


//Values
uint8_t BATTERY;

uint8_t CurrentStateNow=0;
uint8_t waitTime = 0;


bool KILL_SWITCH = false;
bool FAIL_SAFE = false;

enum STATE_NAMES { START, TEST, READY, IGNITION, ASCENT, COASTING, APOGEE, DESCENT, LAND, DISABLE, RESET, SELFDESTRUCT };


// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// packet counter, we increment per transmission
int16_t packetnum = 0;


uint32_t startFlightTime = 0;
uint32_t currentFlightTime = 0;
uint32_t setTime;


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

uint32_t readThrottle()
{
	//The entire voltage range is read with 32 bits and converted to 8 bits (1 byte) for transmission..
	uint32_t temp_VALUE = map(analogRead(PIN_THROTTLE),0,1023,0,100);
	return temp_VALUE;
	
}

void setState(uint8_t state)
{
	switch (state)
	{
	case 0:
		CMD_VALUE = START;
		break;
	case 1:
		CMD_VALUE = TEST;
		break;
	case 2:
		CMD_VALUE = READY;
		break;
	case 3:
		CMD_VALUE = IGNITION;
		waitTime = 2;
		break;
	case 4:
		CMD_VALUE = ASCENT;
		waitTime = 4;
		break;
	case 5:
		CMD_VALUE = COASTING;

		break;
	case 6:
		CMD_VALUE = APOGEE;
		break;
	case 7:
		CMD_VALUE = DESCENT;
		break;
	case 8:
		CMD_VALUE = LAND;
		break;
	case 9:
		CMD_VALUE = DISABLE;
		break;
	case 10:
		CMD_VALUE = RESET;
		break;
	case 11:
		CMD_VALUE = SELFDESTRUCT;
		break;
	default:
		break;
	}
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

bool checkKillButton()
{
	if (readThrottle() < 5){
		sendCOMMAND(CMD_SET_STATE, APOGEE);
		setTimeKeeper(2000);
		CurrentStateNow = APOGEE;
		lcd.setCursor(0, 0);
		lcd.print("KILL ENGINE");
		delay(2000);
		return true; 
	}
	else{
		return false;
	}
}

void printLCD() {
	String tmp;
	if (CurrentStateNow == 0) { tmp = "0 START"; }
	else if (CurrentStateNow == 1) { tmp = "1 TEST"; }
	else if (CurrentStateNow == 3) { tmp = "3 READY"; }
	else if (CurrentStateNow == 4) { tmp = "4 IGNIT"; }
	else if (CurrentStateNow == 5) { tmp = "5 ASCEN"; }
	else if (CurrentStateNow == 6) { tmp = "6 APOGE"; }
	else if (CurrentStateNow == 7) { tmp = "7 DESCE"; }
	else if (CurrentStateNow == 8) { tmp = "8 LAND"; }
	else if (CurrentStateNow == 9) { tmp = "9 DISAB"; }
	else if (CurrentStateNow == 10) { tmp = "10 REST";}

	lcd.clear();
	   	 
	lcd.setCursor(0, 0);
	lcd.print(tmp);
	
	lcd.setCursor(8, 0);
	tmp = "BAT:" + (String)BATTERY+"V";
	lcd.print(tmp);

	lcd.setCursor(0, 1);
	tmp = "THR:" + (String)readThrottle()+"%";
	lcd.print(tmp);

	lcd.setCursor(8,1);
	tmp = "TIM:" + (String)currentFlightTime+"s";
	lcd.print(tmp);

}

void setTimeKeeper(uint32_t wait) {
	setTime = millis() + wait;
	Serial.println("Timer set.");
	return;
}

bool timeKeeper() {
	if (millis() > setTime) {
		Serial.println("Timer done.");
		return true;		
	}
	else { 
		Serial.println("Timer not done.");
		return false; 
	}
}

void flightTimeSet() {
	startFlightTime = millis();
}

void FlightTime() {

	currentFlightTime = (millis()-startFlightTime)/1000;

}

void currentState()
{
	switch (CurrentStateNow)
	{
	case 0:
		sendCOMMAND(CMD_SET_STATE, START);
		delay(2000);
		//Do nothing
		CurrentStateNow++;
		Serial.println("Current stage:" + CurrentStateNow);
		break;

	case 1:
		//Test state
		
		sendCOMMAND(CMD_SET_STATE, TEST);
		delay(500);
		//sendCOMMAND(CMD_SET_STATE, TEST);
		
		Serial.print("Test done. Last RSSI:");
		Serial.println(rf95.lastRssi());
		lcd.clear();
		lcd.setCursor(5, 0);
		lcd.print("Last RSSI:");
		lcd.setCursor(5, 1);
		lcd.print(+rf95.lastRssi());
		
		delay(6000);
		
		CurrentStateNow++;
		Serial.println("Current stage:"+CurrentStateNow);
		
		break;

	case 2:
		//Ready Stage
		lcd.setCursor(0, 0);
		lcd.print("3 READY");
		sendCOMMAND(CMD_SET_STATE, READY);
		delay(10);
		sendCOMMAND(CMD_SET_THROTTLE, readThrottle());
		
		if (readThrottle() > 10) {
			CurrentStateNow++;
			Serial.println("Current stage:" + CurrentStateNow);
			setTimeKeeper(2000);
		}
		break;
	
	case 3:
		printLCD();
		sendCOMMAND(CMD_SET_STATE, IGNITION);
		flightTimeSet();
		FlightTime();
		delay(100);
		sendCOMMAND(CMD_SET_THROTTLE, readThrottle());
		checkKillButton();
		if (timeKeeper()) {
			CurrentStateNow++;
			setTimeKeeper(8000);
			Serial.println("Current stage:" + CurrentStateNow);
		}
		break;
	
	case 4:
		sendCOMMAND(CMD_SET_STATE, ASCENT);
		FlightTime();
		delay(100);
		checkKillButton();
		sendCOMMAND(CMD_SET_THROTTLE, readThrottle());
		if (timeKeeper()) {
			CurrentStateNow++;
			setTimeKeeper(10000);	
			Serial.println("Current stage:" + CurrentStateNow);
		}
		
		break;
	case 5:
		sendCOMMAND(CMD_SET_STATE, COASTING);
		FlightTime();
		delay(100);
		checkKillButton();
		sendCOMMAND(CMD_SET_THROTTLE, 0);
		if (timeKeeper()) {
			CurrentStateNow++;
			setTimeKeeper(2000);
			Serial.println("Current stage:" + CurrentStateNow);
		}
		break;
	case 6:
		sendCOMMAND(CMD_SET_STATE, APOGEE);
		FlightTime();
		delay(100);
		checkKillButton();
		sendCOMMAND(CMD_SET_THROTTLE, 0);
		if (timeKeeper()) {
			CurrentStateNow++;
			setTimeKeeper(9000);
			Serial.println("Current stage:" + CurrentStateNow);
		}	
		break;
	case 7:
		sendCOMMAND(CMD_SET_STATE, DESCENT);
		FlightTime();
		delay(100);
		sendCOMMAND(CMD_SET_THROTTLE, 0);
		if (timeKeeper()) {
			CurrentStateNow++;
			setTimeKeeper(5000);
			Serial.println("Current stage:" + CurrentStateNow);
		}	
		break;
	case 8:
		sendCOMMAND(CMD_SET_STATE, LAND);
		delay(100);
		sendCOMMAND(CMD_SET_THROTTLE, 0);
		if (timeKeeper()) {
			CurrentStateNow++;
			setTimeKeeper(2000);
			Serial.println("Current stage:" + CurrentStateNow);
		}
		break;
	case 9:
		sendCOMMAND(CMD_SET_STATE, DISABLE);
		delay(100);
		sendCOMMAND(CMD_SET_THROTTLE, 0);
		if (readThrottle() < 5) {
			CurrentStateNow++;
			setTimeKeeper(1000);
			Serial.println("Current stage:" + CurrentStateNow);
		}
		break;
	case 10:
		sendCOMMAND(CMD_SET_STATE, RESET);
		delay(100);
		sendCOMMAND(CMD_SET_THROTTLE, 0);
		if (timeKeeper()) {
			CurrentStateNow++;
			Serial.println("Current stage:" + CurrentStateNow);
		}		
		break;
	case 11:
		CurrentStateNow = 0;
		break;
	default:
		break;
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
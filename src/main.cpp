#include <Arduino.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdarg.h>

#define SERIAL_PRINTF_MAX_BUFF 256
#define F_PRECISION 1

#define ONEWIRE_PIN 2
#define SDA 18
#define SDL 19
#define PROBE_NUMBER 5
#define MAX_PROBE_NUMBER	16
#define SCROLL_TIME 3000

const String version = "1.0";

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

DeviceAddress *addresses;
byte numberOfProbes;

/*
	EEPROM memory allocation
	0-127: probes addresses
	128: number of probes
*/


void editProbe();
void editProbesNumber();
void updateAddresses(byte numberOfProbes);
void getProbesNumber();
void setProbesNumber(byte numberOfProbes);
void serialPrintf(const char *fmt, ...);
/**
 * --------------------------------------------------------------
 * Perform simple printing of formatted data
 * Supported conversion specifiers:
 *      d, i     signed int
 *      u        unsigned int
 *      ld, li   signed long
 *      lu       unsigned long
 *      f        double
 *      c        char
 *      s        string
 *      %        '%'
 * Usage: %[conversion specifier]
 * Note: This function does not support these format specifiers:
 *      [flag][min width][precision][length modifier]
 * --------------------------------------------------------------
 */
void serialPrintf(const char *fmt, ...)
{
	/* buffer for storing the formatted data */
	char buf[SERIAL_PRINTF_MAX_BUFF];
	char *pbuf = buf;
	/* pointer to the variable arguments list */
	va_list pargs;

	/* Initialise pargs to point to the first optional argument */
	va_start(pargs, fmt);
	/* Iterate through the formatted string to replace all
	conversion specifiers with the respective values */
	while (*fmt)
	{
		if (*fmt == '%')
		{
			switch (*(++fmt))
			{
			case 'd':
			case 'i':
				pbuf += sprintf(pbuf, "%d", va_arg(pargs, int));
				break;
			case 'u':
				pbuf += sprintf(pbuf, "%u", va_arg(pargs, unsigned int));
				break;
			case 'l':
				switch (*(++fmt))
				{
				case 'd':
				case 'i':
					pbuf += sprintf(pbuf, "%ld", va_arg(pargs, long));
					break;
				case 'u':
					pbuf += sprintf(pbuf, "%lu", va_arg(pargs, unsigned long));
					break;
				}
				break;
			case 'f':
				pbuf += strlen(dtostrf(va_arg(pargs, double),
									   4, F_PRECISION, pbuf));
				break;

			case 'c':
				*(pbuf++) = (char)va_arg(pargs, int);
				break;
			case 's':
				pbuf += sprintf(pbuf, "%s", va_arg(pargs, char *));
				break;
			case '%':
				*(pbuf++) = '%';
				break;
			default:
				break;
			}
		}
		else
		{
			*(pbuf++) = *fmt;
		}

		fmt++;
	}

	*pbuf = '\0';

	va_end(pargs);
	lcd.print(buf);
}

void editProbe()
{
	Serial.println("Disconnect all probes and attach probe you want to assign");

	while (true)
	{
		Serial.println("Type \"ok\" when you are ready");
		while (Serial.available() == 0)
			;

		if (Serial.readString() != "ok")
		{
			continue;
		}
		sensors.begin();
		if (sensors.getDeviceCount() != 1)
		{
			Serial.println("Attach only 1 probe. Again");
			continue;
		}

		Serial.println("Type number to which you want to assign a probe");
		while (Serial.available() == 0)
			;
		int number = Serial.readString().toInt();
		if (number <= numberOfProbes && number >= 1)
		{
			sensors.getAddress(addresses[number - 1], 0);
			EEPROM.put((number - 1) * sizeof(DeviceAddress), addresses[number - 1]);
			Serial.println("Assigned successfully. Exiting \"edit probe\" mode");
			break;
		}
		else
		{
			Serial.println("Incorrect argument. Exiting \"edit probe\" mode");
			break;
		}
	}
}

void editProbesNumber()
{
	Serial.println("Changing number of available probes");
	Serial.print("Current number: ");
	Serial.println(numberOfProbes);

	while (true)
	{
		Serial.println("Type new number of probes");
		while (Serial.available() == 0)
				;
		int number = Serial.readString().toInt();
		if (number >= 1 && number <= MAX_PROBE_NUMBER)
		{
			setProbesNumber(number);
			Serial.println("Assigned succesfully");
			getProbesNumber();
			break;
		}
		else
		{
			Serial.println("Wrong argument. Try again");
		}
	}
}

void serialEvent()
{
	String debug = "debug";
	if (Serial.available() > 0)
	{
		String read = Serial.readString();
		if (read == debug)
		{
			Serial.println("Welcome to debug mode");
			Serial.println("Temperature monitoring has been stopped");
			Serial.println("Type a command");

			while (Serial.available() == 0)
				;

			const String name = "edit probe";

			read = Serial.readString();
			if (read == "edit probe")
			{
				editProbe();
			}
			else if (read == "version")
			{
				Serial.print("Firmware version: ");
				Serial.println(version);
			}
			else if (read == "number of probes")
			{
				editProbesNumber();
			}
			else
			{
				Serial.println("Unknown command. Closing debug mode");
			}
		}
	}
}

void updateAddresses(byte numberOfProbes)
{
	delete addresses;
	addresses = new DeviceAddress[numberOfProbes];
	for (int i = 0; i < numberOfProbes; i++)
	{
		EEPROM.get(i * sizeof(DeviceAddress), addresses[i]);
	}
}

void getProbesNumber()
{
	EEPROM.get(128, numberOfProbes);

	if (numberOfProbes == 0 || numberOfProbes > MAX_PROBE_NUMBER)
	{
		numberOfProbes = 1;
	}
}

void setProbesNumber(byte numberOfProbes)
{
	EEPROM.put(128, numberOfProbes);
}

void setup()
{
	// put your setup code here, to run once:
	Serial.begin(9600);

	sensors.begin();
	lcd.init(); // initialize the lcd
	lcd.backlight();
	lcd.setCursor(0, 0);
	lcd.print("  Temperature   ");
	lcd.setCursor(0, 1);
	lcd.print("    Sensor   ");
	getProbesNumber();
	updateAddresses(numberOfProbes);
	delay(3000);
	lcd.clear();
}
int count = 0;
unsigned long time = millis();
unsigned long moveTime = millis() + 2 * SCROLL_TIME;
float temp;
void loop()
{
	// put your main code here, to run repeatedly:
	sensors.requestTemperatures();

	/* lcd.setCursor(0, 0);
	serialPrintf("T1:%f  T2:%f", sensors.getTempC(addresses[0]), sensors.getTempC(addresses[1]));

	lcd.setCursor(0, 1);
	serialPrintf("T3:%f  T4:%f\n", sensors.getTempC(addresses[2]), sensors.getTempC(addresses[3])); */

	lcd.setCursor(0, 0);
	lcd.print("T");
	lcd.print(count + 1);
	lcd.print(":");
	if (count < numberOfProbes)
	{
		temp = sensors.getTempC(addresses[count]);
		if (temp == -127.0)
		{
			lcd.print(" ERR");
		}
		else
		{
			lcd.print(temp, 1);
		}
	}
	else
	{
		lcd.print(" OFF");
	}
	lcd.print("  ");

	lcd.print("T");
	lcd.print(count + 2);
	lcd.print(":");
	if (count + 1 < numberOfProbes)
	{
		temp = sensors.getTempC(addresses[count + 1]);
		if (temp == -127.0)
		{
			lcd.print(" ERR");
		}
		else
		{
			lcd.print(temp, 1);
		}
	}
	else
	{
		lcd.print(" OFF");
	}

	lcd.setCursor(0, 1);
	lcd.print("T");
	lcd.print(count + 3);
	lcd.print(":");
	if (count + 2 < numberOfProbes)
	{
		temp = sensors.getTempC(addresses[count + 2]);
		if (temp == -127.0)
		{
			lcd.print(" ERR");
		}
		else
		{
			lcd.print(temp, 1);
		}
	}
	else
	{
		lcd.print(" OFF");
	}
	lcd.print("  ");

	lcd.print("T");
	lcd.print(count + 4);
	lcd.print(":");
	if (count + 3 < numberOfProbes)
	{
		temp = sensors.getTempC(addresses[count + 3]);
		if (temp == -127.0)
		{
			lcd.print(" ERR");
		}
		else
		{
			lcd.print(temp, 1);
		}
	}
	else
	{
		lcd.print(" OFF");
	}

	time = millis();
	if (time >= moveTime)
	{
		moveTime = millis() + SCROLL_TIME;

		if (numberOfProbes % 4 != 0)
		{
			count = (count + 4) % (4 * ((numberOfProbes / 4) + 1));
		}
		else
		{
			count = (count + 4) % (numberOfProbes);
		}
	}

	serialEvent();
}
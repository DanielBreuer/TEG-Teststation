 /*
 Name:		TEG_Teststation.ino
 Created:	8/14/2020 5:18:31 PM
 Author:	f
 Beschreibung:
 Kontinuierliche Messungen von Strom und Spannung des TEGs und des Heizeelements
sowie der temperatur der kalten und heißen seite.

aufbau als unterprogramme, welche für jede messung aufgerufen werden und den aktuellen wert zurückgeben
die Übergabeparameter sind jeweils die Arduino pins, an denen das Modul angeschlossen ist. Diese werden im header definiert

im hauptprogramm werden die messungen zyklisch mit eienr variablen zyklusdauer abgefragt und anschließend auf die sd karte gespeichert

auf der sd karte werden die verschiedenen daten auf verschiedene dateien abgespeichert, jeweil dollen wertepaare sowie die zyklusanzahl abgespeichert werden
überlegung ob die vergangene zeit als lineare x achse in excel verwendet werden kann und so alle daten in der y achse angegeben werden kann

ergänzung:
Ausgabe der aktuellen Daten am Oled display

auslesen der zyklusanzahl aus der sd karte
*/

//defines:
#define SD_CARD 10			//definition des arduino pin, an dem das SD-Karten Modul angeschlosssen ist
#define VOLTAGE_TEG 1		//definition des arduino pin, an dem das Modul angeschlosssen ist, welches die spannung am TEG misst
#define VOLTAGE_HEATING 1	//definition des arduino pin, an dem das Modul angeschlosssen ist, welches die spannung am Heizelement misst
#define CURRENT_TEG 1		//definition des arduino pin, an dem das Modul angeschlosssen ist, welches den Strom am TEG misst
#define CURRENT_HEATING 1	//definition des arduino pin, an dem das Modul angeschlosssen ist, welches den Strom am Heizelement misst
//thermocouple pin defines
#define TEMP_HOT_SCK 2		//definition des arduino pin, an dem der SCK pin des Modul angeschlosssen ist, welches die Temperatur auf der Heißen Seite misst
#define TEMP_HOT_CS 3		//definition des arduino pin, an dem der CS pin des Modul angeschlosssen ist, welches die Temperatur auf der Heißen Seite misst
#define TEMP_HOT_SO 4		//definition des arduino pin, an dem der SO pin des Modul angeschlosssen ist, welches die Temperatur auf der Heißen Seite misst
#define TEMP_COLD_SCK 1		//definition des ersten arduino pin, an dem der SCK pin des Modul angeschlosssen ist, welches die Temperatur auf der Kalten Seite misst
#define TEMP_COLD_CS 1		//definition des ersten arduino pin, an dem der CS pin des Modul angeschlosssen ist, welches die Temperatur auf der Kalten Seite misst
#define TEMP_COLD_SO 1		//definition des ersten arduino pin, an dem der SO pin des Modul angeschlosssen ist, welches die Temperatur auf der Kalten Seite misst
//define type for function call
#define Cold 0
#define HOT 1
#define DIFF 2
//Oled pin defines
#define OLED_RESET -1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


//Includes:
#include <Arduino.h>

#include <SPI.h> //for sd card module, oled module 
//SD_Card Module
#include <SD.h>

//temperature Module
#include "max6675.h"
//oled module
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#include <Adafruit_Sensor.h>

//Constructor:
File sdcard_file; //SD card c
//oled 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Adafruit_SDD1306 display(OLED_RESET);//oled 

//muss eventuell noch aufgrund von mehrfachverwendung von SPI umgeschrieben werden, siehe Readme at: https://github.com/SirUli/MAX6675 
MAX6675 tempCold(TEMP_COLD_SCK, TEMP_COLD_CS , TEMP_COLD_SO);
MAX6675 tempHot(TEMP_HOT_SCK, TEMP_HOT_CS, TEMP_HOT_SO);

//inits
//voltage Sensor 
	int value = 0;
	double voltage = 0.0;
	int R1 = 30000;
	int R2 = 7500;

//Current Sensor
	int sensitivity = 66;
	int adcValue = 0;
	int offsetVoltage = 2500;
	double adcVoltage = 0;
	double currentValue = 0;

//temperature sensor
	float tempDiff = 0;
	float tempC = 0;
	float tempH = 0;

//SD card 
String dataString = "";

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600); //Setting baudrate at 9600

	//SD_Card Modul inizialisation
	pinMode(SD_CARD, OUTPUT);//declaring SD-card pin as output pin
	if (SD.begin())
	{
		Serial.println("SD card is initialized and it is ready to use");
	}
	else
	{
		Serial.println("SD card is not initialized");
	}

	//Oled display
	display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
	delay(2000);
	display.clearDisplay();
	display.setTextColor(WHITE);


	return;
}

float m_voltage(int pin) {//Funktion zum messen von Spannung von einem Modul an einem ausgewählten pin
	//Quelle: https://www.electronicshub.org/interfacing-voltage-sensor-with-arduino/ am 14.8.2020
	value = analogRead(pin);
	voltage = (value * 5.0) / 1024.0 * (R2 / (R1 + R2));
	 
	return voltage;

}

float m_current(int pin) {//Funktion zum auslesen des Stromes von einem Modul an einem ausgewählten pin
	//Quelle: https://www.electronicshub.org/interfacing-acs712-current-sensor-with-arduino/ am 14.8.2020
	
	adcValue = analogRead(pin);
	adcVoltage = (adcValue / 1024.0) * 5000;
	currentValue = ((adcVoltage - offsetVoltage) / sensitivity);

	Serial.print("\t Current = ");
	Serial.println(currentValue, 3);

	return currentValue;
}

float m_temperature(int type) {//Funktion zum auslesen der temperatur von einem Modul an einem ausgewählten pin
	tempC = tempCold.readCelsius();
	tempH =  tempHot.readCelsius();
	tempDiff = tempH - tempC;
	if(type == 0){
		return tempC;
	}
	if(type == 1){
		return tempH;
	}
	if(type == 2){
		return tempDiff;
	}
	else 
	return -1;
	
}


void writeToSD(const char *dataName, int data1, int data2, int data3){ //Funktion zum Schreiben von Werten (Data1,...) auf die SD-Karte auf eine Datei (dataName) 
	//Quelle: https://electronicshobbyists.com/arduino-sd-card-shield-tutorial-arduino-data-logger/ am 14.8.2020
	sdcard_file = SD.open(dataName, FILE_WRITE); //Looking for the data.txt in SD card

	if (sdcard_file) { //If the file is found
		Serial.println("Writing to file is under process");
		dataString += String(data1);
		dataString += String(data2);
      	dataString += ",";
		dataString += String(data2);
		sdcard_file.println(dataString ); //Schreiben der beiden Datenreihen auf SD-Karte mit einem Bestrich als Seperator
		sdcard_file.close(); //Closing the file
		Serial.println("Done");
	}
	else {
		Serial.println("Failed to open the file");
	}
}

int getCycleCount() {
	//Quelle: https://electronicshobbyists.com/arduino-sd-card-shield-tutorial-arduino-data-logger/ am 14.8.2020
	int cycleCount = 0;
	sdcard_file = SD.open("Zyklus.txt");
	if (sdcard_file) {
		Serial.println("Reading from the file");
		while (sdcard_file.available()) {
			//letzte zeile auslesen:
			//siehe https://forum.arduino.cc/index.php?topic=95303.0
			//muss ergänzt werden!!
			
		
			//Serial.write(sdcard_file.read());
		}
		sdcard_file.close();
	}
	else {
		Serial.println("Failed to open the file");
	}
	return cycleCount;
}

void regulateTemperature(int power, int maxTemperature) { //Funktion zum Ansprechen und Regeln des Heizmoduls

}

void printToOled(const char *name, int value, int unit, int colum) {//Ausgabe auf das Oled Modul unit: 1 = C 2 = W
	//Quelle:https://www.electroniclinic.com/multiple-max6675-arduino-based-industrial-temperature-monitoring-system/ am 14.8.2020
	//display.clearDisplay();

	if (colum == 1) { //Ausgabe auf die 1. Zeile
		display.setTextSize(1);
		display.setCursor(0, 0);
		display.print(name);
		display.setTextSize(2);
		display.setCursor(38, 0);
		display.print(value);
		display.print(" ");
		
		if (unit == 1) { //wenn die einheit grad Celsius ist
			display.setTextSize(1);
			display.cp437(true);
			display.write(167);
			display.setTextSize(2);
			display.print("C");
		}
		if (unit == 2) {//wenn die einheit watt ist
			display.setTextSize(2);
			display.print("W");
		}
		
	}
	
	if (colum == 2) { //Ausgabe auf die 2. Zeile
		display.setTextSize(1);
		display.setCursor(0, 20);
		display.print(name);
		display.setTextSize(2);
		display.setCursor(38, 20);
		display.print(value);
		display.print(" ");

		if (unit == 1) {//wenn die einheit grad Celsius ist
			display.setTextSize(1);
			display.cp437(true);
			display.write(167);
			display.setTextSize(2);
			display.print("C");
		}
		if (unit == 2) {//wenn die einheit watt ist
			display.setTextSize(2);
			display.print("W");
		}

	}

	if (colum == 3) { //Ausgabe auf die 3. Zeile
		display.setTextSize(1);
		display.setCursor(0, 40);
		display.print(name);
		display.setTextSize(2);
		display.setCursor(38, 40);
		display.print(value);
		display.print(" ");

		if (unit == 1) {//wenn die einheit grad Celsius ist
			display.setTextSize(1);
			display.cp437(true);
			display.write(167);
			display.setTextSize(2);
			display.print("C");
		}
		if (unit == 2) {//wenn die einheit watt ist
			display.setTextSize(2);
			display.print("W");
		}

	}

	if (colum == 4) { //Ausgabe auf die 4. Zeile
		display.setTextSize(1);
		display.setCursor(0, 57);
		display.print(name);
		display.setTextSize(2);
		display.setCursor(50, 57);
		display.print(value);
	}
	display.display();
}


// the loop function runs over and over again until power down or reset
void loop() {
  double temperatureHot = m_temperature(HOT);
  Serial.println(temperatureHot);
  delay(500);
  //int voltageTEG = m_voltage(16);
  printToOled("TempH:",temperatureHot,1,1);
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }
  display.display();
  //printToOled("Volt:",voltageTEG,0,2);
  
}

#include <SPI.h>
#include <EEPROMex.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MemChunck.h"
#include "Error.h"
#include <math.h>
#include <VirtuinoBluetooth.h>

#define SIZE_EEPROM 32
#define MAGIC		0xABCD1235
int _addrEEPROM = -1;

//Quantite maximum de gasoil disponible
#define QTELIMIT 220000000
//Quantite maximum de gasoil dans le reservoir principal
#define QTEMAINLIMIT 72000000

#define MODECONFIG 0
#define MODE1 1

#define BUTTON1 7
#define BUTTON2 9
#define BUTTON3 3
#define BUTTON4 6

#define AGE_ENGINE_INDEX	0
#define RPM_INDEX			1
#define CONSO_INDEX			2
#define QTE_GAZ_INDEX		3
#define TEMP_INDEX			4
#define VOLTAGE_INDEX		5
#define TEMP_COOLANT_INDEX	6


//Constante utilisee pour le rafraichissement de l'ecran
const byte TEMP     = B00000001;
const byte QTETR    = B00000010;
const byte TEMPC	= B00000100;

//Ces 3 variables peuvent etre rafraichit que dans le mode config ...
const byte TEMPALARM= B00000100;
const byte TEMPCOO  = B00001000;
const byte QTEMAIN  = B00010000;

const byte VOLT     = B00001000;
const byte AGE      = B00010000;
const byte QTETOTAL = B01000000;
const byte MODE     = B10000000;

byte _refresh       = B10000000;

bool _hasToBeSaved = false;

//Constante empirique permettant de calculer les RPM en fonction de la fr�quence du rupteur :
//RPM = ALPHA_RPM * freq_rupteur
//Pour mon moteur Nanni N3.30, cette constante vaut 6.51
const float ALPHA_RPM = (float)6.51;

//Constantes utilisees pour l'estimation de la conso (trouve a partir des donnees constructeur)
const float a = (float)1.1330288410882;
const float b = (float) -10;
const float c = (float)3.0243858131;
//Constante multiplicateur de la formule d'estimation de la conso determine empiriquement
const float d = (float)1.05;
const float e = (float)0.5;

//Temperature the buzzer turns on at
byte TEMP_ALERT;
//Temperature the buzzer turns on at
byte COOLANT_TEMP_ALERT;

//Pin analogique pour le capteur de temperature.
const char pin_capteur_temp = A0;
//Pin analogique pour le capteur de temperature.
const char pin_capteur_thermistor = A2;
//Pin digital pour alimenter le capteur de temperature temporairement ...
const int outPinAlimTemp = 8;
//Pin analogique pour mesurer la tension de la batterie moteur
const char pin_capteur_volt = A1;

//A4 and A5 are reserver for I2C ...

//On garde les pin digital 0 et 1 pour une eventuelle liaison serie ...
//Pin digitale num 2, donc il y  a la possibilite de gerer les impulsions par interruption si jamais on veut changer.
const int inpinrpm = 2;
//Pin digitale du relay de la ventilation.
const int outPinThermistor = 4;
//Pin digital du bouton poussoir numero 3 pour gerer le bouton avec des interruptions.
const int inPinButton = BUTTON2;
const int inPinButtonPlus = BUTTON4;
const int inPinButtonMoins = BUTTON3;
const int inPinButtonLum = BUTTON1;
//digital pin for the buzzer
const int outPinBuzzer = 5;

//To know if the buzzer is on or off
boolean isBuzzerOn = false;

//Pour la carte memoire
const int chipSelect = 10;

//Consommation temps reel en L/h
float _conso = 0;
//Regime moteur temps reel en tour par minute
int _rpm = 0;

//age du moteur en seconde
long _ageEngine;

//Quantite de gasoil estimee en micro litre
long _qteGas;

//Duree en seconde du total (Non reinitialisable)
long _total;

long _qteGasTotal;

//Quantite de gasoil dans le reservoir principal
long _qteGasMainTank;

//temperature of the engine coude d'echappement en degre celsius
int _temp;

//temperature of the engine's coolant in degree celsius
int _coolantTemp;

//tension de la batterie en V
float _voltage;

//LCD Screen 4*20
LiquidCrystal_I2C lcd(0x27, 20,4);
boolean isLight = true;

//Permet de connaitre le mode. 3 modes existent, le mode d'affichage 1 et 2 et le mode de config permettant de regler les alarmes et la qte de gasoil total et dans le reservoir principal.
byte _mode = 1;
//Permet de connaitre le parametre edite dans le mode config
byte _editedParam = QTETR;

//Pour les temporisations
const unsigned long _periodWriting = 1000;
unsigned long _lastConsoWriting = 0;

const unsigned long _periodCheckTemp = 10000;
unsigned long _lastCheckTemp = 0;

const unsigned long _periodRefresh = 1000;
unsigned long _lastRefreshDisplay = 0;

//Think about commenting the line #define BLUETOOTH_USE_SOFTWARE_SERIAL in the file libraries/Virtuino/VirtuinoBluetooth.h otherwise you will get an error at the next line.
VirtuinoBluetooth _virtuinoBT(Serial);

int readTemperature();
int readCoolantTemperature();
float readVoltage();
void updateGasConsumtion();
void sendDataBT();
void checkEngineTempVolt();
void refreshDisplay();
void onModeButtonPressed();
void onPlusButtonPressed();
void onMoinsButtonPressed();
unsigned long unBound(int pinButton);

/*!
    @const      charBitmap
    @abstract   Define Character bitmap for the bargraph.
    @discussion Defines a character bitmap to represent a bargraph on a text
    display. The bitmap goes from a blank character to full black.
*/
uint8_t charBitmap[8] = {0x6, 0x9, 0x9, 0x6, 0x0, 0, 0, 0};

void initHardware(){
  //Initialisation of the serial port
  Serial.begin(4800);

  //Initialisation of the digital pin
  pinMode(inpinrpm, INPUT);

  pinMode(outPinThermistor, OUTPUT);
  digitalWrite(outPinThermistor, LOW);

  pinMode(outPinBuzzer, OUTPUT);

  pinMode(inPinButton, INPUT);
  pinMode(inPinButtonPlus, INPUT);
  pinMode(inPinButtonMoins, INPUT);
  pinMode(inPinButtonLum, INPUT);

  //Initialisation of the analogique entries
  pinMode(pin_capteur_temp,INPUT);
  pinMode(outPinAlimTemp,OUTPUT);
  digitalWrite(outPinAlimTemp, LOW);

  pinMode(pin_capteur_volt, INPUT);

  //Initialisation of the SD card memory
  pinMode(chipSelect, OUTPUT);

  //Initialisation of the screen
  lcd.begin(20,4);
  lcd.backlight();
  lcd.clear();
}

void initParams(){
	_addrEEPROM = EEPROM.getAddress(SIZE_EEPROM);
	if(_addrEEPROM == -1){
		ERROR("Error EEPROM address")
		while(1){};
	}

	char eeprom[SIZE_EEPROM];
	// We read the data in the EEPROM
	for(unsigned int i = 0; i < SIZE_EEPROM; i++){
		eeprom[i] = EEPROM.read(_addrEEPROM + i);
	}

	// We wrap the block of memory to protect the out of memory bound ...
	MemChunck mem(eeprom, SIZE_EEPROM, false);

	uint32_t magic;
	mem.read(&magic, sizeof(uint32_t));

	//If the eeprom has been initialized
	if(magic == MAGIC){
		mem.read(&_ageEngine, sizeof(_ageEngine));
		mem.read(&_qteGas, sizeof(_qteGas));
		mem.read(&_total, sizeof(_total));
		mem.read(&_qteGasTotal, sizeof(_qteGasTotal));
		mem.read(&_qteGasMainTank, sizeof(_qteGasMainTank));

		long buffer;
		mem.read(&buffer, sizeof(buffer));TEMP_ALERT = (byte)buffer;
		mem.read(&buffer, sizeof(buffer));COOLANT_TEMP_ALERT = (byte)buffer;
	}
	else{
		_ageEngine = 7500960; //Age of the engine the 10/05/2017
		_qteGas = 0;
		_total = 0;
		_qteGasTotal = 0;
		_qteGasMainTank = 0;

		TEMP_ALERT = 40;
		COOLANT_TEMP_ALERT = 80;

		_hasToBeSaved = true;
	}
}

void saveParams(){
	char eeprom[SIZE_EEPROM];
	// We wrap the block of memory to protect the out of memory bound ...
	MemChunck mem(eeprom, SIZE_EEPROM, false);

	uint32_t buffer = MAGIC;
	mem.write(&buffer, sizeof(uint32_t));
	mem.write(&_ageEngine, sizeof(_ageEngine));
	mem.write(&_qteGas, sizeof(_qteGas));
	mem.write(&_total, sizeof(_total));
	mem.write(&_qteGasTotal, sizeof(_qteGasTotal));
	mem.write(&_qteGasMainTank, sizeof(_qteGasMainTank));

	buffer = TEMP_ALERT;mem.write(&buffer, sizeof(buffer));
	buffer = COOLANT_TEMP_ALERT;mem.write(&buffer, sizeof(buffer));

	// We write the data in the EEPROM
	for(unsigned int i = 0; i < SIZE_EEPROM; i++){
		EEPROM.write(_addrEEPROM + i, eeprom[i]);
	}

	_hasToBeSaved = false;
}

// l'initialisation
void setup()
{
  initHardware();

  //Creation du caractere  pour l'affichage LCD.
  lcd.createChar (0, charBitmap );

  //Init of the synchro var
  _lastConsoWriting = millis();
  _lastCheckTemp = _lastConsoWriting;
  _lastRefreshDisplay = _lastConsoWriting;

  initParams();

  //Init of temperature
  _temp = readTemperature();
  _coolantTemp = readCoolantTemperature();
  _voltage = readVoltage();

  //Test of the buzzer
  tone(outPinBuzzer, 1400, 0);
  delay(2000);
  noTone(outPinBuzzer);
}

// le programme principal
void loop()
{
  if(millis() - _lastConsoWriting >= _periodWriting){
    _lastConsoWriting += _periodWriting;
    updateGasConsumtion();

    sendDataBT();
  }

  if(millis() - _lastCheckTemp >= _periodCheckTemp){
    _lastCheckTemp += _periodCheckTemp;
    checkEngineTempVolt();
  }

  if(millis() - _lastRefreshDisplay >= _periodRefresh){
    refreshDisplay();
  }

  if(isBuzzerOn){
    tone(outPinBuzzer, 1400, 0);
  }
  else{
    noTone(outPinBuzzer);
  }

  int buttonState = digitalRead(inPinButton);
  if(buttonState == HIGH){
    onModeButtonPressed();
  }

  buttonState = digitalRead(inPinButtonPlus);
  if(buttonState == HIGH){
    onPlusButtonPressed();
  }

  buttonState = digitalRead(inPinButtonMoins);
  if(buttonState == HIGH){
    onMoinsButtonPressed();
  }

  buttonState = digitalRead(inPinButtonLum);
  if(buttonState == HIGH){

    if(unBound(inPinButtonLum) > 30){
      if(isLight == false){
        lcd.backlight();
        isLight = true;
      }
      else{
        lcd.noBacklight();
        isLight = false;
      }
    }
  }

  _virtuinoBT.run();
}

void sendDataBT(){
  //We also send the data on the Virtuino interface
  _virtuinoBT.vMemoryWrite(AGE_ENGINE_INDEX, (float)_ageEngine/(float)3600);
  _virtuinoBT.vMemoryWrite(RPM_INDEX, _rpm);
  _virtuinoBT.vMemoryWrite(CONSO_INDEX, _conso);
  _virtuinoBT.vMemoryWrite(QTE_GAZ_INDEX, (float)_qteGas/(float)1000000);
  _virtuinoBT.vMemoryWrite(TEMP_INDEX, (float)_temp);
  _virtuinoBT.vMemoryWrite(VOLTAGE_INDEX, _voltage);
  _virtuinoBT.vMemoryWrite(TEMP_COOLANT_INDEX, (float)_coolantTemp);
}

int getCheckSum(String phrase) {
  int checksum = 0;
  int longueur = 0;
  longueur = phrase.length();
  for(int i = 1; i < longueur; i++){
    checksum = checksum ^ int(phrase[i]);
  }

  return checksum;
}

void updateGasConsumtion(){
	//RPM en tr/min
	int rpm = readRPM();
	if(rpm != 0){
		//Conso en litre par heure
		_conso = getConsoInst(rpm);
		//Calcul de la quantitee de gasoil en microlitre consomme en 1 seconde.
		long qte = (long)(_conso * (float)277.7777777778 + (float)0.5);

		_ageEngine++;
		_refresh |= AGE;

		_qteGas -= qte;
		_qteGasMainTank -=qte;

		_refresh |= QTETR;

		_total++;
		_qteGasTotal += qte;
		_refresh |= QTETOTAL;

		_hasToBeSaved = true;
	}
	else{
		if(_conso != 0 || _rpm != 0){
			_refresh |= QTETR;
			_conso = 0;
		}
		else{
			//If we reach here it means that the engine stopped more than one second ago ...
			//It's the right time to save the params in the eeprom

			//If the params has to be saved and if we are not in configuration mode
			if(_hasToBeSaved && _mode != MODECONFIG){
				saveParams();
			}
		}
	}
	_rpm = rpm;
}


void refreshDisplay(){
  if(_refresh != 0){
    float nbSecondeHour = (float)3600;
    float nbMicroLiter = (float)1000000;

    String rpmStr = lengthStr(String(_rpm), 4);

    float nbHeure = (float)_ageEngine / nbSecondeHour;
    String ageStr = lengthStr(FloatToString(nbHeure, 2), 6);

    //Affichage de la quantite de gaz arrondi au litre
    int qteGasL = (int)((float)_qteGas/nbMicroLiter + (float)0.5);
    String qteGasStr = lengthStr(String(qteGasL), 3);

    //Affichage de la quantite de gaz dans le reservoir principal arrondi au litre
    int qteGasMainL = (int)((float)_qteGasMainTank/nbMicroLiter + (float)0.5);
    String qteGasMainStr = lengthStr(String(qteGasMainL), 2);

    nbHeure = (float)_total/nbSecondeHour;
    String totalStr = lengthStr(FloatToString(nbHeure, 0), 4);

    //Affichage de la quantite de gaz arrondi au litre
    float consoTot = _qteGasTotal/(nbMicroLiter * nbHeure);
    String consoTotStr = lengthStr(FloatToString(consoTot, 2), 4);
    String consoStr = lengthStr(FloatToString(_conso, 2), 4);
    String voltageStr = lengthStr(FloatToString(_voltage, 1), 4);

    if((_refresh & MODE) == MODE){
      _refresh = B00000000;
      lcd.clear();

      if(_mode == MODECONFIG){
        lcd.setCursor(0, 0);
        if(_editedParam == QTETR){
          lcd.print(F("->"));
        }

        lcd.print(F("Vol. tot. : "));
        lcd.print(qteGasStr);
        lcd.print(F("L  "));

        lcd.setCursor(0, 1);
        if(_editedParam == QTEMAIN){
          lcd.print(F("->"));
        }

        lcd.print(F("Vol. res. : "));
        lcd.print(qteGasMainStr);
        lcd.print(F("L   "));

        lcd.setCursor(0, 2);
        if(_editedParam == TEMPALARM){
          lcd.print(F("->"));
        }
        lcd.print('T');
        lcd.print((char)0);
        lcd.print(F("C alerte : "));
        lcd.print(lengthStr(String(TEMP_ALERT) ,2));
        lcd.print((char)0);
        lcd.print(F("C "));

        lcd.setCursor(0, 3);
        if(_editedParam == TEMPCOO){
          lcd.print(F("->"));
        }
        lcd.print('T');
        lcd.print((char)0);
        lcd.print(F("C max eng: "));
        lcd.print(lengthStr(String(COOLANT_TEMP_ALERT), 3));
        lcd.print((char)0);
        lcd.print('C');

      }
      else if (_mode == MODE1){
        lcd.setCursor(0, 0);
        lcd.print(rpmStr);
        lcd.print(F(" RPM "));
        lcd.print(consoStr);
        lcd.print(F(" L/h   "));

        lcd.setCursor(0, 1);
        lcd.print(F("Reste "));
        lcd.print(qteGasStr);
        lcd.print(F("L ("));
        lcd.print(qteGasMainStr);
        lcd.print(F("L)    "));

        lcd.setCursor(0, 2);
        lcd.print(lengthStr(String(_temp), 3));
        lcd.print((char)0);
        lcd.print(F("C "));

        lcd.print(voltageStr);
        lcd.print(F(" V "));

		lcd.print(lengthStr(String(_coolantTemp), 3));
		lcd.print(' ');
		lcd.print((char)0);
		lcd.print(F("C "));

        lcd.setCursor(0, 3);
        lcd.print(F("age : "));
        lcd.print(ageStr);
        lcd.print(F(" H      "));
      }
    }
    else if(_mode == MODE1){
      if((_refresh & TEMP) == TEMP){
        _refresh &= (~TEMP);
        displayScreen(2, 0, lengthStr(String(_temp), 3));
      }

      if((_refresh & TEMPC) == TEMPC){
		  _refresh &= (~TEMPC);
		  displayScreen(2, 13, lengthStr(String(_coolantTemp), 3));
	  }

      if((_refresh & QTETR) == QTETR){
        _refresh &= (~QTETR);
        displayScreen(0, 0, rpmStr);
        displayScreen(0, 9, consoStr);
        displayScreen(1, 6, qteGasStr);
        displayScreen(1, 12, qteGasMainStr);
      }

      if((_refresh & VOLT) == VOLT){
        _refresh &= (~VOLT);
        displayScreen(2, 6, voltageStr);
      }

      if((_refresh & AGE) == AGE){
        _refresh &= (~AGE);
        displayScreen(3, 6, ageStr);
      }

      //Tout le reste on ne rafraichit pas car invisible :
      _refresh = B00000000;
    }
    //MODE config
    else{
      if((_refresh & QTETR) == QTETR){
        _refresh &= (~QTETR);

        int offset = 0;
        if(_editedParam == QTETR){
          offset = 2;
        }

        displayScreen(0, offset + 12, qteGasStr);
      }

      if((_refresh & QTEMAIN) == QTEMAIN){
        _refresh &= (~QTEMAIN);

        displayScreen(1, 14, qteGasMainStr);
      }

      if((_refresh & TEMPALARM) == TEMPALARM){
        _refresh &= (~TEMPALARM);

        displayScreen(2, 15, lengthStr(String(TEMP_ALERT), 2));
      }

      if((_refresh & TEMPCOO) == TEMPCOO){
        _refresh &= (~TEMPCOO);

        displayScreen(3, 15, lengthStr(String(COOLANT_TEMP_ALERT), 3));
      }

      //Pour le reste, on ne rafraichit pas ...
      _refresh = B00000000;
    }
  }
}

void checkEngineTempVolt(){
  //We check engine temperature every minute and NOT every second to avoid to turn on and off the swith of the fan and the buzzer ...
  int newTemp = readTemperature();
  int newCoolantTemp = readCoolantTemperature();
  float newVoltage = readVoltage();

  if(newTemp > TEMP_ALERT && _temp <= TEMP_ALERT){
    isBuzzerOn = true;
  }

  if(newCoolantTemp > COOLANT_TEMP_ALERT && _coolantTemp <= COOLANT_TEMP_ALERT){
	  isBuzzerOn = true;
	}

  if(_temp != newTemp){
    _temp = newTemp;
    _refresh |= TEMP;
  }

  if(_voltage != newVoltage){
    _voltage = newVoltage;
    _refresh |= VOLT;
  }

  if(_coolantTemp != newCoolantTemp){
	  _coolantTemp = newCoolantTemp;
	 _refresh |= TEMPC;
  }
}

//Fonction permettant de calculer la temp�rature
int readTemperature(){
  //On alimente le capteur de temp�rature
  digitalWrite(outPinAlimTemp, HIGH);
  //On attend 1 ms le temps que la valeur du capteur se stabilise (D'apres les mesures à l'oscilloscope, le capteur se stabilise après 50 microSecondes)...
  delay(1);

  // On r�cup�re la valeur de la tension en sortie du capteur 100 fois pour �liminier le bruit
  float valeur_capteur = 0;
  for(int i = 0; i < 100; i++){
    valeur_capteur += analogRead(pin_capteur_temp);
  }
  valeur_capteur /= (float)100;

  float temperature = valeur_capteur * (float)0.48828125;
  digitalWrite(outPinAlimTemp, LOW);

  return (int) (temperature + (float)0.5);
}

int readCoolantTemperature(){
	//On alimente le capteur de température
	digitalWrite(outPinThermistor, HIGH);
	delay(1);
	// On recupere la valeur de la tension en sortie du capteur 100 fois pour eliminier le bruit
	int a = (float)analogRead(pin_capteur_thermistor);
	digitalWrite(outPinThermistor, LOW);

	float b = (float)a / (float)(1024 - a);

	float temperature = -31.1 * log(b) + 47.2;

	return (int) (temperature + (float)0.5);
}

float readVoltage(){
  float valeur_capteur = analogRead(pin_capteur_volt);
  return (float)0.0146484375 * valeur_capteur;
}

//Cette fonction permet de retourner le r�gime moteur.
//0 si le moteur est �teint.
//Attention, cette fonction dure 100ms au moins ...
int readRPM(){
  float f = 0;
  float durationPulseLOW = 0;
  float durationPulseHIGH = 0;
  unsigned long beginning = millis();
  unsigned long nowMillis = millis();

  while(nowMillis - beginning < (unsigned long)100){
    durationPulseLOW += pulseIn(inpinrpm, LOW, (unsigned long)100000);
    durationPulseHIGH += pulseIn(inpinrpm, HIGH, (unsigned long)100000);
    f = f + (float)1;
    nowMillis = millis();
  }

  //Calcul de la dur�e moyenne d'une pulsation LOW et HIGH en seconde
  //Calcul de la dur�e moyenne d'une pulsation LOW et HIGH en seconde
  durationPulseLOW /= (f * (float)1000000);
  durationPulseHIGH /= (f * (float)1000000);


  if(durationPulseLOW == 0 || durationPulseHIGH == 0){
    return 0;
  }
  else{
    //Calcul de la frequence en Hz en faisant l'inverse de la p�riode �gale � la somme des dur�es des pulsations HIGH et LOW
    f = (float)1/ (durationPulseLOW + durationPulseHIGH);
    //Le 6.51 a �t� calcul� de fa�on empirique.
    float rpm = (float)f * ALPHA_RPM;

    //En utilisant la formule trouver sur sonelec, nous aurions :
    //float rpm = f * (float)40
    //On arrondi � l'entier le plus proche
    return (int)(rpm + (float)0.5);
  }
}

//Fonction permettant de calculer la consommation instantann�e.
//Pour d�terminer les param�tres a, b et c, il faut r�cup�rer la courbe de consommation L/h en fonction du r�gime moteur du constructeur, reporter quelques valeurs de cette
//courbe dans un tableur et d�terminer � l'aide de ce tableur l'�quation de la courbe saisie de la forme a * 10^b * rpm^c ...
float getConsoInst(int rpm){
  return d * a * pow(10, b)* pow((float)rpm, c) + e;
}

void onModeButtonPressed(){
  long delayButton = unBound(inPinButton);
  if(delayButton > 30){
    if(isBuzzerOn){
      isBuzzerOn = false;
    }
    else if(delayButton > 2000 && _mode != MODECONFIG){
      _mode = MODECONFIG;
      _editedParam = QTETR;
      _refresh |= MODE;
    }
    else if(delayButton > 2000 && _mode == MODECONFIG){
      _mode = MODE1;
      _refresh |= MODE;
    }
    else if(delayButton <= 2000 && _mode == MODECONFIG){
      if(_editedParam == QTETR)_editedParam = QTEMAIN;
      else if(_editedParam == QTEMAIN)_editedParam = TEMPALARM;
      else if(_editedParam == TEMPALARM)_editedParam = TEMPCOO;
      else if(_editedParam == TEMPCOO)_editedParam = QTETR;
      _refresh |= MODE;
    }
  }
}

void onPlusButtonPressed(){
	int moinsButtonState = digitalRead(inPinButtonMoins);

	if(unBound(inPinButtonPlus) > 30){
		if(isBuzzerOn){
			isBuzzerOn = false;
		}
		else if(_mode == MODECONFIG){
			if(_editedParam == TEMPALARM){
				if(TEMP_ALERT != -1){
					TEMP_ALERT++;
				}

				if(TEMP_ALERT > 90){
					TEMP_ALERT = -1;
				}
				_hasToBeSaved = true;

				_refresh |= TEMPALARM;
			}
			else if(_editedParam == TEMPCOO){
				if(COOLANT_TEMP_ALERT != -1){
					COOLANT_TEMP_ALERT++;
				}

				if(COOLANT_TEMP_ALERT > 105){
					COOLANT_TEMP_ALERT = -1;
				}
				_hasToBeSaved = true;
				_refresh |= TEMPCOO;
			}
			else if(_editedParam == QTETR){
				_qteGas += (long)1000000;

				if(_qteGas > QTELIMIT){
					_qteGas = QTELIMIT;
				}
				_hasToBeSaved = true;
				_refresh |= QTETR;
			}
			else if(_editedParam == QTEMAIN){
				_qteGasMainTank += (long)1000000;

				if(_qteGasMainTank > QTEMAINLIMIT){
					_qteGasMainTank = QTEMAINLIMIT;
				}
				_hasToBeSaved = true;
				_refresh |= QTEMAIN;
			}
		}
	}
}

void onMoinsButtonPressed(){
  if(unBound(inPinButtonMoins) > 30){
    //The aknoledgement of the temperature alarm has the priority ...
    if(isBuzzerOn){
      isBuzzerOn = false;
    }
    else if(_mode == MODECONFIG){
      if(_editedParam == TEMPALARM){
        if(TEMP_ALERT == -1){
          TEMP_ALERT = 90;
        }
        else{
          TEMP_ALERT--;
        }

        if(TEMP_ALERT < 20){
          TEMP_ALERT = 20;
        }
        _hasToBeSaved = true;
        _refresh |= TEMPALARM;
      }
      else if(_editedParam == TEMPCOO){
        if(COOLANT_TEMP_ALERT == -1){
        	COOLANT_TEMP_ALERT = 105;
        }
        else{
        	COOLANT_TEMP_ALERT--;
        }

        if(COOLANT_TEMP_ALERT < 60){
        	COOLANT_TEMP_ALERT = 60;
        }
        _hasToBeSaved = true;
        _refresh |= TEMPCOO;
      }
      else if(_editedParam == QTETR){
        _qteGas -= (long)1000000;

        if(_qteGas < 0){
          _qteGas = 0;
        }
        _hasToBeSaved = true;
        _refresh |= QTETR;
      }
      else if(_editedParam == QTEMAIN){
        _qteGasMainTank -= (long)1000000;

        if(_qteGasMainTank < 0){
          _qteGasMainTank = 0;
        }
        _hasToBeSaved = true;
        _refresh |= QTEMAIN;
      }
    }
  }
}

unsigned long unBound(int pinButton){
  unsigned long delayButton = 0;
  int stateButton = digitalRead(pinButton);
  while(stateButton == HIGH){
    delay(10);
    stateButton = digitalRead(pinButton);
    delayButton += (unsigned long)10;
  }

  return delayButton;
}

//Affiche le texte � la position d�sir�e;
void displayScreen(int numLine, int numCol, String text){
  lcd.setCursor(numCol, numLine);
  lcd.print(text);
}

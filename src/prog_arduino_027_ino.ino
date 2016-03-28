
#include <SPI.h>
#include <SD.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "FileUtilities.h"

using namespace FileUtilities;


//Quantite maximum de gasoil disponible
#define QTELIMIT 220000000
//Quantite maximum de gasoil dans le reservoir principal
#define QTEMAINLIMIT 72000000

#define MODECONFIG 0
#define MODE1 1
#define MODE2 2

#define BUTTON1 7
#define BUTTON2 9
#define BUTTON3 3
#define BUTTON4 6

//Constante utilisée pour le rafraichissement de l'écran
const byte TEMP     = B00000001;
const byte QTETR    = B00000010;

//Ces 3 variables peuvent être rafraichit que dans le mode config ...
const byte TEMPALARM= B00000100;
const byte TEMPFAN  = B00001000;
const byte QTEMAIN  = B00010000;

const byte VOLT     = B00001000;
const byte AGE      = B00010000;
const byte QTETRIP  = B00100000;
const byte QTETOTAL = B01000000;
const byte MODE     = B10000000;

byte _refresh       = B10000000;

//Constante empirique permettant de calculer les RPM en fonction de la fréquence du rupteur :
//RPM = ALPHA_RPM * freq_rupteur
//Pour mon moteur Nanni N3.30, cette constante vaut 6.51
const float ALPHA_RPM = (float)6.51;

//Constantes utilisées pour l'estimation de la conso (trouvé à partir des données constructeur)
const float a = (float)1.1330288410882;
const float b = (float) -10;
const float c = (float)3.0243858131;
//Constante multiplicateur de la formule d'estimation de la conso déterminé empiriquement
const float d = (float)1.05;
const float e = (float)0.5;

//Temperature the fan turns on at
byte TEMP_FAN;
const int POS_TEMP_FAN = 28;
//Temperature the buzzer turns on at
byte TEMP_ALERT;
const int POS_TEMP_ALERT = 24;

//Pin analogique pour le capteur de température.
const char pin_capteur_temp = A0;
//Pin digital pour alimenter le capteur de température temporairement ...
const int outPinAlimTemp = 8;
//Pin analogique pour mesurer la tension de la batterie moteur
const char pin_capteur_volt = A1;

//A4 and A5 are reserver for I2C ...

//On garde les pin digital 0 et 1 pour une eventuelle liaison série ...
//Pin digitale num 2, donc il y  a la possibilité de gérer les impulsions par interruption si jamais on veut changer.
const int inpinrpm = 2;
//Pin digitale du relay de la ventilation.
const int outPinFan = 4;
//Pin digital du bouton poussoir numero 3 pour gérer le bouton avec des interruptions.
const int inPinButton = BUTTON2; 
const int inPinButtonPlus = BUTTON4; 
const int inPinButtonMoins = BUTTON3; 
const int inPinButtonLum = BUTTON1;
//digital pin for the buzzer
const int outPinBuzzer = 5;

//To know if the buzzer is on or off
boolean isBuzzerOn = false;

//Pour la carte mémoire
const int chipSelect = 10;

//Consommation temps réel en L/h
float _conso = 0;
//Régime moteur temps réel en tour par minute
int _rpm = 0;

//age du moteur en seconde
long _ageEngine;
const int POS_AGE = 0;

//Quantité de gasoil estimée en micro litre
long _qteGas;
const int POS_QTE = 4;

//Durée en seconde du voyage (Réinitialisable)
long _trip;
const int POS_TRIP = 8;
//et la quantité de gasoil cumulé en micro litre pour le trip ...
long _qteGasTrip;
const int POS_TRIP_QTE = 12;

//Durée en seconde du total (Non réinitialisable)
long _total;
const int POS_TOT = 16;

long _qteGasTotal;
const int POS_TOT_QTE = 20;

//Quantité de gasoil dans le réservoir principal
long _qteGasMainTank;
const int POS_MAIN_QTE = 32;

//temperature of the engine compartiment en degre celsius
int _temp;

//tension de la batterie en V
float _voltage;

//LCD Screen 4*20
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
boolean isLight = true;

//Permet de connaitre le mode. 3 modes existent, le mode d'affichage 1 et 2 et le mode de config permettant de regler les alarmes et la qte de gasoil total et dans le réservoir principal.
byte _mode = 1;
//Permet de connaitre le paramètre édité dans le mode config
byte _editedParam = QTETR;


//Pour les temporisations
const unsigned long _periodWriting = 1000;
unsigned long _lastConsoWriting = 0;

const unsigned long _periodCheckTemp = 10000;
unsigned long _lastCheckTemp = 0;

const unsigned long _periodRefresh = 1000;
unsigned long _lastRefreshDisplay = 0;

/*!
    @const      charBitmap 
    @abstract   Define Character bitmap for the bargraph.
    @discussion Defines a character bitmap to represent a bargraph on a text
    display. The bitmap goes from a blank character to full black.
*/
uint8_t charBitmap[8] = {0x6, 0x9, 0x9, 0x6, 0x0, 0, 0, 0};

//
const char *filename = "C";


void initHardware(){
  //Initialisation of the serial port
  Serial.begin(4800);
  
  //Initialisation of the digital pin
  pinMode(inpinrpm, INPUT);
  
  pinMode(outPinFan, OUTPUT);
  digitalWrite(outPinFan, HIGH);
  
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
  
  if (!SD.begin(chipSelect)) {
    return;
  }
  
  //Initialisation of the screen
  lcd.begin(20,4);
  lcd.backlight();
  lcd.clear();
}

void initParams(){
  _ageEngine = readlong(filename, POS_AGE);
  _qteGas = readlong(filename, POS_QTE);
  _trip = readlong(filename, POS_TRIP);
  _qteGasTrip = readlong(filename, POS_TRIP_QTE);
  _total = readlong(filename, POS_TOT);
  _qteGasTotal = readlong(filename, POS_TOT_QTE);
  _qteGasMainTank = readlong(filename, POS_MAIN_QTE);
  TEMP_ALERT = (byte)readlong(filename, POS_TEMP_ALERT);
  TEMP_FAN = (byte)readlong(filename, POS_TEMP_FAN);
}

// l’initialisation
void setup()
{
  initHardware();
  
  
  //Création du caractere ° pour l'affichage LCD.
  lcd.createChar (0, charBitmap );
  
  //Init of the synchro var
  _lastConsoWriting = millis();
  _lastCheckTemp = _lastConsoWriting;
  _lastRefreshDisplay = _lastConsoWriting;
  
  initParams();
  
  
  //Init of temperature
  _temp = readTemperature();
  _voltage = readVoltage();
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
}

void sendDataBT(){
  //$ENGIN,0|1,AGE[s],RPM[tr/min],CONSOTR[L/H],RESTE[L],TEMP[°C],TENSION[V],*CHKSUM
  String sentence = "$ENGIN,";
  if(_rpm == 0){
    sentence += "0,";
  }
  else{
    sentence += "1,";
  }
  sentence += String(_ageEngine);
  sentence += ",";
  sentence += String(_rpm);
  sentence += ",";
  sentence += FloatToString(_conso, 2);
  sentence += ",";
  sentence += String(_qteGas);
  sentence += ",";
  sentence += String(_temp);
  sentence += ",";
  sentence += FloatToString(_voltage, 2);
  sentence += ",";
  
  int checkSum = getCheckSum(sentence);
  
  sentence += "*";
  Serial.print(sentence);
  Serial.println(checkSum,HEX); 
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
    //Calcul de la quantité de gasoil en microlitre consommé en 1 seconde.
    long qte = (long)(_conso * (float)277.7777777778 + (float)0.5);
    
    _ageEngine++;
    _refresh |= AGE;
    
    _qteGas -= qte;
    _qteGasMainTank -=qte;
    
    _refresh |= QTETR;
    
    _trip++;
    _qteGasTrip += qte;
    _refresh |= QTETRIP;
    
    _total++;
    _qteGasTotal += qte;
    _refresh |= QTETOTAL;
    
    writelong(filename, POS_AGE, _ageEngine);
    writelong(filename, POS_QTE, _qteGas);
    
    writelong(filename, POS_TRIP, _trip);
    writelong(filename, POS_TRIP_QTE, _qteGasTrip);
    
    writelong(filename, POS_TOT, _total);
    writelong(filename, POS_TOT_QTE, _qteGasTotal);
    
    writelong(filename, POS_MAIN_QTE, _qteGasMainTank);
  }
  else{
    if(_conso != 0 || _rpm != 0){
      _refresh |= QTETR;
      _conso = 0;
    }
  }
  _rpm = rpm;
}


void refreshDisplay(){
  if(_refresh != 0){
    float nbSecondeHour = (float)3600;
    float nbMicroLiter = (float)1000000;
    
    String rpmStr = String(_rpm);
    rpmStr = lengthStr(rpmStr, 4);
    
    float nbHeure = (float)_ageEngine / nbSecondeHour;
    String ageStr = FloatToString(nbHeure, 2);
    ageStr = lengthStr(ageStr, 6);
    
    //Affichage de la quantite de gaz arrondi au litre
    int qteGasL = (int)((float)_qteGas/nbMicroLiter + (float)0.5);
    String qteGasStr = String(qteGasL);
    qteGasStr = lengthStr(qteGasStr, 3);
    
    //Affichage de la quantite de gaz dans le reservoir principal arrondi au litre
    int qteGasMainL = (int)((float)_qteGasMainTank/nbMicroLiter + (float)0.5);
    String qteGasMainStr = String(qteGasMainL);
    qteGasMainStr = lengthStr(qteGasMainStr, 2);
    
    nbHeure = (float)_trip/nbSecondeHour;
    String tripStr = FloatToString(nbHeure, 0);
    tripStr = lengthStr(tripStr, 4);
    
    //Affichage de la conso moyenne du trip
    float consoTrip = _qteGasTrip/(nbMicroLiter * nbHeure);
    String consoTripStr = FloatToString(consoTrip, 2);
    consoTripStr = lengthStr(consoTripStr, 4);
    
    nbHeure = (float)_total/nbSecondeHour;
    String totalStr = FloatToString(nbHeure, 0);
    totalStr = lengthStr(totalStr, 4);
    
    //Affichage de la quantite de gaz arrondi au litre
    float consoTot = _qteGasTotal/(nbMicroLiter * nbHeure);
    String consoTotStr = FloatToString(consoTot, 2);
    consoTotStr = lengthStr(consoTotStr, 4);
    
    String consoStr = FloatToString(_conso, 2);
    consoStr = lengthStr(consoStr, 4);
    
    String voltageStr = FloatToString(_voltage, 2);
    voltageStr = lengthStr(voltageStr, 4);
    
    //_modified = false;
      
    if((_refresh & MODE) == MODE){
      _refresh = B00000000;
      lcd.clear();
      
      if(_mode == MODECONFIG){
        lcd.setCursor(0, 0);
        if(_editedParam == QTETR){
          lcd.print("->");
        }
          
        lcd.print("Vol. tot. : ");
        lcd.print(qteGasStr);
        lcd.print("L  ");
        
        lcd.setCursor(0, 1);
        if(_editedParam == QTEMAIN){
          lcd.print("->");
        }
          
        lcd.print("Vol. res. : ");
        lcd.print(qteGasMainStr);
        lcd.print("L  ");
        
        lcd.setCursor(0, 2);
        if(_editedParam == TEMPALARM){
          lcd.print("->");
        }
        lcd.print("T");
        lcd.print((char)0);
        lcd.print("C alerte : ");
        lcd.print(String(TEMP_ALERT));
        lcd.print((char)0);
        lcd.print("C  ");
        
        lcd.setCursor(0, 3);
        if(_editedParam == TEMPFAN){
          lcd.print("->");
        }
        lcd.print("T");
        lcd.print((char)0);
        lcd.print("C vent : ");
        lcd.print(String(TEMP_FAN));
        lcd.print((char)0);
        lcd.print("C  ");
        
      }
      else if (_mode == MODE1){
        lcd.setCursor(0, 0);
        lcd.print(rpmStr);
        lcd.print(" RPM ");
        lcd.print(consoStr);
        lcd.print(" L/h");
        
        lcd.setCursor(0, 1);
        lcd.print("Reste ");
        lcd.print(qteGasStr);
        lcd.print("L (");
        lcd.print(qteGasMainStr);
        lcd.print("L)");
      
        lcd.setCursor(0, 2);
        lcd.print(String(_temp));
        lcd.print(" ");
        lcd.print((char)0);
        lcd.print("C  ");
        
        lcd.print(voltageStr);
        lcd.print(" V");
        
        lcd.setCursor(0, 3);
        lcd.print("age : ");
        lcd.print(ageStr);
        lcd.print(" H");
      }
      else if (_mode == MODE2){
        lcd.setCursor(0, 0);
        lcd.print(consoStr);
        lcd.print(" L/h reste ");
        lcd.print(qteGasStr);
        lcd.print("L");
      
        lcd.setCursor(0, 1);
        lcd.print("Trp : ");
        lcd.print(tripStr);
        lcd.print("H(");
        lcd.print(consoTripStr);
        lcd.print("L/h)");
        
        lcd.setCursor(0, 2);
        lcd.print("Tot : ");
        lcd.print(totalStr);
        lcd.print("H(");
        lcd.print(consoTotStr);
        lcd.print("L/h)");
      }
    }
    else if(_mode == MODE1){
      if((_refresh & TEMP) == TEMP){
        _refresh &= (~TEMP);
        displayScreen(2, 0, String(_temp));
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
    else if(_mode == MODE2){
      if((_refresh & QTETR) == QTETR){
        _refresh &= (~QTETR);
        displayScreen(0, 15, qteGasStr);
        displayScreen(0, 0, consoStr);
      }
      if((_refresh & QTETRIP) == QTETRIP){
        _refresh &= (~QTETRIP);
        displayScreen(1, 6, tripStr);
        displayScreen(1, 12, consoTripStr);
      }
      if((_refresh & QTETOTAL) == QTETOTAL){
        _refresh &= (~QTETOTAL);
        displayScreen(2, 6, totalStr);
        displayScreen(2, 12, consoTotStr);
      }
      //Pour le reste, on ne rafraichit pas ...
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
        
        displayScreen(2, 15, String(TEMP_ALERT));
      }
      
      if((_refresh & TEMPFAN) == TEMPFAN){
        _refresh &= (~TEMPFAN);
        
        displayScreen(3, 13, String(TEMP_FAN));
      }
      
      //Pour le reste, on ne rafraichit pas ...
      _refresh = B00000000;
    }
  }
}

void checkEngineTempVolt(){
  //We check engine temperature every minute and NOT every second to avoid to turn on and off the swith of the fan and the buzzer ...
  int newTemp = readTemperature();
  float newVoltage = readVoltage();
  
  if(newTemp > TEMP_FAN){
    digitalWrite(outPinFan, LOW);
  }
  
  if(newTemp > TEMP_ALERT && _temp <= TEMP_ALERT){
    isBuzzerOn = true;
  }
  
  if(newTemp <= TEMP_FAN){
    digitalWrite(outPinFan, HIGH);
  }
  
  if(newTemp <= TEMP_ALERT && _temp > TEMP_ALERT){
    isBuzzerOn = false;
  }
  
  if(_temp != newTemp){
    _temp = newTemp;
    _refresh |= TEMP;
  }
  
  if(_voltage != newVoltage){
    _voltage = newVoltage;
    _refresh |= VOLT;
  }
}
  
//Fonction permettant de calculer la température
int readTemperature(){
  //On alimente le capteur de température
  digitalWrite(outPinAlimTemp, HIGH);
  //On attend 1 ms le temps que la valeur du capteur se stabilise (D'apres les mesures à l'oscilloscope, le capteur se stabilise après 50 microSecondes)...
  delay(1);
  
  // On récupère la valeur de la tension en sortie du capteur 100 fois pour éliminier le bruit
  float valeur_capteur = 0;
  for(int i = 0; i < 100; i++){
    valeur_capteur += analogRead(pin_capteur_temp);
  }
  valeur_capteur /= (float)100;
  
  float temperature = valeur_capteur * (float)0.48828125;
  digitalWrite(outPinAlimTemp, LOW);
  
  return (int) (temperature + (float)0.5);
}

float readVoltage(){
  float valeur_capteur = analogRead(pin_capteur_volt);
  return (float)0.0146484375 * valeur_capteur;
}

//Cette fonction permet de retourner le régime moteur.
//0 si le moteur est éteint.
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
  
  //Calcul de la durée moyenne d'une pulsation LOW et HIGH en seconde
  durationPulseLOW /= (f * (float)1000000);
  durationPulseHIGH /= (f * (float)1000000);
  
  
  if(durationPulseLOW == 0 || durationPulseHIGH == 0){
    return 0;
  }
  else{
    //Calcul de la frequence en Hz en faisant l'inverse de la période égale à la somme des durées des pulsations HIGH et LOW
    f = (float)1/ (durationPulseLOW + durationPulseHIGH);
    //Le 6.51 a été calculé de façon empirique.
    float rpm = (float)f * ALPHA_RPM;
    
    //En utilisant la formule trouver sur sonelec, nous aurions :
    //float rpm = f * (float)40 
    //On arrondi à l'entier le plus proche
    return (int)(rpm + (float)0.5);
  }
}

//Fonction permettant de calculer la consommation instantannée.
//Pour déterminer les paramètres a, b et c, il faut récupérer la courbe de consommation L/h en fonction du régime moteur du constructeur, reporter quelques valeurs de cette
//courbe dans un tableur et déterminer à l'aide de ce tableur l'équation de la courbe saisie de la forme a * 10^b * rpm^c ...
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
      else if(_editedParam == TEMPALARM)_editedParam = TEMPFAN;
      else if(_editedParam == TEMPFAN)_editedParam = QTETR;
      _refresh |= MODE;
    }
    else if(delayButton <= 2000 && _mode != MODECONFIG){
      _mode = _mode == 1?2:1;
        
      _refresh |= MODE;
    }
  }
}


void onPlusButtonPressed(){
  int moinsButtonState = digitalRead(inPinButtonMoins);
  if(moinsButtonState == HIGH && _mode == MODE2){
    //Les deux boutons sont enfoncés
    _trip = 0;
    _qteGasTrip = 0;
    writelong(filename, POS_TRIP_QTE, _qteGasTrip);
    writelong(filename, POS_TRIP, _trip);
    _refresh |= QTETRIP;
  }
  else{
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
          writelong(filename, POS_TEMP_ALERT, (long)TEMP_ALERT);
          
          _refresh |= TEMPALARM;
        }
        else if(_editedParam == TEMPFAN){
          if(TEMP_FAN != -1){
            TEMP_FAN++;
          }
          
          if(TEMP_FAN > 90){
            TEMP_FAN = -1;
          }
          writelong(filename, POS_TEMP_FAN, (long)TEMP_FAN);
          
          _refresh |= TEMPFAN;
        }
        else if(_editedParam == QTETR){
          _qteGas += (long)1000000;
          
          if(_qteGas > QTELIMIT){
            _qteGas = QTELIMIT;
          }
          writelong(filename, POS_QTE, _qteGas);
          _refresh |= QTETR;
        }
        else if(_editedParam == QTEMAIN){
          _qteGasMainTank += (long)1000000;
          
          if(_qteGasMainTank > QTEMAINLIMIT){
            _qteGasMainTank = QTEMAINLIMIT;
          }
          writelong(filename, POS_MAIN_QTE, _qteGasMainTank);
          _refresh |= QTEMAIN;
        }
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
        writelong(filename, POS_TEMP_ALERT, (long)TEMP_ALERT);
        _refresh |= TEMPALARM;
      }
      else if(_editedParam == TEMPFAN){
        if(TEMP_FAN == -1){
          TEMP_FAN = 90;
        }
        else{
          TEMP_FAN--;
        }
        
        if(TEMP_FAN < 20){
          TEMP_FAN = 20;
        }
        writelong(filename, POS_TEMP_FAN, (long)TEMP_FAN);
        _refresh |= TEMPFAN;
      }
      else if(_editedParam == QTETR){
        _qteGas -= (long)1000000;
        
        if(_qteGas < 0){
          _qteGas = 0;
        }
        writelong(filename, POS_QTE, _qteGas);
        _refresh |= QTETR;
      }
      else if(_editedParam == QTEMAIN){
        _qteGasMainTank -= (long)1000000;
        
        if(_qteGasMainTank < 0){
          _qteGasMainTank = 0;
        }
        writelong(filename, POS_MAIN_QTE, _qteGasMainTank);
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

//Affiche le texte à la position désirée;
void displayScreen(int numLine, int numCol, String text){
  lcd.setCursor(numCol, numLine);
  lcd.print(text);
}

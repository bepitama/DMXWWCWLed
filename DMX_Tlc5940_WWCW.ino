/*Program to control two LEDs with 12bit interpolation with TLC5940 via 2 DMX channels.
Channels 1-8 of the dipswitch set the DMX channel, 9 the logarithmic or linear mode and 10 adds a 3 master channel.
by Giuseppe Tamanini 18/03/2022
This example code is in the public domain.*/

#include "Tlc5940.h"
#include <DMXSerial.h>

byte DMXWWValue;
byte DMXCWValue;
byte DMXMasterValue;
int WWValue;
int CWValue;
byte oldDMXWWValue;
byte oldDMXCWValue;
byte oldDMXMasterValue;

const int sd1 = 8;    // pin controllo per dip 1-5
const int sd2 = 12;    // pin controllo per dip 6-10

const int dipPin[] = {18,17,16,15,14}; // assegnazione Pin per leggere la condizione del DipSwitch
// tramite i pin 8 vengono alimentati i dip 1-5 e viene letto il valore sui pin 14-18 (A0-A4)
// tramite i pin 12 vengono alimentati i dip 6-10 e viene letto il valore sui pin 14-18 (A0-A4)

int dipVal[] = {1,2,4,8,16,32,64,128,256,512}; // valori da assegnare in base ai dip del dipswitch da sx a dx

int address; // indirizzo DMX
int pin; // pin letto sul dipswitch
int valore; // valore binario del pin
boolean stato; // stato del pin
boolean channelmode; // modalità 2 o 3 canali (con master)
boolean lightmode; // modalità lineare o logaritmica

void setup () {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sd1, OUTPUT); // setta i pin per leggere i primi 5 valori del dipswitch
  pinMode(sd2, OUTPUT); // setta i pin per leggere i secondi 5 valori del dipswitch
  selectchannel(); // esegue la procedura lettura del canale DMX iniziale
  if (channelmode) digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  Tlc.init(); // inizializza il TLC5940
  delay(100);
  Tlc.set(1, 4095); // spegne i canali 1 e 2 del TLC
  Tlc.set(2, 4095);
  Tlc.update();
  delay(1000);
  DMXSerial.init(DMXReceiver); // inizilizza il MAX485 in lettura
}


void loop() {
  if (channelmode) {
    DMXMasterValue = DMXSerial.read(address + 2);
  } else {
    DMXMasterValue = 255;
  }
  DMXWWValue = DMXSerial.read(address); // legge i valori DMX
  DMXCWValue = DMXSerial.read(address + 1);
  if (channelmode && DMXMasterValue != oldDMXMasterValue) {
    interpMaster(DMXMasterValue, oldDMXMasterValue);
    oldDMXMasterValue = DMXMasterValue;
  }
  if (DMXWWValue != oldDMXWWValue) {  // se i valori non sono cambiati
    if (lightmode) {
      interpValLog(1, DMXWWValue, oldDMXWWValue); // esegue la procedura di interpolazione logaritimica a 12bit
    } else {
      interpValLin(1, DMXWWValue, oldDMXWWValue); // esegue la procedura di interpolazione logaritimica a 12bit
    }
    oldDMXWWValue = DMXWWValue;
  }
  if (DMXCWValue != oldDMXCWValue) {
    if (lightmode) {
      interpValLog(2, DMXCWValue, oldDMXCWValue);
    } else {
      interpValLin(2, DMXCWValue, oldDMXCWValue);
    }
    oldDMXCWValue = DMXCWValue;
  }
  delay(10);
}

// procedura che interpola i valori da 8 a 12 bit
// i valori DMX variano da 0 a 255 (8bit) mentre il TLC5940 ha le uscite da 0 a 4096 (12bit)
// se devo variare il valore da 31 a 97 nella scala a 8 bit che produrrebbe 66 scalini (a volte visibili quando si regolano i led)
// la procedura programmerà l'uscita del TLC540 dal valore 31 * 16 = 496 al valore 97 * 16 = 1552 con 1552 - 496 = 1056 scalini con una risoluzione 16 volte maggiore
void interpValLog(byte can, byte Val, byte oldVal) {
  if (Val > oldVal) {
    for (int n = oldVal * 16.06; n < Val * 16.06; n++) {
      Tlc.set(can, 4095.00 - 0.0001 * n * n * 2.442 * DMXMasterValue / 255.00); // formula che simula il comportamento di una lampada ad incandescanza
      Tlc.update();
    }
  } else {
    for (int n = oldVal * 16.06; n >= Val * 16.06; n--) {
      Tlc.set(can, 4095.00 - 0.0001 * n * n * 2.442 * DMXMasterValue / 255.00);
      Tlc.update();
    }
  }
}

void interpValLin(byte can, byte Val, byte oldVal) {
  if (Val > oldVal) {
    for (int n = oldVal * 16.06; n < Val * 16.06; n++) {
      Tlc.set(can, 4095.00 - n * DMXMasterValue / 255.00);
      Tlc.update();
    }
  } else {
    for (int n = oldVal * 16.06; n >= Val * 16.06; n--) {
      Tlc.set(can, 4095.00 - WWValue * DMXMasterValue / 255.00);
      Tlc.update();
    }
  }
}

void interpMaster(byte Val, byte oldVal) {
  if (Val > oldVal) {
    for (int n = oldVal * 16.06; n < Val * 16.06; n++) {
      if (lightmode) {
        Tlc.set(1,  4095.00 - 0.0001 * DMXWWValue * 16.06 * DMXWWValue * 16.06 * 2.442 * n / 4096.00);
      } else {
        Tlc.set(2, 4095 - DMXCWValue * 16.06 / 4096.00);
      }
      Tlc.update();
    }
  } else {
    for (int n = oldVal * 16.06; n >= Val * 16.06; n--) {
      if (lightmode) {
        Tlc.set(1,  4095.00 - 0.0001 * DMXWWValue * 16.06 * DMXWWValue * 16.06 * 2.442 * n / 4096.00);
      } else {
        Tlc.set(2, 4095 - DMXCWValue * 16.06 / 4096.00);
      }
      Tlc.update();
    }
  }  
}

void selectchannel() {
    for (pin = 0; pin < 5;pin++) {
    pinMode(dipPin[pin], INPUT); // setta ingressi del dipswitch
  }
  // legge i primi 5 valori
  digitalWrite(sd1, HIGH); // Attiva l'uscita del pin 8 per leggere i valori dei dip 1-5
  digitalWrite(sd2, LOW);  // Disattiva il pin 12
  
  for (pin = 0;pin < 5;pin++) {
    stato = digitalRead(dipPin[pin]);// legge il valore digitale del pin
    if (stato == HIGH) {
      valore = dipVal[pin];
      address = address + valore;
    }
    delay(50);
  }
  // legge i secondi 5 valori
  digitalWrite(sd1, LOW);  // Disattiva il pin 8
  digitalWrite(sd2, HIGH); // Attiva l'uscita del pin 12 per leggere i valori dei dip 6-10

  for (pin = 4; pin >= 0 ;pin--) {
    stato = digitalRead(dipPin[pin]); // legge il valore digitale del pin
    if (stato == HIGH && pin > 1) {
      valore = dipVal[9 - pin]; // incrementa address aggiungendo valore
      address = address + valore;
    }
    if (pin == 1) {
      lightmode = stato;
    }
    if (pin == 0) {
      channelmode = stato;
    }
    delay(50);
  }
  digitalWrite(sd1, LOW); // Disattiva i pin 8 e 12
  digitalWrite(sd2, LOW);
}

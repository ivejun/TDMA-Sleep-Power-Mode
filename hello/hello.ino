#include <JeeLib.h>
#include <Ports.h>
#include <DHT.h>
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Wire.h>
#include <RTClibExtended.h>
#include <LowPower.h>

#define wakePin 2
#define ledPin 13
#include "DHT.h"

#define DHTPIN A0
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

int pembacaandata = 0;

RTC_DS3231 RTC;

char Sched2[3], Leveltemp[2], jadwal2[3], TDMApacknumb2[2];
char RandNumber[5], LevelCheck[2];
char AlamatSendiri[5];
char AlamatParent[5];
char AlamatTerima[32];
char DataTerima[32];
char DataKirim[32];
char LevelKirim[2], TimeStamp1[4], TimeStamp2[4], TimeStamp4[4];
int  ubahmenit, ubahjam, detik, milidetik, menit, jam, ubahdetik, ubahmili, Level, tunggu, TS1, TS2, TS3, TS4, TimeStampInt;
char buffjam[3], buffmenit[3], buffdetik[3], buffmili[4];
unsigned long Latency, milibaru, detikbaru, menitbaru, jambaru;
int Sched, jadwal, jadwalpatent, q, h;
boolean terdaftar, tersinkron;
int x, TDMApacknumb, detiklama, LevelClear, w;
char  milidetik2[4], detik2[3], menit2[3], jam2[3];
const int buttonPin = 4;
int buttonState = 1;

ISR(WDT_vect) {
  Sleepy::watchdogEvent();  //low power
}
//#define DHTPIN 2
//#define DHTTYPE DHT11

int con = 1;
byte AlarmFlag = 0;
byte ledStatus = 1;

/*==========COMMON NODE===========*/

void setup() {
  Serial.begin(9600);

  pinMode(wakePin, INPUT);
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  RTC.armAlarm(1, false);
  RTC.clearAlarm(1);
  RTC.alarmInterrupt(1, false);
  RTC.armAlarm(2, false);
  RTC.clearAlarm(2);
  RTC.alarmInterrupt(2, false);
  RTC.writeSqwPinMode(DS3231_OFF);
                              //Jam, menit.
  RTC.setAlarm(ALM1_MATCH_HOURS, 21, 00, 0);
  RTC.alarmInterrupt(1, true);


  Sleepy::loseSomeTime(1000);
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setRADDR((byte *)"node");
  Mirf.payload = 32;
  Mirf.channel = 101;
  Mirf.config();
  Serial.println("Waiting...");
  terdaftar = 0;
  tersinkron = 0;
  pinMode(buttonPin, INPUT);
  tunggu = random(analogRead(A0), 60000);
  while (tunggu % 17000 < 4000 || tunggu < 28000) {
    tunggu = random(analogRead(A0), 60000);
  }
  x = 0;
  TDMApacknumb = 0;
  Sched = 0;
}



void loop() {

  if (AlarmFlag == 0) {
    attachInterrupt(0, wakeUp, LOW);
    digitalWrite(ledPin, LOW);
    ledStatus = 0;
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(0);

    //When exiting the sleep mode we clear the alarm
    RTC.armAlarm(1, false);
    RTC.clearAlarm(1);
    RTC.alarmInterrupt(1, false);
    AlarmFlag++;
  }


  //buttonState = digitalRead(buttonPin);
  if (Mirf.dataReady()/* && buttonState == HIGH*/) {
    Mirf.getData((byte *)&DataTerima);
    LevelCheck[0] = DataTerima[8];
    LevelClear = strtoul(LevelCheck, NULL, 0);
    if (!terdaftar && DataTerima[0] == 'B' && DataTerima[1] != 'B') {
      int f = DataTerima[30];
      w = f;
      Serial.print("Jadwal Detik ke : ");
      Serial.println(w);
      discovered();
      if (Level == 2) {
        tunggu = 7000;
      }
      Serial.print("waiting time: ");
      Serial.println(tunggu);

    }
    else if (DataTerima[0] == 'S') {
      waktu();
      TS4 = milidetik;
      detiklama = detik;
      buffering();
      synchronized();
      waktu();
    }
    else if (DataTerima[0] == 'R' && LevelCheck != LevelKirim && tersinkron) {
      waktu();
      sprintf(TimeStamp2, "%d", milidetik);
      TimeStampInt = milidetik;
      answer_sync();
      waktu();
    }
    else if (DataTerima[0] == 'T' && LevelClear == (Level + 1)) {
      TDMArecv();
    }
  }
  else {
    if (!tersinkron && terdaftar && millis() % tunggu == 1) {
      synchronize();
      delay(1);
    }
    if (tersinkron) {
      if (milidetik % 999 == 0) {
        //if(x==0 && detik > detikbaru ){ubahdetik=ubahdetik-1;}
        Sleepy::loseSomeTime(300);
        if (menit == 2 && Level == 1 && w == 5) {
          x++;
          if (x < 12) {
            q = q + 5;
            discovery();
            oke();
            if (q == 55) {
              q = 0;
            }
          }
        }
      }
      if (detik == w && menit >= 3) {
        TDMAsend();
        jadwal = jadwal + 2;
        if ((jadwal - jadwalpatent) > 3) {
          jadwal = jadwalpatent;
        }
      }
    }
  }
  waktu();
  AlarmFlag = 0;
  delay(1000);

}

void oke() {
  Serial.print("jadwal detik : ");
  Serial.println(q);
  int h = DataKirim[30];
  Serial.println(h);
}

void wakeUp()
{
}

void discovered() {
  //catat alamat parent
  for (int d = 0; d < 4; d++) {
    AlamatParent[d] = DataTerima[d + 1];
  }

  Serial.print("packet discovery received from: ");
  Serial.println(AlamatParent);

  //catat level
  Serial.println(DataTerima[5]);
  Leveltemp[0] = DataTerima[5];
  Level = strtoul(Leveltemp, NULL, 0);
  Level = Level + 1;
  Serial.print("node tree level: ");
  Serial.println(Level);

  //ubah alamat sendiri
  UbahAlamat();
  Serial.print("my address: ");
  Serial.println(AlamatSendiri);
  Mirf.setRADDR((byte *)AlamatSendiri);
  terdaftar = 1;
}

void UbahAlamat() {
  int Acak = random(analogRead(A0), 9999); //generate random number
  while (Acak < 1000) {
    Acak = random(analogRead(A0), 9999);
  }
  sprintf(AlamatSendiri, "%d", Acak);
}

void synchronize() {
  waktu();
  Mirf.setTADDR((byte *)AlamatParent);
  strcpy(DataKirim, "R");
  strcat(DataKirim, AlamatSendiri);
  sprintf(TimeStamp1, "%d", milidetik);
  if (milidetik < 10) {
    strcat(DataKirim, "0");
    strcat(DataKirim, "0");
  } else if (milidetik < 100) {
    strcat(DataKirim, "0");
  }
  strcat(DataKirim, TimeStamp1);
  sprintf(LevelKirim, "%d", Level);
  strcat(DataKirim, LevelKirim);
  Latency = millis();
  Mirf.send((byte *)&DataKirim);
  while (Mirf.isSending()) {
    Serial.println("request synchronization sent!");
    Serial.println(DataKirim);
    cetakwaktu();
    delay(2);
  }
}

void synchronized() {
  Serial.print("begin: ");
  waktu();
  cetakwaktu();
  Serial.println("");
  waktu();
  Latency = millis() - Latency;
  ubahwaktu();
  waktu();
  cetakwaktu();
  Serial.println("synchronized!");
  Serial.print("delay total: ");
  Serial.println(Latency);
  Serial.print("delay propagasi: ");
  Serial.println(TS4);
  if (TS4 < 20 && TS4 > 0) {
    tersinkron = 1;
  }
  Sched = jadwalpatent + 30;
}

void waktu() {
  milidetik = millis() % 1000;
  detik = (millis() + ubahmili) / 1000;
  milidetik = milidetik + ubahmili;
  detik = detik + ubahdetik;
  menit = detik / 60;
  menit = menit + ubahmenit;
  jam = menit / 60;
  jam = jam + ubahjam;

  if (milidetik > 999) { //reset milidetik
    milidetik = milidetik % 1000;
  } else if (milidetik < 0) {
    milidetik = 1000 + milidetik;
  }

  if (detik > 59) { //reset detik
    detik = detik % 60;
  } else if (detik < 0) {
    detik = detik + 60;
  }

  if (menit > 59) { //reset menit
    menit = menit % 60;
  } else if (menit < 0) {
    menit = menit + 60;

    if (jam > 23) { //reset jam
      jam = jam % 24;
    }
  }
}


void cetakwaktu() {
  Serial.print(jam); Serial.print(":");
  Serial.print(menit); Serial.print(":");
  Serial.print(detik); Serial.print(":");
  Serial.print(milidetik);
  //  delay(1000);
}

void ubahwaktu() {
  ubahmili = milibaru - milidetik;
  ubahdetik = detikbaru - detiklama;
  ubahmenit = menitbaru - menit;
  ubahjam = jambaru - jam;
  ubahmili = ubahmili + TS4;
}

void discovery() {
  sprintf(LevelKirim, "%d", Level);
  Mirf.setTADDR((byte *)"node");
  strcpy(DataKirim, "B"); //first, adding header character
  strcat(DataKirim, AlamatSendiri); //concat 2 var
  strcat(DataKirim, LevelKirim);
  DataKirim[30] = q;
  Mirf.send((byte *)&DataKirim);
  Serial.println("packet discovery sent to all node!");
  while (Mirf.isSending()) {

  }
}

void kirimwaktu() {
  waktu();
  sprintf(milidetik2, "%d", milidetik);
  sprintf(detik2, "%d", detik);
  sprintf(menit2, "%d", menit);
  sprintf(jam2, "%d", jam);

  strcpy(DataKirim, "S"); //first, adding header character
  if (jam < 10) {
    strcat(DataKirim, "0");
  }
  strcat(DataKirim, jam2);
  strcat(DataKirim, ":");
  if (menit < 10) {
    strcat(DataKirim, "0");
  }
  strcat(DataKirim, menit2);
  strcat(DataKirim, ":");
  if (detik < 10) {
    strcat(DataKirim, "0");
  }
  strcat(DataKirim, detik2);
  strcat(DataKirim, ":");
  if (milidetik < 10) {
    strcat(DataKirim, "0");
    strcat(DataKirim, "0");
  } else if (milidetik < 100) {
    strcat(DataKirim, "0");
  }
  strcat(DataKirim, milidetik2);
}

void answer_sync() {
  TDMAsched();
  for (int e = 0; e < 4; e++) {
    AlamatTerima[e] = DataTerima[e + 1];
  }
  for (int f = 0; f < 3; f++) {
    TimeStamp1[f] = DataTerima[f + 5];
  }
  Serial.print("synchronization request from: ");
  Serial.println(AlamatTerima);
  cetakwaktu();
  Mirf.setTADDR((byte *)AlamatTerima);//kirim balasan
  kirimwaktu();
  strcat(DataKirim, TimeStamp1); //timestamp1
  if (TimeStampInt < 10) {
    strcat(DataKirim, "0");
    strcat(DataKirim, "0");
  } else if (TimeStampInt < 100) {
    strcat(DataKirim, "0");
  }
  strcat(DataKirim, TimeStamp2); //timestamp2
  sprintf(Sched2, "%d", Sched);
  strcat(DataKirim, Sched2);
  Serial.println(Sched2);
  Mirf.send((byte *)&DataKirim);
  while (Mirf.isSending()) {
    Serial.print("synchronization message sent on: ");
    cetakwaktu();
  }
}

void buffering() {
  buffjam[0] = DataTerima[1];
  buffjam[1] = DataTerima[2];
  buffmenit[0] = DataTerima[4];
  buffmenit[1] = DataTerima[5];
  buffdetik[0] = DataTerima[7];
  buffdetik[1] = DataTerima[8];
  buffmili[0] = DataTerima[10];
  buffmili[1] = DataTerima[11];
  buffmili[2] = DataTerima[12];
  TimeStamp2[0] = DataTerima[16];
  TimeStamp2[1] = DataTerima[17];
  TimeStamp2[2] = DataTerima[18];
  milibaru = strtoul(buffmili, NULL, 0);
  detikbaru = strtoul(buffdetik, NULL, 0);
  menitbaru = strtoul(buffmenit, NULL, 10);
  jambaru = strtoul(buffjam, NULL, 10);
  TS1 = strtoul(TimeStamp1, NULL, 0);
  TS2 = strtoul(TimeStamp2, NULL, 0);
  TS3 = milibaru;
  TS4 = ((TS2 - TS1) + (TS4 - TS3)) / 2;
  jadwal2[0] =  DataTerima[19];
  jadwal2[1] =  DataTerima[20];
  jadwal = strtoul(jadwal2, NULL, 10);
  jadwalpatent = jadwal;
}

void TDMAsend() {
  TDMApacknumb = TDMApacknumb + 1;
  TDMApacknumb = TDMApacknumb % 10;
  sprintf(TDMApacknumb2, "%d", TDMApacknumb);
  float suhu = dht.readTemperature();
  float data = suhu;
  Mirf.setTADDR((byte *)"1111");
  strcpy(DataKirim, "T");
  strcat(DataKirim, AlamatSendiri);
  strcat(DataKirim, TDMApacknumb2);
  strcat(DataKirim, "0");
  sprintf(LevelKirim, "%d", Level);
  strcat(DataKirim, LevelKirim);
  Mirf.setTADDR((byte *)"1111");
  DataKirim[31] = data;
  Mirf.send((byte *)&DataKirim);
  while (Mirf.isSending());
  Serial.print("temperature");
  Serial.print(" : ");
  Serial.print(suhu);
  Serial.println("oC");
  Serial.print("from level : ");
  Serial.println(Level);
  cetakwaktu();
  Serial.println("temperature sent!");
}

void TDMArecv() {
  for (int c = 0; c < 4; c++) {
    AlamatTerima[c] = DataTerima[c + 1];
  }
  Serial.print("TDMA confirmation message received from: ");
  Serial.print(AlamatTerima);
  Serial.print("(");
  Serial.print(DataTerima[5]);
  Serial.print(")");
  Serial.println();
}

void TDMAsched() {
  Sched = Sched + (w + 10);
}


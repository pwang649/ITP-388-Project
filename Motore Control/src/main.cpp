#include <Arduino.h>
#include <analogWrite.h>
const int AIN1 = 27;
const int AIN2 = 33;
const int PWMA = 12;
const int BIN1 = 15;
const int BIN2 = 32;
const int PWMB = 14;
int counter = 0;

void setup()
{
  // put your setup code here, to run once:
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
}

void loop()
{
  if (counter < 100)
  {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);

    analogWrite(PWMA, 100, 255);
    analogWrite(PWMB, 100, 255);

    counter = counter + 1;
  }
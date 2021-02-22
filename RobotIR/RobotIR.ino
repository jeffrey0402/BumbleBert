int IR1 = 34;
int IR2 = 35;
void setup() {
pinMode(IR1, INPUT);
pinMode(IR2, INPUT);
Serial.begin(115200);
}

void loop() {
  int statusSensor1 = analogRead(IR1);
  int statusSensor2 = analogRead(IR2);
  Serial.print("IR1: ");
  Serial.println(statusSensor1);
  Serial.print("IR2: ");
  Serial.println(statusSensor2);
  delay(5000);
}

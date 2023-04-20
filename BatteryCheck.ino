// Define the voltage divider resistor value
const float R1 = 10000.0; // 10kOhm


float batteryCheck(const int analogInPin){

   // Read the voltage across the voltage divider
  int rawValue = analogRead(analogInPin); // read voltage from analog input A0
  float voltage = rawValue * (5.0 / 1023.0); // convert raw value to voltage

  // Calculate the battery level in percentage
  float batteryLevel = (voltage * R1 / (R1 + 3.7) - 3.7) / (4.15 - 3.7) * 100.0; // calculate battery level in percentage
  batteryLevel = constrain(batteryLevel, 0, 100); // limit the battery level to 0-100%

  // Print the battery level to the serial monitor
  Serial.print("Battery Level: ");
  Serial.print(batteryLevel, 2);
  Serial.println("%");

  delay(1000); // wait for a second before reading again


  return batteryLevel;
}

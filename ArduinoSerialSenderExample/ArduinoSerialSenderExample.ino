void setup() {
  // Start UART communication
  Serial.begin(9600);
}

void loop() {
  String data = "t:36|l:1|la:50.00|c:1|br:1|bt:58";

  // Send the data over UART
  Serial.println(data);

  delay(1000);
}

void callWebsite(){

  WiFiClientSecure client;
  client.setInsecure(); // Falls das SSL-Zertifikat der Webseite selbstsigniert ist

  Serial.println("Connecting to server...");

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  Serial.println("Connected to server");

  String url = "/wetter/index.php";
  Serial.print("Requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ArduinoWiFi/1.1\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 5000) {
    if (client.available()) {
      // HTTP-Statuscode analysieren
      String line = client.readStringUntil('\r');
      if (line.startsWith("HTTP/1.")) {
        int statusCode = line.substring(9, 12).toInt();
        if (statusCode < 200 || statusCode >= 300) {
          Serial.println("Failed to load webpage, status code " + String(statusCode));
          return;
        }
      }
      timeout = millis();
    }
  }

  client.stop();

  Serial.println("Connection closed");
}

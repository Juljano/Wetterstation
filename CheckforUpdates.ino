void update_started() {
  Serial.println("CALLBACK: HTTP update process started");
}

void update_finished() {
  Firebase.pushString(fb, "/executed Updates/", "Version " + String(currentVersion) + " ist installiert!");
}

void update_error(int err) {
  //Firebase.pushString(fb, "/Updates failed/", String(err));
}

void getUpdateFile() {
  client.setInsecure();
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, "https://www.schrotthandel-moeller.de/wetter/Main.ino.nodemcu.bin");
  Serial.println(ret == HTTP_UPDATE_OK ? "HTTP_UPDATE_OK" : "HTTPS_UPDATE_FAILD Error: " + ESPhttpUpdate.getLastErrorString());
}

void checkForUpdate() {
  Serial.println("Current Version: " + String(currentVersion));
  client.setInsecure();

  if (client.connect("www.schrotthandel-moeller.de", 443)) {
    Serial.println("Connected to server");
    client.print(String("GET ") + "/wetter/Version.txt" + " HTTP/1.1\r\n" +
                 "Host: " + "www.schrotthandel-moeller.de" + "\r\n" +
                 "Connection: close\r\n\r\n");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      line.trim();

      if (line == "") {
        break;
      }
    }

    String versionString = client.readStringUntil('\n');
    versionString.trim();
    latestVersion = versionString.toFloat();

    client.stop();
  }

  if (latestVersion > currentVersion) {
    Serial.println("New version available. Starting update...");
    getUpdateFile();
    
  }else if(latestVersion == currentVersion){
    Serial.println("You are already using the latest version.");
    
    }else {
  
    Serial.println("No update available.");
  }
}

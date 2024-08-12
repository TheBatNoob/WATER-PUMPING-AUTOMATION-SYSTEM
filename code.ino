if (moisturepercent <= 0) {
    moisturepercent = 0;
  }

  data.distance = Ultralsonic_reading;
  data.humidity = bme.readHumidity();         // Get humidityidity value
  data.temprature = bme.readTemperature();    // Get tempratureerature value
  data.pressure = bme.readPressure() / 1000;  // 100.0F;                 //bme.readPressure() / 100.0F;
  data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  data.soilmoisturepercent = moisturepercent;
  data.TDS_PERC = TDS_PERC / 1000;
  data.EC_PERC = EC_PERC;

  radio.write(&data, sizeof(Data_Package));

  Serial.print("  DIS=");
  Serial.print(data.distance);

  Serial.print("  TEMP=");
  Serial.print(data.temprature);

  Serial.print("  HUM=");
  Serial.print(data.humidity);

  Serial.print("  PRE=");
  Serial.print(pressure);

  Serial.print("  ALT=");
  Serial.print(altitude);

  Serial.print("  SM=");
  Serial.print(data.soilmoisturepercent);

  Serial.print("  TDS=");
  Serial.print(tdsValue, 0);
  Serial.print("ppm  ");

  Serial.print("  TDS_mg=");
  Serial.print(TDS_mg);
  Serial.print("mg/l  ");

  Serial.print("  TDs_lev=");
  Serial.print(TDS_PERC);
  Serial.print("%  ");

  Serial.print("  EC=");
  Serial.print(EC);
  Serial.print("p");

  Serial.print("  EC_PERC=");
  Serial.print(EC_PERC);
  Serial.print("%  ");
}



void RX_MODE() {

  radio.startListening();
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package));  // Read the whole data and store it into the 'data' structure

  }

  RX_STARTSTOP_STATE = data.RX_STARTSTOP_STATE;

  if (RX_STARTSTOP_STATE == 1) {
    digitalWrite(relay, HIGH);

    for (int i = 0; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(0, pixels.Color(25, 255, 5));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }

  else if (RX_STARTSTOP_STATE == 0) {
    digitalWrite(relay, LOW);

    for (int i = 0; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }

  Serial.print("  RX_START= ");
  Serial.println(data.RX_STARTSTOP_STATE);
}

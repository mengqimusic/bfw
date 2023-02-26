void update_led(byte led, byte color) {
  for (int pin = 0; pin < 2; pin++) {
    digitalWrite(ledPin[led][pin], !bitRead(ledColor[color], pin));
  }
}

void update_led_octave() {
  if (octave < 3) {
    //digitalWrite(ledPin[0][pin], !bitRead(ledColor[octave], pin));
    //digitalWrite(ledPin[1][pin], !bitRead(ledColor[3], pin));
    update_led(0, octave);
    update_led(1, 3);
  }
  else if (octave == 3) {
    //digitalWrite(ledPin[0][pin], !bitRead(ledColor[3], pin));
    //digitalWrite(ledPin[1][pin], !bitRead(ledColor[3], pin));
    update_led(0, 3);
    update_led(1, 3);
  }
  else {
    //digitalWrite(ledPin[0][pin], !bitRead(ledColor[3], pin));
    //digitalWrite(ledPin[1][pin], !bitRead(ledColor[6 - octave], pin));
    update_led(0, 3);
    update_led(1, 6 - octave);
  }
}

void save_load(int slot, bool sl) {
  char buff[100];
  char *addr;
  snprintf(buff, sizeof(buff), "/blippoo/blippoo_%d.txt", slot);
  addr = buff;

  if (sl == SAVE) {
    patch = SPIFFS.open(addr, FILE_WRITE);
    char tmp[8];
    dtostrf(rateA, 8, 7, tmp); patch.println(tmp);
    dtostrf(rateB, 8, 7, tmp); patch.println(tmp);
    dtostrf(r_to_rateA, 8, 7, tmp); patch.println(tmp);
    dtostrf(r_to_rateB, 8, 7, tmp); patch.println(tmp);
    dtostrf(sh_to_rateA, 8, 7, tmp); patch.println(tmp);
    dtostrf(sh_to_rateB, 8, 7, tmp); patch.println(tmp);
    dtostrf(peak1, 8, 7, tmp); patch.println(tmp);
    dtostrf(peak2, 8, 7, tmp); patch.println(tmp);
    dtostrf(r_to_peak1, 8, 7, tmp); patch.println(tmp);
    dtostrf(r_to_peak2, 8, 7, tmp); patch.println(tmp);
    dtostrf(sh_sp_peaks, 8, 7, tmp); patch.println(tmp);
    dtostrf(octave, 8, 7, tmp); patch.println(tmp);
    patch.close();
  }
  else {
    patch = SPIFFS.open(addr);
    while (patch.available()) {
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &rateA);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &rateB);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &r_to_rateA);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &r_to_rateB);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &sh_to_rateA);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &sh_to_rateB);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &peak1);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &peak2);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &r_to_peak1);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &r_to_peak2);
      sscanf((patch.readStringUntil('\n')).c_str(), "%f", &sh_sp_peaks);
      sscanf((patch.readStringUntil('\n')).c_str(), "%d", &octave);
    }
    patch.close();

    //    Serial.printf("rateA = %f, rateB = %f\nr_to_rateA = %f, r_to_rateB = %f\nsh_to_rateA = %f, sh_to_rateB = %f\n", rateA, rateB, r_to_rateA, r_to_rateB, sh_to_rateA, sh_to_rateB);
    //    Serial.printf("peak1 = %f, peak2 = %f\nr_to_peak1 = %f, r_to_peak2 = %f\nsh_sp_peaks = %f\n", peak1, peak2, r_to_peak1, r_to_peak2, sh_sp_peaks);
    //    Serial.printf("octave = %d\n", octave);

    dsp.setParamValue("rateA", rate_to_semitone(rateA));
    dsp.setParamValue("rateB", rate_to_semitone(rateB));
    dsp.setParamValue("r_to_rateA", r_to_rateA);
    dsp.setParamValue("r_to_rateB", r_to_rateB);
    dsp.setParamValue("sh_to_rateA", sh_to_rateA);
    dsp.setParamValue("sh_to_rateB", sh_to_rateB);
    dsp.setParamValue("peak1", peak_to_semitone(peak1));
    dsp.setParamValue("peak2", peak_to_semitone(peak2));
    dsp.setParamValue("r_to_peak1", r_to_peak1);
    dsp.setParamValue("r_to_peak2", r_to_peak2);
    dsp.setParamValue("sh_sp_peaks", sh_sp_peaks);
  }
}

void save_global_settings() {
  prefs.begin("settings", RW_MODE);

  if (dirty[0]) {
    dirty[0] = false;
    if (prefs.putUChar("midi_ch_l", midi_ch_l)) Serial.printf("midi_ch_l is saved, value is %d.\n", midi_ch_l);
  }
  if (dirty[1]) {
    dirty[1] = false;
    if (prefs.putUChar("midi_ch_r", midi_ch_r)) Serial.printf("midi_ch_r is saved, value is %d.\n", midi_ch_r);
  }
  if (dirty[2]) {
    dirty[2] = false;
    if (prefs.putUChar("midi_ch_both", midi_ch_both)) Serial.printf("midi_ch_both is saved, value is %d.\n", midi_ch_both);
  }

  if (dirty[3]) {
    dirty[3] = false;
    float freq_offset = a3_freq - 440.;
    if (prefs.putFloat("a3_freq_offset", freq_offset)) Serial.printf("a3_freq_offset (%.2fHz) is saved. a3 = %.2fHz.\n", freq_offset, a3_freq);
  }

  prefs.end();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.print (file.name());
      time_t t = file.getLastWrite();
      struct tm * tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, ( tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm * tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, ( tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

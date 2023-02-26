void control(void* pvParameters) {
  Serial.print("control running on core ");
  Serial.println(xPortGetCoreID());

  //
  // Hardware Init Begin
  //

  for (int ch = 0; ch < 2; ch++) {
    for (int pin = 0; pin < 2; pin++) {
      pinMode(ledPin[ch][pin], OUTPUT);
    }
  }

  Wire.begin(17, 18);

  for (int i = 0; i < 2; i++) {
    pinMode(rstn[i], OUTPUT);
    digitalWrite(rstn[i], true);
    pinMode(intn[i], INPUT);
  }

  delay(5);

  Serial.println("AW9523 : Connecting...");

  if (!aw0.begin(0x58)) Serial.println("AW9523 0 : Connection Failed");
  else Serial.println("AW9523 0 : Connected");

  if (!aw1.begin(0x59)) Serial.println("AW9523 1 : Connection Failed");
  else Serial.println("AW9523 1 : Connected");

  aw0.reset();
  aw1.reset();

  for (int i = 0; i < 16; i++) {  // setup aw9523
    aw0.pinMode(i, INPUT);
    aw0.enableInterrupt(i, true);
    aw1.pinMode(i, INPUT);
    aw1.enableInterrupt(i, true);
  }

  for (int i = 0; i < 16; i += 8) {  // anti-stuck
    bool tmp = aw0.digitalRead(i);
    tmp = aw1.digitalRead(i);
  }

  attachInterrupt(digitalPinToInterrupt(intn[0]), Pressed0, FALLING);
  attachInterrupt(digitalPinToInterrupt(intn[1]), Pressed1, FALLING);

  Serial.println("AC101 : Connecting...");

  while (not ac.begin()) {
    Serial.println("AC101 : Failed! Trying...");
    delay(100);
  }
  Serial.println("AC101 : Connected");

  acWriteReg(ADC_SRCBST_CTRL, MIC_BOOST);
  acWriteReg(OMIXER_BST1_CTRL, 0x56DB);

  ac.SetVolumeHeadphone(volume);
  ac.SetVolumeSpeaker(0);

  acWriteReg(OMIXER_SR, 0x0102);      // 摆正左右通道 : 左 DAC -> 左输出 右 DAC -> 右输出
  acWriteReg(DAC_VOL_CTRL, DAC_VOL);  // 左右通道输出音量 A0 = 0dB A4 = 3dB

  source = !aw1.digitalRead(sourcePin);
  acWriteReg(ADC_SRC, sources[source]);

  ac.DumpRegisters();

  //
  // Hardware Init End
  //

  //
  // Software Init Begin
  //

  // Preferences Section Begin

  prefs.begin("counter");
  unsigned int counter = prefs.getUInt("counter", 0);
  counter++;
  Serial.printf("这是此小羽第 %u 次启动。\n", counter);
  prefs.putUInt("counter", counter);
  prefs.end();

  prefs.begin("settings", RO_MODE);

  bool nvs_init = prefs.isKey("nvs_init");

  if (!nvs_init) {
    prefs.end();
    prefs.begin("settings", RW_MODE);

    prefs.putUChar("midi_ch_l", 1);
    prefs.putUChar("midi_ch_r", 2);
    prefs.putUChar("midi_ch_both", 3);
    prefs.putFloat("a3_freq_offset", 0.);
    prefs.putBool("nvs_init", true);
    prefs.end();
    Serial.println("NVS initialized");
    prefs.begin("settings", RO_MODE);
  }

  midi_ch_l = prefs.getUChar("midi_ch_l");
  midi_ch_r = prefs.getUChar("midi_ch_r");
  midi_ch_both = prefs.getUChar("midi_ch_both");
  Serial.printf("midi_ch_l = %d / midi_ch_r = %d / midi_ch_both = %d\n", midi_ch_l, midi_ch_r, midi_ch_both);
  float a3_freq_offset = prefs.getFloat("a3_freq_offset", 99);
  a3_freq = 440. + a3_freq_offset;
  dsp.setParamValue("a3_freq", a3_freq);
  Serial.printf("a3_freq = %.2f\n", a3_freq);

  prefs.end();

  // Preferences Section End

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  listDir(SPIFFS, "/", 0);

  // total space = 585, save 12 blippoo sets - free prefs space = 177

  dsp.setParamValue("Q", 31.);
  dsp.setParamValue("mod_depth", 100.);


  //
  // Key Init State Read 用来面对，开机时有些键是被按着的情况
  //
  for (int i = 0; i < 16; i++) {
    bool tmp = aw0.digitalRead(i);
    if (i < 8) key[0][7 - i] = tmp;
    else if (i < 12 && i > 7) key[0][11 - i + 8] = tmp;
    else key[1][i - 12] = tmp;
  }

  for (int i = 0; i < 4; i++) {
    bool tmp = aw1.digitalRead(i);
    key[1][3 - i + 4] = tmp;
  }

  for (int i = 8; i < 12; i++) {
    bool tmp = aw1.digitalRead(i);
    key[1][11 - i + 8] = tmp;
  }

  for (int ch = 0; ch < 2; ch++) {
    for (int i = 0; i < 12; i++) keyPrev[ch][i] = key[ch][i];
  }

  //
  // 循环在下面
  //

  for (;;) {
    interrupts();
    currentMillis = millis();

    //Serial.println(uxTaskGetStackHighWaterMark(NULL)); // get unused memory

    //
    // save global settings
    //
    if (dirty[0] | dirty[1] | dirty[2] | dirty[3]) {
      if (currentMillis - save_global_settings_timer > 1000) {
        save_global_settings_timer = currentMillis;
        save_global_settings();
      }
    }


    //
    // startup
    //
    if (startup) {
      if (currentMillis - startupMillis > 25) {
        startupMillis = currentMillis;
        if (volume < 63) {
          volume += 1;
          ac.SetVolumeHeadphone(volume);
        } else startup = false;
      }
    }


    //
    // 9523 Anti-Stuck
    //
    if (currentMillis - routineReadTimer > 500) {
      routineReadTimer = currentMillis;
      for (int i = 0; i < 16; i += 8) {
        bool tmp = aw0.digitalRead(i);
        tmp = aw1.digitalRead(i);
      }
    }


    //
    // Interface Reading
    //
    for (int i = 0; i < 3; i++) {
      pot_val_realtime[i] = analogRead(potPin[i]);
      int difference = abs(pot_val_realtime[i] - pot_val_sampled[i]);
      if (!realtime_value_valid[i])
        if (difference > slider_movement_detect) realtime_value_valid[i] = true;
    }

    Mix = pot_val_realtime[0] / 4095.;
    if (realtime_value_valid[MIX]) dsp.setParamValue("mix", Mix);

    sh_source_mix = pot_val_realtime[1] / 4095.;
    if (realtime_value_valid[SH_SOURCE_MIX]) dsp.setParamValue("sh_source_mix", sh_source_mix);

    Volume = pot_val_realtime[2] / 4095.;
    if (realtime_value_valid[VOL]) dsp.setParamValue("gain", Volume);

    bool sourceSwitchPos = !aw1.digitalRead(sourcePin);

    switch_val_realtime[0] = -!aw1.digitalRead(lOctPin[0]) + !aw1.digitalRead(lOctPin[1]) + 1;
    switch_val_realtime[1] = -!aw1.digitalRead(rOctPin[0]) + !aw1.digitalRead(rOctPin[1]) + 1;
    if (!realtime_value_valid[R1_SOURCE])
      if (switch_val_realtime[0] != switch_val_sampled[0]) realtime_value_valid[R1_SOURCE] = true;
    if (!realtime_value_valid[R2_SOURCE])
      if (switch_val_realtime[1] != switch_val_sampled[1]) realtime_value_valid[R2_SOURCE] = true;
    if (realtime_value_valid[R1_SOURCE]) dsp.setParamValue("source0", switch_val_realtime[0]);
    if (realtime_value_valid[R2_SOURCE]) dsp.setParamValue("source1", switch_val_realtime[1]);


    //
    // source change
    //
    if (sourceSwitchPos != source) {
      source = sourceSwitchPos;
      acWriteReg(ADC_SRC, sources[source]);
      //sourceChanged = true;
      //ac.SetVolumeHeadphone(0);
      //sourceChangedMillis = currentMillis;
    }

    //    if (sourceChanged) {
    //      if (currentMillis - sourceChangedMillis > 5) {
    //        sourceChanged = false;
    //        sourceChanged2 = true;
    //        acWriteReg(ADC_SRC, sources[source]);
    //        sourceChangedMillis = currentMillis;
    //      }
    //    }

    //    if (sourceChanged2) {
    //      if (currentMillis - sourceChangedMillis > 50) {
    //        sourceChanged2 = false;
    //        ac.SetVolumeHeadphone(volume);
    //      }
    //    }


    //
    // No Interrupts
    //
    noInterrupts();


    //
    // LED display sr output
    //
    if (!keyboard_mode) {
      byte led_color[2];
      led_color[0] = ((((byte)dsp.getParamValue("srA_bit_out_1") << 1) | (byte)dsp.getParamValue("srA_bit_out_2")) + (byte)dsp.getParamValue("srA_bit_out_0")) % 4;
      led_color[1] = ((((byte)dsp.getParamValue("srB_bit_out_1") << 1) | (byte)dsp.getParamValue("srB_bit_out_2")) + (byte)dsp.getParamValue("srB_bit_out_0")) % 4;

      for (int led = 0; led < 2; led++) {
        if (led_color[led] != prev_led_color[led]) {
          prev_led_color[led] = led_color[led];
          update_led(led, led_color[led]);
        }
      }
    }


    //
    // Key Read
    //
    if (an[0]) {

      an[0] = 0;
      keyChanged = true;

      for (int i = 0; i < 16; i++) {
        bool tmp = aw0.digitalRead(i);

        //      Serial.print(tmp);
        //      Serial.print(" ");

        if (i < 8) key[0][7 - i] = tmp;
        else if (i < 12 && i > 7) key[0][11 - i + 8] = tmp;
        else key[1][i - 12] = tmp;
      }

      //Serial.println();
    }

    if (an[1]) {

      an[1] = 0;
      keyChanged = true;

      for (int i = 0; i < 4; i++) {
        bool tmp = aw1.digitalRead(i);
        key[1][3 - i + 4] = tmp;
      }

      for (int i = 8; i < 12; i++) {
        bool tmp = aw1.digitalRead(i);
        key[1][11 - i + 8] = tmp;
      }

      modeButtonState[0] = aw1.digitalRead(4);
      modeButtonState[1] = aw1.digitalRead(7);
      mode_button_value = !modeButtonState[0] + (!modeButtonState[1] << 1);
    }

    // 用 state change 来触发动作，不在 ISR 中做动作，使连续的两次 false 只做一次动作，消弭 double trigger。

    //
    // Key Change Routine
    //
    if (keyChanged) {
      keyChanged = false;


      //
      // mode change
      //
      switch (mode_button_value) {
        case 0:
          saving = false;
          loading = false;
          if (change_keyboard_mode) {
            keyboard_mode = !keyboard_mode;
            if (keyboard_mode) update_led_octave();
            change_keyboard_mode = false;
            Serial.printf("keyboard_mode is %d\n", keyboard_mode);
          }

          if (octave_change != 0) {
            octave += octave_change;
            octave_change = 0;
            octave = max(octave, 0);
            octave = min(octave, 6);
            Serial.printf("octave = %d\n", octave);
            update_led_octave();
          }
          break;
        case 1:
          if (!keyboard_mode) {
            saving = true;
            loading = false;
          } else if (!change_keyboard_mode) octave_change = -1;
          break;
        case 2:
          if (!keyboard_mode) {
            saving = false;
            loading = true;
          } else if (!change_keyboard_mode) octave_change = 1;
          break;
        case 3:
          saving = false;
          loading = false;
          change_keyboard_mode = true;
          break;
      }


      //
      // note change
      //
      for (int ch = 0; ch < 2; ch++) {
        for (int i = 0; i < 12; i++) {
          bool tmp = key[ch][i];
          bitWrite(allKeys[ch], i, !tmp);

          if (key[ch][i] != keyPrev[ch][i]) {
            if (!key[ch][i]) {

              if (!saving && !loading) {
                if (!keyboard_mode) keyFlag[ch][i] = true;
                else {
                  int note = (octave + BASE_OCT) * 12 + i;
                  char buff[100];
                  if (!ch) {
                    if (!osc_current_poly) dsp.setParamValue("rateA", note);
                    else dsp.setParamValue("rateB", note);
                    if (osc_current_poly) rateA = (note + 100.) / 227.;
                    else rateB = (note + 100.) / 227.;
                    osc_current_poly = !osc_current_poly;
                  } else {
                    snprintf(buff, sizeof(buff), "peak%d", filter_current_poly + 1);
                    const std::string str = buff;
                    dsp.setParamValue(str, note);
                    if (!filter_current_poly) peak1 = (note + 20.) / 155.;
                    else peak2 = (note + 20.) / 155.;
                    filter_current_poly = !filter_current_poly;
                  }
                }
              }

              if (saving) {
                int slot = ch * 12 + i;
                save_load(slot, SAVE);
              }

              if (loading) {
                int slot = ch * 12 + i;
                save_load(slot, LOAD);
              }

              //              if (note_ratio_mode) {
              //                float ratio_note;
              //                switch (ch) {
              //                  case 0:
              //                    switch (i) {
              //                      case 0:
              //                        if (note_ratio[0] > 0) note_ratio[0] -= 1;
              //                        ratio_note = ratio_pitch(ratios[note_ratio[0]], base_note, 440.);
              //                        dsp.setParamValue("rateA", ratio_note * 227. - 100.);
              //                        break;
              //                      case 1:
              //                        if (note_ratio[0] < 29) note_ratio[0] += 1;
              //                        ratio_note = ratio_pitch(ratios[note_ratio[0]], base_note, 440.);
              //                        dsp.setParamValue("rateA", ratio_note * 227. - 100.);
              //                        break;
              //                    }
              //                    break;
              //
              //                  case 1:
              //                    switch (i) {
              //                      case 0:
              //                        if (note_ratio[1] > 0) note_ratio[1] -= 1;
              //                        ratio_note = ratio_pitch(ratios[note_ratio[1]], base_note, 440.);
              //                        dsp.setParamValue("rateB", ratio_note * 227. - 100.);
              //                        break;
              //                      case 1:
              //                        if (note_ratio[1] < 29) note_ratio[1] += 1;
              //                        ratio_note = ratio_pitch(ratios[note_ratio[1]], base_note, 440.);
              //                        dsp.setParamValue("rateB", ratio_note * 227. - 100.);
              //                        break;
              //                    }
              //                }
              //              }
              //
              //              if (peak_ratio_mode) {
              //                float ratio_note;
              //                switch (ch) {
              //                  case 0:
              //                    switch (i) {
              //                      case 10:
              //                        if (peak_ratio[0] > 0) peak_ratio[0] -= 1;
              //                        ratio_note = ratio_pitch(ratios[peak_ratio[0]], base_peak, 440.);
              //                        dsp.setParamValue("peak1", ratio_note * 155. - 20.);
              //                        break;
              //                      case 11:
              //                        if (peak_ratio[0] < 29) peak_ratio[0] += 1;
              //                        ratio_note = ratio_pitch(ratios[peak_ratio[0]], base_peak, 440.);
              //                        dsp.setParamValue("peak1", ratio_note * 155. - 20.);
              //                        break;
              //                    }
              //                    break;
              //
              //                  case 1:
              //                    switch (i) {
              //                      case 10:
              //                        if (peak_ratio[1] > 0) peak_ratio[1] -= 1;
              //                        ratio_note = ratio_pitch(ratios[peak_ratio[1]], base_peak, 440.);
              //                        dsp.setParamValue("peak2", ratio_note * 155. - 20.);
              //                        break;
              //                      case 11:
              //                        if (peak_ratio[1] < 29) peak_ratio[1] += 1;
              //                        ratio_note = ratio_pitch(ratios[peak_ratio[1]], base_peak, 440.);
              //                        dsp.setParamValue("peak2", ratio_note * 155. - 20.);
              //                        break;
              //                    }
              //                }
              //              }

            }       // Key Press Action End
            else {  // Key Release Action Start
              keyFlag[ch][i] = false;
            }  // Key Release Action End
          }
          keyPrev[ch][i] = key[ch][i];
        }
      }
    }

    //
    // keyboard control param
    //

    for (int ch = 0; ch < 2; ch++) {
      for (int i = 0; i < 12; i++) {

        if (keyFlag[ch][i]) {

          switch (ch) {
            case 0:
              switch (i) {
                case 0:
                  //if (!note_ratio_mode) {
                  if (rateA > 0) rateA -= param_speed;
                  else rateA = 0;
                  dsp.setParamValue("rateA", rate_to_semitone(rateA));
                  //}
                  break;
                case 1:
                  //if (!note_ratio_mode) {
                  if (rateA < 1) rateA += param_speed;
                  else rateA = 1;
                  dsp.setParamValue("rateA", rate_to_semitone(rateA));
                  //}
                  break;
                case 2:
                  if (r_to_rateA > 0) r_to_rateA -= param_speed;
                  dsp.setParamValue("r_to_rateA", r_to_rateA);
                  break;
                case 3:
                  if (r_to_rateA < 1) r_to_rateA += param_speed;
                  dsp.setParamValue("r_to_rateA", r_to_rateA);
                  break;
                case 4:
                  param_speed = SLOW;
                  break;
                case 5:
                  param_speed = FAST;
                  break;
                case 6:
                  if (sh_to_rateA < 1) sh_to_rateA += param_speed;
                  dsp.setParamValue("sh_to_rateA", sh_to_rateA);
                  break;
                case 7:
                  if (sh_to_rateA > 0) sh_to_rateA -= param_speed;
                  dsp.setParamValue("sh_to_rateA", sh_to_rateA);
                  break;
                case 8:
                  if (r_to_peak1 < 1) r_to_peak1 += param_speed;
                  dsp.setParamValue("r_to_peak1", r_to_peak1);
                  break;
                case 9:
                  if (r_to_peak1 > 0) r_to_peak1 -= param_speed;
                  dsp.setParamValue("r_to_peak1", r_to_peak1);
                  break;
                case 10:
                  //if (!peak_ratio_mode) {
                  if (peak1 < 1) peak1 += param_speed;
                  dsp.setParamValue("peak1", peak_to_semitone(peak1));
                  //}
                  break;
                case 11:
                  //if (!peak_ratio_mode) {
                  if (peak1 > 0) peak1 -= param_speed;
                  dsp.setParamValue("peak1", peak_to_semitone(peak1));
                  //}
                  break;
              }
              break;

            case 1:
              switch (i) {
                case 0:
                  if (rateB > 0) rateB -= param_speed;
                  else rateB = 0;
                  dsp.setParamValue("rateB", rate_to_semitone(rateB));
                  break;
                case 1:
                  if (rateB < 1) rateB += param_speed;
                  else rateB = 1;
                  dsp.setParamValue("rateB", rate_to_semitone(rateB));
                  break;
                case 2:
                  if (r_to_rateB > 0) r_to_rateB -= param_speed;
                  dsp.setParamValue("r_to_rateB", r_to_rateB);
                  break;
                case 3:
                  if (r_to_rateB < 1) r_to_rateB += param_speed;
                  dsp.setParamValue("r_to_rateB", r_to_rateB);
                  break;
                case 4:
                  if (sh_sp_peaks > 0) sh_sp_peaks -= param_speed;
                  dsp.setParamValue("sh_sp_peaks", sh_sp_peaks);
                  break;
                case 5:
                  if (sh_sp_peaks < 1) sh_sp_peaks += param_speed;
                  dsp.setParamValue("sh_sp_peaks", sh_sp_peaks);
                  break;
                case 6:
                  if (sh_to_rateB < 1) sh_to_rateB += param_speed;
                  dsp.setParamValue("sh_to_rateB", sh_to_rateB);
                  break;
                case 7:
                  if (sh_to_rateB > 0) sh_to_rateB -= param_speed;
                  dsp.setParamValue("sh_to_rateB", sh_to_rateB);
                  break;
                case 8:
                  if (r_to_peak2 < 1) r_to_peak2 += param_speed;
                  dsp.setParamValue("r_to_peak2", r_to_peak2);
                  break;
                case 9:
                  if (r_to_peak2 > 0) r_to_peak2 -= param_speed;
                  dsp.setParamValue("r_to_peak2", r_to_peak2);
                  break;
                case 10:
                  //if (!peak_ratio_mode) {
                  if (peak2 < 1) peak2 += param_speed;
                  dsp.setParamValue("peak2", peak_to_semitone(peak2));
                  //}
                  break;
                case 11:
                  //if (!peak_ratio_mode) {
                  if (peak2 > 0) peak2 -= param_speed;
                  dsp.setParamValue("peak2", peak_to_semitone(peak2));
                  //}
                  break;
              }
              break;
          }
        }
      }
    }

    //
    // end
    //
  }
}

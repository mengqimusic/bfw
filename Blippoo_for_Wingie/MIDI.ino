void handleNoteOn (byte channel, byte pitch, byte velocity) {
  if (velocity) {
    if (channel == midi_ch_l) {
      midi_set_pitch(0, poly_flip_rate, pitch);
      poly_flip_rate = !poly_flip_rate;
    }
    if (channel == midi_ch_r) {
      midi_set_pitch(1, poly_flip_peak, pitch);
      poly_flip_peak = !poly_flip_peak;
    }
    if (channel == midi_ch_both) {
      if (!poly_lipper) {
        midi_set_pitch(0, poly_flip_rate, pitch);
        poly_flip_rate = !poly_flip_rate;
        if (!poly_flip_rate) poly_lipper = !poly_lipper;
      }
      if (poly_lipper) {
        midi_set_pitch(1, poly_flip_peak, pitch);
        poly_flip_peak = !poly_flip_peak;
        if (!poly_flip_peak) poly_lipper = !poly_lipper;
      }
    }
  }
}

void midi_set_pitch(byte target, byte poly, int pitch) {
  if (!target) { // rate
    if (!poly) {
      rateA = semitone_to_rate(pitch);
      dsp.setParamValue("rateA", pitch);
    }
    else {
      rateB = semitone_to_rate(pitch);
      dsp.setParamValue("rateB", pitch);
    }
  }
  else { // peak
    if (!poly) {
      peak1 = semitone_to_rate(pitch);
      dsp.setParamValue("peak1", pitch);
    }
    else {
      peak2 = semitone_to_rate(pitch);
      dsp.setParamValue("peak2", pitch);
    }
  }
}

void handleControlChange (byte channel, byte number, byte value) {

  if (channel == midi_ch_l) midi_set_param(0, number, value);
  if (channel == midi_ch_r) midi_set_param(1, number, value);
  if (channel == midi_ch_both) midi_set_param(2, number, value);

  if (channel == 16) { // Global Settings

    if (number == CC_MIDI_CH_L) midi_ch_l = value; dirty[0] = true;
    if (number == CC_MIDI_CH_R) midi_ch_r = value; dirty[1] = true;
    if (number == CC_MIDI_CH_BOTH) midi_ch_both = value; dirty[2] = true;

    if (number == CC_A3_FREQ_MSB || number == CC_A3_FREQ_LSB) {

      if (number == CC_A3_FREQ_MSB) a3_freq_midi_value[MSB] = value;
      if (number == CC_A3_FREQ_LSB) a3_freq_midi_value[LSB] = value;

      if (number == CC_A3_FREQ_MSB) {
        int midi_value_14bit = (a3_freq_midi_value[MSB] << 7) | a3_freq_midi_value[LSB];
        float freq_offset = midi_value_14bit / 100. - 81.92;

        a3_freq = 440. + freq_offset;
        dsp.setParamValue("a3_freq", a3_freq);
        dirty[3] = true;
      }
    }
  }

}

void midi_set_param(int ch, byte number, byte value) {

  int index;

  if (number == CC_RATEA or number == CC_RATEA + 32) {
    index = 0;
    if (number == CC_RATEA) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_RATEA) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      rateA = v;
      dsp.setParamValue("rateA", rate_to_semitone(rateA));
    }
  }

  if (number == CC_RATEB or number == CC_RATEB + 32) {
    index = 1;
    if (number == CC_RATEB) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_RATEB) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      rateB = v;
      dsp.setParamValue("rateB", rate_to_semitone(rateB));
    }
  }

  if (number == CC_R_TO_RATEA or number == CC_R_TO_RATEA + 32) {
    index = 2;
    if (number == CC_R_TO_RATEA) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_R_TO_RATEA) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      r_to_rateA = v;
      dsp.setParamValue("r_to_rateA", r_to_rateA);
    }
  }

  if (number == CC_R_TO_RATEB or number == CC_R_TO_RATEB + 32) {
    index = 3;
    if (number == CC_R_TO_RATEB) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_R_TO_RATEB) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      r_to_rateB = v;
      dsp.setParamValue("r_to_rateB", r_to_rateB);
    }
  }

  if (number == CC_SH_TO_RATEA or number == CC_SH_TO_RATEA + 32) {
    index = 4;
    if (number == CC_SH_TO_RATEA) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_SH_TO_RATEA) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      sh_to_rateA = v;
      dsp.setParamValue("sh_to_rateA", sh_to_rateA);
    }
  }

  if (number == CC_SH_TO_RATEB or number == CC_SH_TO_RATEB + 32) {
    index = 5;
    if (number == CC_SH_TO_RATEB) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_SH_TO_RATEB) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      sh_to_rateB = v;
      dsp.setParamValue("sh_to_rateB", sh_to_rateB);
    }
  }

  if (number == CC_PEAK1 or number == CC_PEAK1 + 32) {
    index = 6;
    if (number == CC_PEAK1) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_PEAK1) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      peak1 = v;
      dsp.setParamValue("peak1", peak_to_semitone(peak1));
    }
  }

  if (number == CC_PEAK2 or number == CC_PEAK2 + 32) {
    index = 7;
    if (number == CC_PEAK2) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_PEAK2) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      peak2 = v;
      dsp.setParamValue("peak2", peak_to_semitone(peak2));
    }
  }

  if (number == CC_R_TO_PEAK1 or number == CC_R_TO_PEAK1 + 32) {
    index = 8;
    if (number == CC_R_TO_PEAK1) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_R_TO_PEAK1) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      r_to_peak1 = v;
      dsp.setParamValue("r_to_peak1", r_to_peak1);
    }
  }

  if (number == CC_R_TO_PEAK2 or number == CC_R_TO_PEAK2 + 32) {
    index = 9;
    if (number == CC_R_TO_PEAK2) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_R_TO_PEAK2) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      r_to_peak2 = v;
      dsp.setParamValue("r_to_peak2", r_to_peak2);
    }
  }

  if (number == CC_SH_SP_PEAKS or number == CC_SH_SP_PEAKS + 32) {
    index = 10;
    if (number == CC_SH_SP_PEAKS) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_SH_SP_PEAKS) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      sh_sp_peaks = v;
      dsp.setParamValue("sh_sp_peaks", sh_sp_peaks);
    }
  }

  if (number == CC_WET_DRY_MIX or number == CC_WET_DRY_MIX + 32) {
    index = 11;
    if (number == CC_WET_DRY_MIX) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_WET_DRY_MIX) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      Mix = v;
      dsp.setParamValue("mix", Mix);
      if (realtime_value_valid[MIX]) {
        pot_val_sampled[MIX] = pot_val_realtime[MIX];
        realtime_value_valid[MIX] = false;
      }
    }
  }

  if (number == CC_SH_SOURCE_MIX or number == CC_SH_SOURCE_MIX + 32) {
    index = 12;
    if (number == CC_SH_SOURCE_MIX) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_SH_SOURCE_MIX) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      sh_source_mix = v;
      dsp.setParamValue("sh_source_mix", sh_source_mix);
      if (realtime_value_valid[SH_SOURCE_MIX]) {
        pot_val_sampled[SH_SOURCE_MIX] = pot_val_realtime[SH_SOURCE_MIX];
        realtime_value_valid[SH_SOURCE_MIX] = false;
      }
    }
  }

  if (number == CC_VOLUME or number == CC_VOLUME + 32) {
    index = 13;
    if (number == CC_VOLUME) midi_value[index][MSB] = value;
    else midi_value[index][LSB] = value;

    if (number == CC_VOLUME) {
      int midiVal14Bit = (midi_value[index][MSB] << 7) | midi_value[index][LSB];
      float v = midiVal14Bit / 16383.;
      Volume = v;
      dsp.setParamValue("gain", Volume);
      if (realtime_value_valid[VOL]) {
        pot_val_sampled[VOL] = pot_val_realtime[VOL];
        realtime_value_valid[VOL] = false;
      }
    }
  }

  if (number == CC_R1_SOURCE) {
    int v = value / 43;
    dsp.setParamValue("source0", v);
    if (realtime_value_valid[R1_SOURCE]) {
      switch_val_sampled[0] = switch_val_realtime[0];
      realtime_value_valid[R1_SOURCE] = false;
    }
  }

  if (number == CC_R2_SOURCE) {
    int v = value / 43;
    dsp.setParamValue("source1", v);
    if (realtime_value_valid[R2_SOURCE]) {
      switch_val_sampled[1] = switch_val_realtime[1];
      realtime_value_valid[R2_SOURCE] = false;
    }
  }
}

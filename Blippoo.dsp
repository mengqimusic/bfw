declare name        "Blippoo";
declare version     "0.1";
declare author      "Meng Qi";
declare license     "BSD";
declare copyright   "(c)Meng Qi 2022";
declare date        "2022-09-15";

import("stdfaust.lib");


//   ▄▀▀█▄▄   ▄▀▀▀▀▄      ▄▀▀█▀▄    ▄▀▀▄▀▀▀▄  ▄▀▀▄▀▀▀▄  ▄▀▀▀▀▄   ▄▀▀▀▀▄  
//  ▐ ▄▀   █ █    █      █   █  █  █   █   █ █   █   █ █      █ █      █ 
//    █▄▄▄▀  ▐    █      ▐   █  ▐  ▐  █▀▀▀▀  ▐  █▀▀▀▀  █      █ █      █ 
//    █   █      █           █        █         █      ▀▄    ▄▀ ▀▄    ▄▀ 
//   ▄▀▄▄▄▀    ▄▀▄▄▄▄▄▄▀  ▄▀▀▀▀▀▄   ▄▀        ▄▀         ▀▀▀▀     ▀▀▀▀   
//  █    ▐     █         █       █ █         █                           
//  ▐          ▐         ▐       ▐ ▐         ▐                           
//   ▄▀▀▀█▄    ▄▀▀▀▀▄   ▄▀▀▄▀▀▀▄                                         
//  █  ▄▀  ▀▄ █      █ █   █   █                                         
//  ▐ █▄▄▄▄    █      █ ▐  █▀▀█▀                                          
//   █    ▐   ▀▄    ▄▀  ▄▀    █                                          
//   █          ▀▀▀▀   █     █                                           
//  █                  ▐     ▐                                           
//  ▐                                                                    
//   ▄▀▀▄    ▄▀▀▄  ▄▀▀█▀▄    ▄▀▀▄ ▀▄  ▄▀▀▀▀▄    ▄▀▀█▀▄    ▄▀▀█▄▄▄▄       
//  █   █    ▐  █ █   █  █  █  █ █ █ █         █   █  █  ▐  ▄▀   ▐       
//  ▐  █        █ ▐   █  ▐  ▐  █  ▀█ █    ▀▄▄  ▐   █  ▐    █▄▄▄▄▄        
//    █   ▄    █      █       █   █  █     █ █     █       █    ▌        
//     ▀▄▀ ▀▄ ▄▀   ▄▀▀▀▀▀▄  ▄▀   █   ▐▀▄▄▄▄▀ ▐  ▄▀▀▀▀▀▄   ▄▀▄▄▄▄         
//           ▀    █       █ █    ▐   ▐         █       █  █    ▐         
//                ▐       ▐ ▐                  ▐       ▐  ▐              


//-----------------------------------------------
// Parameters
//-----------------------------------------------

rateA = hslider("rateA", 25.31, -50, 127, 0.01); // bottom C ⬇ C# ⬆ / -10
rateB = hslider("rateB", -15.5, -50, 127, 0.01); // top C ⬇ C# ⬆ / -11.84

source0 = hslider("source0", 1, 0, 2, 1); // left octave toggle
source1 = hslider("source1", 1, 0, 2, 1); // right octave toggle

r_to_rateA = hslider("r_to_rateA", .5082, 0, 1, 0.001) : si.smoo; // bottom D ⬇ D# ⬆
r_to_rateB = hslider("r_to_rateB", .5791, 0, 1, 0.001) : si.smoo; // top D ⬇ D# ⬆

sh_to_rateA = hslider("sh_to_rateA", .2659, 0, 1, 0.001) : si.smoo; // bottom G ⬇ F# ⬆
sh_to_rateB = hslider("sh_to_rateB", .5791, 0, 1, 0.001) : si.smoo; // top G ⬇ F# ⬆

peak1 = hslider("peak1", 37.52, -20, 135, 0.01) : si.smoo; // bottom B ⬇ A# ⬆
peak2 = hslider("peak2", 72.8, -20, 135, 0.01) : si.smoo; // top B ⬇ A# ⬆

r_to_peak1 = hslider("r_to_peak1", .3723, 0, 1, 0.001) : si.smoo; // bottom A ⬇ G# ⬆
r_to_peak2 = hslider("r_to_peak2", .5732, 0, 1, 0.001) : si.smoo; // top A ⬇ G# ⬆

sh_sp_peaks = hslider("sh_sp_peaks", .2518, 0, 1, 0.001) : si.smoo; // top E ⬇ F ⬆

gain = hslider("gain", 1, 0, 1, 0.01) : si.smoo; // volume slider

Q = hslider("Q", 31, 1, 200, 0.1); // fixed

sh_source_mix = hslider("sh_source_mix", 0, 0, 1, 0.01); // decay slider

mix = hslider("mix", 1, 0, 1, 0.01) : si.smoo; // mix slider

amp_follower_decay = 0.;

mod_depth = hslider("mod_depth", 100, 10, 200, 1);

a3_freq = hslider("a3_freq", 440, 300, 600, 0.01);

// in normal mode, bottom E / F are used for selecting the speed of parameter ⬇ ⬆
// mode buttons for tap tempo of each osc, hold together then release for keyboard mode, in keyboard mode they are used for octave ⬇ ⬆
// bottom keyboard cycle between 2 osc, top keyboard cycles between 2 filter
// source toggle switch external input source

//-----------------------------------------------
// Functions
//-----------------------------------------------

mtof(note) = a3_freq * pow(2., (note - 69) / 12);

sr(in, cl, data_source) = // 一个带有 8 步循环，16 步正反相循环和持续不停输入模式的移位寄存器
(_ <: _, in, (_ * -1) : ba.selectn(3, data_source)
: ba.latch(cl''''''')
<: _, ba.latch(cl'''''')
<: _, _, !, ba.latch(cl''''')
<: _, _, _, !, !, ba.latch(cl'''')
<: _, _, _, _, !, !, !, ba.latch(cl''')
<: _, _, _, _, _, !, !, !, !, ba.latch(cl'')
<: _, _, _, _, _, _, !, !, !, !, !, ba.latch(cl')
<: _, _, _, _, _, _, _, !, !, !, !, !, !, ba.latch(cl)
: ro.cross(8)) ~*(1);

one_peak(cutoff, Q) = fi.tf2np(b0,b1,b2,a1,a2)
with {
K = tan(ma.PI * cutoff / ma.SR);
norm = 1 / (1 + K / Q + K * K);
b0 = K / Q * norm;
b1 = 0;
b2 = -b0;
a1 = 2 * (K * K - 1) * norm;
a2 = (1 - K / Q + K * K) * norm;
};

blippoo(in_l, in_r) =
    (
        (
            (
                (
                    (
                        (rateA + _ * r_to_rateA * mod_depth + _ * sh_to_rateA * mod_depth : mtof : ma.SR * 0.5, _ : min),
                        (rateB + _ * r_to_rateB * mod_depth + _ * sh_to_rateB * mod_depth : mtof : ma.SR * 0.5, _ : min),
                        _
                        : os.lf_triangle, os.lf_triangle, _
                        <: _ > 0, _ > 0, !, source0, (_ > 0, _ > 0 : ro.cross(2)), !, source1, _ > 0, (_ * (1 - sh_source_mix), _ * sh_source_mix :> _), (_ > _ : _), !
                        : sr, sr, ba.latch, _
                        : _, _, _, !, !, !, !, !, _, _, _, !, !, !, !, !, _, _ // last 3-bits from sr, last 3 bits from sr, SH, Comparator
                        : par(i, 3, (_ <: attach(_, _ : an.amp_follower(amp_follower_decay) : hbargraph("srA_bit_out_%i", 0, 1)))), 
                          par(i, 3, (_ <: attach(_, _ : an.amp_follower(amp_follower_decay) : hbargraph("srB_bit_out_%i", 0, 1)))),
                          _, _
                        : (_ * 0.572, _ * 0.286, _ * 0.143, _ * 0.572, _ * 0.286, _ * 0.143 : ro.interleave(3,2) :> _, _), (_ <: _, _, _), _
                        : _, _, _, _, _, _ // runglerA, runglerB, SH, SH, SH, Comparator
                        : _, ro.cross(2), _, _, _ // runglerA, SH, runglerB, SH, SH, Comparator
                    ) ~*(1) // runglerA recursed
                    : ro.cross(2), _, _, _, _ // SH, runglerA, runglerB, SH, SH, Comparator
                ) ~*(1) // SH recursed
                : !, _, _, _, _, _ // runglerA, runglerB, SH, SH, Comparator
                : ro.cross(2), _, _, _ // runglerB, runglerA, SH, SH, Comparator
            ) ~*(1) // runglerB recursed
            : ro.crossNM(2,1), _, _ // SH, runglerA, runglerB, SH, Comparator
        ) ~*(1) // SH recursed
        : !, _, _, _, _ // runglerA, runglerB, SH, Comparator
        <: _, _, !, !, _, _, _, _ // runglerA, runglerB, runglerA, runglerB, SH, Comparator
        : (_, _ :> _ * 0.5), _, _, _, _ // (runglerA + runglerB) * 0.5, runglerA, runglerB, SH, Comparator
    ) ~*(1) // (runglerA + runglerB) * 0.5 recursed
    : !, _, _, _, _ // runglerA, runglerB, SH, Comparator
: _, _, (_ <: _, _), (_ <: _, _) // runglerA, runglerB, SH, SH, Comparator, Comparator
: _, (_, _ : ro.cross(2)), _, _, _ // runglerA, SH, runglerB, SH, Comparator, Comparator
: _, _, ro.crossNM(2,1), _ // runglerA, SH, Comparator, runglerB, SH, Comparator
: (_ * r_to_peak1 * mod_depth + _ * -1 * sh_sp_peaks * mod_depth + peak1 : mtof : 10., _ : max : ma.SR / 2, _ : min), Q, (_ * mix, in_l * (1 - mix) :> _),
  (_ * r_to_peak2 * mod_depth + _ * 1 * sh_sp_peaks * mod_depth + peak2 : mtof : 10., _ : max : ma.SR / 2, _ : min), Q, (_ * mix, in_r * (1 - mix) :> _)
: one_peak, one_peak
: fi.dcblocker, fi.dcblocker
: _ * gain, _ * gain
: ef.cubicnl(0.8, 0), ef.cubicnl(0.8, 0)
;

//-----------------------------------------------
// Process
//-----------------------------------------------

process = _, _ : blippoo;

/* Thank you Rob!

                  _  /)
                 mo / )
                 |/)\)
                  /\_
                  \__|=
                 (    )
                 __)(__
           _____/      \\_____
          |                  ||
          |  _     ___   _   ||
          | | \     |   | \  ||
          | |  |    |   |  | ||
          | |_/     |   |_/  ||
          | | \     |   |    ||
          | |  \    |   |    ||
          | |   \. _|_. | .  ||
          |                  ||
  *       | *   **    * **   |**      **
   \)).\..//.,(//,,..,,\||(,,.,\\,.((/*/
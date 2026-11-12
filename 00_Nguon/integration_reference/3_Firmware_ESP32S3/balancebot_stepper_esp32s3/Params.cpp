#include "Params.h"
#include <Preferences.h>
#include "Log.h"

Params params;

Params::Params() { setDefaults(); }

void Params::setDefaults() {
  K[0] = DEF_K0; K[1] = DEF_K1; K[2] = DEF_K2;
  A41 = DEF_A41; B4 = DEF_B4; FrefGain = DEF_FREF_GAIN;
  posKp = POS_KP; posKi = POS_KI; posKd = POS_KD;
  zeropoint = 0.0f;
}

void Params::load() {
  Preferences p;
  p.begin("bb", true);
  // Bo qua gain da luu cho co khi cu; giu WiFi va zeropoint.
  if (p.getFloat("modelrev", 0.0f) == MODEL_REVISION) {
    K[0] = p.getFloat("k0", DEF_K0); K[1] = p.getFloat("k1", DEF_K1);
    K[2] = p.getFloat("k2", DEF_K2);
    A41  = p.getFloat("a41", DEF_A41); B4 = p.getFloat("b4", DEF_B4);
    FrefGain = p.getFloat("frg", DEF_FREF_GAIN);
    posKp = p.getFloat("pkp", POS_KP); posKi = p.getFloat("pki", POS_KI); posKd = p.getFloat("pkd", POS_KD);
  }
  zeropoint = p.getFloat("zp", 0.0f);
  p.end();
}

void Params::save() const {
  Preferences p;
  p.begin("bb", false);
  p.putFloat("modelrev", MODEL_REVISION);
  p.putFloat("k0", K[0]); p.putFloat("k1", K[1]);
  p.putFloat("k2", K[2]);
  p.putFloat("a41", A41); p.putFloat("b4", B4); p.putFloat("frg", FrefGain);
  p.putFloat("pkp", posKp); p.putFloat("pki", posKi); p.putFloat("pkd", posKd);
  p.putFloat("zp", zeropoint);
  p.end();
}

void Params::clearStored() {
  Preferences p;
  p.begin("bb", false);
  p.clear();
  p.end();
}

void Params::print() const {
  outf("LQR3 K=[%.4f, %.4f, %.4f] A41=%.4f B4=%.4f Fref=%.4f | PIDpos=[%.3f, %.3f, %.3f] | zp=%.3fdeg STEPS_PER_M=%.1f\n",
       K[0], K[1], K[2], A41, B4, FrefGain, posKp, posKi, posKd, zeropoint / DEG2RAD, STEPS_PER_M);
}

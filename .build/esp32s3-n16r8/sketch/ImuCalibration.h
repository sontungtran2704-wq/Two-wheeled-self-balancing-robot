#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot\\ImuCalibration.h"
#pragma once
#include <cmath>
#include <algorithm>
namespace imu {
// Engineering acceptance limits, not accelerometer scale/offset calibration.
// A stable magnitude alone cannot establish angle accuracy; verify orientation
// and perform multi-position accelerometer calibration before free running.
struct CalibrationWindow {
  unsigned n=0; bool finite=true;
  double amean[3]{},gmean[3]{},am2[3]{},gm2[3]{};
  double normMean=0,gyroPeak=0;
  void add(const double a[3],const double g[3]) {
    for(int j=0;j<3;++j)if(!std::isfinite(a[j])||!std::isfinite(g[j])){finite=false;return;}
    ++n;
    for(int j=0;j<3;++j){
      const double da=a[j]-amean[j],dg=g[j]-gmean[j];
      amean[j]+=da/n;gmean[j]+=dg/n;
      am2[j]+=da*(a[j]-amean[j]);gm2[j]+=dg*(g[j]-gmean[j]);
      gyroPeak=std::max(gyroPeak,std::abs(g[j]));
    }
    const double norm=std::sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    normMean+=(norm-normMean)/n;
  }
  double accelStd() const {return n>1?std::sqrt(std::max({0.,am2[0],am2[1],am2[2]})/(n-1)):0;}
  double gyroStd() const {return n>1?std::sqrt(std::max({0.,gm2[0],gm2[1],gm2[2]})/(n-1)):0;}
  unsigned result() const {
    if(!finite || n<500 || normMean<.90 || normMean>1.10)return 5;
    if(accelStd()>.02 || gyroStd()>.02 || gyroPeak>.15)return 6;
    return 1;
  }
};
}

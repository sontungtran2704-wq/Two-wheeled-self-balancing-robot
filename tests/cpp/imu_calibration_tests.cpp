#include "ImuCalibration.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
int main(){
  imu::CalibrationWindow stable;
  for(int i=0;i<500;++i){double a[]={0,0,1.05+(i%2?.002:-.002)},g[]={.01,-.015,.005};stable.add(a,g);}
  assert(stable.result()==1);
  assert(stable.gmean[1]<-.0149);
  imu::CalibrationWindow moving,bad,rotating;
  for(int i=0;i<500;++i){
    double a[]={i%2?.1:-.1,0,1},g[]={0,0,0};moving.add(a,g);
    a[0]=0;a[2]=2;bad.add(a,g);
    a[2]=1;g[0]=.3;rotating.add(a,g);
  }
  assert(moving.result()==6);assert(bad.result()==5);assert(rotating.result()==6);
  imu::CalibrationWindow incomplete;assert(incomplete.result()!=1);
  double a[]={0,0,std::numeric_limits<double>::quiet_NaN()},g[]={0,0,0};
  stable.add(a,g);assert(stable.result()!=1);
}

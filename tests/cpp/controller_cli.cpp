#include "ControlCore.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
int main(int argc,char** argv) {
  using namespace robot;
  if(argc==4 && std::string(argv[1])=="--vectors") {
    std::ifstream in(argv[2]);std::ofstream out(argv[3]);if(!in||!out)return 2;
    std::string line;std::getline(in,line);
    out<<"u,alpha,left,right,theta_ref,aref,vref,wref,saturated,motion_active,ub,um\n"<<std::setprecision(17);
    NumericController ctrl;double maxerr=0;size_t count=0;
    while(std::getline(in,line)) {
      std::stringstream s(line);std::string part;std::vector<double> v;
      while(std::getline(s,part,','))v.push_back(std::stod(part));if(v.size()!=20)return 3;
      const auto o=ctrl.tick(v[0],v[1],v[2],v[3],v[4],v[5],v[6]!=0,v[7]!=0);
      const double got[]={o.u,o.alpha,o.left,o.right,o.theta_ref,o.aref,o.vref,o.wref,double(o.saturated),double(o.motion_active),o.ub,o.um};
      for(size_t i=0;i<12;++i){if(i)out<<',';out<<got[i];maxerr=std::max(maxerr,std::abs(got[i]-v[i+8]));}out<<'\n';++count;
    }
    std::cout<<"vectors="<<count<<" max_abs_error="<<std::setprecision(17)<<maxerr<<'\n';return maxerr<1e-5?0:4;
  }
  if(argc==2 && std::string(argv[1])=="--device") {
    Device dev;uint32_t now=1000;Sensors s{0,0,0,0,now,true};dev.tick(now,s);
    std::cout<<dev.telemetry(now)<<std::endl;
    std::string line;
    while(std::getline(std::cin,line)) {
      if(!line.empty()&&line.back()=='\r')line.pop_back();
      if(line.rfind("STEP,",0)==0) {
        char* end;const long n=std::strtol(line.c_str()+5,&end,10);
        if(*end||n<0||n>200000){std::cout<<"ERR:STEP"<<std::endl;continue;}
        for(long i=0;i<n;++i){now+=5000;s.timestamp_us=now;s.v=(dev.output().left+dev.output().right)/2;dev.tick(now,s);}
        std::cout<<dev.telemetry(now)<<std::endl;
      } else if(line.rfind("SENSOR,",0)==0) {
        std::stringstream fields(line.substr(7));std::string p;std::vector<double> v;
        try {while(std::getline(fields,p,','))v.push_back(std::stod(p));}catch(...){v.clear();}
        if(v.size()!=4){std::cout<<"ERR:SENSOR"<<std::endl;continue;}
        s.theta=v[0];s.omega=v[1];s.v=v[2];s.yaw=v[3];std::cout<<"ACK:SENSOR"<<std::endl;
      } else std::cout<<dev.command(line.c_str(),now)<<std::endl;
    }
    return 0;
  }
  std::cerr<<"Usage: controller_cli --vectors input.csv output.csv | --device\n";return 1;
}

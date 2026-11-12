#include "ControlCore.h"
#include <cassert>
#include <iostream>
int main(){robot::Device d;unsigned t=1000;robot::Sensors s{0,0,0,0,t,true};d.tick(t,s);assert(d.command("START",t)=="ACK:START");for(int i=0;i<200;i++){t+=5000;s.timestamp_us=t;d.tick(t,s);}assert(d.enabled());assert(!d.output().motion_active);for(int i=0;i<201;i++){t+=5000;s.timestamp_us=t;d.tick(t,s);}assert(d.output().motion_active);assert(d.output().vref==0 && d.output().wref==0);d.command("STOP",t);assert(!d.enabled() && !d.output().motion_active);std::cout<<"PASS: automatic zero velocity hold after stability, zero targets, STOP disables\n";}

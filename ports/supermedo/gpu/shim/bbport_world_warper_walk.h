#pragma once
#include "bbport_world_warper_movement.h"
#include "bbport_world_warper_boss_scripts.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace BbWorldWarperEncounter {
inline float Distance(Point a,Point b) { return std::hypot(a.x-b.x,a.z-b.z); }
inline bool Finite(Point p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
// Closed-loop walking. Measure the camera-relative stick basis from real motion
// with a fixed camera. No world position writes, random probes or obstacle hops.
class Walker {
    bool begun=false,basis=false;
    Point sample{};
    double sample_time=0,last_time=0,last_progress=0,integral_x=0,integral_y=0;
    float fx=0,fz=0,rx=0,rz=0,sx=0,sy=0,closest=0;
    unsigned probe=0,probe_attempt=0;
    float probe_sign=1;
    using Input=BbWorldWarperMovement::Input;
    Input Emit(float x,float y) {
        const int lx=std::clamp(128+static_cast<int>(x*96),0,255),ly=std::clamp(128-static_cast<int>(y*96),0,255);
        sx=(lx-128)/96.f; sy=(128-ly)/96.f;
        return {lx,ly,128,128,0};
    }
    void SampleAt(double now,Point p) { sample=p; sample_time=now; integral_x=integral_y=0; }
public:
    bool stalled=false;
    void Reset() { *this=Walker{}; }
    bool Calibrated() const { return basis; }
    void PermitContact(double now) { stalled=false; last_progress=now; }
    BbWorldWarperMovement::Input Steer(Point p,Point goal) const {
        if(!basis || !Finite(p)||!Finite(goal)||Distance(p,goal)<0.01f) return {128,128,128,128,0};
        const float dx=goal.x-p.x,dz=goal.z-p.z,n=std::hypot(dx,dz);
        const float determinant=rx*fz-rz*fx;
        float x=(fz*dx-fx*dz)/(n*determinant),y=(-rz*dx+rx*dz)/(n*determinant);
        const float scale=std::max(1.f,std::hypot(x,y)); x/=scale; y/=scale;
        return {std::clamp(128+static_cast<int>(x*96),0,255),std::clamp(128-static_cast<int>(y*96),0,255),128,128,0};
    }
    BbWorldWarperMovement::Input Toward(double now,Point p,Point goal,float radius=1.f) {
        const Input neutral{128,128,128,128,0};
        if(stalled || !Finite(p)||!Finite(goal)||!std::isfinite(now)) { stalled=true; return neutral; }
        if(Distance(p,goal)<=radius) return neutral;
        if(!begun) { begun=true; SampleAt(now,p); last_time=last_progress=now; closest=Distance(p,goal); }
        const double dt=now-last_time; last_time=now;
        if(dt>0 && dt<0.5) { integral_x+=sx*dt; integral_y+=sy*dt; }
        if(Distance(p,goal)<closest-0.08f) { closest=Distance(p,goal); last_progress=now; }
        if(now-sample_time>=0.35) {
            const float dx=p.x-sample.x,dz=p.z-sample.z,travel=std::hypot(dx,dz);
            const bool moved=travel>=0.06f && travel<3.f && std::fabs(p.y-sample.y)<1.5f;
            if(!basis) {
                const double control=probe==0?integral_y:integral_x;
                if(moved && std::fabs(control)>0.03) {
                    const float sign=control>0?1.f:-1.f;
                    if(probe==0) { fx=dx/travel*sign; fz=dz/travel*sign; }
                    else { rx=dx/travel*sign; rz=dz/travel*sign; }
                    ++probe; probe_attempt=0; probe_sign=1;
                    if(probe==2) {
                        const float determinant=rx*fz-rz*fx;
                        if(std::fabs(determinant)<0.3f) { stalled=true; return neutral; }
                        basis=true; closest=Distance(p,goal); last_progress=now;
                        std::printf("[warper] steering-basis forward=%.3f,%.3f right=%.3f,%.3f determinant=%.3f independently-measured\n",fx,fz,rx,rz,determinant);
                    }
                } else {
                    // A tiny reverse probe can calibrate beside a wall. Never
                    // invent the perpendicular axis, or keep circling to learn it.
                    probe_sign=-probe_sign;
                    if(++probe_attempt>=2) { stalled=true; return neutral; }
                }
            } else if(moved) {
                const double px=rx*integral_x+fx*integral_y,pz=rz*integral_x+fz*integral_y;
                if(std::hypot(px,pz)>0.03) {
                    // Track follow-camera rotation from the actual integrated
                    // controls, not just the last stick value in this interval.
                    const float error=std::clamp(static_cast<float>(std::atan2(px*dz-pz*dx,px*dx+pz*dz)),-0.25f,0.25f);
                    const float c=std::cos(error),s=std::sin(error),old_fx=fx,old_rx=rx;
                    fx=c*fx-s*fz; fz=s*old_fx+c*fz; rx=c*rx-s*rz; rz=s*old_rx+c*rz;
                }
            }
            SampleAt(now,p);
        }
        if(now-last_progress>5) { stalled=true; return neutral; }
        if(!basis) return probe==0?Emit(0,probe_sign*0.6f):Emit(probe_sign*0.6f,0);
        const auto result=Steer(p,goal); sx=(result.lx-128)/96.f; sy=(128-result.ly)/96.f;
        return result;
    }
};
// A fall ends this stop; no artificial rescue floor or boss-relative teleport.
class FloorGuard {
    Point origin{},last{};
    float minimum=0,maximum=0;
    double at=0,fast_fall=0;
    bool begun=false;
public:
    void Begin(Point p,const Script& script) {
        *this=FloorGuard{}; origin=last=p;
        minimum=std::min(p.y,script.region?script.entry.y:p.y)-4;
        maximum=std::max(p.y,script.region?script.entry.y:p.y)+10;
    }
    void AfterCutscene() { begun=false; fast_fall=0; }
    bool Check(double now,Point p) {
        if(!Finite(p)||p.y<minimum||p.y>maximum||Distance(p,origin)>180) return false;
        const double dt=now-at;
        if(begun && dt>0 && dt<0.5) {
            if((p.y-last.y)/dt < -12) fast_fall+=dt; else fast_fall=0;
            if(fast_fall>0.25) return false;
        }
        last=p; at=now; begun=true; return true;
    }
};
}

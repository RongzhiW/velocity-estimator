#ifndef ARUCOTRACKER_H
#define ARUCOTRACKER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include "aruco/aruco.h"

struct ArucoTrackData{
    float tx;
    float ty;
    float tz;
    float qx;
    float qy;
    float qz;
    float qw;
    float roll;
    float pitch;
    float yaw;
    ArucoTrackData(){
        tx=0;
        ty=0;
        tz=0;
        qx=0;
        qy=0;
        qz=0;
        qw=1;
        roll=0;
        pitch=0;
        yaw=0;
    }
};

class ArucoTracker
{
public:
    ArucoTracker(ros::NodeHandle *n){
        nh=*n;
        setup();
    }

public:
    ArucoTrackData arucoTrackData;

private:
   string inputVideoPath;
   string inputArucoMapPath;
   string cameraParamPath;
   ros::NodeHandle nh;
   ros::Subscriber imgSub;

public:
   void arucoTrackFunc(cv::Mat imgFrame,std::map<int,cv::Mat>& frame_pose_map);
   void  getQuaternionAndTranslationfromMatrix44(const cv::Mat &M_in ,float &qx,float &qy,float &qz,float &qw,float &tx,float &ty,float &tz);
   float SIGN(float x) {return (x >= 0.0f) ? +1.0f : -1.0f;}
   float NORM(float a, float b, float c, float d) {return sqrt(a * a + b * b + c * c + d * d);}
   void qtoEuler(float& _roll,float& _pitch,float& _yaw,float q[]);
private:
    void setup();
    void imgCb(const sensor_msgs::Image &msg);

};

#endif // ARUCOTRACKER_H

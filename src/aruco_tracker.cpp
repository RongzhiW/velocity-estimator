#include "aruco_tracker.h"

using namespace cv;
using namespace aruco;

string TheMarkerMapConfigFile;
bool The3DInfoAvailable = false;
float TheMarkerSize = 0.06;
VideoCapture TheVideoCapturer;
Mat TheInputImage,TheInputImageCopy;
CameraParameters TheCameraParameters;
MarkerMap TheMarkerMapConfig;
MarkerDetector TheMarkerDetector;
MarkerMapPoseTracker TheMSPoseTracker;
void cvTackBarEvents(int pos, void *);
 double ThresParam1, ThresParam2;
int iThresParam1, iThresParam2;
int waitTime = 10;
std::map<int,cv::Mat> frame_pose_map;//set of poses and the frames they were detected
char key;
int frameIndex;
void processKey(char k) {
    switch (k) {
    case 's':
        if (waitTime == 0)
            waitTime = 10;
        else
            waitTime = 0;
        break;

    }
}



void ArucoTracker::setup(){
    inputVideoPath="/home/tianu/catkin_ws/src/aruco-2.0.19/build/utils_markermap/test_mapper.avi";
    inputArucoMapPath="/home/tianu/catkin_ws/src/aruco-2.0.19/build/utils_markermap/deskMap.yml";
    cameraParamPath="/home/tianu/catkin_ws/src/aruco-2.0.19/build/utils_markermap/logitech_camera_calibration.yml";

    const char* imgTopic="/sensor_msg/image";
    imgSub=nh.subscribe(imgTopic,1,&ArucoTracker::imgCb,this);

    TheMarkerMapConfig.readFromFile(inputArucoMapPath);
    TheMarkerMapConfigFile=inputArucoMapPath;
    TheCameraParameters.readFromXMLFile(cameraParamPath);
    string dic=TheMarkerMapConfig.getDictionary();
    if(dic.empty())dic="ARUCO";
    TheMarkerDetector.setDictionary(dic);
    TheMarkerDetector.setCornerRefinementMethod(MarkerDetector::LINES);
//    MarkerDetector::Params params=TheMarkerDetector.getParams();
//    params._cornerMethod=MarkerDetector::SUBPIX;
//    params._subpix_wsize= (15./2000.)*float(TheInputImage.cols) ;//search corner subpix in a 5x5 widow area
//    TheMarkerDetector.setParams(params);
    if ( TheMarkerMapConfig.isExpressedInPixels() && TheMarkerSize>0)
                TheMarkerMapConfig=TheMarkerMapConfig.convertToMeters(TheMarkerSize);
    cout<<"TheCameraParameters.isValid()="<<TheCameraParameters.isValid()<<" "<<TheMarkerMapConfig.isExpressedInMeters()<<endl;
    if (TheCameraParameters.isValid() && TheMarkerMapConfig.isExpressedInMeters()  )
                TheMSPoseTracker.setParams(TheCameraParameters,TheMarkerMapConfig);
    TheMarkerDetector.getThresholdParams(ThresParam1, ThresParam2);
    iThresParam1 = ThresParam1;
    iThresParam2 = ThresParam2;
    cv::createTrackbar("ThresParam1", "in", &iThresParam1, 13, cvTackBarEvents);
    cv::createTrackbar("ThresParam2", "in", &iThresParam2, 13, cvTackBarEvents);

    key=0;
    frameIndex=0;
    cout<<"Press 's' to start/stop video"<<endl;

}

void ArucoTracker::imgCb(const sensor_msgs::Image& msg){
    cout<<"recieved image"<<endl;
}

void ArucoTracker::arucoTrackFunc(cv::Mat imgFrame, std::map<int,cv::Mat>& frame_pose_map){
    imgFrame.copyTo(TheInputImage);
    imgFrame.copyTo(TheInputImageCopy);
    TheCameraParameters.resize(TheInputImage.size());
    frameIndex++;
    vector<aruco::Marker> detectedMarkers=TheMarkerDetector.detect(TheInputImage);
    for(auto idx:TheMarkerMapConfig.getIndices(detectedMarkers))
        detectedMarkers[idx].draw(TheInputImageCopy,Scalar(0,0,255),2);
    if(TheMSPoseTracker.isValid()){
        if(TheMSPoseTracker.estimatePose(detectedMarkers)){
            aruco::CvDrawingUtils::draw3dAxis(TheInputImageCopy,TheCameraParameters,TheMSPoseTracker.getRvec(),TheMSPoseTracker.getTvec(),TheMarkerMapConfig[0].getMarkerSize()*2);
            frame_pose_map.insert(make_pair(frameIndex,TheMSPoseTracker.getRTMatrix()));
            cv::Mat T=TheMSPoseTracker.getRTMatrix();
            if(!T.empty()){
                getQuaternionAndTranslationfromMatrix44(TheMSPoseTracker.getRTMatrix(),arucoTrackData.qx,arucoTrackData.qy,arucoTrackData.qz,arucoTrackData.qw,arucoTrackData.tx,arucoTrackData.ty,arucoTrackData.tz);
                float quaternion[4];
                quaternion[0]=arucoTrackData.qw;
                quaternion[1]=arucoTrackData.qx;
                quaternion[2]=arucoTrackData.qy;
                quaternion[3]=arucoTrackData.qz;
                qtoEuler(arucoTrackData.roll,arucoTrackData.pitch,arucoTrackData.yaw,quaternion);

            }
            else
                cout<<"T is empty!"<<endl;
            cout<<"T:\n"<<TheMSPoseTracker.getRTMatrix()<<endl;
        }
    }
    key=cv::waitKey(waitTime);
    processKey(key);
    cv::imshow("in",TheInputImageCopy);
    cv::imshow("thres",TheMarkerDetector.getThresholdedImage());

}

void ArucoTracker::qtoEuler(float& _roll,float& _pitch,float& _yaw,float q[]){
    float_t w,x,y,z;
    //diffrent coordinates
    w=q[0];
    x=q[1];
    y=q[2];
    z=q[3];
    //cout<<"q:"<<w<<"    "<<x<<"    "<<y<<"    "<<z<<endl;
    _roll  = atan2(2 * (w * x + y* z) , 1 - 2 * (x * x + y * y));
    if(2 * (w * y - z * x)>1)_pitch =asin(1.0) ;
    else if(2 * (w * y - z * x)<-1.0)_pitch = asin(-1.0);
    else _pitch = asin(2 * (w * y - z * x));
    _yaw   = atan2(2 * (w * z + x * y) , 1 - 2 * (y * y + z * z));
}

void  ArucoTracker::getQuaternionAndTranslationfromMatrix44(const cv::Mat &M_in ,float &qx,float &qy,float &qz,float &qw,float &tx,float &ty,float &tz){
    //get the 3d part of matrix and get quaternion
    assert(M_in.total()==16);
    cv::Mat M;M_in.convertTo(M,CV_64F);
     //use now eigen
    float r11=M.at<double>(0,0);
    float r12=M.at<double>(0,1);
    float r13=M.at<double>(0,2);
    float r21=M.at<double>(1,0);
    float r22=M.at<double>(1,1);
    float r23=M.at<double>(1,2);
    float r31=M.at<double>(2,0);
    float r32=M.at<double>(2,1);
    float r33=M.at<double>(2,2);



    double  q0 = ( r11 + r22 + r33 + 1.0f) / 4.0f;
    double  q1 = ( r11 - r22 - r33 + 1.0f) / 4.0f;
    double     q2 = (-r11 + r22 - r33 + 1.0f) / 4.0f;
    double     q3 = (-r11 - r22 + r33 + 1.0f) / 4.0f;
    if(q0 < 0.0f) q0 = 0.0f;
    if(q1 < 0.0f) q1 = 0.0f;
    if(q2 < 0.0f) q2 = 0.0f;
    if(q3 < 0.0f) q3 = 0.0f;
    q0 = sqrt(q0);
    q1 = sqrt(q1);
    q2 = sqrt(q2);
    q3 = sqrt(q3);
    if(q0 >= q1 && q0 >= q2 && q0 >= q3) {
        q0 *= +1.0f;
        q1 *= SIGN(r32 - r23);
        q2 *= SIGN(r13 - r31);
        q3 *= SIGN(r21 - r12);
    } else if(q1 >= q0 && q1 >= q2 && q1 >= q3) {
        q0 *= SIGN(r32 - r23);
        q1 *= +1.0f;
        q2 *= SIGN(r21 + r12);
        q3 *= SIGN(r13 + r31);
    } else if(q2 >= q0 && q2 >= q1 && q2 >= q3) {
        q0 *= SIGN(r13 - r31);
        q1 *= SIGN(r21 + r12);
        q2 *= +1.0f;
        q3 *= SIGN(r32 + r23);
    } else if(q3 >= q0 && q3 >= q1 && q3 >= q2) {
        q0 *= SIGN(r21 - r12);
        q1 *= SIGN(r31 + r13);
        q2 *= SIGN(r32 + r23);
        q3 *= +1.0f;
    } else {
        cerr<<"Coding error"<<endl;
    }
    double r = NORM(q0, q1, q2, q3);
    qx =q0/ r;
    qy =q1/ r;
    qz =q2/ r;
    qw =q3/ r;



    tx=M.at<double>(0,3);
    ty=M.at<double>(1,3);
    tz=M.at<double>(2,3);


}

void cvTackBarEvents(int pos, void *) {
    (void)(pos);
    if (iThresParam1 < 3)
        iThresParam1 = 3;
    if (iThresParam1 % 2 != 1)
        iThresParam1++;
    if (ThresParam2 < 1)
        ThresParam2 = 1;
    ThresParam1 = iThresParam1;
    ThresParam2 = iThresParam2;
    TheMarkerDetector.setThresholdParams(ThresParam1, ThresParam2);

   //detect, print, get pose, and print

   //detect
    vector<aruco::Marker> detected_markers=TheMarkerDetector.detect(TheInputImage);
    //print the markers detected that belongs to the markerset
     for(auto idx:TheMarkerMapConfig.getIndices(detected_markers))
         detected_markers[idx].draw(TheInputImageCopy, Scalar(0, 0, 255), 2);
     //detect 3d info if possible
     if (TheMSPoseTracker.isValid()){
         TheMSPoseTracker.estimatePose(detected_markers);
         aruco::CvDrawingUtils::draw3dAxis(TheInputImageCopy,  TheCameraParameters,TheMSPoseTracker.getRvec(),TheMSPoseTracker.getTvec(),TheMarkerMapConfig[0].getMarkerSize()*2);
    }


    cv::imshow("in", TheInputImageCopy);
    cv::imshow("thres",TheMarkerDetector.getThresholdedImage());
}



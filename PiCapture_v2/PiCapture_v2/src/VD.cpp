//#include "PiCapture.h"
//#include <opencv2/opencv.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <dirent.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include "PeopleDetector.h"
//
//#define WIN_NAME "PiCapture"
//
//#define IMAGE_DIR  "./"		 // directory where the images will be stored
//#define DIR_FORMAT  "%d%h%Y"	 // 1Jan1970
//#define FILE_FORMAT "%d%h%Y_%H%M%S"
//#define EXT ".jpg"  //extension
//using namespace cv;
//using namespace std;
//
//
//PiCapture cap;
//
//
//int brightness,effect;
//
//void onBrightness( int, void* ){
//    cap.setBrightness(brightness);
//}
//
//void onEffect(int, void* ){
//    cap.setImageFX((MMAL_PARAM_IMAGEFX_T)effect);
//}
//
//
//// Check if the directory exists, if not create it
//// This function will create a new directory if the image is the first
//// image taken for a specific day
//inline void directoryExistsOrCreate(const char* pzPath)
//{
//    DIR *pDir;
//    // directory doesn't exists -> create it
//    if ( pzPath == NULL || (pDir = opendir (pzPath)) == NULL)
//        mkdir(pzPath, 0777);
//    // if directory exists we opened it and we
//    // have to close the directory again.
//    else if(pDir != NULL)
//        (void) closedir (pDir);
//}
//
//// When motion is detected we write the image to disk
////    - Check if the directory exists where the image will be stored.
////    - Build the directory and image names.
//int incr = 0;
//inline bool saveImg(Mat _image, const string _directory, const string _extension, const char * _dir_format, const char * _file_format)
//{
//    stringstream ss;
//    time_t seconds;
//    struct tm * timeinfo;
//    char TIME[80];
//    time (&seconds);
//    // Get the current time
//    timeinfo = localtime (&seconds);
//
//    // Create name for the date directory
//    strftime (TIME, 80, _dir_format, timeinfo);
//    ss.str("");
//    ss << _directory << TIME;
//    directoryExistsOrCreate(ss.str().c_str());
//    ss << "/cropped";
//    directoryExistsOrCreate(ss.str().c_str());
//
//    // Create name for the image
//    strftime (TIME,80,_file_format,timeinfo);
//    ss.str("");
//    if(incr < 200) incr++; // quick fix for when delay < 1s && > 10ms, (when delay <= 10ms, images are overwritten)
//    else incr = 0;
//    ss << _directory << TIME << "_" << static_cast<int>(incr) << _extension;
//    return imwrite(ss.str().c_str(), _image);
//}
//
////link https://blog.cedric.ws/raspberry-pi-opencv-create-a-security-system-web-interface
//
//
//int mainaaa(int argc,char **argv) {
//
//    PeopleDetector people_detect;
//    string face_path = "./data/haarcascade_frontalface_alt.xml";
//    string body_path ="./case.xml";
//    string path = "./Image";
//
//    string _img_dir = IMAGE_DIR;
//    string _dir_format = DIR_FORMAT;
//    string _file_format = string(DIR_FORMAT) + "/" + string(FILE_FORMAT);
//    string _ext = EXT;
//
//
//
//    //fn_haar = "/home/pi/training/training_data16.xml";   // Load face model
//
//    people_detect.loadFaceCascadeName(face_path);
//    people_detect.loadBodyCascadeName(body_path);
//
//    // myfile.open("./3_12_2016/Information.txt"); 
//
//
//    bool isColor = argc >= 2;
//    namedWindow(WIN_NAME);
//    cap.open(640, 480, isColor ? true : false);  // (640,480), (320,240)
//    ///Mat im = imread("image_00000011.jpg",0);
//    Mat im;
//    double time = 0;
//    unsigned int frames = 0;
//    cap.grab();
//    waitKey(500);
//    while(char(waitKey(1)) != 'q') {
//        double t0 = getTickCount();
//        im = cap.grab();
//        Mat frame;
//        cvtColor(im,frame,CV_RGB2YCrCb);
//        ///cvtColor(im,im,CV_YUV2GRAY_I420);
//
//        //vector< Rect_ <int> > bodies = people_detect.detectBody(im);
//        //Mat  tmp2 = people_detect.draw(tmp1,bodies, 1);
//
//
//
//        imshow(WIN_NAME, frame);
//
//        time = (getTickCount() - t0)*1000 / getTickFrequency();
//        cout << "Thoi gian xu ly:" <<time << " ms" << endl;
//    }
//        return 0;
//}
//
//

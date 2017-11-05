#include "PiCapture.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include "PeopleDetector.h"
#include "../face_rec/include/facerec.hpp"
#define WIN_NAME "PiCapture"

#define IMAGE_DIR  "./pics/body/"		 // directory where the images will be stored
#define DIR_FORMAT  "%d%h%Y"	 // 1Jan1970
#define FILE_FORMAT "%d%h%Y_%H%M%S"
#define EXT ".jpg"  //extension
#define THUC_NGHIEM 1

// for debug and trace
#define TRACE 1
#define DEBUG_MODE 0
#define DEBUG if (DEBUG_MODE==1)

using namespace cv;
using namespace std;


Eigenfaces model;
PiCapture cap;
volatile int flag = 0;

// some constants to manage nb of people to learn+ id of people 
#define MAX_PEOPLE 		4
#define P_BINH			0
#define P_HIEN			1
#define P_HIEU			2
#define P_PHU			3

// name of people
string  people[MAX_PEOPLE];


// nb of picture learnt by people
int nPictureById[MAX_PEOPLE];
///////////////////////

vector<Mat> images;
vector<int> labels;
Mat gray,frame,face,face_resized;
int PREDICTION_SEUIL = 4500;


// Check if the directory exists, if not create it
// This function will create a new directory if the image is the first
// image taken for a specific day
inline void directoryExistsOrCreate(const char* pzPath)
{
    DIR *pDir;
    // directory doesn't exists -> create it
    if ( pzPath == NULL || (pDir = opendir (pzPath)) == NULL)
        mkdir(pzPath, 0777);
    // if directory exists we opened it and we
    // have to close the directory again.
    else if(pDir != NULL)
        (void) closedir (pDir);
}

// When motion is detected we write the image to disk
//    - Check if the directory exists where the image will be stored.
//    - Build the directory and image names.
int incr = 0;
inline bool saveImg(Mat _image, const string _directory, const string _extension, const char * _dir_format, const char * _file_format)
{
    stringstream ss;
    time_t seconds;
    struct tm * timeinfo;
    char TIME[80];
    time (&seconds);
    // Get the current time
    timeinfo = localtime (&seconds);
    
    // Create name for the date directory
    strftime (TIME, 80, _dir_format, timeinfo);
    ss.str("");
    ss << _directory << TIME;
    directoryExistsOrCreate(ss.str().c_str());
    ss << "/cropped";
    directoryExistsOrCreate(ss.str().c_str());

    // Create name for the image
    strftime (TIME,80,_file_format,timeinfo);
    ss.str("");
    if(incr < 200) incr++; // quick fix for when delay < 1s && > 10ms, (when delay <= 10ms, images are overwritten)
    else incr = 0;
    ss << _directory << TIME << "_" << static_cast<int>(incr) << _extension;
    return imwrite(ss.str().c_str(), _image);
}

////////////////////////////////////////
/////////////////////////////////////////////////
// trace if TRACE==1
/////////////////////////////////////////////////
void trace(string s)
{
	if (TRACE==1)
	{
		cout<<s<<"\n";
	}
}

//link https://blog.cedric.ws/raspberry-pi-opencv-create-a-security-system-web-interface

//
// read csv files. 
// Fully copied from Philipp Wagner works
// http://docs.opencv.org/trunk/modules/contrib/doc/facerec/tutorial/facerec_video_recognition.html
// 
////////////////////////////////////////////////
static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
    std::ifstream file(filename.c_str(), ifstream::in);
    if (!file) {
        string error_message = "(E) No valid input file was given, please check the given filename.";
        CV_Error(CV_StsBadArg, error_message);
    }
    string line, path, classlabel;
    int nLine=0;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel);
        if(!path.empty() && !classlabel.empty()) 
        {
        	// read the file and build the picture collection
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
            nPictureById[atoi(classlabel.c_str())]++;
        	nLine++;
        }
    }
    
    // write number of picture by people
    // did you notice? I'm not familiar with string classe :-)
    // I prefer my old good char* ...
    // (what a pity)
	char sTmp[128];
    sprintf(sTmp,"(init) %d pictures read to train",nLine);
    trace((string)(sTmp));
	for (int j=0;j<MAX_PEOPLE;j++)
	{
		sprintf(sTmp,"(init) %d pictures of %s (%d) read to train",nPictureById[j],people[j].c_str(),j);
   	 	trace((string)(sTmp));
	}
}


Mat recog_Face(const Mat &_im, vector<Rect>faces)
{
    Mat im = _im;
    for(int i = 0; i < faces.size(); i++)
	{
	    // crop face (pretty easy with opencv, don't you think ? 
	    Rect face_i = faces[i];  
	   
	    
	    face = im(face_i);  
	    //  resized face and display it
	    cv::resize(face, face_resized, Size(100, 100), 1.0, 1.0, CV_INTER_NN); //INTER_CUBIC);
	    
	    // now, we try to predict who is it ? 
	    char sTmp[256];		
	    double predicted_confidence	= 0.0;
	    int prediction = -1;
	    model.predict(face_resized,prediction,predicted_confidence);
		
	    // create a rectangle around the face      
	    
	    rectangle(im, face_i, CV_RGB(255, 255 ,255), 1);
	    
	    if (predicted_confidence>PREDICTION_SEUIL)
		{
	 	// display name of the guy on the picture
		string box_text;
		if (prediction<MAX_PEOPLE)
		{
			box_text = "Id=" + people[prediction];
		}
		else
		{
			box_text = "Id = ???";
		}
		int pos_x = std::max(face_i.tl().x - 10, 0);
		int pos_y = std::max(face_i.tl().y - 10, 0);			   
		putText(im, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(255,255,255), 1.0);	
	        		
	    }
	}
    return im;
}

int mainaaaaaaaaaaaaa(int argc, char **argv){
    //Khoi tao thong tin face
    people[P_BINH] 	= "Binh";
    people[P_HIEN] 	= "Hien";
    people[P_HIEU] 	= "Hieu";
    people[P_PHU]	= "Phu";
    
    
    // Doc thong tin file csv cho viec nhan dien
    string fn_csv = string(argv[1]);
    cout << fn_csv << endl;
////    try {
////            read_csv(fn_csv, images, labels);
////	    DEBUG cout<<"(OK) read CSV ok\n";
////    	} 
////    catch (cv::Exception& e) 
////    {
////        cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
////        exit(1);
////    }
////    
    // train the model with your nice collection of pictures	
    trace("(init) start train images");
    model.train(images, labels);
    trace("(init) train images : ok");
    
    
    PeopleDetector people_detect;
    //string face_path = "./data/fontface3.xml";
    string face_path = "./data/haarcascade_frontalface_default.xml";
    string body_path ="./case.xml";
    string path = "./Image";
   
    string _img_dir = IMAGE_DIR;
    string _dir_format = DIR_FORMAT;
    string _file_format = string(DIR_FORMAT) + "/" + string(FILE_FORMAT);
    string _ext = EXT;
    
    
    if(!people_detect.loadFaceCascadeName(face_path))
	exit(1);
//   if(people_detect.loadBodyCascadeName(body_path))
//	exit(1);
   
   
    bool isColor = 1;
    namedWindow(WIN_NAME);
    cap.open(640,480, isColor ? true : false);  // (640,480), (320,240)
   
    Mat im;
    double time = 0;
    unsigned int frames = 0;
    
   
    //Cho camera duoc bat
    while(cap.grab().empty())
    {
	cout << "Camera dang duoc bat ....\n";
    }
    // Global variables

    Mat frame; //current frame
    Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
    Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
    char keyboard; //input from keyboard

    char keyboard; //input from keyboard
  while( keyboard != 'q' && keyboard != 27 ){
        //read the current frame
        frame = cap.grab();
        //update the background model
        pMOG2->apply(frame, fgMaskMOG2);
        //get the frame number and write it on the current frame
        stringstream ss;
        rectangle(frame, cv::Point(10, 2), cv::Point(100,20),
                  cv::Scalar(255,255,255), -1);
        //ss << capture.get(CAP_PROP_POS_FRAMES);
        string frameNumberString = ss.str();
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
                FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
        //show the current frame and the fg masks
        imshow("Frame", frame);
        imshow("FG Mask MOG 2", fgMaskMOG2);
        //get the input from the keyboard
        keyboard = (char)waitKey( 30 );
    }
}


// Add thu vien camera va thu vien opencv
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "PiCapture.h"
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <wiringPi.h>
#include "PeopleDetector.h"
#include "../face_rec/include/facerec.hpp"
// Dinh nghia mot so thong tin co ban ve thu muc luu anh, 
// ten khung hinh show video,...
#define WIN_NAME "PiCapture"
#define IMAGE_DIR  "./pics/"                
#define DIR_FORMAT  "%d%h%Y"                
#define FILE_FORMAT "%d%h%Y_%H%M%S"
#define EXT ".jpg"  
#define THUC_NGHIEM 0
#define DETECT_HUMAN 1
// Dinh nghia mot so bien dung de debug
#define TRACE 1
#define DEBUG_MODE 0
#define DEBUG if (DEBUG_MODE==1)

using namespace cv;
using namespace std;

Eigenfaces model;
PiCapture cap;
volatile int flag = 0;

// Dinh nghia hang so de phat trien chu nang nhan dang sau nay.
#define MAX_PEOPLE 		4
#define P_BINH			0
#define P_HIEN			1
#define P_HIEU			2
#define P_PHU			3
string  people[MAX_PEOPLE];
int nPictureById[MAX_PEOPLE];

// Khai bao mot so bien dung cho anh 
vector<Mat> images;
vector<int> labels;
Mat gray,frame,face,face_resized;
int PREDICTION_SEUIL = 4500;

// Ham kiem tra thu muc chua anh duoc tao chu, neu chua thi tao thu muc chua anh,
// cho biet thong tin (ngay thang chup)ve anh
inline void directoryExistsOrCreate(const char* pzPath)
{
    DIR *pDir;
    // Thu muc khong ton tai -> tao thu muc
    if ( pzPath == NULL || (pDir = opendir (pzPath)) == NULL)
        mkdir(pzPath, 0777);
    else if(pDir != NULL)
        (void) closedir (pDir);
}

// Ham luu hinh anh tu camera
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
    ss << "/Gray";
    directoryExistsOrCreate(ss.str().c_str());

    // Create name for the image
    strftime (TIME,80,_file_format,timeinfo);
    ss.str("");
    if(incr < 200) incr++; // quick fix for when delay < 1s && > 10ms, (when delay <= 10ms, images are overwritten)
    else incr = 0;
    ss << _directory << TIME << "_" << static_cast<int>(incr) << _extension;
    return imwrite(ss.str().c_str(), _image);
}

// Ham trace dung de debug
void trace(string s)
{
    if (TRACE==1)
    {
            cout<<s<<"\n";
    }
}

// Ham doc file chua thong tin doi tuong nhan dien
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
            // Doc cac tap tin xay dung bo du lieu hinh anh
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
            nPictureById[atoi(classlabel.c_str())]++;
        	nLine++;
        }
    }
    char sTmp[128];
    sprintf(sTmp,"(init) %d pictures read to train",nLine);
    trace((string)(sTmp));
	for (int j=0;j<MAX_PEOPLE;j++)
	{
		sprintf(sTmp,"(init) %d pictures of %s (%d) read to train",nPictureById[j],people[j].c_str(),j);
   	 	trace((string)(sTmp));
	}
}
// Ham nhan dang doi tuong
Mat recog_Face(const Mat &_im, vector<Rect>faces)
{
    Mat im = _im;
    for(int i = 0; i < faces.size(); i++)
	{
	    // Cat vung khuon mat doi tuong can nhan dien
	    Rect face_i = faces[i];  
	    face = im(face_i);  
	    // Thay doi kich thuoc va hien thi khuon mat
	    cv::resize(face, face_resized, Size(100, 100), 1.0, 1.0, CV_INTER_NN); //INTER_CUBIC)
	    // Xac dinh ai trong khung hinh
	    char sTmp[256];		
	    double predicted_confidence	= 0.0;
	    int prediction = -1;
	    model.predict(face_resized,prediction,predicted_confidence);
	    // Ve vung khuon mat phat hien duoc  
	    rectangle(im, face_i, CV_RGB(255, 255 ,255), 1);
            
	    if (predicted_confidence>PREDICTION_SEUIL)
		{
	 	// Gan nhan khuon mat la ai.
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

int main(int argc, char **argv){
    // Thiet lap GPIO de phat tin hieu ra speaker 
    // sau khi phat hien duoc khuon mat hoac co the
    wiringPiSetupGpio();
    pinMode(17, OUTPUT);
    bool isColor = 1;
#if THUC_NGHIEM
     int saved_img_count = 0;
#endif 
     
#if RECOG_HUMAN   
     // Khoi tao thong tin khuon mat de phat trien nhan dien sau nay
    people[P_BINH] 	= "Binh";
    people[P_HIEN] 	= "Hien";
    people[P_HIEU] 	= "Hieu";
    people[P_PHU]	= "Phu";
    
 
    // Doc thong tin file csv cho viec nhan dien
    string fn_csv = string(argv[1]);
    cout << fn_csv << endl;
    try 
    {
        read_csv(fn_csv, images, labels);
        DEBUG cout<<"(OK) read CSV ok\n";
    } 
    catch (cv::Exception& e) 
    {
        cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
        exit(1);
    }
    // Hoc khuon mat can phat hien	
    trace("(init) start train images");
    model.train(images, labels);
    trace("(init) train images : ok");
    
#endif    
    // Khai bao doi tuong
    PeopleDetector people_detect;
    // Khai bao chuoi duong dan toi file dac trung phat hien khuon mat,
    // phat hien co the
    string face_path = "./data/haartraining_final_6.xml";
    // string face_path = "./data/haarcascade_frontalface_default.xml";
    // string face_path = "./data/myfacedetector_v2.xml";
    // string face_path = "./data/haarcascade_frontalface_alt.xml";
    string body_path ="./data/haarcascade_upperbody.xml";
  
    string _img_dir = IMAGE_DIR;
    string _dir_format = DIR_FORMAT;
    string _file_format = string(DIR_FORMAT) + "/" + string(FILE_FORMAT);
    string _gray_file_format = string(DIR_FORMAT) + "/Gray/" + string(FILE_FORMAT);
    string _ext = EXT;
//#endif    
    // Kiem tra xem file dac trung xml co duoc load hay khong 
    if(!people_detect.loadFaceCascadeName(face_path))
	exit(1);
    if(!people_detect.loadBodyCascadeName(body_path))
	exit(1);
  
    namedWindow(WIN_NAME);      // Set ten khung hinh show video
    cap.open(320,240,isColor);  // Set kich co khung hinh (640,480), (320,240)
   
    Mat im;
    double time = 0;
    unsigned int frames = 0;
    
    // Cho camera duoc bat
    while(cap.grab().empty())
    {
	cout << "Camera dang duoc bat ....\n";
    }
    while(char(waitKey(1)) != 'q') {
	double t0 = getTickCount();
	im = cap.grab();
#if DETECT_HUMAN
    Mat frame = im.clone();
    cvtColor(frame,frame, CV_RGB2GRAY);
    vector<Rect> faces = people_detect.detectFace(frame);
    vector<Rect> bodies = people_detect.detectBodyHoG(frame);
    // Khai bao goi ham phat hien khuon mat.
    Mat tmp = people_detect.draw(frame,faces,0);
    Mat tmp1 = people_detect.draw(tmp,bodies,1);
    time = (getTickCount() - t0)*1000 / getTickFrequency();
    cout << "Thoi gian xu ly:" <<time << " ms" << endl;
    imshow(WIN_NAME, tmp1);//recog_Face(im,faces));
#endif 	
	
#if THUC_NGHIEM
    time = (getTickCount() - t0)*1000 / getTickFrequency();
    cout << "Thoi gian xu ly:" <<time << " ms" << endl;
    imshow(WIN_NAME, im);//recog_Face(im,faces));

    if(saved_img_count <= 20)
    {
        // Luu anh mau
        saveImg(im, _img_dir, _ext, _dir_format.c_str(), _file_format.c_str());  
        cvtColor(im,im, CV_RGB2GRAY);
        // Luu anh xam
        saveImg(im, _img_dir, _ext, _dir_format.c_str(), _gray_file_format.c_str());  
    }
#endif
    }
}

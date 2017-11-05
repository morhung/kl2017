#include "PeopleDetector.h"
#include "wiringPi.h"
#define WINDOW_NAME "People Detector"
#define COLOR_FACE CV_RGB(255, 255 ,255)
#define COLOR_BODY CV_RGB(0, 255 ,0)
#define TYPE_FACE 0
#define TYPE_BODY 1

PeopleDetector::PeopleDetector() {
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
}

PeopleDetector::PeopleDetector(const PeopleDetector& orig){
}

PeopleDetector::~PeopleDetector() {
}

bool PeopleDetector::loadFaceCascadeName(string path){
   if (!face_cascade.load(path))
    {
	    cout <<"(E) face cascade model not loaded :"<<path <<"\n";
	    return false;
    }
    return true;
}
// Ham kiem tra xem load duoc csdl phat hien khuon mat hay khong
bool PeopleDetector::loadBodyCascadeName(string path){
    if (!body_cascade.load(path))
    {
	    cout <<"(E) face cascade model not loaded :"<<path <<"\n"; 
	    return false;
    }
    return true;
}
// Ham phat hien khuon mat
vector< Rect > PeopleDetector::detectFace(const Mat &img){
    vector< Rect >  _faces;
  
    if(face_cascade.empty())  
    {
	cout << "File face_cascade chua duoc load\n";
	return _faces;
    }
    
    if(!img.empty())
       face_cascade.detectMultiScale(img, _faces, 1.2, 6,CV_HAAR_SCALE_IMAGE, Size(30, 30));
       //face_cascade.detectMultiScale(img, _faces, 1.2, 6 , CV_HAAR_SCALE_IMAGE,Size(80,80));
       //face_cascade.detectMultiScale(img, _faces, 1.105, 6 , CV_HAAR_SCALE_IMAGE,Size(20,20),Size(60,60));
    return _faces;
}
 // Ham phat hien co the
vector< Rect_<int> > PeopleDetector::detectBody(const Mat &img){
   vector< Rect_<int> >  _bodies;
   
   if(body_cascade.empty()){
       cout << "File body_cascade chua duoc load\n";
       return _bodies;
   }
   if(!img.empty())
       body_cascade.detectMultiScale(img, _bodies,1.1,50,0|1,Size(5, 10),Size(300,480 ));
   return _bodies;
}

vector<Rect> PeopleDetector::detectBodyHoG(const Mat &img)
{    
    vector<Rect> found;
    hog.detectMultiScale(img, found, 0, Size(8,8), Size(32,32), 1.1, 2);
   
    return found;
}
// Ham ve hinh chu nhat khoanh vung khuon mat va vung co the phat hien duoc
Mat PeopleDetector::draw(const Mat &img, vector< Rect_<int> > detected_object,int type){
    int num = detected_object.size();
    Mat _img = img;
    for(int i = 0; i <num; i++)
    {
	if(type == TYPE_FACE){  
	    rectangle(_img, detected_object[i], COLOR_FACE, 1);
            // Bat GPIO 17 phat tin hieu phat hien khuon mat ra speaker
            digitalWrite(17, HIGH); 
            delay(20);
            digitalWrite(17, LOW);
	}
	else
	{
	    
            for(size_t i = 0; i < detected_object.size(); i++ )
            {
                Rect r = detected_object[i];
                size_t j;
                for ( j = 0; j < detected_object.size(); j++ )
		if ( j != i && (r & detected_object[j]) == r )
		    break;
                if ( j == detected_object.size() )
                    detected_object.push_back(r);
            }
            for (size_t i = 0; i < detected_object.size(); i++)
            {
                Rect r = detected_object[i];
                r.x += cvRound(r.width*0.1);
                r.width = cvRound(r.width*0.8);
                r.y += cvRound(r.height*0.07);
                r.height = cvRound(r.height*0.8);
                rectangle(_img, r.tl(), r.br(), cv::Scalar(255,193,193), 1);
                // Bat GPIO 17 phat tin hieu phat hien co the ra speaker
                digitalWrite(17, HIGH); 
                delay(20);
                digitalWrite(17, LOW);  
            }   
	}
    } 
    return _img;
}
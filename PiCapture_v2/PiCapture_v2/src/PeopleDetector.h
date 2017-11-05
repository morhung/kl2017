/* 
 * File:   Detector.h
 * Author: Hien-PC
 *
 * Created on December 14, 2016, 11:18 PM
 */

#ifndef PEOPLE_DETECTOR_H
#define	PEOPLE_DETECTOR_H

#include <string>
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class PeopleDetector {
public:
    PeopleDetector();
    PeopleDetector(const PeopleDetector& orig);
    virtual ~PeopleDetector();
    
    bool loadFaceCascadeName(string path);
    bool loadBodyCascadeName(string path);

    vector< Rect_<int> > detectFace(const Mat &img); // tra ve danh sach so khuon mat phat hien duoc
    vector< Rect_<int> > detectBody(const Mat &img); // tra ve danh sach so co the phat hien duoc
    vector<Rect> detectBodyHoG(const Mat &img); // detect HOG
    Mat draw(const Mat &img, vector< Rect_<int> > detected_object,int type);
private:
    CascadeClassifier face_cascade;
    CascadeClassifier body_cascade;
    HOGDescriptor hog;
};

#endif	/* EOPLE_DETECTOR_H */


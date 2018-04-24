#include <jni.h>
#include <string>
#include <vector>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <sstream>

#include <android/log.h>


using namespace std;
using namespace cv;

extern "C"
{
Mat flare, flare_res, flare_alpha, flare_alpha_res;
CascadeClassifier face_classifier, eye_classifier;
ostringstream os;

Rect left_eye, right_eye;

bool mode = false;

void JNICALL
Java_com_bolito2_flarifyandroid_MainActivity_preparativos(JNIEnv *env, jobject instance,
                                                                      jstring flare_path, jstring face_cascade_path, jstring eye_cascade_path) {
    const char* face_path;
    face_path = env->GetStringUTFChars(face_cascade_path, NULL);

    const char* eye_path;
    eye_path = env->GetStringUTFChars(eye_cascade_path, NULL);

    eye_classifier.load(eye_path);
    face_classifier.load(face_path);

    const char *path;
    path = env->GetStringUTFChars(flare_path, NULL);

    left_eye = Rect(0,0,0,0);
    right_eye = Rect(0,0,0,0);

    Mat flare_file = cv::imread(path, IMREAD_UNCHANGED);
    vector<Mat> ch;

    split(flare_file, ch);
    ch[0] = ch[3];
    ch[1] = ch[3];
    ch[2] = ch[3];
    ch.pop_back();
    merge(ch, flare_alpha);
    ch.clear();

    split(flare_file, ch);
    ch.pop_back();
    merge(ch, flare);
    ch.clear();

    cvtColor(flare, flare, COLOR_RGB2BGR);

    flare.convertTo(flare, CV_32FC3, 1.0 / 255.0);
    flare_alpha.convertTo(flare_alpha, CV_32FC3, 1.0 / 255);

    multiply(flare_alpha, flare, flare);

    flare.convertTo(flare, CV_8UC3, 255.0);

    env -> ReleaseStringUTFChars(flare_path, path ) ;
    env->ReleaseStringUTFChars(face_cascade_path, face_path);
    env->ReleaseStringUTFChars(eye_cascade_path, eye_path);
}

float multiplier = 4;

Mat gris_roi, img_roi;

vector<int> rejectLevels;
vector<double> levelWeights;

vector<Rect> eyes;

Rect getEye(Mat &gris, Mat &img, Rect& eyeZone, int &maxEyeSize){
    gris_roi = gris(eyeZone);
    img_roi = img(eyeZone);

    eye_classifier.detectMultiScale(gris_roi, eyes, rejectLevels,levelWeights,  1.05, 0, 0, Size(50,50), Size(), true);

    /*
    for (int i = 0; i < eyes.size(); i++) {
        if (eyes[i].width > maxEyeSize)maxEyeSize = eyes[i].width;


        rectangle(img_roi, eyes[i], Scalar(255, 0, 0));
        os.str("");
        os << levelWeights[i];
        putText(img_roi, os.str(), Point(eyes[i].x, eyes[i].y - 25), FONT_HERSHEY_PLAIN, 2, Scalar(0, 0, 255), 2);
        os.str("");

        line(img_roi, Point(eyes[i].x, eyes[i].y - 25), Point(eyes[i].x, eyes[i].y), Scalar(255,0,0));

    }
*/
    if (eyes.size() > 0)
    {
        int maxIndex = distance(levelWeights.begin(), max_element(levelWeights.begin(), levelWeights.end()));

        eyes[maxIndex].x += eyeZone.x;
        eyes[maxIndex].y += eyeZone.y;

        if(eyes[maxIndex].width > maxEyeSize)maxEyeSize = eyes[maxIndex].width;

        return eyes[maxIndex];
    }
    return Rect(0,0,0,0);
}
float eyeCenterX, eyeCenterY;
Rect img_rect, roi_rect, roi_rect_bounded, flare_roi_rect;
Mat flare_roi, roi, flare_alpha_roi;
void drawFlare(Mat& img, int& maxEyeSize, Rect& eye, float &flareCenterX, float& flareCenterY, float& flareWidth, float& flareHeight){
    if(maxEyeSize > 0 && eye.width != 0 && eye.height != 0) {
        //rectangle(img, eye, Scalar(0,0,255), 2);

        eyeCenterX = eye.x + eye.width / 2;
        eyeCenterY = eye.y + eye.height / 2;

        circle(img, Point(eyeCenterX, eyeCenterY), 3, Scalar(255, 0, 0), 3);


        img_rect = Rect(0, 0, img.cols, img.rows);
        roi_rect = Rect(eyeCenterX - flareCenterX, eyeCenterY - flareCenterY,
                        flareWidth, flareHeight);
        roi_rect_bounded = roi_rect & img_rect;
        flare_roi_rect = Rect(0, 0, roi_rect.width, roi_rect.height) &
                         Rect(roi_rect_bounded.x - (eyeCenterX - flareCenterX),
                              roi_rect_bounded.y - (eyeCenterY - flareCenterY),
                              roi_rect_bounded.width, roi_rect_bounded.height);

        flare_roi = flare_res(flare_roi_rect);
        roi = img(roi_rect_bounded);

        if(mode){
            flare_alpha_roi = flare_alpha_res(flare_roi_rect);
            multiply(Scalar::all(1.0) - flare_alpha_roi, roi, roi, 1, CV_8UC3);
        }
        add(flare_roi, roi, roi);

        //img.convertTo(img,-1, 1.5);
    }
}

Mat temp, gris;

void JNICALL Java_com_bolito2_flarifyandroid_MainActivity_flarify(JNIEnv *env, jobject instance, jlong matAddrGray, jint orientation, jint cameraIndex) {
    Mat &img = *(Mat *) matAddrGray;
    cvtColor(img, img, COLOR_BGRA2BGR);
    flip(img, temp, 1);
    temp.copyTo(img);

    if(cameraIndex == 0)flip(img, img, 1);

    if(orientation < 45 || orientation >= 315)rotate(img, img,  ROTATE_90_CLOCKWISE);
    if(orientation >= 45 && orientation < 135)rotate(img, img,  ROTATE_180);
    if(orientation >= 135 && orientation < 225)rotate(img, img,  ROTATE_90_COUNTERCLOCKWISE);

    cvtColor(img, gris, COLOR_BGR2GRAY);
    equalizeHist(gris, gris);

    vector<Rect> faces;

    if(face_classifier.empty()) {
        circle(img, Point(300, 300), 50, Scalar(255,0,0), 5);
    }
    else{
        face_classifier.detectMultiScale(gris, faces, 1.1, 3, 0,Size(300, 300), Size());
        for (int f = 0; f < faces.size(); f++) {
            rectangle(img, faces[f], Scalar(0, 255, 0), 2);

            int maxEyeSize = 0;
            //Ojo izquierdo
            Rect left_roi_rect = Rect(faces[f].x, faces[f].y, faces[f].width*0.4, faces[f].height*0.4);
            rectangle(img, left_roi_rect, Scalar(0, 255, 255));

            left_eye = getEye(gris, img, left_roi_rect,maxEyeSize);

            //Ojo derecho
            Rect right_roi_rect = Rect(faces[f].x + faces[f].width*0.6, faces[f].y, faces[f].width*0.4, faces[f].height*0.4);
            rectangle(img, right_roi_rect, Scalar(0, 255, 255));

            right_eye = getEye(gris, img, right_roi_rect, maxEyeSize);


            float flareWidth = (float)faces[f].width*multiplier*1.5f;
            float flareHeight = (float)faces[f].width*multiplier / 1.5f;

            float flareCenterX = 655 * flareWidth / 1600;
            float flareCenterY = 445 * flareHeight / 889;

            if(flareWidth > 0 & flareHeight > 0){
                resize(flare, flare_res, Size(flareWidth, flareHeight), 0, 0);
                if(mode)resize(flare_alpha, flare_alpha_res, Size(flareWidth, flareHeight), 0, 0);
            }

            drawFlare(img, maxEyeSize, left_eye, flareCenterX, flareCenterY, flareWidth, flareHeight);
            drawFlare(img, maxEyeSize, right_eye, flareCenterX, flareCenterY, flareWidth, flareHeight);
        }
    }

    cvtColor(img, img, COLOR_BGR2BGRA);
    if(orientation < 45 || orientation >= 315)rotate(img, img,  ROTATE_90_COUNTERCLOCKWISE);
    if(orientation >= 45 && orientation < 135)rotate(img, img,  ROTATE_180);
    if(orientation >= 135 && orientation < 225)rotate(img, img,  ROTATE_90_CLOCKWISE);
}
}

#include <jni.h>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/video.hpp>
#include "android/bitmap.h"
#include <cstring>
#include <vector>
#include <iostream>

using namespace std;
using namespace cv;

void bitmapToMat(JNIEnv * env, jobject bitmap, cv::Mat &dst, jboolean needUnPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void* pixels = 0;
    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        dst.create(info.height, info.width, CV_8UC4);
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ) {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(needUnPremultiplyAlpha) cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
}

void matToBitmap(JNIEnv * env, cv::Mat src, jobject bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void* pixels = 0;
    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( src.dims == 2 && info.height == (uint32_t)src.rows && info.width == (uint32_t)src.cols );
        CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ) {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1) {
                cvtColor(src, tmp, cv::COLOR_GRAY2RGBA);
            } else if(src.type() == CV_8UC3) {
                cvtColor(src, tmp, cv::COLOR_RGB2RGBA);
            } else if(src.type() == CV_8UC4) {
                if(needPremultiplyAlpha) cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1) {
                cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3) {
                cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4) {
                cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}

#define compab_mask_inc(ptr,shift) \
{ value |= ((unsigned int)(cntr - *ptr) & 0x80000000) >> (31-shift); ptr++; }

class LBP {
private:
    int* LBP8(const int* , int , int);
public:
    LBP();
    vector<int> calcularLBP(Mat);
    Mat calcularLBPImage(Mat imagen);
};

LBP::LBP() {}

int* LBP::LBP8(const int* data, int rows, int columns) {
    const int
            *p0 = data,
            *p1 = p0 + 1,
            *p2 = p1 + 1,
            *p3 = p2 + columns,
            *p4 = p3 + columns,
            *p5 = p4 - 1,
            *p6 = p5 - 1,
            *p7 = p6 - columns,
            *center = p7 + 1;
    int r,c,cntr;
    unsigned int value;
    int* result = (int*)malloc(256*sizeof(int));
    memset(result, 0, 256*sizeof(int));
    for (r=0;r<rows-2;r++){
        for (c=0;c<columns-2;c++){
            value = 0;
            cntr = *center - 1;
            compab_mask_inc(p0,0);
            compab_mask_inc(p1,1);
            compab_mask_inc(p2,2);
            compab_mask_inc(p3,3);
            compab_mask_inc(p4,4);
            compab_mask_inc(p5,5);
            compab_mask_inc(p6,6);
            compab_mask_inc(p7,7);
            center++;
            result[value]++;
        }
        p0 += 2;
        p1 += 2;
        p2 += 2;
        p3 += 2;
        p4 += 2;
        p5 += 2;
        p6 += 2;
        p7 += 2;
        center += 2;
    }
    return result;
}

vector<int> LBP::calcularLBP(Mat imagen) {
    int *datos = (int *) malloc(imagen.rows*imagen.cols*sizeof(int));
    for(int i=0,k=0;i<imagen.rows;i++){
        for(int j=0;j<imagen.cols;j++){
            datos[k++] = imagen.at<uchar>(i,j);
        }
    }

    int *res = this->LBP8(datos, imagen.rows, imagen.cols);
    vector<int> histo;
    for(int i=0;i<256;i++){
        histo.push_back(res[i]);
    }

    free(datos);
    free(res);
    return histo;
}

Mat LBP::calcularLBPImage(Mat imagen) {
    Mat lbpImage = Mat::zeros(imagen.rows, imagen.cols, CV_8UC1);

    int *datos = (int *) malloc(imagen.rows * imagen.cols * sizeof(int));
    if (!datos) {
        cerr << "Error al asignar memoria" << endl;
        return lbpImage; // Devolver una imagen vacía en caso de error
    }

    for(int i = 0, k = 0; i < imagen.rows; i++) {
        for(int j = 0; j < imagen.cols; j++) {
            datos[k++] = imagen.at<uchar>(i, j);
        }
    }

    int *res = this->LBP8(datos, imagen.rows, imagen.cols);
    if (!res) {
        cerr << "Error al calcular LBP" << endl;
        free(datos);
        return lbpImage; // Devolver una imagen vacía en caso de error
    }

    for(int r = 0; r < imagen.rows - 2; r++) {
        for(int c = 0; c < imagen.cols - 2; c++) {
            lbpImage.at<uchar>(r + 1, c + 1) = res[(r * (imagen.cols - 2)) + c];
        }
    }

    free(datos);
    free(res);

    return lbpImage;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_boletinpractica3parte2_MainActivity_lbp(
        JNIEnv* env,
        jobject /* this */,
        jobject fotoObj) {

    Mat fotoMat;
    bitmapToMat(env, fotoObj, fotoMat, false);

    LBP lbp;
    Mat lbpImage = lbp.calcularLBPImage(fotoMat);

    matToBitmap(env, lbpImage, fotoObj, false);
}

#include <jni.h>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/ml.hpp>
#include <android/bitmap.h>
#include <fstream>
#include <android/log.h>

#define LOG_TAG "NativeCode"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace cv;
using namespace cv::ml;

std::string svmModelPath;

void bitmapToMat(JNIEnv * env, jobject bitmap, cv::Mat &dst, jboolean needUnPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void* pixels = 0;
    try {
        CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
        CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                  info.format == ANDROID_BITMAP_FORMAT_RGB_565);
        CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
        CV_Assert(pixels);
        dst.create(info.height, info.width, CV_8UC4);
        if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if (needUnPremultiplyAlpha) cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch (const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        LOGE("Exception in bitmapToMat: %s", e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {bitmapToMat}");
        LOGE("Unknown exception in bitmapToMat");
        return;
    }
}

cv::Mat calculateLBP(const cv::Mat& src) {
    cv::Mat lbp_image;
    src.convertTo(lbp_image, CV_32F);

    for (int i = 1; i < src.rows - 1; i++) {
        for (int j = 1; j < src.cols - 1; j++) {
            uchar center = src.at<uchar>(i, j);
            unsigned char code = 0;
            code |= (src.at<uchar>(i - 1, j - 1) > center) << 7;
            code |= (src.at<uchar>(i - 1, j) > center) << 6;
            code |= (src.at<uchar>(i - 1, j + 1) > center) << 5;
            code |= (src.at<uchar>(i, j + 1) > center) << 4;
            code |= (src.at<uchar>(i + 1, j + 1) > center) << 3;
            code |= (src.at<uchar>(i + 1, j) > center) << 2;
            code |= (src.at<uchar>(i + 1, j - 1) > center) << 1;
            code |= (src.at<uchar>(i, j - 1) > center) << 0;
            lbp_image.at<float>(i, j) = code;
        }
    }

    return lbp_image;
}

cv::Mat calculateLBPHistogram(const cv::Mat& lbp_image) {
    int histSize = 256; // LBP produces 256 possible values
    float range[] = {0, 256}; // Range of LBP values
    const float* histRange = {range};

    cv::Mat hist;
    cv::calcHist(&lbp_image, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, true, false);
    cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

    return hist;
}

std::string predictImage(const cv::Mat& image, cv::Ptr<cv::ml::SVM> svm) {
    cv::Mat lab_image;
    cv::cvtColor(image, lab_image, cv::COLOR_BGR2Lab);

    std::vector<cv::Mat> lab_planes;
    cv::split(lab_image, lab_planes);

    cv::Mat lbp_image = calculateLBP(lab_planes[0]);
    cv::Mat hist = calculateLBPHistogram(lbp_image);

    hist = hist.reshape(1, 1);
    int response = svm->predict(hist);

    LOGI("Predicted response: %d", response);

    if (response == 0) {
        return "Roca";
    } else {
        return "Madera";
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_boletinpractica3parte2_MainActivity_initModelPath(
        JNIEnv* env,
        jobject /* this */,
        jstring modelPath) {
    const char *modelPathChars = env->GetStringUTFChars(modelPath, 0);
    svmModelPath = std::string(modelPathChars);
    env->ReleaseStringUTFChars(modelPath, modelPathChars);
    LOGI("Ruta del modelo SVM: %s", svmModelPath.c_str());

    // Verificar si el archivo existe
    std::ifstream file(svmModelPath);
    if (file.good()) {
        LOGI("El archivo %s existe.", svmModelPath.c_str());
    } else {
        LOGE("El archivo %s no existe.", svmModelPath.c_str());
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_boletinpractica3parte2_MainActivity_lbp(
        JNIEnv* env,
        jobject /* this */,
        jobject bitmap) {
    Mat fotoMat;
    bitmapToMat(env, bitmap, fotoMat, false);

    // Asegúrate de que la imagen sea de tamaño adecuado
    cv::resize(fotoMat, fotoMat, cv::Size(256, 256));
    LOGI("Imagen convertida a Mat y redimensionada.");

    // Cargar el modelo SVM desde la ruta previamente inicializada
    cv::Ptr<cv::ml::SVM> svm = cv::ml::SVM::load(svmModelPath);
    if (svm.empty()) {
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "No se pudo cargar el modelo SVM");
        LOGE("No se pudo cargar el modelo SVM desde la ruta: %s", svmModelPath.c_str());
        return nullptr;
    }

    std::string category = predictImage(fotoMat, svm);
    LOGI("Categoría predicha: %s", category.c_str());

    // Devolver solo la categoría predicha
    return env->NewStringUTF(category.c_str());
}

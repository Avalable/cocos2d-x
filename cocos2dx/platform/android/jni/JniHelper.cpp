/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "JniHelper.h"
#include <android/log.h>
#include <string.h>

#if 1
#define  LOG_TAG    "JniHelper"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define  LOGD(...)
#define  LOGE(...)
#endif

#define JAVAVM    cocos2d::JniHelper::getJavaVM()

using namespace std;

static pthread_key_t g_key;

extern "C"
{
    static void detach_current_thread (void *env)
    {
        JAVAVM->DetachCurrentThread();
    }

    jclass _getClassID(const char *className)
    {
        if (nullptr == className) {
            return nullptr;
        }

        JNIEnv* env = cocos2d::JniHelper::getEnv();

        jstring _jstrClassName = env->NewStringUTF(className);

        jclass _clazz = (jclass) env->CallObjectMethod(cocos2d::JniHelper::classloader,
                cocos2d::JniHelper::loadclassMethod_methodID,
                _jstrClassName);

        if (nullptr == _clazz) {
            LOGE("Classloader failed to find class of %s", className);
            env->ExceptionClear();
        }

        env->DeleteLocalRef(_jstrClassName);

        return _clazz;
    }

    static string jstring2string_(jstring jstr)
    {
        if (jstr == NULL)
        {
            return "";
        }

        JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

        if (!pEnv)
        {
            return 0;
        }

        const char* chars = pEnv->GetStringUTFChars(jstr, NULL);
        string ret(chars);
        pEnv->ReleaseStringUTFChars(jstr, chars);

        return ret;
    }
}

NS_CC_BEGIN

jmethodID JniHelper::loadclassMethod_methodID = nullptr;
jobject JniHelper::classloader = nullptr;

JavaVM* JniHelper::m_psJavaVM = NULL;

JavaVM* JniHelper::getJavaVM()
{
    return m_psJavaVM;
}

bool JniHelper::setClassLoaderFrom(jobject activityinstance) {

//    LOGD("JniHelper::setClassLoaderFrom");

    JniMethodInfo _getclassloaderMethod;
    if (!JniHelper::getMethodInfo_DefaultClassLoader(_getclassloaderMethod,
            "android/content/Context",
            "getClassLoader",
            "()Ljava/lang/ClassLoader;")) {
        return false;
    }

    jobject _c = cocos2d::JniHelper::getEnv()->CallObjectMethod(activityinstance,
            _getclassloaderMethod.methodID);

    if (nullptr == _c) {
        return false;
    }

    JniMethodInfo _m;
    if (!JniHelper::getMethodInfo_DefaultClassLoader(_m,
            "java/lang/ClassLoader",
            "loadClass",
            "(Ljava/lang/String;)Ljava/lang/Class;")) {
        return false;
    }

    JniHelper::classloader = cocos2d::JniHelper::getEnv()->NewGlobalRef(_c);
    JniHelper::loadclassMethod_methodID = _m.methodID;

//    LOGD("JniHelper::setClassLoaderFrom PASS");

    return true;
}

bool JniHelper::getStaticMethodInfo(JniMethodInfo &methodinfo,
        const char *className,
        const char *methodName,
        const char *paramCode) {
    if ((nullptr == className) ||
            (nullptr == methodName) ||
            (nullptr == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv");
        return false;
    }

    jclass classID = _getClassID(className);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetStaticMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("Failed to find static method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;
    return true;
}

bool JniHelper::getMethodInfo_DefaultClassLoader(JniMethodInfo &methodinfo,
        const char *className,
        const char *methodName,
        const char *paramCode) {
    if ((nullptr == className) ||
            (nullptr == methodName) ||
            (nullptr == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return false;
    }

    jclass classID = env->FindClass(className);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;

    return true;
}

bool JniHelper::getMethodInfo(JniMethodInfo &methodinfo,
        const char *className,
        const char *methodName,
        const char *paramCode) {
    if ((nullptr == className) ||
            (nullptr == methodName) ||
            (nullptr == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return false;
    }

    jclass classID = _getClassID(className);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;

    return true;
}

JNIEnv* JniHelper::cacheEnv(JavaVM* jvm)
{
    JNIEnv *_env = nullptr;
    // get jni environment
    jint ret = jvm->GetEnv((void **) &_env, JNI_VERSION_1_4);

    switch (ret) {
        case JNI_OK :
            // Success!
//            LOGD("JniHelper::cacheEnv: JNI_OK");
            pthread_setspecific(g_key, _env);
            return _env;

        case JNI_EDETACHED :
            // Thread not attached
//            LOGD("JniHelper::cacheEnv: JNI_EDETACHED");
            if (jvm->AttachCurrentThread(&_env, nullptr) < 0) {
                LOGE("Failed to get the environment using AttachCurrentThread()");

                return nullptr;
            } else {
                // Success : Attached and obtained JNIEnv!
                pthread_setspecific(g_key, _env);
                return _env;
            }

        case JNI_EVERSION :
            // Cannot recover from this error
            LOGE("JNI interface version 1.4 not supported");
        default :
            LOGE("Failed to get the environment using GetEnv()");
            return nullptr;
    }
}

JNIEnv* JniHelper::getEnv()
{
    JNIEnv *_env = (JNIEnv *) pthread_getspecific(g_key);

//    LOGD("JniHelper::getEnv: %p", _env);

    if (_env == nullptr)
        _env = JniHelper::cacheEnv(m_psJavaVM);

    return _env;
}

void JniHelper::setJavaVM(JavaVM *javaVM)
{
    pthread_t thisthread = pthread_self();
//    LOGD("JniHelper::setJavaVM(%p), pthread_self() = %ld", javaVM, thisthread);
    m_psJavaVM = javaVM;

    pthread_key_create(&g_key, detach_current_thread);
}

jclass JniHelper::getClassID(const char *className, JNIEnv *env)
{
    return _getClassID(className);
}

string JniHelper::jstring2string(jstring str)
{
    return jstring2string_(str);
}

long JniHelper::getNativeHeapAllocatedSize()
{
    JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

    if (!pEnv)
    {
        return -1L;
    }

    jclass clazz = getClassID("android/os/Debug", pEnv);
    if (clazz)
    {
        JniMethodInfo methodInfo;
        if (! getStaticMethodInfo(methodInfo, "android/os/Debug", "getNativeHeapAllocatedSize", "()J"))
        {            
            return -1L;
        }

        return methodInfo.env->CallStaticLongMethod(methodInfo.classID, methodInfo.methodID);
    }
    return -1L;
}

long JniHelper::getNativeHeapSize()
{
    JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

    if (!pEnv)
    {
        return -1L;
    }

    jclass clazz = getClassID("android/os/Debug", pEnv);
    if (clazz)
    {
        JniMethodInfo methodInfo;
        if (! getStaticMethodInfo(methodInfo, "android/os/Debug", "getNativeHeapSize", "()J"))
        {            
            return -1L;
        }

        return methodInfo.env->CallStaticLongMethod(methodInfo.classID, methodInfo.methodID);
    }
    return -1L;
}

jobjectArray JniHelper::makeStringArray(jsize count, std::string array[])
{
    JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

    if (!pEnv)
    {
        return NULL;
    }

    jclass stringClass = pEnv->FindClass("java/lang/String");
    jobjectArray row = pEnv->NewObjectArray(count, stringClass, 0);
    jsize i;

    for (i = 0; i < count; ++i) {
        pEnv->SetObjectArrayElement(row, i, pEnv->NewStringUTF(array[i].c_str()));
    }
    return row;
}

jobjectArray JniHelper::makeStringArray(const std::vector<std::string>& array)
{
    JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

    if (!pEnv)
    {
        return NULL;
    }

    jclass stringClass = pEnv->FindClass("java/lang/String");
    jobjectArray row = pEnv->NewObjectArray(array.size(), stringClass, 0);
    jsize i;

    for (i = 0; i < array.size(); ++i) {
        pEnv->SetObjectArrayElement(row, i, pEnv->NewStringUTF(array[i].c_str()));
    }
    return row;
}

jintArray JniHelper::makeIntArray(jsize count, int array[])
{
    JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

    if (!pEnv)
    {
        return NULL;
    }

    jintArray result;
    result = pEnv->NewIntArray(count);
    if (result == NULL) {
         return NULL; /* out of memory error thrown */
    }
    
    int i;
    jint fill[count];
    for (i = 0; i < count; i++) {
         fill[i] = array[i];
    }

     // move from the temp structure to the java structure
    pEnv->SetIntArrayRegion(result, 0, count, fill);
    return result;
}

jbyteArray JniHelper::makeByteArray(std::string data)
{
    JNIEnv* pEnv = cocos2d::JniHelper::getEnv();

    if (!pEnv) {
        return NULL;
    }

    const int length = data.length();

    jbyteArray array = pEnv->NewByteArray(length);
    pEnv->SetByteArrayRegion(array, 0, length, reinterpret_cast<const signed char *>(data.c_str()));

    return array;
}

NS_CC_END

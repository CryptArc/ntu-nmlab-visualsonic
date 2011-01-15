
#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/Tool.h>
#include <QCAR/Tracker.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/Marker.h>

#include "SampleUtils.h"
#include "Texture.h"
#include "CubeShaders.h"
#include "Q_object.h"
#include "C_object.h"
#include "A_object.h"
#include "R_object.h"

#ifdef __cplusplus
extern "C"
{
	using namespace std;
#endif

#define TRACK_NUM 9
#define TOKEN_NUM 11
#define FILE_LENGTH 219392

jshort track[1000];
jint SampleRate = 32000;

float volume = 1.0f;

const char* trackFileName[TRACK_NUM] = 
{
	"/sdcard/nmlabtest/test1.raw",
	"/sdcard/nmlabtest/test2.raw",
	"/sdcard/nmlabtest/test3.raw",
	"/sdcard/nmlabtest/test4.raw",
	"/sdcard/nmlabtest/test5.raw",
	"/sdcard/nmlabtest/test6.raw",
	"/sdcard/nmlabtest/test7.raw",
	"/sdcard/nmlabtest/test8.raw",
	"/sdcard/nmlabtest/test9.raw",
};

short* TrackPcm[TRACK_NUM];

JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_AudioMgr_trackInit(JNIEnv* env, jobject obj)
{
	for(int i = 0; i < TRACK_NUM; ++i){
		FILE* fp = fopen(trackFileName[i], "rb");
		TrackPcm[i] = new short[FILE_LENGTH];
		fread(TrackPcm[i], 2, FILE_LENGTH, fp);
		fclose(fp);
		LOG("read succeed!");
	}
}



JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_AudioMgr_audioMixer(JNIEnv* env, jobject obj, jintArray track_vol, jshortArray Track)
{
	jshort* buf = (jshort*)env->functions->GetPrimitiveArrayCritical(env, Track, 0);
	jint* t_vol = env->functions->GetIntArrayElements(env, track_vol, 0);
	if(buf == NULL){
		LOG("NO COPY!!!!!!!!!!!!!!!!!");
		return;
	}
	int test;
	for(int i = 0; i < FILE_LENGTH; ++i){
		test = 0;
		for(int j = 0; j < TRACK_NUM; ++j)
			test += (TrackPcm[j][i] * t_vol[j]) >> 3; 
		if (test > 32767){
			buf[i] = 32767;
		} else if (test < -32768){
			buf[i] = -32768;
		} else{
			buf[i] = (short)test;
		}
	}
	env->functions->ReleasePrimitiveArrayCritical(env, Track, buf, 0);
	env->functions->ReleaseIntArrayElements(env, track_vol, t_vol, 0);
}

clock_t c_start;

JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_AudioMgr_setClock(JNIEnv* env, jobject obj)
{
	c_start = clock();
}


JNIEXPORT jfloat JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_AudioMgr_getVolumeFromBuf
(JNIEnv* env, jobject obj, int offset, int writelen, jshortArray buf, int bufsize)
{
	clock_t end = clock();
	int startidx = offset + (SampleRate * (end - c_start))/CLOCKS_PER_SEC - 500;
	if(startidx < 0) startidx = 0;
	int size = 1000;
	if((startidx + size) > bufsize){
		startidx = bufsize - size - 1;
	}
	env->functions->GetShortArrayRegion(env, buf, startidx, size, track);
	int vol = 0;
	int temp;
	for(int i = 0; i < size; ++i){
		temp = (track[i] < 0)? -(int)track[i] : (int)track[i];
		if(temp > vol) vol = temp;
	}
	if(vol == 0) vol = 1;
	volume = (float)(log(vol)/50)+1;

	return volume;//(jfloat)vol/5000.0f+1;
}


#ifdef __cplusplus
}
#endif
	

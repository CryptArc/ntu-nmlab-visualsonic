/*==============================================================================
            Copyright (c) 2010 QUALCOMM Incorporated.
            All Rights Reserved.
            Qualcomm Confidential and Proprietary
            
@file 
    FrameMarkers.cpp

@brief
    Sample for FrameMarkers

==============================================================================*/


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

// Textures:
int textureCount                = 0;
Texture** textures              = 0;

// OpenGL ES 2.0 specific:
unsigned int shaderProgramID    = 0;
GLint vertexHandle              = 0;
GLint normalHandle              = 0;
GLint textureCoordHandle        = 0;
GLint mvpMatrixHandle           = 0;

// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;

// Indicates whether screen is in portrait (true) or landscape (false) mode
bool isActivityInPortraitMode   = false;

// The projection matrix used for rendering virtual objects:
QCAR::Matrix44F projectionMatrix;

// Constants:
static const float kLetterScale        = 25.0f;
static const float kLetterTranslate    = 25.0f;


extern float volume;


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    isActivityInPortraitMode = isPortrait;
}

char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

#define TOK_NUM 11

bool tokenAppear[TOK_NUM] = {0};
int tokenVol[TOK_NUM] = {0};
float tokenAngle[TOK_NUM] = {0};

bool first = true;
	

JNIEXPORT jintArray JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_renderFrame(JNIEnv *env, jobject)
{
    //LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_GLRenderer_renderFrame");
 
    // Clear color and depth buffer 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render video background:
    QCAR::State state = QCAR::Renderer::getInstance().begin();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
	
	
	if(first){
		tokenVol[9] = 4;
		first = false;
	}

	jintArray result;
	jboolean isCopy;
	jsize size = state.getNumActiveTrackables();
	result = env->functions->NewIntArray(env, TOK_NUM);
	jint* VolArray = env->functions->GetIntArrayElements(env, result, &isCopy);
	int resultIdx = 0;
	
	
    // Did we find any trackables this frame?
    for(int tIdx = 0; tIdx < size; tIdx++)
    {
        // Get the trackable:
        const QCAR::Trackable* trackable = state.getActiveTrackable(tIdx);
        QCAR::Matrix44F modelViewMatrix =
            QCAR::Tool::convertPose2GLMatrix(trackable->getPose());
			

        // Choose the texture based on the target name:
        int textureIndex = 0;
        
        // Check the type of the trackable:
        assert(trackable->getType() == QCAR::Trackable::MARKER);
        const QCAR::Marker* marker = static_cast<const QCAR::Marker*>(trackable);

        textureIndex = marker->getMarkerId();
		
		//ADD by LAMB
		while(resultIdx != textureIndex){
			tokenAppear[resultIdx] = 0;
			VolArray[resultIdx] = tokenVol[resultIdx];
			++resultIdx;
		}
		
		//get angle
		float cost = modelViewMatrix.data[0];
		float sint = modelViewMatrix.data[1];
		float theta = acos(cost)*360.0f/2/3.14;
		if(sint < 0) theta = 360 - theta;
		char str[10];
		itoa((int)theta, str, 10);
		LOG(str);
		
		if(!tokenAppear[resultIdx]){
			//first show
			tokenAngle[resultIdx] = theta;
			VolArray[resultIdx] = tokenVol[resultIdx];
			tokenAppear[resultIdx] = 1;
		}else{
			int shiftLevel = ((int)(theta - tokenAngle[resultIdx]))/16;
			if(abs(shiftLevel) > 3){
				shiftLevel = 0;
				tokenAngle[resultIdx] = theta;
			}
			if(shiftLevel != 0){
				itoa(shiftLevel,str,10);
				LOG("shift!!");
				LOG(str);
				int test = tokenVol[resultIdx] + shiftLevel;
				if(test > 7) test = 7;
				else if(test < 0) test = 0;
				VolArray[resultIdx] = tokenVol[resultIdx] = test;
				tokenAngle[resultIdx] = theta;
			}else
				VolArray[resultIdx] = tokenVol[resultIdx];
		}
		++resultIdx;
		
		
		//add by lamb ,all "R"
		if(textureIndex > 3) textureIndex = 3;
        
        assert(textureIndex < textureCount);
        const Texture* const thisTexture = textures[textureIndex];

        // Select which model to draw:
        const GLvoid* vertices = 0;
        const GLvoid* normals = 0;
        const GLvoid* indices = 0;
        const GLvoid* texCoords = 0;
        int numIndices = 0;
		
        switch (marker->getMarkerId())
        {
        case 0:
            vertices = &QobjectVertices[0];
            normals = &QobjectNormals[0];
            indices = &QobjectIndices[0];
            texCoords = &QobjectTexCoords[0];
            numIndices = NUM_Q_OBJECT_INDEX;
            break;
        case 1:
            vertices = &CobjectVertices[0];
            normals = &CobjectNormals[0];
            indices = &CobjectIndices[0];
            texCoords = &CobjectTexCoords[0];
            numIndices = NUM_C_OBJECT_INDEX;
            break;
        case 2:
            vertices = &AobjectVertices[0];
            normals = &AobjectNormals[0];
            indices = &AobjectIndices[0];
            texCoords = &AobjectTexCoords[0];
            numIndices = NUM_A_OBJECT_INDEX;
            break;
        default:
            vertices = &RobjectVertices[0];
            normals = &RobjectNormals[0];
            indices = &RobjectIndices[0];
            texCoords = &RobjectTexCoords[0];
            numIndices = NUM_R_OBJECT_INDEX;
            break;
        }

        QCAR::Matrix44F modelViewProjection;

        SampleUtils::translatePoseMatrix(-kLetterTranslate,
                                         -kLetterTranslate,
                                         0.f,
                                         &modelViewMatrix.data[0]);
        SampleUtils::scalePoseMatrix(kLetterScale*volume, kLetterScale*volume, kLetterScale*volume,
                                     &modelViewMatrix.data[0]);
        SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
                                    &modelViewMatrix.data[0],
                                    &modelViewProjection.data[0]);

        glUseProgram(shaderProgramID);
 
        glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glVertexAttribPointer(normalHandle, 3, GL_FLOAT, GL_FALSE, 0, normals);
        glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE,
                              0, texCoords);

        glEnableVertexAttribArray(vertexHandle);
        glEnableVertexAttribArray(normalHandle);
        glEnableVertexAttribArray(textureCoordHandle);

        glBindTexture(GL_TEXTURE_2D, thisTexture->mTextureID);
        glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
                           (GLfloat*)&modelViewProjection.data[0]);
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indices);

        SampleUtils::checkGlError("FrameMarkers render frame");

    }
	
	//ADD by LAMB
	while(resultIdx != (TOK_NUM - 1)){
		tokenAppear[resultIdx] = 0;
		VolArray[resultIdx] = tokenVol[resultIdx];
		++resultIdx;
	}

    glDisable(GL_DEPTH_TEST);
    glDisableVertexAttribArray(vertexHandle);
    glDisableVertexAttribArray(normalHandle);
    glDisableVertexAttribArray(textureCoordHandle);

    QCAR::Renderer::getInstance().end();

	env->functions->ReleaseIntArrayElements(env, result, VolArray, 0);
	return result;
}


void
configureVideoBackground()
{
    // Get the default video mode:
    QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
    QCAR::VideoMode videoMode = cameraDevice.
                                getVideoMode(QCAR::CameraDevice::MODE_OPTIMIZE_QUALITY);

    // Configure the video background
    QCAR::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mSynchronous = true;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;
    
    if (isActivityInPortraitMode)
    {
        //LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenHeight;
    }
    else
    {
        //LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = screenWidth;
        config.mSize.data[1] = videoMode.mHeight
                            * (screenWidth / (float)videoMode.mWidth);
    }

    // Set the config:
    QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_initApplicationNative");
    
    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;
        
    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    jmethodID getTextureCountMethodID = env->GetMethodID(activityClass,
                                                    "getTextureCount", "()I");
    if (getTextureCountMethodID == 0)
    {
        LOG("Function getTextureCount() not found.");
        return;
    }

    textureCount = (int) env->CallObjectMethod(obj, getTextureCountMethodID);    
    if (!textureCount)
    {
        LOG("getTextureCount() returned zero.");
        return;
    }

    textures = new Texture*[textureCount];

    jmethodID getTextureMethodID = env->GetMethodID(activityClass,
        "getTexture", "(I)Lcom/qualcomm/QCARSamples/FrameMarkers/Texture;");

    if (getTextureMethodID == 0)
    {
        LOG("Function getTexture() not found.");
        return;
    }

    // Register the textures
    for (int i = 0; i < textureCount; ++i)
    {

        jobject textureObject = env->CallObjectMethod(obj, getTextureMethodID, i); 
        if (textureObject == NULL)
        {
            LOG("GetTexture() returned zero pointer");
            return;
        }

        textures[i] = Texture::create(env, textureObject);
    }
}


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_deinitApplicationNative(
                                                        JNIEnv* env, jobject obj)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_deinitApplicationNative");

    // Release texture resources
    if (textures != 0)
    {    
        for (int i = 0; i < textureCount; ++i)
        {
            delete textures[i];
            textures[i] = NULL;
        }
    
        delete[]textures;
        textures = NULL;
        
        textureCount = 0;
    }
}


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_startCamera(JNIEnv *,
                                                                         jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_startCamera");

    // Initialize the camera:
    if (!QCAR::CameraDevice::getInstance().init())
        return;

    // Configure the video background
    configureVideoBackground();

    // Select the default mode:
    if (!QCAR::CameraDevice::getInstance().selectVideoMode(
                                QCAR::CameraDevice::MODE_OPTIMIZE_QUALITY))
        return;

    // Start the camera:
    if (!QCAR::CameraDevice::getInstance().start())
        return;

    // Start the tracker:
    QCAR::Tracker::getInstance().start();
    
    // Cache the projection matrix:
    const QCAR::Tracker& tracker = QCAR::Tracker::getInstance();
    const QCAR::CameraCalibration& cameraCalibration =
                                    tracker.getCameraCalibration();
    projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f,
                                            2000.0f);
}


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_stopCamera(JNIEnv *,
                                                                   jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkers_stopCamera");

    QCAR::Tracker::getInstance().stop();

    QCAR::CameraDevice::getInstance().stop();
    QCAR::CameraDevice::getInstance().deinit();
}


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_initRendering(
                                                    JNIEnv* env, jobject obj)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_initRendering");

    // Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, QCAR::requiresAlpha() ? 0.0f : 1.0f);
    
    // Now generate the OpenGL texture objects and add settings
    for (int i = 0; i < textureCount; ++i)
    {
        glGenTextures(1, &(textures[i]->mTextureID));
        glBindTexture(GL_TEXTURE_2D, textures[i]->mTextureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->mWidth,
                textures[i]->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                (GLvoid*)  textures[i]->mData);
    }
  
    shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                            cubeFragmentShader);

    vertexHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexPosition");
    normalHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexNormal");
    textureCoordHandle  = glGetAttribLocation(shaderProgramID,
                                                "vertexTexCoord");
    mvpMatrixHandle     = glGetUniformLocation(shaderProgramID,
                                                "modelViewProjectionMatrix");

}


JNIEXPORT void JNICALL
Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_updateRendering(
                        JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_qualcomm_QCARSamples_FrameMarkers_FrameMarkersRenderer_updateRendering");
    
    // Update screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}

#ifdef __cplusplus
}
#endif

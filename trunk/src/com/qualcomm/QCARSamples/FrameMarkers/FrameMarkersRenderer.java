/*==============================================================================
            Copyright (c) 2010 QUALCOMM Incorporated.
            All Rights Reserved.
            Qualcomm Confidential and Proprietary
            
@file 
    FrameMarkersRenderer.java

@brief
    Sample for FrameMarkers

==============================================================================*/


package com.qualcomm.QCARSamples.FrameMarkers;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLSurfaceView;
import android.util.Log;

import com.qualcomm.QCAR.QCAR;


/** The renderer class for the FrameMarkers sample. */
public class FrameMarkersRenderer implements GLSurfaceView.Renderer
{
	private AudioMgr TrackPlayer;
	 
    public boolean mIsActive = false;
    
    /** Native function for initializing the renderer. */
    public native void initRendering();
    
    
    /** Native function to update the renderer. */
    public native void updateRendering(int width, int height);

    
    /** Called when the surface is created or recreated. */
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        DebugLog.LOGD("GLRenderer::onSurfaceCreated");

        // Call native function to initialize rendering:
        initRendering();
        
        // Call QCAR function to (re)initialize rendering after first use
        // or after OpenGL ES context was lost (e.g. after onPause/onResume):
        QCAR.onSurfaceCreated();
        
        TrackPlayer = new AudioMgr();
    }
    
    
    /** Called when the surface changed size. */
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
        DebugLog.LOGD("GLRenderer::onSurfaceChanged");
        
        // Call native function to update rendering when render surface parameters have changed:
        updateRendering(width, height);

        // Call QCAR function to handle render surface size changes:
        QCAR.onSurfaceChanged(width, height);
    }    
    
    
    /** The native render function. */    
    public native int[] renderFrame();
    
    
    /** Called to draw the current frame. */
    public void onDrawFrame(GL10 gl)
    {
        if (!mIsActive)
            return;
        
        // Call our native function to render content
        float vol = TrackPlayer.getVolume();
        //Log.d("VolTest", Float.toString(vol));
        int[] test = renderFrame();
        
        /*
        for(int i = 0; i < test.length;++i)
        	Log.d("NMLAB","id: "+test[i]);*/
        
        TrackPlayer.GetData(test);
    }
}

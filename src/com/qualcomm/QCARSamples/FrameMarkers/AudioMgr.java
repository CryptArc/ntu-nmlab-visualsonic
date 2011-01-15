package com.qualcomm.QCARSamples.FrameMarkers;



import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

public class AudioMgr {
    private short[][] Track_pcm = new short[9][];
    //private boolean[] Track_enable = new boolean[11];
    private int[] Track_vol = new int[11];
    
    private AudioTrack AudioTrack01 = null;
    private int FileLength = 219392*2;
    private int SampleRate = 32000;
    private short[] TrackBuf1;
    private short[] TrackBuf2;
    private int CurTrack = 1;
    
    private Thread aThread;
    private Thread rThread;
    
    private boolean a_running = true;
    private boolean a_reset = false;
    private boolean r_reprocess = false;
    private boolean r_processing = false;
    private boolean r_needReprocess = false;
    
    
    public int testVolume = 0;
    public native void trackInit();
    
    public AudioMgr()
    {
    	trackInit();
        aThread = new Thread(){
        	public void run(){
        		PlayAudio();
        	}
        };
        
        rThread = new Thread(){
        	public void run(){
        		ReProcess();
        	}
        };
        TrackBuf1 = new short[FileLength/2];
		TrackBuf2 = new short[FileLength/2];
		aThread.start();
    }
    
    public void GetData(int[] vol)
    {
		int i;
		for(i = 0; i < vol.length; ++i){
			if(vol[i] != Track_vol[i])
				break;
		}
		if(i == vol.length) return;
    	Track_vol = vol;
    	
		if(r_processing)
			r_needReprocess = true;
		else{
			rThread = new Thread(){
				public void run(){
	        		ReProcess();
	        	}
			};
			rThread.start();
		}
		System.gc();
    }
    
    private short[] buffer;
    private int v_offset;
    private int v_writelen;
    
    public native void setClock();
    public native float getVolumeFromBuf(int offset, int writelen, short[] buf, int bufsize);
    
    public float getVolume()
    {
    	return getVolumeFromBuf(v_offset, v_writelen, buffer, buffer.length);
    }
    
    public void PlayAudio(){
    	buffer = TrackBuf1;
    	int iMinBufSize = AudioTrack.getMinBufferSize(32000,  
                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                        AudioFormat.ENCODING_PCM_16BIT);
        AudioTrack01 = new AudioTrack(AudioManager.STREAM_MUSIC, 32000, 
                AudioFormat.CHANNEL_CONFIGURATION_MONO, 
                AudioFormat.ENCODING_PCM_16BIT, 
                iMinBufSize*2, 
                AudioTrack.MODE_STREAM);
        
        AudioTrack01.setStereoVolume(1,1);
        int offset = 0;
        int writelen = 0;
        while(a_running)
        {
        	if(a_reset){
        		a_reset = false;
        	}
        	if(r_reprocess){
        		if(CurTrack == 1)
        			buffer = TrackBuf1;
        		else
        			buffer = TrackBuf2;
        		r_reprocess = false;
        	}
        	
        	writelen = AudioTrack01.write(buffer, offset, iMinBufSize);
        	/*voltest*/
        	setClock();
        	v_offset = offset;
        	v_writelen = writelen;
        	/*voltest*/
        	if(writelen  > 0)
        		AudioTrack01.play();

        	offset += writelen ; 
        	if(writelen < iMinBufSize || offset == FileLength/2)
        		offset = 0;
        }
    }
    public native void audioMixer(int[] track_vol, short[] Track);
    public void ReProcess()
    {
    	r_processing = true;
    	short[] Track;
    	if(CurTrack == 1){
    		Track = TrackBuf2;
    		CurTrack = 2;
    	}
    	else{
    		Track = TrackBuf1;
    		CurTrack = 1;
    	}
    	audioMixer(Track_vol, Track);
    	int newRate = 32000 + 4000*(Track_vol[9] - 4);
    	if(newRate != SampleRate){
    		SampleRate = newRate;
    		AudioTrack01.setPlaybackRate(SampleRate);
    	}

    	r_reprocess = true;
    	if(r_needReprocess){
    		r_needReprocess = false;
    		ReProcess();
    	}else{
    		r_processing = false;
    	}
    }
}

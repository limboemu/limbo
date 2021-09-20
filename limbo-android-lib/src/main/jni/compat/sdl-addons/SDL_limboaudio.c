/*
Copyright (C) Max Kastanas 2012

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// Requires minSdkVersion 26+
#if defined(__ENABLE_AAUDIO__)
#include <jni.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <semaphore.h>
#include <aaudio/AAudio.h>
#include "SDL_limboaudio.h"

// currently no need to resample unless we have a sample rate 
// that aaudio cannot handle, if so we prefer 22050
int enableAaudioResample = 0;
//float aaudioResampleRate = 11025.0;
float aaudioResampleRate = 22050.0;
// we drop the frames you can use 0 to do a median filter instead
int aaudioDropFrames = 1;

int enableAaudio = 0;

// FIXME: buggy
int enableAaudioHighPriority = 0;

// LIMBO: we use aaudio to remove the JNI calls to AudioTrack
// We also try to use a lower sample rate for better performance
// Make sure you update: 
// the freq in qemu driver located in qemu/audio/sdlaudio.c with 22050
// and optionally the buffer frames in qemu/audio/audio.c
AAudioStreamBuilder *builder;
AAudioStream *stream;
int aaudioBurstFrames;
int aaudioCapacityFrames;
int aaudioFrames;
int aaudioChannels;

// high priority aaudio callback
short* aaudioBuffer;
int aaudioBufferStart = 0;
int aaudioBufferEnd = 0;
int aaudioBufferSize = 0;
int aaudioMidBufferSize = 0;

short * aaudioMidBuffer = NULL;
short * aaudioBuffer = NULL;

int aaudioResampleStep = 0;
int aaudioResampleFrames = 0;

sem_t mutex;

int isAaudioEnabled() {
    return enableAaudio;
}

// FIXME: this is buggy, though since the aaudio write function is
// fast enough we don't bother for now
aaudio_data_callback_result_t aaudio_callback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {
        	sem_wait(&mutex);
    /*	
	printf("bytes writting, numFrames = %d, aaudioBufferSize = %d "
	",aaudioBufferStart = %d, aaudioBufferEnd = %d\n", numFrames, "
    "aaudioBufferSize, aaudioBufferStart, aaudioBufferEnd);
    */				
	int bytesWritten = 0;
    for(int i=0; i<numFrames*aaudioChannels; i++) {
    	if(aaudioBufferStart >= aaudioBufferSize)
    		aaudioBufferStart = 0;
    	if(aaudioBufferStart < aaudioBufferEnd) {
    		((short*)audioData)[i] = aaudioBuffer[aaudioBufferStart++];
    		bytesWritten++;
    	} else {
	    	break;
    	}
    }
    /*
    printf("bytes written, bytesWritten = %d, aaudioBufferSize = %d"
    ",aaudioBufferStart = %d, aaudioBufferEnd = %d\n", "
    "bytesWritten, aaudioBufferSize, aaudioBufferStart, aaudioBufferEnd);
    */				
    sem_post(&mutex);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
     
void createAAudioDevice(int sampleRate, int channelCount, int desiredBufferFrames){	
    // Does this prevent the vm from crashing with a stackoverflowerror?
	sleep(1);
	AAUDIO_API aaudio_result_t res = AAudio_createStreamBuilder(&builder);
	if(res != AAUDIO_OK){
		printf("Error while creating builder: %s\n", AAudio_convertResultToText(res));	
	}
    if(enableAaudioResample)
        printf("requested resampling rate: %f\n", aaudioResampleRate);
    else
        printf("requested sampling rate: %d\n", sampleRate);
	AAudioStreamBuilder_setSampleRate(builder, enableAaudioResample?aaudioResampleRate:sampleRate);
	AAudioStreamBuilder_setChannelCount(builder, channelCount);
	AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
	//AAudioStreamBuilder_setBufferCapacityInFrames(builder, desiredBufferFrames);
	AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
	if(enableAaudioHighPriority)
		AAudioStreamBuilder_setDataCallback(builder, aaudio_callback, aaudioBuffer);

	res = AAudioStreamBuilder_openStream(builder, &stream);	
	if(res != AAUDIO_OK){
		printf("Error while opening stream: %s\n", AAudio_convertResultToText(res));	
	}
	
	printf("Stream deviceId: %d\n", AAudioStream_getDeviceId(stream));
	printf("Stream direction: %d\n", AAudioStream_getDirection(stream));
	
	AAUDIO_API aaudio_sharing_mode_t sharingMode = AAudioStream_getSharingMode(stream);
	if(sharingMode != AAUDIO_SHARING_MODE_SHARED)	
		printf("Stream sharingMode invalid: %d\n", sharingMode);
		
    aaudioResampleRate = AAudioStream_getSampleRate(stream);
	printf("Stream sampleRate: %f\n", aaudioResampleRate);
	aaudioChannels = AAudioStream_getChannelCount(stream);
	printf("Stream channelCount: %d\n", aaudioChannels);

	aaudioBurstFrames = AAudioStream_getFramesPerBurst(stream);
	printf("Got optimal numFrames: %d\n", aaudioBurstFrames);
	
	AAUDIO_API aaudio_format_t dataFormat = AAudioStream_getFormat(stream);
	if (dataFormat != AAUDIO_FORMAT_PCM_I16) {
    	printf("Stream format invalid: %d\n", dataFormat);	
	}
	
	aaudioCapacityFrames = AAudioStream_getBufferCapacityInFrames(stream);
	printf("Stream frames capacity: %d\n", aaudioCapacityFrames);
	
    //TODO: we could use the optimal number of frames
    // only if supported by SDL
	// aaudioFrames = aaudioBurstFrames;
	aaudioFrames = desiredBufferFrames;
	
    sem_init(&mutex, 0, 1);
    printf("Samples: %d\n", aaudioFrames);
    printf("Channels: %d\n", aaudioChannels);
    
    // setup resampling
    if(enableAaudioResample) {
        printf("final resampling rate: %f\n", aaudioResampleRate);
        aaudioFrames = ceil(aaudioFrames * aaudioResampleRate / sampleRate);
        printf("resampled frames: %d\n", aaudioFrames);
        aaudioResampleStep = sampleRate / aaudioResampleRate;
        printf("resample step: %d\n", aaudioResampleStep);
    }
    
    aaudioBufferSize = aaudioFrames * aaudioChannels;
    
    //TODO: high priority callback
    // we allocate 5 times more capacity to be safe
    if(enableAaudioHighPriority) {
        aaudioBufferSize = aaudioBufferSize * 5;
    }
    // This buffer is for receiving the data
    aaudioMidBufferSize = desiredBufferFrames * aaudioChannels;
    aaudioMidBuffer = (short*) calloc(aaudioMidBufferSize, sizeof(short));
    
    // This is for resampling
    aaudioBuffer = (short*) calloc(aaudioBufferSize, sizeof(short));
            
    printf("aaudio final frames: %d\n", aaudioFrames);
    printf("aaudio final buffer size: %d\n", aaudioBufferSize);
        
	res = AAudioStream_requestStart(stream);
	if(res != AAUDIO_OK){
		printf("Error while starting stream: %d\n", res);	
	}
	printf("Started playing\n");
}

void destroyAaudioDevice() {
    sem_destroy(&mutex);
}

int isAaudioBufferEmpty() {
    for(int i=0; i<aaudioMidBufferSize; i++){
        if(aaudioMidBuffer[i] != 0)
            return false;
    }
    return true;
}

int batchCount = 0;
void resampleAaudio() {
    int batch = batchCount++;
    //printf("resampling batch: %d, step: %d, bufferSize: %d\n", batch, aaudioResampleStep, aaudioBufferSize);
    memset(aaudioBuffer, 0, aaudioBufferSize);
    int sum = 0; 
    int oldFrame = 0;
    int sampleCount = aaudioDropFrames?1:aaudioResampleStep;
    for (int frame = 0; frame < aaudioBufferSize; frame+=aaudioChannels) {
        int oldPos = oldFrame;
        for (int channel = 0; channel < aaudioChannels; channel++) {
            sum = 0;
            for (int sample = 0; sample < aaudioResampleStep*aaudioChannels; sample+=aaudioChannels) {
                if(!aaudioDropFrames || sample == 0)
                    sum += aaudioMidBuffer[oldPos+sample];
                if(aaudioDropFrames)
                    break;
      //          printf("batch %d, adding amb[%d] = %d => sum = %d\n", batch,
      //              oldPos+sample, aaudioMidBuffer[oldPos+sample], sum);
            }
            aaudioBuffer[frame + channel] = sum / sampleCount;
            //printf("batch %d, set avg ab[%d] <= %d\n", batch,
                    //frame + channel, sum / sampleCount);
            oldPos++;
        }
        oldFrame += aaudioChannels * aaudioResampleStep ;
    }
}

void writeAaudio() {
    if(aaudioMidBuffer == NULL) {
            return;
        }
        if (enableAaudioHighPriority) {
            writeAaudioQueue();
        } else {
            writeAaudioStream();
        }
}

void writeAaudioStream() {
    if(aaudioMidBuffer == NULL)
        return;
    int index = 0;
    int res = 0;
    // FIXME: Unfortunately we need to block here for at least 1 ms so 
    // sdl and qemu will feed us new data. Perhaps there is an issue 
    // in sdl or qemu.
    if(enableAaudioResample) {
        resampleAaudio();
        res = AAudioStream_write(stream, (short*) aaudioBuffer, aaudioFrames, 1);
    } else {
        res = AAudioStream_write(stream, (short*) aaudioMidBuffer, aaudioFrames, 1);
    }
    if(res < 0) {
        printf("Error writting: %s\n", AAudio_convertResultToText(res));
    } else {
        // printf("Frames written: %d\n", res);
    }
}

void writeAaudioQueue() {
    // LIMBO: we qeueue the data to our cyclical buffer 
    // which is used by a high priority aaudio callback
    // that consumes the data
    // NEEDS TESTING
    if(aaudioMidBuffer == NULL) {
        printf("aaudio stream not ready\n");
        return;
    }
    sem_wait(&mutex);
    /*
    printf("bytes reading from buffer, aaudioFrames = %d, aaudioBufferSize = %d "
    ", aaudioBufferStart = %d, aaudioBufferEnd = %d\n",
        aaudioFrames, aaudioBufferSize, aaudioBufferStart, aaudioBufferEnd				 
      );
    */				
    // TODO: resample and copy at the same time?
    resampleAaudio();
    int bytesRead = 0;
    for(int i=0; i<aaudioFrames * aaudioChannels; i++) {
        if(aaudioBufferEnd >= aaudioBufferSize) {
            //printf("wrapping around aaudioBufferEnd\n");
            aaudioBufferEnd = 0;
        }
        //printf("reading aaudioBufferEnd = %d\n" , aaudioBufferEnd);
        aaudioBuffer[aaudioBufferEnd++] = ((short*)  aaudioBuffer)[i];
        bytesRead++;
        }
        // artificially wait
        usleep(7 * 1000);
     /*
     printf("bytes wrote to queue, bytesRead = %d, aaudioBufferSize = %d " 
     ", aaudioBufferStart = %d, aaudioBufferEnd = %d\n",
        bytesRead, aaudioBufferSize, aaudioBufferStart, aaudioBufferEnd);
        */
    
    sem_post(&mutex);
}

void* getAaudioBuffer() {
    return aaudioMidBuffer;
}

JNIEXPORT void JNICALL Java_com_max2idea_android_limbo_jni_VMExecutor_nativeEnableAaudio(
        JNIEnv* env, jobject thiz, 
        int value) {
            printf("set enable aaudio: %d\n", value);
    enableAaudio = value;
}

#endif

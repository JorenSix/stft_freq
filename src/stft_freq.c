// STFT_FREQ
// Copyright (C) 2019-2020  Joren Six

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <libgen.h>
#include <math.h>

#include "pffft.h"
#include "stft_freq_reader.h"

void gaussian_window(float * window,size_t window_length, double sigma){
	double length = window_length;
	double offset = (length -1) / 2.0;
	for(size_t i = 0 ; i < window_length; i++){
		double n = i - offset;
		double x = n / sigma;
		x = -0.5 * x * x;
		window[i] = exp(x);
	}
}

void maxFilter(float* data, float * max, int length,int halfFilterSize ){
	for(int i = 0 ; i < length;i++){
		size_t startIndex = i - halfFilterSize > 0 ? i - halfFilterSize : 0;
		size_t stopIndex = i + halfFilterSize < length ? i + halfFilterSize + 1: length;
		max[i] = -100000;
		for(size_t j = startIndex ; j < stopIndex; j++){
			if(data[j]>max[i])
				max[i]=data[j];
		}
	}
}

inline int max ( int a, int b ) { return a > b ? a : b; }
inline int min ( int a, int b ) { return a < b ? a : b; }

int main(int argc, const char* argv[]){

	if(argc != 7){
		puts("stft_freq audio_sr block_size step_size min_freq max_freq audio_file.raw");
		exit(-10);
		return -10;
	}

	uint32_t audio_sample_rate = strtoul(argv[1],NULL,0);
	size_t audio_block_size = strtoul(argv[2],NULL,0);
	size_t half_audio_block_size = audio_block_size/2;
	size_t audio_step_size = strtoul(argv[3],NULL,0);
	size_t min_freq = strtoul(argv[4],NULL,0);
	size_t max_freq = strtoul(argv[5],NULL,0);
	size_t byte_per_audio_sample = 4;
	
	//audio reader
	STFTFreq_Reader *reader = stft_freq_reader_new(argv[6],audio_block_size,audio_step_size);

	//the samples should be a 32bit float
	size_t bytes_per_audio_block = audio_block_size * byte_per_audio_sample;
	//initialize the pfft object
	// We will use a size of audioblocksize 
	// We are only interested in real part
	PFFFT_Setup *fft_setup = pffft_new_setup(audio_block_size,PFFFT_REAL);

	float* fft_in = (float *) pffft_aligned_malloc(bytes_per_audio_block);//fft input
	float* fft_out= (float *) pffft_aligned_malloc(bytes_per_audio_block);//fft output
	float* audio_data = (float *) malloc(bytes_per_audio_block);//audio block
	float* fft_window = (float *) malloc(bytes_per_audio_block);//fft window
	float* magnitudes = (float *) malloc(bytes_per_audio_block/2);//magnitudes
	float* max_magnitudes = (float *) malloc(bytes_per_audio_block/2);//max filtered magnitudes

	//create a gaussian window with sigma equal to a seventh of the size
	double sigma = audio_block_size/7.0;
	gaussian_window(fft_window,audio_block_size,sigma);	

	size_t tot_samples_read = 0;

	clock_t start, end;
    double cpu_time_used;
    start = clock();

    //read the first audio block
    size_t samples_read = stft_freq_reader_read(reader,audio_data);
	tot_samples_read += samples_read;

	size_t start_index = (min_freq * audio_block_size) / audio_sample_rate;
	size_t stop_index = (max_freq * audio_block_size) / audio_sample_rate;

	size_t audio_block_start_sample = 0;
	while(samples_read==audio_block_size){

		audio_block_start_sample = stft_freq_current_sample(reader);
		samples_read = stft_freq_reader_read(reader,audio_data);
		
		// windowing + copy to fft input
		for(size_t j = 0 ; j <  audio_block_size ; j++){
			fft_in[j] = audio_data[j] * fft_window[j];
		}

		//printf("%f \n",audio_data[1]);
		//do the transform
		pffft_transform_ordered(fft_setup, fft_in, fft_out, 0, PFFFT_FORWARD);

		//calculate the magnitudes
		size_t magnitude_index = 0;
		for(size_t j = 0 ; j < audio_block_size ; j+=2){
		
			//do not regard any frequencies outside the range set by min_freq and max_freq
			//set them to zero
			if(magnitude_index < start_index)
				magnitudes[magnitude_index] = 0;
			else if(magnitude_index > stop_index)
				magnitudes[magnitude_index] = 0;
			else
				magnitudes[magnitude_index] = fft_out[j] * fft_out[j] + fft_out[j+1] * fft_out[j+1];
			
			magnitude_index++;
		}

		maxFilter(magnitudes,max_magnitudes,half_audio_block_size,half_audio_block_size-1);

		for(size_t fft_bin = 0 ; fft_bin < half_audio_block_size; fft_bin++){
			//printf("%zu  mag %f max %f\n",fft_bin,magnitudes[fft_bin],max_magnitudes[fft_bin]);
			if(magnitudes[fft_bin] == max_magnitudes[fft_bin]){
				float ym1, y0, yp1;
		
				ym1 = magnitudes[max(0,fft_bin-1)];
				y0 = magnitudes[fft_bin];
				yp1 = magnitudes[min(half_audio_block_size-1,fft_bin+1)];

				float fractional_offset = log(yp1/ym1)/(2*log((y0 * y0) / (yp1 * ym1)));
				float fractional_frequency_bin = fft_bin + fractional_offset;
				float freq_in_hz = fractional_frequency_bin * (float) audio_sample_rate / (float) audio_block_size;
				float time_in_s = audio_block_start_sample / (float) audio_sample_rate;

				printf("%f,%f,%f\n",time_in_s,freq_in_hz,magnitudes[fft_bin]);
			}
		}

		tot_samples_read += samples_read;
	}

	//for timing statistics
	end = clock();
	//fprintf(stderr,"end %lu \n",end);
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    double audioDuration = (double) tot_samples_read / (double) audio_sample_rate;
    double ratio = audioDuration / cpu_time_used;
    fprintf(stderr,"Proccessed %.1fs in %.3fs (%.0f times realtime) \n", audioDuration,cpu_time_used,ratio);

    stft_freq_reader_destroy(reader);

    //free data blocks
    free(magnitudes);
    free(max_magnitudes);
	free(fft_window);
	free(audio_data);

	//cleanup fft structures
	pffft_aligned_free(fft_in);
	pffft_aligned_free(fft_out);
	pffft_destroy_setup(fft_setup);
	
	return 0;
}

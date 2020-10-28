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

#include <stdlib.h>
#include <stdio.h>

#include "stft_freq_reader.h"

struct STFTFreq_Reader{
	//the file size in samples
	//a float is expected to be 4 bytes
	size_t file_size_in_samples;

	size_t audio_block_size;
	size_t step_size;

	//the index of the current sample being read
	//it should be smaller than file_size_in_samples
	size_t current_sample_index;

	//the audio data itself
	float* audio_data;
};

size_t min(size_t a, size_t b){
	if(a > b)
		return b;
	return a;
}

//reads the complete audio file from disk
STFTFreq_Reader * stft_freq_reader_new(const char * source,size_t audio_block_size,size_t step_size){

	FILE *file;
	size_t fileSize;
	size_t result;

	file = fopen(source,"rb");  // r for read, b for binary

	if (file==NULL) {
		fprintf(stderr,"Audio file %s not found or unreadable.\n",source);
		exit (1);
	}

	//determine the duration in samples
	fseek (file , 0 , SEEK_END);
	fileSize = ftell (file);
	rewind (file);

	STFTFreq_Reader *reader = (STFTFreq_Reader*) malloc(sizeof(STFTFreq_Reader));
	reader->file_size_in_samples = fileSize/sizeof(float);
	reader->current_sample_index = 0;
	reader->audio_block_size = audio_block_size;
	reader->step_size = step_size;

	// allocate memory to contain the whole file:
	reader->audio_data =  (float *)(malloc(fileSize));

	if (reader->audio_data == NULL) {fputs ("Not enough memory error",stderr); exit (2);}

	// copy the file into the audioData:
	result = fread (reader->audio_data,1,fileSize,file);
	if (result != fileSize) {fputs ("Reading error",stderr); exit (3);}
	fclose(file); // after reading the file to memory, close the file

	return reader;
}

//copy the next block 
size_t stft_freq_reader_read(STFTFreq_Reader *reader ,float * audio_block){
	size_t start_index = reader->current_sample_index;
	size_t stop_index = min(reader->current_sample_index + reader->audio_block_size,reader->file_size_in_samples);

	//start from the middle of the array
	size_t number_of_samples_read = 0;
	for(size_t i = start_index ; i < stop_index ; i++){
		audio_block[number_of_samples_read] = reader->audio_data[i];
		number_of_samples_read++;
	}

	//When reading the last buffer, make sure that the block is zero filled
	for(size_t i = number_of_samples_read ; i < reader->audio_block_size; i++){
		audio_block[i] = 0;
	}

	reader->current_sample_index += reader->step_size;

	return number_of_samples_read;
}

size_t stft_freq_current_sample(STFTFreq_Reader *reader){
	return reader->current_sample_index;
}

//free the claimed resources
void stft_freq_reader_destroy(STFTFreq_Reader *  reader){
	free(reader->audio_data);
	free(reader);
}

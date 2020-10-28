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
#ifndef STFT_FREQ_READER_H
#define STFT_FREQ_READER_H

typedef struct STFTFreq_Reader STFTFreq_Reader;

STFTFreq_Reader * stft_freq_reader_new(const char * source,size_t audio_block_size,size_t step_size);


size_t stft_freq_reader_read(STFTFreq_Reader * reader,float * audio_block);

//which is the current sample (end of block)?
size_t stft_freq_current_sample(STFTFreq_Reader *reader);

// free up memory and release resources
void stft_freq_reader_destroy(STFTFreq_Reader * reader);


#endif // STFT_FREQ_READER_H

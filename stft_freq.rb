#!/usr/bin/env ruby

# STFT freq
# Copyright (C) 2019-2020  Joren Six

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.

# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


require 'json'
require 'fileutils'
require 'tempfile'
require 'open3'

#change configuration here to modify frequency estimation
AUDIO_SAMPLE_RATE = 48000
AUDIO_BLOCK_SIZE = 128
AUDIO_BLOCK_STEP_SIZE = 64
MIN_FREQ = 300
MAX_FREQ = 24000


EXECUTABLE_LOCATION = "/usr/local/bin/stft_freq_c"
ALLOWED_AUDIO_FILE_EXTENSIONS = "**/*.{m4a,wav,mp4,wv,ape,ogg,mp3,flac,wma,M4A,WAV,MP4,WV,APE,OGG,MP3,FLAC,WMA}"
AUDIO_CONVERT_COMMAND = "ffmpeg -hide_banner -y -loglevel panic  -i \"__input__\" -ac 1 -ar 48000 -f f32le -acodec pcm_f32le \"__output__\""

#expand the argument to a list of files to process.
# a file is simply added to the list
# a text file is read and each line is interpreted as a path to a file
# for a folder each audio file within that folder (and subfolders) is added to the list
def audio_file_list(arg,files_to_process)
	arg = File.expand_path(arg)
	if File.directory?(arg)
		audio_files_in_dir = Dir.glob(File.join(arg,ALLOWED_AUDIO_FILE_EXTENSIONS))
		audio_files_in_dir.each do |audio_filename|
			files_to_process << audio_filename
		end
	elsif File.extname(arg).eql? ".txt"
		audio_files_in_txt = File.read(arg).split("\n")
		audio_files_in_txt.each do |audio_filename|
			audio_filename = File.expand_path(audio_filename)
			if File.exists?(audio_filename)
				files_to_process << audio_filename
			else
				STDERR.puts "Could not find: #{audio_filename}"
			end
		end
	elsif File.exists? arg
		files_to_process << arg
	else
		STDERR.puts "Could not find: #{arg}"
	end
	files_to_process
end

def with_converted_audio(audio_filename_escaped)
	tempfile = Tempfile.new(["audio_#{rand(20000)}", '.raw'])
	convert_command = AUDIO_CONVERT_COMMAND
	convert_command = convert_command.gsub("__input__",audio_filename_escaped)
	convert_command = convert_command.gsub("__output__",tempfile.path)
	system convert_command

	yield tempfile

	#remove the temp file afer use
	tempfile.close
	tempfile.unlink
end

def escape_audio_filename(audio_filename)
	begin
		audio_filename.gsub(/(["])/, '\\\\\1')
	rescue
		puts "ERROR, probably invalid byte sequence in UTF-8 in #{audio_filename}"
		return nil
	end
end

def process(index,length,audio_filename)
	audio_filename_escaped = escape_audio_filename(audio_filename)
	return unless audio_filename_escaped
	
	#Do not store same audio twice
	with_converted_audio(audio_filename_escaped) do |tempfile|
		out = `#{EXECUTABLE_LOCATION} #{AUDIO_SAMPLE_RATE} #{AUDIO_BLOCK_SIZE} #{AUDIO_BLOCK_STEP_SIZE} #{MIN_FREQ} #{MAX_FREQ} \"#{tempfile.path}\"`
		puts out
	end
end

audio_files = Array.new
ARGV.each do |audio_argument|
	audio_files = audio_file_list(audio_argument,audio_files)
end

audio_files.each_with_index do |audio_file, index|
	process(index,audio_files.size,audio_file)
end


compile:
	gcc -c src/pffft.c 				-W -Wall -std=gnu11 -pedantic -O2 #pfft needs M_PI and other constants not in the ANSI c standard
	gcc -c src/stft_freq_reader_single.c -W -Wall -std=c11 -pedantic -O2
	gcc -c src/stft_freq.c 			-W -Wall -std=c11 -pedantic -O2
	mkdir -p bin
	gcc -o bin/stft_freq_c *.o 		-lc -lm -ffast-math -pthread

clean:
	-rm -f *.o
	-rm -f bin/*

install:
	sudo cp bin/stft_freq_c /usr/local/bin/stft_freq_c
	sudo chmod +x /usr/local/bin/stft_freq_c
	sudo cp stft_freq.rb /usr/local/bin/stft_freq
	sudo chmod +x /usr/local/bin/stft_freq

uninstall:
	sudo rm /usr/local/bin/stft_freq /usr/local/bin/stft_freq_c

INCLUDE_DIRECTORIES = -I/opt/homebrew/Cellar/opencv/4.7.0_4/include/opencv4/ \
                      -I/opt/homebrew/Cellar/portaudio/19.7.0/include/ \
                      -I/opt/homebrew/Cellar/fftw/3.3.10_1/include/

CXXFLAGS = -O3 -std=c++11

LIBS = -L/opt/homebrew/Cellar/opencv/4.7.0_4/lib/ \
       -L/opt/homebrew/Cellar/portaudio/19.7.0/lib/ \
       -L/opt/homebrew/Cellar/fftw/3.3.10_1/lib/ \
       -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs \
	   -lportaudio -lfftw3 -lm

OBJS = main.o Spectrogram.o

all: voice_tableau

voice_tableau: $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS)
	rm -rf *.o

%.o: %.cpp
	g++ -c -pipe $(CXXFLAGS) $(INCLUDE_DIRECTORIES) $< -o $@

clean:
	rm -rf *.o voice_tableau
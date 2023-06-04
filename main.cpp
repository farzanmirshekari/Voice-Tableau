#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#include <portaudio.h>

#include "Spectrogram.h"
#include "utilities.h"
#include "constants.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

int main() {
    int i = 0, j = 0, k = 0;

    PaStreamParameters input_parameters;
    PaError err;
    PaStream *stream;

    err = Pa_Initialize();
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    input_parameters.device = Pa_GetDefaultInputDevice();
    if (input_parameters.device == paNoDevice) {
        Pa_Terminate();
        return 1;
    }

    input_parameters.channelCount = 1;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = Pa_GetDeviceInfo(input_parameters.device)->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(&stream, &input_parameters, NULL, SAMPLE_RATE, 1024, paClipOff, NULL, NULL);
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    if (RESERVE_SIZE < 0) {
        return 1;
    }

    int length = SPECTROGRAM_HEIGHT * (SAMPLE_RATE / 20 / SPECTROGRAM_HEIGHT + 1);
    Spectrogram *spectrogram = initialize(length);
    float **magnitude = (float **)calloc(SPECTROGRAM_WIDTH, sizeof(float *));
    for (i = 0; i < SPECTROGRAM_WIDTH; ++i) {
        if ((magnitude[i] = (float *)calloc(SPECTROGRAM_HEIGHT, sizeof(float))) == NULL) {
            return 1;
        }
    }

    float *clip = (float *)calloc(WINDOW_SIZE, sizeof(float));
    float *clip_fill_in_position = clip + RESERVE_SIZE;
    float *clip_step_in_position = clip + STEP_SIZE;
    int frame_index = 0;
    Pa_ReadStream(stream, clip, RESERVE_SIZE);

    cv::Mat image(SPECTROGRAM_HEIGHT, SPECTROGRAM_WIDTH, CV_8UC3);
    unsigned char *image_data = image.data;
    unsigned char colour[3] = {0, 0, 0};

    while (true) {
        Pa_ReadStream(stream, clip_fill_in_position, STEP_SIZE);

        for (j = 0; j < SPECTROGRAM_WIDTH; ++j) {
            int data_length = 2 * length;
            double *data = spectrogram->time_domain;
            for (i = 0; i < data_length; ++i) {
                data[i] = 0.0;
            }

            int start = (j * WINDOW_SIZE) / SPECTROGRAM_WIDTH - length;
            if (start >= 0) {
                int copy_length = 0;
                if (start + data_length > WINDOW_SIZE) {
                    copy_length = WINDOW_SIZE - start;
                }
                else {
                    copy_length = data_length;
                }
                for (i = 0; i < copy_length; ++i) {
                    data[i] = clip[i + start];
                }
            }
            else {
                start = -start;
                data += start;
                data_length -= start;
                for (i = 0; i < data_length; ++i) {
                    data[i] = clip[i];
                }
            }
            get_magnitude(spectrogram);
            map_spectrogram_to_magnitude(magnitude[j], SPECTROGRAM_HEIGHT, spectrogram->magnitude, length, MINIMUM_FREQUENCY, MAXIMUM_FREQUENCY, SAMPLE_RATE);
        }

        for (j = 0; j < SPECTROGRAM_WIDTH; ++j) {
            for (k = 0; k < SPECTROGRAM_HEIGHT; ++k) {
                magnitude[j][k] /= 100.0;
                magnitude[j][k] = (magnitude[j][k] < pow(10.0, FLOOR_DECIBELS / 20.0)) ? FLOOR_DECIBELS : 20.0 * log10(magnitude[j][k]);
                colour_map(magnitude[j][k], FLOOR_DECIBELS, colour);
                image_data[((SPECTROGRAM_HEIGHT - 1 - k) * image.cols + j) * 3] = colour[2];
                image_data[((SPECTROGRAM_HEIGHT - 1 - k) * image.cols + j) * 3 + 1] = colour[1];
                image_data[((SPECTROGRAM_HEIGHT - 1 - k) * image.cols + j) * 3 + 2] = colour[0];
            }
        }
        cv::imshow("Voice Tableau", image);
        unsigned char key = cv::waitKey(10);
        if (key == 27) {
            break;
        }
        for (i = 0; i < RESERVE_SIZE; ++i) {
            clip[i] = clip_step_in_position[i];
        }
        ++frame_index;
    }


    destroy(spectrogram);
    for (i = 0; i < SPECTROGRAM_WIDTH; ++i) {
        free(magnitude[i]);
    }
    free(magnitude);

    err = Pa_CloseStream(stream);
    if(err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    Pa_Terminate();
    free(clip);
    return 0;
}
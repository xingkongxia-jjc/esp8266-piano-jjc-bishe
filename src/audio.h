#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include "config.h"

// 音频设置变量
extern int noteDuration;
extern int octaveShift;

// 混音播放相关变量
extern volatile bool isMixPlaying;
extern volatile int mixKeys[12];
extern volatile int mixKeyCount;
extern volatile unsigned long mixStartTime;
extern volatile unsigned long lastMixUpdate;

// 基础音频频率数组
extern int baseFrequencies[];

// 音符名称数组
extern String keyString[];

// 函数声明
void initAudio();
int getNoteFrequency(int noteIndex);
void updateMixedTone();
void playMultipleNotes(int* keys, int keyCount);
void stopAllAudio();

#endif
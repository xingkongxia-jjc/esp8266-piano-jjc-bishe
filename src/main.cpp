#include <Arduino.h>
#include "config.h"
#include "audio.h"
#include "touch.h"
#include "display.h"
#include "pic.cpp"

void setup() {
  Serial.begin(115200);
  
  // 初始化各个模块
  initAudio();
  initTouch();
  initDisplay();
  
  // 显示启动画面
  showStartupScreen();
  
  delay(2000);
  
  // 显示初始钢琴界面
  displayMultipleKeys(0, 0);
}

void loop() {
  /* ========== 混音播放状态维护 ========== */
  if (isMixPlaying) {
    unsigned long currentMicros = micros();
    
    if (currentMicros - lastMixUpdate >= MIX_UPDATE_INTERVAL) {
      updateMixedTone();
      lastMixUpdate = currentMicros;
    }
  }
  
  /* ========== 熄屏检测 ========== */
  checkScreenTimeout();
  
  /* ========== 触摸控制检测 ========== */
  {
    unsigned long currentTime = millis();
    
    if (currentTime - lastSampleTime > SAMPLE_INTERVAL) {
      
      uint16_t keyValue = detectMultipleKeys();
      
      if (keyValue != previousKeys && currentTime - lastKeyTime > KEY_DEBOUNCE_TIME) {
        
        // 触摸时更新屏幕活动时间（包括亮屏）
        updateScreenActivity();
        
        currentKeys = keyValue;
        previousKeys = keyValue;
        lastKeyTime = currentTime;
        
        int pressedKeys[12];
        int keyCount = 0;
        
        parseKeys(keyValue, pressedKeys, &keyCount);
        
        // 显示按键
        displayMultipleKeys(pressedKeys, keyCount);
        
        if (keyCount > 0) {
          playMultipleNotes(pressedKeys, keyCount);
        } else {
          stopAllAudio();
        }
      }
      
      lastSampleTime = currentTime;
    }
  }
  
  /* ========== 中断驱动的触摸检测 ========== */
  if (iftouch) {
    iftouch = false;
    
    unsigned long currentTime = millis();
    
    if (currentTime - lastSampleTime > SAMPLE_INTERVAL) {
      uint16_t keyValue = detectMultipleKeys();
      
      if (keyValue != previousKeys) {
        
        // 触摸时更新屏幕活动时间（包括亮屏）
        updateScreenActivity();
        
        currentKeys = keyValue;
        previousKeys = keyValue;
        lastKeyTime = currentTime;
        
        int pressedKeys[12];
        int keyCount = 0;
        
        parseKeys(keyValue, pressedKeys, &keyCount);
        
        // 显示按键
        displayMultipleKeys(pressedKeys, keyCount);
        
        if (keyCount > 0) {
          playMultipleNotes(pressedKeys, keyCount);
        } else {
          stopAllAudio();
        }
      }
      
      lastSampleTime = currentTime;
    }
  }
}
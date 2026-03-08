# OLED 显示详解

这篇文档讲的是项目里 OLED 是怎么显示内容的，包括：

1. 开机画面怎么显示
2. 当前按键怎么显示
3. 教学模式怎么显示
4. 熄屏和亮屏怎么实现

## 1. 相关文件

OLED 相关实现主要在：

- `src/display.cpp`
- `src/display.h`
- `src/config.h`
- `src/audio.cpp`
- `src/touch.cpp`
- `src/network.cpp`

这里为什么会和 `audio.cpp`、`touch.cpp`、`network.cpp` 有关系？

因为显示屏上展示的内容，不只是图形，还包括：

- 当前按下的键名
- 八度信息
- 触摸灵敏度
- 网络地址
- 教学模式提示

所以显示模块会引用别的模块里的状态变量。

## 2. OLED 初始化是怎么做的

显示屏对象定义如下：

```cpp
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
```

这里用到了 `Adafruit_SSD1306` 库和 `Wire`。

说明两件事：

1. 这个屏是 SSD1306 控制器的 OLED。
2. 它也是通过 I2C 总线通信。

初始化函数：

```cpp
void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("not found oled ssd1306!");
  }
  lastActivityTime = millis();
  screenOn = true;
}
```

### 2.1 这里在做什么

- `display.begin(...)`：初始化 OLED 控制器
- `SCREEN_ADDRESS`：屏幕 I2C 地址，项目里是 `0x3C`
- 如果初始化失败，就通过串口打印错误信息
- 记录最近活动时间，为熄屏功能做准备

## 3. 为什么 OLED 显示要先“画”再“刷新”

很多初学者会以为：

“我调用 `drawRect()` 就已经显示了。”

其实不是。

这类库通常先把图像画在内存缓冲区里，最后你调用：

```cpp
display.display();
```

它才会真正把缓冲区内容发送到 OLED 屏幕上。

所以你会在代码中频繁看到这种模式：

```cpp
display.clearDisplay();
...各种 draw / print ...
display.display();
```

这个顺序非常重要。

## 4. 开机画面怎么显示

启动界面由 `showStartupScreen()` 负责：

```cpp
void showStartupScreen() {
  updateScreenActivity();
  display.clearDisplay();
  display.setRotation(2);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("EDA-Piano");
  display.setTextSize(1);
  display.println("v2.0 Network");
  display.println("Initializing...");
  display.display();
}
```

这里包含几个常见显示操作：

- `clearDisplay()`：清屏
- `setRotation(2)`：设置屏幕方向
- `setTextSize()`：设置文字大小
- `setCursor(x, y)`：设置文字起始位置
- `println()`：写文本
- `display()`：真正刷新到屏幕

## 5. 网络信息界面怎么显示

`showNetworkInfo()` 会显示：

- 项目名称
- 当前模式状态
- Web 地址
- 音符时长
- 当前八度

这说明 OLED 不只是“画钢琴”，它也承担系统状态面板的作用。

从产品角度看，这很合理，因为小屏幕正适合显示少量关键状态。

## 6. 正常弹奏时的钢琴界面怎么绘制

核心函数是 `drawPianoKeyboard(int* pressedKeys, int keyCount)`。

它的职责是：

1. 清空画面
2. 在上方显示当前状态文字
3. 在下方画出钢琴键盘
4. 把按下的键高亮显示

## 7. 先看顶部文字区

函数前半段会先判断当前有没有按键：

```cpp
if (keyCount == 0) {
  display.print("Oct:");
  ...
  display.print(" Sens:L");
  display.println(touchSensitivity);
} else {
  display.print("Key");
  display.print(": ");

  for (int i = 0; i < keyCount; i++) {
    display.print(keyString[pressedKeys[i]]);
    if (i < keyCount - 1) display.print(",");
  }
}
```

意思是：

- 如果当前没按键，就显示系统状态，比如八度和灵敏度
- 如果当前有按键，就显示按下的音名，例如 `C,E,G`

这里的 `keyString[]` 来自 `audio.cpp`，它把键号映射成音名字符串。

## 8. 键盘图形是怎么画出来的

### 8.1 先定义布局参数

```cpp
int keyboardY = 10;
int whiteKeyHeight = 22;
int blackKeyHeight = 14;
int keyWidth = 128 / 7;
```

这几个参数决定了键盘画在屏幕哪里、每个键多宽多高。

因为屏幕宽度是 128 像素，而白键一共有 7 个，所以每个白键宽度大约是 `128 / 7`。

### 8.2 为什么白键是 7 个，黑键是 5 个

一个完整的自然音阶是：

- 白键：C D E F G A B
- 黑键：C# D# F# G# A#

这刚好是：

- 7 个白键
- 5 个黑键

所以项目里用这两个数组来表示：

```cpp
int whiteKeys[] = {1, 3, 5, 6, 8, 10, 12};
int blackKeys[] = {2, 4, 7, 9, 11};
```

这是根据键号和音阶位置做的映射。

### 8.3 如何判断哪些键被按下

```cpp
bool keyPressed[13] = {false};

for (int i = 0; i < keyCount; i++) {
  if (pressedKeys[i] >= 1 && pressedKeys[i] <= 12) {
    keyPressed[pressedKeys[i]] = true;
  }
}
```

这里先创建一个布尔数组，相当于“按键状态表”。

例如：

- 如果键 1 被按下，就让 `keyPressed[1] = true`
- 如果键 8 被按下，就让 `keyPressed[8] = true`

后面画每个键的时候，只要查这个表就知道要不要高亮。

### 8.4 白键怎么画

```cpp
display.drawRect(keyX, keyboardY, keyWidth, whiteKeyHeight, SSD1306_WHITE);

if (keyPressed[keyNum]) {
  display.fillRect(keyX + 1, keyboardY + 1, keyWidth - 2, whiteKeyHeight - 2, SSD1306_WHITE);
}
```

逻辑是：

1. 先画白键边框
2. 如果这个键被按下，就把内部填充成白色
3. 再根据是否按下，调整文字颜色

这会形成一种明显的高亮效果。

### 8.5 黑键怎么画

黑键和白键不一样，因为黑键本来就应该是“实心”的视觉效果。

代码里做法是：

- 未按下时，先画白色块，再用黑色边框/内框表现黑键
- 按下时，直接把这个区域高亮显示

同时，黑键的位置也不是平均铺开的，而是通过 `blackKeyPositions[]` 来定义插在白键之间：

```cpp
int blackKeyPositions[] = {0, 1, 3, 4, 5};
```

这表示黑键分别位于某些白键之间，而不是每个白键后面都有黑键。

## 9. 教学模式显示做了什么

教学模式使用的是 `showTeachingMode()`。

它和普通键盘界面的思路很像，但重点不一样。

普通模式强调：

- “你现在按了什么”

教学模式强调：

- “你下一步应该按什么”

所以这个函数里会：

1. 根据 `message` 显示 `Good!`、`Error!` 或其他提示
2. 用 `nextNote` 高亮出下一步应该按的键

这就是一个很典型的人机交互设计：

- 输入正确，给正反馈
- 输入错误，给纠正提示
- 同时继续给视觉引导

## 10. 为什么触摸时会自动亮屏

显示模块里有一个统一函数：

```cpp
void updateScreenActivity() {
  lastActivityTime = millis();
  if (!screenOn) {
    turnOnScreen();
  }
}
```

每当发生这些事情时，程序通常会调用它：

- 触摸按键
- 显示界面切换
- 启动画面显示

它做两件事：

1. 记录“最近一次有人操作”的时间
2. 如果屏幕已经灭了，就重新打开

## 11. 熄屏功能怎么实现

项目支持多种熄屏时间：

- 不熄屏
- 10 秒
- 1 分钟
- 5 分钟
- 15 分钟
- 30 分钟

对应宏定义在 `config.h` 中。

主循环里会不断调用：

```cpp
checkScreenTimeout();
```

这个函数的逻辑是：

1. 如果设置成不熄屏，直接返回
2. 如果当前已经灭屏，也直接返回
3. 根据熄屏模式换算出超时时间
4. 判断 `当前时间 - 最近活动时间` 是否超过超时
5. 如果超过，就 `turnOffScreen()`

## 12. 真正灭屏和亮屏的代码

灭屏：

```cpp
void turnOffScreen() {
  if (screenOn) {
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    screenOn = false;
  }
}
```

亮屏：

```cpp
void turnOnScreen() {
  if (!screenOn) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    screenOn = true;
    showNetworkInfo();
  }
}
```

这里有两个值得注意的点：

### 12.1 不是只清屏，而是真正关闭显示

`SSD1306_DISPLAYOFF` 是直接给 OLED 控制器发命令，让屏幕进入关闭显示状态。

这比单纯“画面全黑”更像真正熄屏。

### 12.2 亮屏后会重新显示界面

现在代码里亮屏后调用的是 `showNetworkInfo()`。

也就是说，屏幕重新点亮时，会先显示网络信息页，而不是自动恢复到最后一个钢琴键盘画面。

这不是错误，而是当前项目作者选择的一种行为。

## 13. 这个显示模块里值得学习的知识点

### 13.1 坐标系统

OLED 绘图本质上是在二维坐标系里操作：

- 左上角通常是 `(0, 0)`
- 向右是 `x` 增大
- 向下是 `y` 增大

所有图形位置，本质上都是靠坐标和尺寸算出来的。

### 13.2 图形和文字混合界面

这个项目不是只显示文字，也不是只画图，而是把：

- 顶部状态文字
- 中下部钢琴图形

组合成了一个完整界面。

这是一种很常见的嵌入式 UI 设计方式。

### 13.3 先建状态，再渲染画面

显示模块自己并不“发明”数据，它只是读取别的模块状态，然后渲染成界面。

这说明一个工程上的好习惯：

- 业务逻辑和界面逻辑分开
- 输入状态先准备好
- 显示模块只负责表现

## 14. 如果你想自己改显示效果，可以从哪里下手

适合新手尝试的方向有：

1. 修改开机画面的字体大小和内容。
2. 调整白键、黑键尺寸，让键盘更好看。
3. 在顶部加上当前音长 `noteDuration`。
4. 让亮屏后恢复到最后一次钢琴界面，而不是网络信息页。
5. 给教学模式增加更多提示文字。

## 15. 一句话总结

这个项目的 OLED 显示，本质上是：

“根据系统当前状态，把文字和钢琴键盘图形先画到缓冲区，再统一刷新到 SSD1306 屏幕；同时用活动时间记录实现自动熄屏和触摸亮屏。”

理解了这一点，你就已经掌握这个显示模块的核心思路了。
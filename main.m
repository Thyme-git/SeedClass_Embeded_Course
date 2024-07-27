fs = 10000;       % 采样频率，单位Hz
duration = 2;     % 信号持续时间，单位秒
f = 4000;          % 信号频率，单位Hz（例如A4音符的频率）

% 生成时间向量
t = 0:1/fs:duration;

% 生成正弦信号
signal = sin(2 * pi * f * t) / 8;

% 播放音频
sound(signal, fs, 16);

% 0.000000 45419.566406 26105.494141 11582.064453 7809.133789 6024.576660 4990.522461 4321.863281 3867.874023 3546.861816 3334.560303 3220.203369 3234.649170 3513.381348 5192.705566 4473.436035
#pragma once
/*
 * ARDUINO 命令行类型
 * 可自由添加回调函数
 * 
 * 20250513:
 * 基本上就是用一个数组（或向量）来进行命令的保存
 * 然后遍历所有命令，直到遇到返回值为true的。
 *
 * （！）按照约定，返回值为true时该命令已被执行
 */


//#include <ArduinoSTL.h>
#include <vector>
using namespace std;

/* 回调函数数组最大容量 */
constexpr int MAXV = 64;
/* 回调函数变量类型定义 */
//typedef String (shell_t*) (String);
using shell_t = String (*) (String);


namespace qing {

  class Match {
    public:
      virtual String test(String param) =  0;
      virtual String show() = 0;
  };
  /* 
   * 改为储存一个Cmd基类
   */
   class Sh_ma {
      public:
        void add(Match *ma) {
          vec.push_back(ma);
        }
        String process(String cmd) {
          for (auto i=vec.begin(); i!=vec.end(); ++i) {
            String res = (*i)->test(cmd);
            if (res != "") return res;
          }

          /* 未能匹配 */
          return "Unkown Command: " + cmd;
        }
        String show() {
          JSONVar arr;
          int j = 0;
          for (auto i=vec.begin(); i!=vec.end(); ++i) {
           arr[j] =  (*i)->show();
           ++j;
          }

          // 返回
          return JSON.stringify(arr);
        }
      private:
        vector<Match*> vec;
   };

  /* 
   * 使用vec的shell，有的板子可能会
   * 出现 multiple definition of `std::nothrow'
   */
  class ShellVec {
    public:
      void add(shell_t callback) {
        v.push_back(callback);
      }

      /* 执行命令 需要输入目标命令，返回执行结果*/
      String processCommand(String command) {
        // 尝试每一个回调函数，命中即结束。
        for (auto i=v.begin(); i!=v.end(); ++i) {
          String res = (*i)(command);
          if (res != "") return res;
        }
        return "Unkown Command: " + command;
      }

    private:
      vector<shell_t> v;
  };
}
/* 命令行类型 */
class Shell {
public:
  /* 添加功能回调 */
  void add(shell_t callback) {
    if (top < MAXV)
      v[top++] = callback;
  }

  /* 执行命令 需要输入目标命令，返回执行结果*/
  String processCommand(String command) {
    // 尝试每一个回调函数，命中即结束。
    for (int i=0; i < this->top; ++i) {
      String res = v[i](command);
      if (res != "") return res;
    }
    /*
     * 不知名的指令。
     * 
     * 20250427: 有些时候，协处理器会接收到一些乱码
     * 如果仍要回复默认值，将会导致有时不能解析结果
     * 比较奇怪的事情是，如果使用ARDUINO串口调试器
     * 发送一次消息，就都能成功发送。
     */
    return "Unkown Command: " + command;
  }
  
private:
  /* 用于存储回调函数的数组 */
  shell_t v[MAXV];
  /* 回调函数栈顶指针 */
  int top = 0;

};


/* MPU6050 加速度与温度传感器 */
class Match_mpu6050 : public qing::Match {
public:
  bool begin() {
    return (HasMPU6050 = mpu.begin());
  }
  String show() override {
    return _NAME;
  }
  String test(String cmd) override {
    if (cmd != _NAME) return "";
    JSONVar obj;

    // Try to initialize!
    if (!HasMPU6050) {
      obj["msg"] = "ERR";
      obj["data"] = "Failed to find MPU6050 chip in init process";
      return JSON.stringify(obj);
    }

    obj["msg"] = "OK";
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    switch (mpu.getAccelerometerRange()) {
      case MPU6050_RANGE_2_G:
        obj["Accelerometer range"] = "+-2G";
        break;
      case MPU6050_RANGE_4_G:
        obj["Accelerometer range"] = "+-4G";
        break;
      case MPU6050_RANGE_8_G:
        obj["Accelerometer range"] = "+-8G";
        break;
      case MPU6050_RANGE_16_G:
        obj["Accelerometer range"] = "\n+-16G";
        break;
    }
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);

    switch (mpu.getGyroRange()) {
      case MPU6050_RANGE_250_DEG:
        obj["Gyro range"] = "+- 250 deg/s";
        break;
      case MPU6050_RANGE_500_DEG:
        obj["Gyro range"] = "+- 500 deg/s";
        break;
      case MPU6050_RANGE_1000_DEG:
        obj["Gyro range"] = "+- 1000 deg/s";
        break;
      case MPU6050_RANGE_2000_DEG:
        obj["Gyro range"] = "+- 2000 deg/s";
        break;
    }

    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    switch (mpu.getFilterBandwidth()) {
      case MPU6050_BAND_260_HZ:
        obj["Filter bandwidth"] = "260 Hz";
        break;
      case MPU6050_BAND_184_HZ:
        obj["Filter bandwidth"] = "184 Hz";
        break;
      case MPU6050_BAND_94_HZ:
        obj["Filter bandwidth"] = "94 Hz";
        break;
      case MPU6050_BAND_44_HZ:
        obj["Filter bandwidth"] = "44 Hz";
        break;
      case MPU6050_BAND_21_HZ:
        obj["Filter bandwidth"] = "21 Hz";
        break;
      case MPU6050_BAND_10_HZ:
        obj["Filter bandwidth"] = "10 Hz";
        break;
      case MPU6050_BAND_5_HZ:
        obj["Filter bandwidth"] = "5 Hz";
        break;
    }
    delay(100);


    /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    /* Print out the values */
    obj["Acceleration X(m/s^2)"] = a.acceleration.x;
    obj["Acceleration Y(m/s^2)"] = a.acceleration.y;
    obj["Acceleration Z(m/s^2)"] = a.acceleration.z;

    obj["Rotation X(rad/s)"] = g.gyro.x;
    obj["Rotation Y(rad/s)"] = g.gyro.y;
    obj["Rotation Z(rad/s)"] = g.gyro.z;

    obj["Temperature(degC)"] = temp.temperature;

    //delay(500);
    return JSON.stringify(obj);
  }
private:
  Adafruit_MPU6050 mpu;
  bool HasMPU6050;
  String _NAME = "MPU6050";
};

/* SGP30 空气质量传感器 */
class Match_sgp30 : public qing::Match {
public:
  bool begin() {
    return (HasSGP30 = sgp.begin());
  }
  String show() override {
    return _NAME;
  }
  String test(String cmd) override {
    if (cmd != _NAME) return "";

    JSONVar obj;

    if (!HasSGP30) {
      obj["msg"] = "ERR";
      obj["data"] = "FAILED: Can't find the sensor in init process.";
      return JSON.stringify(obj);
    }

    // 发送成功
    obj["msg"] = "OK";

    // 记录编号
    //String msg = "Found SGP30 serial #";
    String msg = "#";
    msg += String(sgp.serialnumber[0], HEX);  //转换为字符串
    msg += String(sgp.serialnumber[1], HEX);
    msg += String(sgp.serialnumber[2], HEX);
    obj["id"] = msg;

    if (sgp.IAQmeasure()) {
      obj["TVOC(ppb)"] = sgp.TVOC;
      obj["eCO2(ppm)"] = sgp.eCO2;
    }

    if (sgp.IAQmeasureRaw()) {
      obj["Raw H2"] = sgp.rawH2;
      obj["Raw Ethanol"] = sgp.rawEthanol;
    }

    return JSON.stringify(obj);
  }
private:
  Adafruit_SGP30 sgp;
  bool HasSGP30;
  String _NAME = "SGP30";
};

/* SHT3X 温湿度传感器 */
class Match_sht3x : public qing::Match {
public:
  bool begin() {
    return (HasSHT3X = sht.begin());
  }
  String show() override {
    return _NAME;
  }
  String test(String cmd) override {
    if (cmd != _NAME) return "";

    JSONVar obj;

    if (!HasSHT3X) {
      obj["msg"] = "ERR";
      obj["data"] = "Not Found In Init";
      return JSON.stringify(obj);
    }

    delay(10);

    if (!sht.measure()) {
      obj["msg"] = "ERR";
      obj["data"] = "SHT3x read error";
      return JSON.stringify(obj);
    }

    obj["msg"] = "OK";
    obj["Temperature(*C)"] = String(sht.temperature(), 1);
    obj["Humidity(%RH)"] = String(sht.humidity(), 1);

    return JSON.stringify(obj);
  }
private:
  ArtronShop_SHT3x sht = ArtronShop_SHT3x(0x44, &Wire);  // ADDR: 0 => 0x44, ADDR: 1 => 0x45
  bool HasSHT3X;
  String _NAME = "SHT3X";
};

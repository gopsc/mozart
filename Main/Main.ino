/**
 * 项目：Arduino Shell 作者：qing 版本：0.0.1
 *
 * 此代码使得ARDUINO微控制器重复地接收一个串口命令，然后返回此命令的结果。
 * 可以通过回调数组来模块化地添加指令处理方案。
 * 
 * 消息的接收在某些情况下可能会失灵，需要多次尝试。
 * 
 *
 * Project: Arduino Shell Author: qing
 * This code enables the ARDUINO microcontroller to continuously receive serial commands and return the corresponding results.
 * Command handling solutions can be modularly added through a callback array.
 *
 * ---------------------------
 * ---------------------------
 * 版本：0.0.2
 *
 * 因为串口的抽象级别比较低，新版本我们将采用网口进行通信。
 * 初步拟定使用交换机进行组网（静态IP），使用HTTP协议建立服务。
 * 
 * ---------------------------
 * ---------------------------
 * 版本：0.0.3
 *
 * 使COPS设备能够通过API被设置，并且具有欢迎页面
 *
 *  curl -X POST http://192.168.254.101:8001/api -d '{"cmd": "MPU6050"}'
 * ---------------------------
 * ---------------------------
 * 版本：0.0.4
 *
 * 重构项目，使用SH_MA类进行匹配
 * 
 * ---------------------------
 * ---------------------------
 * 版本：0.0.5
 * 可以展示可用方法
 * ---------------------------
 * ---------------------------
 * 版本：0.0.6
 * 能够访问SD卡文件
 * ---------------------------
 * ---------------------------
 * 版本：0.0.7
 * 能够修改SD卡文件
 * eg. curl -X POST http://192.168.254.101:8002/api/remove -d '{"filename": "xxx.htm"}'
 * ---------------------------
 * ---------------------------
 * 版本：0.1.0
 * 计划支持tryC解释器
 * ---------------------------
 * ---------------------------
 */
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <Arduino_JSON.h>
#include <ArtronShop_SHT3x.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "Shell.h"

const unsigned long TIMEOUT_MS = 5000;  // 5秒超时

/* 这是来自官方的库对象 */
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 254, 101);
EthernetServer server(8001);

/* 这些是原生库对象 */
//Shell sh;
//qing::ShellVec sh;
qing::Sh_ma sh;
Match_mpu6050 mpu6050;
Match_sgp30 sgp30;
Match_sht3x sht3x;

void setup() {

  Serial.begin(115200);
  Wire.begin();

  /* -------- 初始化SD卡 -------- */
  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {  // 以太网盾通常使用引脚10作为SD卡CS
    Serial.println("SD card initialization failed!");
  } else {
    Serial.println("SD card initialized.");
  }

  /* -------- 初始化以太网 -------- */
  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH Shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  Ethernet.begin(mac, ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found. Sorry, cant't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing no point running without Ethernet hardware
    }
  }

  /* ------ 初始化cmd ------ */
  Serial.println("Mozart start!");
  //sh.add(ECHO);
  //sh.add(HELP);
  //sh.add(ON);
  //sh.add(OFF);
  //sh.add(GET);
  //sh.add(READ);
  //sh.add(WRITE);
  //sh.add(I2CSCAN); //这个太慢
  //sh.add(SHT3X);
  //sh.add(SGP30);
  //sh.add(MPU6050);
  if (mpu6050.begin()) sh.add(&mpu6050);
  if (sgp30.begin()) sh.add(&sgp30);
  if (sht3x.begin()) sh.add(&sht3x);
}

void loop() {
  //read_from_serial();
  listen_http();
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

/*
 * 监听http请求
 */
void listen_http() {
  EthernetClient client = server.available();

  if (client) {
    Serial.println("\nNew client connected");

    String currentLine = "";
    String requestMethod = "";
    String requestPath = "";
    String requestData = "";  // 重命名变量，使其适用于POST和DELETE
    int contentLength = 0;
    unsigned long connectionStart = millis();

    while (client.connected() && (millis() - connectionStart < TIMEOUT_MS)) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {
          // 处理GET方法
          if (currentLine.startsWith("GET")) {
            int firstSpace = currentLine.indexOf(' ');
            int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
              requestMethod = currentLine.substring(0, firstSpace);
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
            }
          } 
          // 处理POST方法
          else if (currentLine.startsWith("POST")) {
            int firstSpace = currentLine.indexOf(' ');
            int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
              requestMethod = currentLine.substring(0, firstSpace);
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
            }
          } 
          // 新增：处理DELETE方法
          else if (currentLine.startsWith("DELETE")) {
            int firstSpace = currentLine.indexOf(' ');
            int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
              requestMethod = currentLine.substring(0, firstSpace);
              requestPath = currentLine.substring(firstSpace + 1, secondSpace);
            }
          } 
          // 处理Content-Length头（适用于POST和DELETE）
          else if (currentLine.startsWith("Content-Length: ")) {
            contentLength = currentLine.substring(16).toInt();
          } 
          // 空行表示请求头结束，开始处理请求体
          else if (currentLine.length() == 0) {
            // 读取请求数据（适用于POST和DELETE）
            if ((requestMethod == "POST" || requestMethod == "DELETE") && contentLength > 0) {
              unsigned long dataStart = millis();
              // 等待数据到达，超时时间2秒
              while (client.available() < contentLength && (millis() - dataStart < 2000)) {
                delay(10);
              }

              // 读取指定长度的数据
              for (int i = 0; i < contentLength; i++) {
                if (client.available()) {
                  requestData += (char)client.read();
                } else {
                  Serial.println("Error: Data incomplete");
                  break;
                }
              }
            }

            // 发送响应
            sendResponse(client, requestMethod, requestPath, requestData);
            break;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    client.stop();
    Serial.println("Client disconnected");
  }
}


void sendResponse(EthernetClient &client, const String &method,
                  const String &path, const String &data) {
  
  if (method == "GET") {
    String filename = path;
    if (filename.startsWith("/")) {
      filename = filename.substring(1);
    }
    
    // 尝试打开文件或目录
    File file = SD.open(filename);
    if (file) {
      // 检查是否是目录
      if (file.isDirectory()) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        
        // 生成目录列表HTML
        client.println("<!DOCTYPE html><html><head><title>Directory Listing</title>");
        client.println("<style>body { font-family: Arial, sans-serif; margin: 20px; }");
        client.println("ul { list-style-type: none; padding: 0; }");
        client.println("li { margin: 5px 0; }");
        client.println("a { text-decoration: none; color: #0366d6; }");
        client.println("a:hover { text-decoration: underline; }</style></head>");
        client.println("<body><h1>Directory: " + path + "</h1><ul>");
        
        // 添加父目录链接（如果不是根目录）
        if (path != "/") {
          String parentPath = path;
          int lastSlash = parentPath.lastIndexOf('/');
          if (lastSlash > 0) {
            parentPath = parentPath.substring(0, lastSlash);
          } else {
            parentPath = "/";
          }
          client.println("<li><a href=\"" + parentPath + "\">../</a></li>");
        }
        
        // 列出目录内容
        while (true) {
          File entry = file.openNextFile();
          if (!entry) {
            break;
          }
          
          String entryName = entry.name();
          // 获取基本文件名（去掉路径前缀）
          if (entryName.startsWith(filename) && entryName.length() > filename.length()) {
            entryName = entryName.substring(filename.length());
            if (entryName.startsWith("/")) {
              entryName = entryName.substring(1);
            }
          }
          
          if (entryName.length() > 0) {
            String entryPath = path;
            if (!entryPath.endsWith("/")) {
              entryPath += "/";
            }
            entryPath += entryName;
            
            client.print("<li><a href=\"");
            client.print(entryPath);
            client.print("\">");
            client.print(entryName);
            
            if (entry.isDirectory()) {
              client.print("/");
            }
            
            client.print("</a>");
            
            if (!entry.isDirectory()) {
              client.print(" (");
              client.print(entry.size());
              client.print(" bytes)");
            }
            
            client.println("</li>");
          }
          entry.close();
        }
        
        client.println("</ul></body></html>");
        file.close();
      } else {
        // 是文件，按原方式处理
        // 根据文件扩展名设置Content-Type
        String contentType = "text/plain";
        String lowerFilename = filename;
        lowerFilename.toLowerCase();
        
        if (lowerFilename.endsWith(".html") || lowerFilename.endsWith(".htm") || 
            lowerFilename.indexOf(".htm") != -1) {
          contentType = "text/html";
        } else if (lowerFilename.endsWith(".css")) {
          contentType = "text/css";
        } else if (lowerFilename.endsWith(".js")) {
          contentType = "application/javascript";
        } else if (lowerFilename.endsWith(".png")) {
          contentType = "image/png";
        } else if (lowerFilename.endsWith(".jpg") || lowerFilename.endsWith(".jpeg")) {
          contentType = "image/jpeg";
        } else if (lowerFilename.endsWith(".gif")) {
          contentType = "image/gif";
        }
        
        client.println("HTTP/1.1 200 OK");
        client.print("Content-Type: ");
        client.println(contentType);
        client.println("Connection: close");
        client.println();
        
        // 发送文件内容
        while (file.available()) {
          client.write(file.read());
        }
        file.close();
      }
    } else {
      // 文件不存在，返回404
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("File not found: ");
      client.print(filename);
    }
  }
  else if (method == "POST") {
    // 处理 /show POST请求
    if (path == "/show") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      String msg = sh.show();
      client.print(msg);
    }
    // 处理 /api/filesystem POST请求 - 保存文件
    else if (path == "/api/filesystem") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" 
            && json.hasOwnProperty("filename") 
            && json.hasOwnProperty("content")) {
        
        String filename = json["filename"];
        String content = json["content"];
        
        // 确保文件名不以斜杠开头
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
        // 如果文件已存在，先删除它（实现覆盖写入）
        if (SD.exists(filename)) {
          SD.remove(filename);
        }
        
        // 尝试打开文件进行写入（如果文件不存在，FILE_WRITE会创建它）
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
          size_t bytesWritten = file.print(content);
          file.close();
          
          if (bytesWritten == content.length()) {
            client.print("{\"status\":\"success\",\"message\":\"File saved successfully\",\"filename\":\"");
            client.print(filename);
            client.print("\",\"bytesWritten\":");
            client.print(bytesWritten);
            client.print("}");
          } else {
            client.print("{\"status\":\"error\",\"message\":\"Failed to write all content\",\"filename\":\"");
            client.print(filename);
            client.print("\",\"expected\":");
            client.print(content.length());
            client.print(",\"actual\":");
            client.print(bytesWritten);
            client.print("}");
          }
        } else {
          client.print("{\"status\":\"error\",\"message\":\"Failed to open file for writing\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
        }
      } else {
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required fields: filename, content\"}");
      }
    }
    // 处理 /api POST请求（保持原有功能）
    else if (path == "/api") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" 
            && json.hasOwnProperty("cmd")) {
        String cmd = json["cmd"];
        String res = sh.process(cmd);
        client.print(res);
      } else {
        client.print("ERR");
      }
    }
    // 处理未知的POST路径
    else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("POST endpoint not found: ");
      client.print(path);
    }
    
    Serial.println("\nReceived POST:");
    Serial.print("Path: ");
    Serial.println(path);
    Serial.print("Data: ");
    Serial.println(data);
  }
  // 新增：处理DELETE请求，用于删除文件
  else if (method == "DELETE") {
    // 处理 /api/remove 请求
    if (path == "/api/remove") {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      
      // 解析JSON数据获取要删除的文件名
      JSONVar json = JSON.parse(data);
      if (JSON.typeof(json) == "object" && json.hasOwnProperty("filename")) {
        String filename = json["filename"];
        
        // 确保文件名不以斜杠开头
        if (filename.startsWith("/")) {
          filename = filename.substring(1);
        }
        
        // 检查文件是否存在
        if (SD.exists(filename)) {
          // 尝试删除文件
          if (SD.remove(filename)) {
            client.print("{\"status\":\"success\",\"message\":\"File deleted successfully\",\"filename\":\"");
            client.print(filename);
            client.print("\"}");
            
            Serial.print("Deleted file: ");
            Serial.println(filename);
          } else {
            client.print("{\"status\":\"error\",\"message\":\"Failed to delete file\",\"filename\":\"");
            client.print(filename);
            client.print("\"}");
            
            Serial.print("Failed to delete file: ");
            Serial.println(filename);
          }
        } else {
          client.print("{\"status\":\"error\",\"message\":\"File not found\",\"filename\":\"");
          client.print(filename);
          client.print("\"}");
          
          Serial.print("File not found for deletion: ");
          Serial.println(filename);
        }
      } else {
        client.print("{\"status\":\"error\",\"message\":\"Invalid JSON format. Required field: filename\"}");
      }
    }
    // 处理未知的DELETE路径
    else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("DELETE endpoint not found: ");
      client.print(path);
    }
    
    Serial.println("\nReceived DELETE:");
    Serial.print("Path: ");
    Serial.println(path);
    Serial.print("Data: ");
    Serial.println(data);
  }
  // 处理不支持的HTTP方法
  else {
    client.println("HTTP/1.1 405 Method Not Allowed");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("Method not supported: ");
    client.print(method);
  }
}


/* 
 * 这是个辅助函数，用于匹配一个带数字后缀的指令 
 *
 * （其实我觉得它的专用性可能有点高了）
 */
int numscan(String inputString, String compareString) {
  if (!inputString.startsWith(compareString)) {
    return -1;
  } else {

    // 取匹配过后的子串
    String numStr = inputString.substring(compareString.length());

    // 检查字串是否是纯数字
    for (int i = 0; i < numStr.length(); ++i) {
      if (!isDigit(numStr.charAt(i))) {
        return -1;
      }
    }

    // 字串为纯数字，返回
    return numStr.toInt();
  }
}

/* 开启一个引脚的数字输出 */
bool ON(String cmd) {
  int n;
  if ((n = numscan(cmd, "ON")) > 0) {
    pinMode(n, OUTPUT);
    digitalWrite(n, HIGH);
    Serial.print("OK");
    return true;
  } else {
    return false;
  }
}

/* 关闭一个引脚的数字输出 */
bool OFF(String cmd) {
  int n;
  if ((n = numscan(cmd, "OFF")) > 0) {
    pinMode(n, OUTPUT);
    digitalWrite(n, LOW);
    Serial.print("OK");
    return true;
  } else {
    return false;
  }
}

/* 取得一个引脚的数字读数 */
bool GET(String cmd) {
  int n;
  if ((n = numscan(cmd, "GET")) > 0) {
    pinMode(n, INPUT);
    int m = digitalRead(n);
    Serial.print(m);
    return true;
  } else {
    return false;
  }
}

/*
 * 从一个A口读取模拟量
 *
 * 这个函数需要传入A口的引脚编号
 */
bool READ(String cmd) {
  int n;
  if ((n = numscan(cmd, "READ")) > 0) {
    pinMode(n, INPUT);
    int m = analogRead(n);
    Serial.print(m);
    return true;
  } else {
    return false;
  }
}

/*
 * 向一个A口引脚写入模拟信号
 *
 * 必须输入目标A口的引脚编号，例如 “WRITE10 512”
 */
bool WRITE(String cmd) {
  // 向一个A口写模拟量
  if (cmd.startsWith("WRITE")) {
    int pos = cmd.indexOf(' ');
    if (pos < 0) {  //找不到空格分隔
      Serial.print("Invalid Write Command");
    } else {  // 定位了空格分隔符
      String first = cmd.substring(0, pos);
      String second = cmd.substring(pos + 1);
      int pin = numscan(first, "WRITE");
      if (pin == -1) {  // 没有扫描到目标针脚
        Serial.print("Invalid Write Command");
      } else {  //已经扫描到目标针脚
        pinMode(pin, OUTPUT);
        analogWrite(pin, second.toInt());
        Serial.print("OK");
        return true;
      }
    }
  }
  return false;
}

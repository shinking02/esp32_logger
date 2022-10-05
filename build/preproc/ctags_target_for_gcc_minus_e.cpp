# 1 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino"
# 2 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 3 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 4 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 5 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 6 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 7 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 8 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 9 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 10 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 11 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 12 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2
# 13 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 2


#define CFACTOR 0.12500 /* GAIN_ONE: +/-4.096V range: Calibration factor (mV/bit) for ADS1115*/
#define AVERAGENUM 30

// WiFi設定
const int sd_led = 13;
const char *ssid = "ESP32_LOGGER";
const char *pass = "esp32wifi";
const IPAddress ip(192, 168, 20, 2);
const IPAddress subnet(255, 255, 255, 0);

static unsigned long startup_time = 0;
static unsigned long logging_time = 0;

const int stop_sw = 16;
const int reset_sw = 4;
const int chipSelect = 5;

String filePath = "/log.txt";
bool stop_status = false;
bool interrupt_flag = false;

float voltage, current;

Adafruit_ADS1115 ads;
hw_timer_t * timer = 
# 39 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino" 3 4
                    __null
# 39 "/Users/shin/Documents/Arduino/esp32_logger/esp32_logger.ino"
                        ; //timer初期化

SSD1306 display(0x3c, 21, 22); //SSD1306インスタンスの作成（I2Cアドレス,SDA,SCL）

WebServer webServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
const char SENSOR_JSON[] = R"=====({"voltage":%.1f,"current":%.1f,"power":%.1f})=====";

//タイマー割り込みで実行される関数です
void __attribute__((section(".iram1" "." "16"))) onTimer() {
    interrupt_flag = true;
    startup_time++;
}

void setup() {
    Serial.begin(115200);

    pinMode(stop_sw, 0x01);
    pinMode(reset_sw, 0x01);

    display.init();
    display.setFont(ArialMT_Plain_10);
    display.flipScreenVertically();

    Serial.println("WIFI Activating...");
    display.clear();
    display.drawString(0, 0, "WIFI Activating...");
    display.display();

    WiFi.disconnect(true);
    delay(1000);
    WiFi.softAP(ssid, pass);
    delay(100);
    WiFi.softAPConfig(ip, ip, subnet);
    IPAddress myIP = WiFi.softAPIP();
    Serial.println("WiFi activation completed!");
    display.clear();
    display.drawString(0, 0, "WiFi activation completed!");
    display.display();
    delay(2000);

    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASS: ");
    Serial.println(pass);
    Serial.print("IP: ");
    Serial.println(myIP);
    display.clear();
    display.drawString(0, 0, "SSID: " + String(ssid));
    display.drawString(0, 16, "PASS: " + String(pass));
    display.drawString(0, 32, "IP: " + myIP.toString());
    display.display();

    SPIFFS.begin();
    webServer.serveStatic("/espressif.ico", SPIFFS, "/espressif.ico");
    webServer.serveStatic("/Chart.min.js", SPIFFS, "/Chart.min.js");
    webServer.on("/", handleRoot);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
    webSocket.begin();

    delay(2000);
    display.drawString(0, 48, "Please push the buttons...");
    display.display();
    while(!checkSwitch(reset_sw) && !checkSwitch(stop_sw)) {}

    ads.setGain(GAIN_ONE);
    ads.begin();

    SD.begin();

    timer = timerBegin(0, 80, true); //timer=1us, 80MHz
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true); // 1000ms
    timerAlarmEnable(timer);

    sdWrite("-------POWER_ON-------");
}

void loop() {
    //WebSever処理
    webSocket.loop();
    webServer.handleClient();
    //スイッチの入力を監視し、実行します
    if(checkSwitch(stop_sw)) {
        stop_status = !stop_status;
        if(stop_status) {
            sdWrite("-------STOP-------");
        }else {
            sdWrite("-------START-------");
        }
    }else if(checkSwitch(reset_sw)) {
        logging_time = 0;
        sdWrite("-------RESET-------");
    }
    //タイマー割り込み時の処理です
    if(interrupt_flag) {
        interrupt_flag = false;
        voltage = getVoltage();
        current = getCurrent();
        drawScreen();
        broadcastWeb();
        if(stop_status) {
            return; //停止状態ならばここで処理を中断
        }
        logging_time++;
        sdWrite(getCsvMessage());
    }
}

//SDにデータ(引数)を書き込みます
void sdWrite(String msg) {
    digitalWrite(sd_led, 0x1);
    File file = SD.open(filePath, "a");
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.println(msg)){
        Serial.println(msg);
    } else {
        Serial.println("WRITTEN ERROR!");
    }
    file.close();
}

//1行のメッセージ(csv用)を生成し返却します
String getCsvMessage() {
    String msg = getLogTime() + ", " + String(startup_time) + ", " + String(voltage) + ", " + String(current);
    return msg;
}

//電圧を取得し返却します
float getVoltage() {
    float voltage_tmp = 0;
    float read_from_ads = 0;
    for(int i = 0; i < 30; i++) {
        read_from_ads = float(ads.readADC_Differential_0_1()); //チャネル0から単動（Single End）入力
        voltage_tmp = voltage_tmp + read_from_ads / 30; //読んだ値を足していく：AVERAGENUMで割ることで平均化している
        voltage_tmp = voltage_tmp / 24.6;
    }
    return voltage_tmp;
}

//電流を取得し返却します
float getCurrent() {
    float current_tmp = 0;
    float read_from_ads = 0;
    for(int i = 0; i < 30; i++) {
        read_from_ads = float(ads.readADC_Differential_2_3()); //チャネル1から単動（Single End）入力
        current_tmp = current_tmp + read_from_ads / 30; //読んだ値を足していく：AVERAGENUMで割ることで平均化している
        current_tmp = current_tmp / 25.8;
    }
    return current_tmp;
}

//logtimeを生成し返却します
String getLogTime() {
    int hhmmss_buff;
    char hhmmss_array[] = "00:00:00";
    // hh
    hhmmss_buff = (logging_time / 3600) % 100;
    hhmmss_array[1] += hhmmss_buff % 10; hhmmss_buff /= 10;
    hhmmss_array[0] += hhmmss_buff % 10;
    // mm
    hhmmss_buff = (logging_time / 60) % 60;
    hhmmss_array[4] += hhmmss_buff % 10; hhmmss_buff /= 10;
    hhmmss_array[3] += hhmmss_buff % 10;
    // ss
    hhmmss_buff = logging_time % 60;
    hhmmss_array[7] += hhmmss_buff % 10; hhmmss_buff /= 10;
    hhmmss_array[6] += hhmmss_buff % 10;

    return String(hhmmss_array);
}

//引数で渡されたスイッチの状態を返却します
bool checkSwitch(int switch_num) {
    int button_state = digitalRead(switch_num);
    if(button_state == 0x0) {
        delay(100);
        return true;
    }else {
        return false;
    }
}

//oledにデフォルトの情報を表示します
void drawScreen() {
    display.clear();
    display.drawString(0, 0, "LogTime: " + String(getLogTime()));
    display.drawString(0, 16, String(voltage) + " V");
    display.drawString(50, 16, String(current) + " A");
    display.drawString(0, 32, String(current*voltage) + " W");
    if(stop_status) {
        display.drawString(0, 48, "Stop");
    }
    display.display();
}

//index_html.h用にJSONデータをセットします
void broadcastWeb() {
    char payload[200];
    float power = voltage * current;
    snprintf(payload, sizeof(payload), SENSOR_JSON, voltage, current, power);
    // WebSocketでデータ送信(全端末へブロードキャスト)
    webSocket.broadcastTXT(payload, strlen(payload));
}

// Webコンテンツのイベントハンドラ
void handleRoot() {
  String s = INDEX_HTML; // index_html.hより読み込み
  webServer.send(200, "text/html", s);
}
void handleNotFound() {
  webServer.send(404, "text/plain", "File not found.");
}

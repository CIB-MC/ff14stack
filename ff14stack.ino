#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <time.h>
#include <sha1.h>
#include <hmac.h>
#include "config.hpp"
#include "base32.hpp"

FF14StackConfig ff14stackConfig;
WiFiMulti wifiMulti;

const uint16_t BG_COLOR = BLACK;
const uint16_t MAIN_COLOR = WHITE;

const int one_day_sec = 60 * 60 * 24;
const int hour_sec = 60 * 60;

int e_hours_prev = -1;
int e_minutes_prev = -1;
int hours_prev = -1;
int minutes_prev = -1;
time_t prev_rest_sec = -1;
int totp_t_prev = -1;

TFT_eSprite sprite(&M5.Lcd);
int page_mode = 0;

int breath_table[2] = {12, 8};
int screen_breath_step = 0;

unsigned char *totp_s = NULL;
size_t totp_s_size = 0;

void setup() {
  const int MAX_RETRY_GLT_CHECK = 25;

  M5.begin();
  Serial.begin(9600);
  M5.Lcd.setRotation(0);
  sprite.createSprite(M5.Lcd.width(), M5.Lcd.height());
  M5.Lcd.fillRect(0, 0, 135, 240, BG_COLOR);
  draw_notice();

  unsigned char *result = NULL;
  size_t result_size = 0;
  int exitCode = base32::encodedStringToByteArray(
    FF14StackConfig::TOTP_SECRET_BASE32,
    &result, &result_size, true
  );
  if (exitCode == 0) {
    totp_s = result;
    totp_s_size = result_size;
  }

  delay(3000);
  M5.Lcd.fillRect(0, 0, 135, 240, BG_COLOR);
  delay(1000);
}

void loop() {
  M5.update();
  if (M5.Axp.GetBtnPress() == 2) {
    screen_breath_step = (screen_breath_step + 1) % 2;
    M5.Axp.ScreenBreath(breath_table[screen_breath_step]);
  }
  
  if (M5.BtnA.wasPressed()) {
    page_mode = ((page_mode + 1) % 4);
    page_initialize();
  }

  if (M5.BtnB.wasPressed()) {
    M5.Lcd.fillRect(0, 0, 135, 240, BG_COLOR);
    M5.Lcd.setTextColor(MAIN_COLOR);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawCentreString("CLOCK", 135/2+2, 30, 1);
    M5.Lcd.drawCentreString("AJAST", 135/2+2, 50, 1);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawCentreString("Checking NTP.", 135/2+2, 70, 1);
    int exit_status = config_time_by_ntp();
    if (exit_status == 0) {
      M5.Lcd.drawCentreString("Success", 135/2+2, 80, 1);
      delay(1000);
    } else {
      M5.Lcd.drawCentreString("Faild.", 135/2+2, 80, 1);
      M5.Lcd.drawCentreString("Try again", 135/2+2, 90, 1);
      M5.Lcd.drawCentreString("later!", 135/2+2, 100, 1);
      delay(2000);
    }
    page_initialize();
  }

  if (page_mode == 0) {
    sprite.fillScreen(BG_COLOR);
    draw_start_page(&sprite);
    draw_crystal(&sprite);
    sprite.pushSprite(0, 0);
    delay(33);
  } else if (page_mode == 1) {
    draw_eorzea_clock();
    delay(33);
  } else if (page_mode == 2) {
    draw_local_clock();
    delay(33);
  } else if (page_mode == 3) {
    draw_otp_page();
    delay(33);
  }
}

void page_initialize() {
  M5.Lcd.fillRect(0, 0, 135, 240, BG_COLOR);
  e_hours_prev = -1;
  e_minutes_prev = -1;
  hours_prev = -1;
  minutes_prev = -1;
  totp_t_prev = -1;
  prev_rest_sec = -1;
}

void draw_notice() {
  M5.Lcd.setTextDatum(1);
  M5.Lcd.setTextColor(MAIN_COLOR);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCentreString("NOTICE!", 135/2, 45, 1);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString("FF14Stack is", 135/2, 70, 1);
  M5.Lcd.drawCentreString("NOT an official", 135/2, 80, 1);
  M5.Lcd.drawCentreString("device by SQEX.", 135/2, 90, 1);
  M5.Lcd.drawCentreString("Please feel", 135/2, 105, 1);
  M5.Lcd.drawCentreString("free to ask", 135/2, 115, 1);
  M5.Lcd.drawCentreString("@CIB_MC", 135/2, 125, 1);
  M5.Lcd.drawCentreString("about this", 135/2, 135, 1);
  M5.Lcd.drawCentreString("on Twitter!", 135/2, 145, 1);
}

void draw_start_page(TFT_eSprite *sprite) {
  sprite->setTextDatum(1);
  sprite->setTextColor(MAIN_COLOR);
  sprite->setTextSize(4);
  sprite->drawCentreString("FF14", 135/2+2, 20, 1);
  sprite->setTextSize(1);
  sprite->drawCentreString("Stack", 135/2, 48, 4);
  sprite->setTextSize(2);
  sprite->drawCentreString("(C)CIB-MC", 135/2, 240 - 20, 1);
}


void draw_eorzea_clock(){
  RTC_DateTypeDef DateStruct;
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetData(&DateStruct);
  M5.Rtc.GetTime(&TimeStruct);

  int week_sec = (one_day_sec * DateStruct.WeekDay) + (hour_sec * TimeStruct.Hours) + (60 * TimeStruct.Minutes) + TimeStruct.Seconds + (3600 * 9);
  int eorzea_week_sec = (double)week_sec * (1440.0d / 70.0d);
  int e_hours = (eorzea_week_sec / hour_sec) % 24;
  int e_minutes = (eorzea_week_sec / 60) % 60;

  draw_tick_tock("Mog!");
  if (e_minutes_prev == e_minutes) return;
  draw_clock_panel("EORZEA", e_hours, e_minutes, e_hours_prev, e_minutes_prev);

  e_hours_prev = e_hours;
  e_minutes_prev = e_minutes;
}

void draw_local_clock(){
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetTime(&TimeStruct);

  int hours = (TimeStruct.Hours + 9) % 24;
  int minutes = TimeStruct.Minutes;

  draw_tick_tock("Cupo!");
  if (minutes_prev == minutes) return;
  draw_clock_panel("LOCAL", hours, minutes, hours_prev, minutes_prev);

  hours_prev = hours;
  minutes_prev = minutes;
}

void draw_clock_panel(String title, int hours, int minutes, int hours_prev, int minutes_prev) {
  char hours_prev_str[8] = "";
  char minutes_prev_str[8] = "";
  char hours_str[8] = "";
  char minutes_str[8] = "";
  sprintf(hours_prev_str, "%02d", hours_prev);
  sprintf(minutes_prev_str, "%02d", minutes_prev);
  sprintf(hours_str, "%02d", hours);
  sprintf(minutes_str, "%02d", minutes);

  M5.Lcd.setTextDatum(1);
  M5.Lcd.setTextColor(MAIN_COLOR);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCentreString(title.c_str(), 135/2, 25, 1);
  M5.Lcd.drawCentreString("CLOCK", 135/2, 50, 1);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(BG_COLOR);
  M5.Lcd.drawCentreString("88", 135/2, 80, 7);
  M5.Lcd.drawCentreString("88", 135/2, 135, 7);
  M5.Lcd.setTextColor(MAIN_COLOR);
  M5.Lcd.drawCentreString(hours_str, 135/2, 80, 7);
  M5.Lcd.drawCentreString(minutes_str, 135/2, 135, 7);
}

void draw_otp_page() {
  time_t epoch = get_epoch_from_rtc() + 1;
  Serial.printf("%d\n", epoch);

  time_t rest_sec = (time_t)30 - (epoch % (time_t)30);
  draw_count(rest_sec);

  unsigned long long totp_t = (unsigned long long)epoch / (unsigned long long)30;
  if (totp_t_prev == totp_t) return;
  int otp = 0;
  if (totp_s != NULL) {
    otp = get_totp(totp_t, totp_s, totp_s_size);
  }
  draw_otp_panel(otp);
  totp_t_prev = totp_t;
}

int get_totp(unsigned long long totp_t_ll, unsigned char *totp_s, size_t totp_s_size) {
  unsigned char totp_t[8];
  memset(totp_t, 0, (size_t)8);
  memcpy(totp_t, &totp_t_ll, (size_t)8);
  unsigned char totp_t_reversed[8];
  memset(totp_t_reversed, 0, (size_t)8);
  for (int i = 0; i < 8; i++) {
    totp_t_reversed[i] = totp_t[7 - i];
  }
  std::string hash = hmac<SHA1>(totp_t_reversed, (size_t)8, totp_s, totp_s_size);
  unsigned char *hash_bytes;
  hash_bytes = (unsigned char *)malloc(hash.length()/2);

  for (size_t i = 0; i < hash.length()/2; i++) {
    std::string b = hash.substr(i * 2, 2);
    hash_bytes[i] = std::stoi(b, NULL, 16);
  }
  int offset = hash_bytes[(hash.length()/2) - 1] & 0x0f;
  unsigned int int_totp = ((unsigned int)(hash_bytes[offset] & 0x7f) << 24)
    | ((unsigned int)(hash_bytes[offset + 1] & 0xff) << 16)
    | ((unsigned int)(hash_bytes[offset + 2] & 0xff) << 8)
    | ((unsigned int)(hash_bytes[offset + 3] & 0xff));

  free(hash_bytes);
  return int_totp;
}

void draw_count(int c_now) {
  if (prev_rest_sec == c_now) return;
  char s_now[8] = "";
  char s_prev[8] = "";
  sprintf(s_now, "%02d", c_now);
  sprintf(s_prev, "%02d", prev_rest_sec);

  M5.Lcd.setTextDatum(1);
  M5.Lcd.setTextColor(MAIN_COLOR, BG_COLOR);
  M5.Lcd.setTextSize(2);
  if (prev_rest_sec != -1) {
    M5.Lcd.setTextColor(BG_COLOR);
    M5.Lcd.drawCentreString(s_prev, 135/2, 240 - 30, 1);
  }
  M5.Lcd.setTextColor(MAIN_COLOR);
  M5.Lcd.drawCentreString(s_now, 135/2, 240 - 30, 1);
  prev_rest_sec = c_now;
}

void draw_otp_panel(int otp) {
  M5.Lcd.setTextDatum(1);
  M5.Lcd.setTextColor(MAIN_COLOR);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCentreString("OTP", 135/2, 10, 1);

  char code_full[32] = "";
  char code_top[8] = "";
  char code_middle[8] = "";
  char code_bottom[8] = "";
  sprintf(code_full, "%030d", otp);
  size_t code_len = strlen(code_full);
  strncpy(code_top, code_full + (code_len - (size_t)6), (size_t)2);
  strncpy(code_middle, code_full + (code_len - (size_t)4), (size_t)2);
  strncpy(code_bottom, code_full + (code_len - (size_t)2), (size_t)2);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(BG_COLOR);
  M5.Lcd.drawCentreString("88", 135/2, 40, 7);
  M5.Lcd.drawCentreString("88", 135/2, 95, 7);
  M5.Lcd.drawCentreString("88", 135/2, 150, 7);
  M5.Lcd.setTextColor(MAIN_COLOR);
  M5.Lcd.drawCentreString(code_top, 135/2, 40, 7);
  M5.Lcd.drawCentreString(code_middle, 135/2, 95, 7);
  M5.Lcd.drawCentreString(code_bottom, 135/2, 150, 7);
}

bool is_tick_prev = false;
void draw_tick_tock(String str) {
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetTime(&TimeStruct);
  bool is_tick = (TimeStruct.Seconds % 2) > 0;
  if (is_tick_prev == is_tick) return;

  M5.Lcd.setTextDatum(1);

  M5.Lcd.setTextSize(2);
  if(is_tick) {
    M5.Lcd.setTextColor(BG_COLOR);
  } else {
    M5.Lcd.setTextColor(MAIN_COLOR);
  }
  M5.Lcd.drawCentreString(str.c_str(), 135/2, 240 - 40, 1);

  is_tick_prev = is_tick;
}

int prev_num_for_peeking = 0;
void draw_crystal(TFT_eSprite *sprite) {
  const int center_top_x = 135/2;
  const int center_top_y = 64;
  const int center_bottom_x = 135/2;
  const int center_bottom_y = 215;
  const int middle_y = 180;
  const int middle_width = 78;
  const int middle_half_width = middle_width / 2;
  const int offset = 3;

  int limited_millis =  (int)(millis() % 5000);
  const int line_num = 6;
  //int peek_check = ((middle_width / line_num + (limited_millis / 40)) % middle_width);
  //if(prev_num_for_peeking == peek_check) return;
  //prev_num_for_peeking = peek_check;
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetTime(&TimeStruct);
  bool is_tick = (TimeStruct.Seconds % 5) < 3;

  sprite->fillTriangle(center_top_x, center_top_y - offset, center_top_x + middle_half_width + offset, middle_y, center_top_x - middle_half_width - offset, middle_y, CYAN);
  sprite->fillTriangle(center_bottom_x, center_bottom_y + offset, center_bottom_x + middle_half_width + offset, middle_y, center_bottom_x - middle_half_width - offset, middle_y, CYAN);
  sprite->drawTriangle(center_top_x, center_top_y, center_top_x + middle_half_width, middle_y, center_top_x - middle_half_width, middle_y, DARKCYAN);
  sprite->drawTriangle(center_bottom_x, center_bottom_y, center_bottom_x + middle_half_width, middle_y, center_bottom_x - middle_half_width, middle_y, DARKCYAN);

  int line_x_datnum = center_top_x + middle_half_width;
  for(int i = 1; i <= line_num; i++) {
    int current_x = line_x_datnum - ((middle_width / line_num * i + (limited_millis / 30)) % middle_width);
    sprite->drawLine(center_top_x, center_top_y, current_x, middle_y, DARKCYAN);
    sprite->drawLine(center_bottom_x, center_bottom_y, current_x, middle_y, DARKCYAN);
  }
  if (is_tick) {
    sprite->setTextSize(1);
    sprite->setTextColor(MAIN_COLOR, BG_COLOR);
    sprite->drawCentreString("PRESS", 135/2, 140, 4);
    sprite->drawCentreString("M5", 135/2, 168, 4);
  }
}

void draw_datetime_bottom() {
  RTC_DateTypeDef DateStruct;
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetData(&DateStruct);
  M5.Rtc.GetTime(&TimeStruct);

  char str_date[16] = "";
  char str_time[16] = "";
  sprintf(str_date, "%04d-%02d-%02d", DateStruct.Year, DateStruct.Month, DateStruct.Date);
  sprintf(str_time, "%02d:%02d:%02d", TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds);
  M5.Lcd.setTextDatum(1);
  M5.Lcd.setTextColor(MAIN_COLOR, BG_COLOR);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCentreString(str_date, 135/2, 240 - 36, 1);
  M5.Lcd.drawCentreString(str_time, 135/2, 240 - 20, 1);
}

int config_time_by_ntp() {
  const char* WIFI_SSID = ff14stackConfig.WIFI_SSID.c_str();
  const char* WIFI_PASS = ff14stackConfig.WIFI_PASS.c_str();
  const char* NTP_SERVER = ff14stackConfig.NTP_SERVER.c_str();
  const long GMT_OFFSET_SEC = 0;
  const int DAYLIGHT_OFFSET_SEC = 0;
  const int MAX_RETRY_NTP = 1;
  const int MAX_RETRY_GLT_CHECK = 25;

  int retry_count_ntp = 0;
  int retry_count_glt_check = 0;
  struct tm lt;

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
  while (wifiMulti.run() != WL_CONNECTED) {
    if(retry_count_ntp++ > MAX_RETRY_NTP) return 1;
    delay(5000);
  }

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  while (!getLocalTime(&lt)) {
    if(retry_count_glt_check++ > MAX_RETRY_GLT_CHECK) return 2;
    delay(200);
  }

  RTC_TimeTypeDef TimeStruct;
  TimeStruct.Hours   = lt.tm_hour;
  TimeStruct.Minutes = lt.tm_min;
  TimeStruct.Seconds = lt.tm_sec;
  M5.Rtc.SetTime(&TimeStruct);
  RTC_DateTypeDef DateStruct;
  DateStruct.WeekDay = lt.tm_wday;
  DateStruct.Month = lt.tm_mon + 1;
  DateStruct.Date = lt.tm_mday;
  DateStruct.Year = lt.tm_year + 1900;
  M5.Rtc.SetData(&DateStruct);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  return 0;
}

time_t get_epoch_from_rtc() {
  RTC_DateTypeDef DateStruct;
  RTC_TimeTypeDef TimeStruct;
  M5.Rtc.GetData(&DateStruct);
  M5.Rtc.GetTime(&TimeStruct);
  struct tm t;
  t.tm_year = DateStruct.Year - 1900;
  t.tm_mon  = DateStruct.Month - 1;
  t.tm_mday = DateStruct.Date;
  t.tm_hour = TimeStruct.Hours;
  t.tm_min  = TimeStruct.Minutes;
  t.tm_sec  = TimeStruct.Seconds;
  t.tm_isdst= -1;
  return mktime(&t);
}

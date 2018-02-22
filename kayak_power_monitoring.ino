#include <SPI.h>
#include <SdFat.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>

#define DEBUG 0

enum HARDWARE {
  BBM_EBOX_OUT_VOLTAGE = A0,
  STATOR_AND_MAIN_BATT_IN_VOLTAGE = A1,
  BBM_WINCH_OUT_VOLTAGE = A2,

  STATOR_TO_BBM_CURRENT = A3,
  MAIN_BATT_TO_BBM_CURRENT = A6,
  BBM_TO_WINCH_AND_BATT_CURRENT = A7,
  WINCH_BATT_AND_BBM_TO_WINCH_CURRENT = A8,
  BBM_TO_EBOX_CURRENT = A9,

  SD_CHIP_SELECT = 10
};

// Scalars
float BBM_EBOX_OUT_VOLTAGE_SCALAR = 18.05458;
float STATOR_AND_MAIN_BATT_IN_VOLTAGE_SCALAR = 18.15359;
float BBM_WINCH_OUT_VOLTAGE_SCALAR = 17.97053;

float STATOR_TO_BBM_CURRENT_SCALAR = 18.51086258;
float MAIN_BATT_TO_BBM_CURRENT_SCALAR = 19.06635638;
float BBM_TO_WINCH_AND_BATT_CURRENT_SCALAR = 19.00188561;
float WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_SCALAR = 94.22085283;
float BBM_TO_EBOX_CURRENT_SCALAR = 20.06647586;

// Measured values
float BBM_EBOX_OUT_VOLTAGE_MEASURED = 0;
float STATOR_AND_MAIN_BATT_IN_VOLTAGE_MEASURED = 0;
float BBM_WINCH_OUT_VOLTAGE_MEASURED = 0;

float STATOR_TO_BBM_CURRENT_MEASURED = 0;
float MAIN_BATT_TO_BBM_CURRENT_MEASURED = 0;
float BBM_TO_WINCH_AND_BATT_CURRENT_MEASURED = 0;
float WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_MEASURED = 0;
float BBM_TO_EBOX_CURRENT_MEASURED = 0;

// Other variables
SdFat SD;

bool sd_present = false;
bool opened_existing_file = false;
bool new_file_opened = false;

File current_file;
char file_name[] = "log_data_";
char file_extension[] = ".csv";
char string_buffer[255];

time_t current_time;
char last_second;
uint16_t last_sequence_number = 0;


void setup() {
  pinMode(HARDWARE::BBM_EBOX_OUT_VOLTAGE, INPUT);
  pinMode(HARDWARE::STATOR_AND_MAIN_BATT_IN_VOLTAGE, INPUT);
  pinMode(HARDWARE::BBM_WINCH_OUT_VOLTAGE, INPUT);

  pinMode(HARDWARE::STATOR_TO_BBM_CURRENT, INPUT);
  pinMode(HARDWARE::MAIN_BATT_TO_BBM_CURRENT, INPUT);
  pinMode(HARDWARE::BBM_TO_WINCH_AND_BATT_CURRENT, INPUT);
  pinMode(HARDWARE::WINCH_BATT_AND_BBM_TO_WINCH_CURRENT, INPUT);
  pinMode(HARDWARE::BBM_TO_EBOX_CURRENT, INPUT);

  analogReadResolution(13);

  setSyncProvider(RTC.get);

#if DEBUG == 1
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting Logger");
#endif

  sd_present = SD.begin(HARDWARE::SD_CHIP_SELECT);

  if (!sd_present) {
#if DEBUG == 1
    Serial.println("Card failed, or not present. Data will only be shown over serial");
#endif
  } else {
#if DEBUG == 1
    Serial.println("Card installed. Data will be logged.");
#endif
  }

  open_file_by_datetime();

#if DEBUG == 1
  print_header();
#endif

  last_second = second();

}

void loop() {
  get_measurement_time();
  take_measurement();

#if DEBUG == 1
  print_measurements();
#endif

  write_to_card_if_present();

  delay(50);

}

void open_file_by_datetime() {
  sprintf(string_buffer, "%s%d-%02d-%02dT%02d%02d%s", file_name, year(), month(), day(), hour(), minute(), file_extension);
  opened_existing_file = SD.exists(string_buffer);
  current_file = SD.open(string_buffer, FILE_WRITE);
  write_sd_header_if_new_file();
  new_file_opened = true;
}

#if DEBUG == 1
void print_header() {
  Serial.print("TIMESTAMP");
  Serial.print("\t");
  Serial.print("BBM_EBOX_OUT_VOLTAGE_MEASURED");
  Serial.print(",\t");
  Serial.print("STATOR_AND_MAIN_BATT_IN_VOLTAGE_MEASURED");
  Serial.print(",\t");
  Serial.print("BBM_WINCH_OUT_VOLTAGE_MEASURED");
  Serial.print(",\t");
  Serial.print("STATOR_TO_BBM_CURRENT_MEASURED");
  Serial.print(",\t");
  Serial.print("MAIN_BATT_TO_BBM_CURRENT_MEASURED");
  Serial.print(",\t");
  Serial.print("BBM_TO_WINCH_AND_BATT_CURRENT_MEASURED");
  Serial.print(",\t");
  Serial.print("WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_MEASURED");
  Serial.print(",\t");
  Serial.print("BBM_TO_EBOX_CURRENT_MEASURED");
  Serial.println();
}
#endif

void write_sd_header_if_new_file() {
  if (!opened_existing_file) {
    current_file.print("TIMESTAMP");
    current_file.print(",");
    current_file.print("BBM_EBOX_OUT_VOLTAGE_MEASURED");
    current_file.print(",");
    current_file.print("STATOR_AND_MAIN_BATT_IN_VOLTAGE_MEASURED");
    current_file.print(",");
    current_file.print("BBM_WINCH_OUT_VOLTAGE_MEASURED");
    current_file.print(",");
    current_file.print("STATOR_TO_BBM_CURRENT_MEASURED");
    current_file.print(",");
    current_file.print("MAIN_BATT_TO_BBM_CURRENT_MEASURED");
    current_file.print(",");
    current_file.print("BBM_TO_WINCH_AND_BATT_CURRENT_MEASURED");
    current_file.print(",");
    current_file.print("WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_MEASURED");
    current_file.print(",");
    current_file.print("BBM_TO_EBOX_CURRENT_MEASURED");
    current_file.println();
  }
}

void take_measurement() {
  BBM_EBOX_OUT_VOLTAGE_MEASURED = (BBM_EBOX_OUT_VOLTAGE_SCALAR * (analogRead(HARDWARE::BBM_EBOX_OUT_VOLTAGE) / 8192.0));
  STATOR_AND_MAIN_BATT_IN_VOLTAGE_MEASURED = (STATOR_AND_MAIN_BATT_IN_VOLTAGE_SCALAR * (analogRead(HARDWARE::STATOR_AND_MAIN_BATT_IN_VOLTAGE) / 8192.0));
  BBM_WINCH_OUT_VOLTAGE_MEASURED = (BBM_WINCH_OUT_VOLTAGE_SCALAR * (analogRead(HARDWARE::BBM_WINCH_OUT_VOLTAGE) / 8192.0));

  STATOR_TO_BBM_CURRENT_MEASURED = (STATOR_TO_BBM_CURRENT_SCALAR * (analogRead(HARDWARE::STATOR_TO_BBM_CURRENT) - 4096) / 4096.0);
  MAIN_BATT_TO_BBM_CURRENT_MEASURED = (MAIN_BATT_TO_BBM_CURRENT_SCALAR * (analogRead(HARDWARE::MAIN_BATT_TO_BBM_CURRENT) - 4096) / 4096.0);
  BBM_TO_WINCH_AND_BATT_CURRENT_MEASURED = (BBM_TO_WINCH_AND_BATT_CURRENT_SCALAR * (analogRead(HARDWARE::BBM_TO_WINCH_AND_BATT_CURRENT) - 4096) / 4096.0);
  WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_MEASURED = (WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_SCALAR * (analogRead(HARDWARE::WINCH_BATT_AND_BBM_TO_WINCH_CURRENT) - 4096) / 4096.0);

  BBM_TO_EBOX_CURRENT_MEASURED = (BBM_TO_EBOX_CURRENT_SCALAR * (analogRead(HARDWARE::BBM_TO_EBOX_CURRENT) - 4096) / 4096.0);
}

void get_measurement_time() {
  sprintf(string_buffer, "%d-%02d-%02dT%02d-%02d-%02d_%04d", year(current_time), month(current_time), day(current_time), hour(current_time), minute(current_time), second(current_time), last_sequence_number);
}

#if DEBUG == 1
void print_measurements() {
  Serial.print(string_buffer);
  Serial.print(",");
  Serial.print(BBM_EBOX_OUT_VOLTAGE_MEASURED);
  Serial.print(",");
  Serial.print(STATOR_AND_MAIN_BATT_IN_VOLTAGE_MEASURED);
  Serial.print(",");
  Serial.print(BBM_WINCH_OUT_VOLTAGE_MEASURED);
  Serial.print(",");
  Serial.print(STATOR_TO_BBM_CURRENT_MEASURED);
  Serial.print(",");
  Serial.print(MAIN_BATT_TO_BBM_CURRENT_MEASURED);
  Serial.print(",");
  Serial.print(BBM_TO_WINCH_AND_BATT_CURRENT_MEASURED);
  Serial.print(",");
  Serial.print(WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_MEASURED);
  Serial.print(",");
  Serial.print(BBM_TO_EBOX_CURRENT_MEASURED);
  Serial.println();
}
#endif

void write_to_card_if_present() {
  if (sd_present) {
    if ((minute() == 0 || minute() == 30) && !new_file_opened) {
      open_file_by_datetime();
    } else if (minute() != 0 && minute() != 30) {
      new_file_opened = false;
    }

    if (current_file) {
      current_time = now();

      if (second(current_time) == last_second) {

        last_sequence_number += 1;
      } else {
        last_second = second(current_time);
        last_sequence_number = 0;
      }


      current_file.print(string_buffer);
      current_file.print(",");
      current_file.print(BBM_EBOX_OUT_VOLTAGE_MEASURED);
      current_file.print(",");
      current_file.print(STATOR_AND_MAIN_BATT_IN_VOLTAGE_MEASURED);
      current_file.print(",");
      current_file.print(BBM_WINCH_OUT_VOLTAGE_MEASURED);
      current_file.print(",");
      current_file.print(STATOR_TO_BBM_CURRENT_MEASURED);
      current_file.print(",");
      current_file.print(MAIN_BATT_TO_BBM_CURRENT_MEASURED);
      current_file.print(",");
      current_file.print(BBM_TO_WINCH_AND_BATT_CURRENT_MEASURED);
      current_file.print(",");
      current_file.print(WINCH_BATT_AND_BBM_TO_WINCH_CURRENT_MEASURED);
      current_file.print(",");
      current_file.print(BBM_TO_EBOX_CURRENT_MEASURED);
      current_file.println();
      current_file.flush();
    }
  }
}


#include <Wire.h>

const int MPU_ADDR = 0x68;  
const int ALARM_PIN = 8;    

// رفعنا الرقم إلى 19000 لتقليل الحساسية وتجاهل دوران المروحة الطبيعي
const long VIBRATION_THRESHOLD = 19000; 

void setup() {
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW); 

  Wire.begin();
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission();

  // تأخير زمني لمدة ثانيتين: 
  // هذا السطر يمنح المروحة وقتاً لتدور وتستقر قبل أن يبدأ الحساس بالعمل
  delay(2000); 
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  if (Wire.available() >= 6) {
    int16_t ax = Wire.read() << 8 | Wire.read();
    int16_t ay = Wire.read() << 8 | Wire.read();
    int16_t az = Wire.read() << 8 | Wire.read();

    long totalAccel = sqrt((long)ax * ax + (long)ay * ay + (long)az * az);

    if (totalAccel > VIBRATION_THRESHOLD) {
      digitalWrite(ALARM_PIN, HIGH); 
      
      // التجميد النهائي: إيقاف النظام ولن يعمل إلا بضغط زر Reset
      while(true) {
        
      }
    }
  }

  delay(50); 
}

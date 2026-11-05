// تعريف عنوان موديول I2C للشاشة (0x27 بعد إزاحة البت تصبح 0x4E)
// إذا كانت شريحة الـ I2C لديك بعنوان 0x3F استبدلها بـ 0x7E
#define LCD_ADDR 0x4E 

char current_state = 0; // 0 = Normal, 1 = Trip

// دوال التحكم بشاشة LCD عبر شريحة التوسعة PCF8574
void LCD_I2C_Write(unsigned char data_val) {
    I2C1_Start();
    I2C1_Wr(LCD_ADDR);
    I2C1_Wr(data_val);
    I2C1_Stop();
}

void LCD_Pulse_Enable(unsigned char val) {
    LCD_I2C_Write(val | 0x04); // EN = 1
    Delay_us(50);
    LCD_I2C_Write(val & ~0x04); // EN = 0
    Delay_us(50);
}

void LCD_Send_Nibble(unsigned char nibble, unsigned char mode) {
    unsigned char data_packet;
    data_packet = (nibble & 0xF0) | 0x08 | mode; // 0x08 لإبقاء إضاءة الشاشة الخلفية ON
    LCD_Pulse_Enable(data_packet);
}

void LCD_Command(unsigned char cmd) {
    LCD_Send_Nibble(cmd & 0xF0, 0x00);        // النصف العلوي (RS=0)
    LCD_Send_Nibble((cmd << 4) & 0xF0, 0x00); // النصف السفلي
    Delay_ms(2);
}

void LCD_Char(unsigned char ch) {
    LCD_Send_Nibble(ch & 0xF0, 0x01);        // النصف العلوي (RS=1)
    LCD_Send_Nibble((ch << 4) & 0xF0, 0x01); // النصف السفلي
    Delay_us(50);
}

void LCD_Init_I2C() {
    Delay_ms(50);
    LCD_Send_Nibble(0x30, 0x00);
    Delay_ms(5);
    LCD_Send_Nibble(0x30, 0x00);
    Delay_us(150);
    LCD_Send_Nibble(0x30, 0x00);
    LCD_Send_Nibble(0x20, 0x00); // نمط 4-bit

    LCD_Command(0x28); // سطرين، خط 5x8
    LCD_Command(0x0C); // تشغيل العرض وإخفاء المؤشر
    LCD_Command(0x06); // إزاحة المؤشر تلقائياً لليمين
    LCD_Command(0x01); // مسح الشاشة
    Delay_ms(2);
}

void LCD_Set_Cursor(unsigned char row, unsigned char col) {
    unsigned char address = (row == 1) ? (0x80 + col - 1) : (0xC0 + col - 1);
    LCD_Command(address);
}

void LCD_Print(char *str) {
    while (*str) {
        LCD_Char(*str++);
    }
}

void main() {
    // 1. تحويل كافة أطراف البورت A إلى مداخل ومخارج رقمية
    ADCON1 = 0x06; 
    
    // 2. ضبط اتجاه الأطراف
    TRISA0_bit = 1; // Pin 2 (RA0) مدخل لإشارة الخطر من الأردوينو
    TRISB0_bit = 0; // Pin 33 (RB0) مخرج تحكم بالريليه
    TRISB1_bit = 0; // Pin 34 (RB1) مخرج الطنان Buzzer
    TRISB2_bit = 0; // Pin 35 (RB2) مخرج الليد الأخضر Normal
    TRISB3_bit = 0; // Pin 36 (RB3) مخرج الليد الأحمر Danger

    // 3. تهيئة منفذ I2C في الـ PIC بسرعة 100KHz
    I2C1_Init(100000);
    LCD_Init_I2C();

    // 4. الوضع المبدئي عند بدء التشغيل (مروحة تعمل - نظام سليم)
    PORTB.F0 = 0; // تشغيل موديول الريليه (Active-LOW) لدوران المروحة
    PORTB.F1 = 0; // الطنان مطفأ
    PORTB.F2 = 1; // الليد الأخضر يعمل
    PORTB.F3 = 0; // الليد الأحمر مطفأ

    LCD_Set_Cursor(1, 1);
    LCD_Print("SYSTEM: NORMAL  ");
    LCD_Set_Cursor(2, 1);
    LCD_Print("MOTOR : RUNNING ");

    while (1) {
        // فحص إشارة الخطر القادمة من الأردوينو على الطرف RA0
        if (PORTA.F0 == 1 && current_state == 0) {
            // حدث خطر واهتزاز زائد (Trip Triggered)
            PORTB.F0 = 1; // فصل الريليه فوراً لإيقاف المروحة
            PORTB.F1 = 1; // تشغيل الطنان الصوتي
            PORTB.F2 = 0; // إطفاء الليد الأخضر
            PORTB.F3 = 1; // إضاءة الليد الأحمر

            LCD_Set_Cursor(1, 1);
            LCD_Print("STATUS: DANGER! ");
            LCD_Set_Cursor(2, 1);
            LCD_Print("MOTOR : STOPPED ");

            current_state = 1;
        } 
        else if (PORTA.F0 == 0 && current_state == 1) {
            // عودة الحالة إلى الاستقرار
            PORTB.F0 = 0; // إعادة تشغيل الريليه
            PORTB.F1 = 0; // إيقاف الطنان
            PORTB.F2 = 1; // إضاءة الليد الأخضر
            PORTB.F3 = 0; // إطفاء الليد الأحمر

            LCD_Set_Cursor(1, 1);
            LCD_Print("SYSTEM: NORMAL  ");
            LCD_Set_Cursor(2, 1);
            LCD_Print("MOTOR : RUNNING ");

            current_state = 0;
        }
        
        Delay_ms(20);
    }
}

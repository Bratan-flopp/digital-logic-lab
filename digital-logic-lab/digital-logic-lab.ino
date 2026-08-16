//  ЦИФРОВАЯ ЛАБОРАТОРИЯ v1.0
//  Arduino Mega + OLED SH1106 128x64
//
//  СПИСОК ЛАБОРАТОРНЫХ РАБОТ:
//  Лаба 0  Засытавка (анимация глаз, пока ничего не выбрано)
//  Лаба 1  Логические элементы (AND, OR, NOT, XOR, NAND, NOR, XNOR)
//  Лаба 2  Триггеры (RS, T) 
//  Лаба 3  Подавитель дребезга (Debounce) 
//  Лаба 4  Регистр сдвига 
//  Лаба 5  Двоичный счётчик  

//  КАК ПЕРЕКЛЮЧАТЬ ЛАБЫ?
//  KNOPKA8 — каждое нажатие = следующая лаба

//  РАСПИНОВКА ТУМБЛЕРОВ (Лаба 1):
//  KNOPKA1 = выбор AND    KNOPKA5 = выбор NAND
//  KNOPKA2 = выбор OR     KNOPKA6 = выбор NOR
//  KNOPKA3 = выбор NOT    KNOPKA7 = выбор XNOR
//  KNOPKA4 = выбор XOR
//  KNOPKA5 = вход A (первый операнд)
//  KNOPKA6 = вход B (второй операнд)  
//  тумблер ВКЛЮЧЁН   пин читает HIGH (1)  digitalRead() = true
//  тумблер ВЫКЛЮЧЕН  пин читает LOW  (0)  digitalRead() = false

#include <U8g2lib.h>
#include <Wire.h>

// Инициализация дисплея SH1106, 128x64, I2C
// _1_ означает что используется минимальный буфер (экономия RAM)
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);


// ПИНЫ

#define KNOPKA1 A0
#define KNOPKA2 A1
#define KNOPKA3 A2
#define KNOPKA4 A3
#define KNOPKA5 A4
#define KNOPKA6 A5
#define KNOPKA7 A6
#define KNOPKA8 A7   // кнопка переключения лабораторной работы
#define SVETODIOD_AND 22  
#define SVETODIOD_OR 23   
#define SVETODIOD_NOT 24   
#define SVETODIOD_XOR 25  
#define SVETODIOD_NAND 26   
#define SVETODIOD_NOR 27   
#define SVETODIOD_XNOR 28   
#define SVETODIOD_STATUS 29   // мигает раз в секунду — плата работает

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ

//  Номер текущей лабораторной работы 
// 0 = заставка с глазами
// 1 = логические элементы
// 2 = триггеры   
// 3 = debounce  
// 4 = регистр  
// 5 = счётчик
int nomerLaby = 0;

// Сколько лабораторных работ всего (для зацикливания переключения)
#define VSEGO_LAB 6

//Переключение лабы кнопкой KNOPKA8 
// Нужно запоминать предыдущее состояние чтобы реагировать
// только на МОМЕНТ нажатия, а не на удержание
bool knopka8_predydushee = false;

//  Номер режима внутри Лабы 1 (логические элементы) 
// 0 = ничего не выбрано
// 1 = AND   2 = OR   3 = NOT   4 = XOR
// 5 = NAND  6 = NOR  7 = XNOR
int vybrannyiElement = 0;

// Читаются каждый loop() и используются везде
bool vhod_A = false;   // KNOPKA5 — первый  операнд
bool vhod_B = false;   // KNOPKA6 — второй операнд

// RS-триггер 
// Триггер запоминает своё состояние между нажатиями
bool trigger_sostoyanie = false;// текущий выход триггера Q
bool trigger_knopkaS_pred = false;// предыдущее состояние входа S
bool trigger_knopkaR_pred = false; // предыдущее состояние входа R

// T-триггер (переключающий) 
// Меняет состояние при каждом нажатии тактового входа
bool t_trigger_sostoyanie = false;
bool t_trigger_clk_pred = false;

// Счётчик нажатий (Лаба 5)
int   schetchik_znachenie = 0; // текущее значение счётчика
bool  schetchik_knopka_pred = false;  // предыдущее состояние кнопки CLK

// Регистр сдвига 8 бит (Лаба 4)
// Каждое нажатие CLK сдвигает биты влево, добавляя новый бит справа
byte  registr_znachenie = 0b00000000; // текущее содержимое регистра
bool  registr_clk_pred  = false;  // предыдущее состояние CLK
 //ПЕРЕМЕННЫЕ АНИМАЦИИ ГЛАЗ (Лаба 0)

// Конечный автомат анимации 4 состояния:
// 0 = покой (полуокружности стоят)
// 1 = морф в прямоугольник
// 2 = прямоугольники смотрят в сторону
// 3 = морф обратно в полуокружности
int  glaza_sostoyanie = 0;
unsigned long glaza_taimer= 0;      // время начала текущего состояния (мс)
unsigned long glaza_pauza= 3500;   // пауза перед следующей анимацией (мс)

// morphT — степень "прямоугольности":
// 0.0 = чистая полуокружность, 1.0 = чистый прямоугольник
float morphT = 0.0;

// Смещение глаз (куда смотрят):
// offX < 0 = влево, offX > 0 = вправо
// offY < 0 = вверх, offY > 0 = вниз
float glaza_smeshenie_X = 0.0;  // текущее смещение по X
float glaza_tsel_X  = 0.0;  // целевое  смещение по X
float glaza_smeshenie_Y  = 0.0;  // текущее смещение по Y
float glaza_tsel_Y = 0.0;  // целевое  смещение по Y
float glaza_skorost= 0.25; // скорость движения к цели

// Масштаб глаз: 1.0 = нормальный, 1.5 = в полтора раза больше
float glaza_masshtab= 1.0;
float glaza_masshtab_tsel = 1.0;
//счетчики
int debounce_bez_zaschity = 0;
int debounce_s_zaschitoy  = 0;
bool debounce_pred_bez = false;
bool debounce_pred_s   = false;

//ВСПОМОГАТЕЛЬНЫЕ МАТЕМАТИЧЕСКИЕ ФУНКЦИИ

// smooth(t) — плавное ускорение/торможение (ease in-out)
// Формула: t квадрат × (3 − 2t)
// smooth(0.0)=0  smooth(0.5)=0.5  smooth(1.0)=1
float smooth(float t) {
  return t * t * (3 - 2 * t);
}
// lerp(a, b, t) — линейная интерполяция между двумя числами
// t=0.0 → результат=a,  t=0.5  посередине,  t=1.0 результат=b
// Пример: lerp(0, 100, 0.25) = 25
float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}
// ПОДАВЛЕНИЕ ДРЕБЕЗГА КОНТАКТОВ
// Что такое дребезг?
// При замыкании любого механического контакта сигнал не сразу становится стабильным, а он быстро "прыгает" 0-1-0-1 несколько миллисекунд. Это называется дребезг (bounce).
// Как подавляем?
// Читаем пин два раза с паузой 5мс.
// Если оба чтения одинаковы, то сигнал стабильный, возвращаем его.
// Если отличаются, то это дребезг, возвращаем false (не считаем нажатием).
bool schitat_knopku_bez_drebezga(int nomer_pina) {
  bool pervoe_chtenie  = digitalRead(nomer_pina);
  delay(5);
  bool vtoroe_chtenie  = digitalRead(nomer_pina);
  return (pervoe_chtenie == vtoroe_chtenie) ? pervoe_chtenie : false;
}

// ЧТЕНИЕ ВСЕХ ВХОДОВ

// Вызывается в начале каждого loop()
// Все значения тумблеров читаются здесь и сохраняются в глобальные переменные
void chitat_vse_vhody() {
  // KNOPKA5 = вход A, KNOPKA6 = вход B
  vhod_A = digitalRead(KNOPKA5);
  vhod_B = digitalRead(KNOPKA6);
}


//ПЕРЕКЛЮЧЕНИЕ ЛАБОРАТОРНЫХ РАБОТ

// KNOPKA8 переключает лабу.
// чтобы одно нажатие = один переход, а не бесконечное переключение.
void perekluchit_labu_esli_nazhata() {
  bool knopka8_seychas = digitalRead(KNOPKA8);

  // кнопка только что нажата (была 0, стала 1)
  if (knopka8_seychas && !knopka8_predydushee) {
    nomerLaby++;
    if (nomerLaby >= VSEGO_LAB) nomerLaby = 0; // зацикливание
  }

  // Запоминаем для следующего кадра
  knopka8_predydushee = knopka8_seychas;
}


//  ЛОГИЧЕСКИЕ ЭЛЕМЕНТЫ
// Тумблеры KNOPKA1-7 выбирают логический элемент.
// Тумблеры KNOPKA5-6 задают входы A и B.
// На экране будет таблица истинности с подсветкой текущей строки.
// Все 7 светодиодов горят одновременно по своей логике.
// ВАЖНО: KNOPKA5 и KNOPKA6 используются и для выбора элемента (NAND и NOR), и как входы A и B. Это ограничение железа всего 8 тумблеров на всё.
// Определяем какой элемент выбран (первый включённый тумблер 1-7)
void opredelit_vybrannyy_element() {
  if (digitalRead(KNOPKA1)) vybrannyiElement = 1; // AND
  else if (digitalRead(KNOPKA2)) vybrannyiElement = 2; // OR
  else if (digitalRead(KNOPKA3)) vybrannyiElement = 3; // NOT
  else if (digitalRead(KNOPKA4)) vybrannyiElement = 4; // XOR
  else if (digitalRead(KNOPKA5)) vybrannyiElement = 5; // NAND
  else if (digitalRead(KNOPKA6)) vybrannyiElement = 6; // NOR
  else if (digitalRead(KNOPKA7)) vybrannyiElement = 7; // XNOR
  else vybrannyiElement = 0; // ничего → глаза
}
// Считаем результат логической операции для заданных входов
bool schitat_rezultat(int element, bool a, bool b) {
  switch (element) {
    case 1: return a && b; // AND:1 только если оба = 1
    case 2: return a || b; // OR: 1 если хотя бы один = 1
    case 3: return !a; // NOT:инверсия A (B не используется)
    case 4: return a ^ b;// XOR 1 если только один = 1
    case 5: return !(a && b); // NAND: инверсия AND
    case 6: return !(a || b); // NOR:  инверсия OR
    case 7: return !(a ^ b); // XNOR: инверсия XOR (равнозначность)
    default: return false;
  }
}
// Обновляем все 7 светодиодов. они горят всегда, независимо от режима экрана
void obnovit_svetodiody() {
  // Каждый светодиод показывает результат своего элемента
    digitalWrite(SVETODIOD_AND,    vhod_A && vhod_B);
  digitalWrite(SVETODIOD_OR,     vhod_A || vhod_B);
  digitalWrite(SVETODIOD_NOT,    !vhod_A);
  digitalWrite(SVETODIOD_XOR,    vhod_A ^ vhod_B);
  digitalWrite(SVETODIOD_NAND,   !(vhod_A && vhod_B));
  digitalWrite(SVETODIOD_NOR,    !(vhod_A || vhod_B));
  digitalWrite(SVETODIOD_XNOR,   !(vhod_A ^ vhod_B));
  // Мигает раз в секунду ЗНАЧИТ ВСЕ КЛАААССС
  // millis() / 500 = каждые 500мс увеличивается на 1, % 2 = чётное/нечётное = 0 или 1
  digitalWrite(SVETODIOD_STATUS, (millis() / 500) % 2);
}

// Рисуем таблицу истинности на экране
void narisovat_tablicu_istinnosti() {
  u8g2.setFont(u8g2_font_6x12_t_cyrillic);

  // Названия элементов — индекс соответствует vybrannyiElement
  const char* nazvaniya[] = {
    "",       // 0 — не используется
    "AND",    // 1
    "OR",     // 2
    "NOT",    // 3
    "XOR",    // 4
    "NAND",   // 5
    "NOR",    // 6
    "XNOR"    // 7
  };
  // Заголовок: название элемента + шапка таблицы
  u8g2.drawStr(0, 10, nazvaniya[vybrannyiElement]);
  u8g2.drawStr(40, 10, "A B | OUT");
  u8g2.drawHLine(0, 12, 128); // горизонтальная линия под заголовком

  // NOT имеет 1 вход, 2 строки (A=0 и A=1)
  // Остальные имеют 2 входа, 4 строки (00, 01, 10, 11)
  int kolichestvo_strok = (vybrannyiElement == 3) ? 2 : 4;

  for (int i = 0; i < kolichestvo_strok; i++) {
    // Разбиваем номер строки i на биты входов A и B:
    // i=0: A=0,B=0 | i=1: A=0,B=1 | i=2: A=1,B=0 | i=3: A=1,B=1
    bool A_stroki = (i>>1) & 1; // старший бит i
    bool B_stroki = (i>>0) & 1; // младший бит i
    bool rezultat_stroki = schitat_rezultat(vybrannyiElement, A_stroki, B_stroki);
    int y = 24 + i * 11; // Y-координата строки на экране
    bool eta_stroka_aktivna;
    if (vybrannyiElement == 3) {
      eta_stroka_aktivna = (A_stroki == vhod_A);
    } else {
      eta_stroka_aktivna = (A_stroki == vhod_A && B_stroki == vhod_B);
    }
    if (eta_stroka_aktivna) {
      u8g2.drawBox(38, y - 9, 90, 11); // белый прямоугольник-подсветка
      u8g2.setDrawColor(0); // текст чёрным по белому фону
    }
    // Печатаем строку таблицы
    char stroka[16];
    if (vybrannyiElement == 3) {
      sprintf(stroka, "%d   | %d", A_stroki, rezultat_stroki); // NOT: один вход
    } else {
      sprintf(stroka, "%d %d | %d", A_stroki, B_stroki, rezultat_stroki); // два входа
    }
    u8g2.drawStr(40, y, stroka);

    if (eta_stroka_aktivna) u8g2.setDrawColor(1); // вернуть белый цвет
  }

  // Внизу — живые значения прямо сейчас
  bool zhivoy_rezultat = schitat_rezultat(vybrannyiElement, vhod_A, vhod_B);
  char zhivaya_stroka[24];
  sprintf(zhivaya_stroka, ">>A=%d B=%d OUT=%d", vhod_A, vhod_B, zhivoy_rezultat);
  u8g2.drawStr(0, 62, zhivaya_stroka);
}

// ЛАБА 2: ТРИГГЕРЫ (закомментирована)
// Триггер — устройство с памятью. Запоминает своё состояние и меняет его только при определённых входных сигналах.
// RS-триггер:
// S (Set) = KNOPKA1  устанавливает выход Q в 1
// R (Reset) = KNOPKA2 сбрасывает выход Q в 0
// Запрещённое состояние: S=1 и R=1 одновременно
// T-триггер (Toggle): CLK = KNOPKA3  каждое нажатие переключает Q (0в1в0в1...)


void laba2_triggery() {
  bool vhod_S = digitalRead(KNOPKA1); // Set
  bool vhod_R = digitalRead(KNOPKA2); // Reset
  bool vhod_CLK = schitat_knopku_bez_drebezga(KNOPKA3); // тактовый вход

  // RS-триггер
  if (vhod_S && !vhod_R) {
    trigger_sostoyanie = true;   // S=1, R=0 → запомнить 1
  } else if (!vhod_S && vhod_R) {
    trigger_sostoyanie = false;  // S=0, R=1 → сбросить в 0
  }
  // S=1 R=1 запрещено, состояние не меняем
  // S=0 R=0 хранение, состояние не меняем

  // T-триггер: реагируем на передний фронт CLK
  if (vhod_CLK && !t_trigger_clk_pred) {
    t_trigger_sostoyanie = !t_trigger_sostoyanie; // переключаем
  }
  t_trigger_clk_pred = vhod_CLK;

  // Показываем на светодиодах
  digitalWrite(SVETODIOD_AND, trigger_sostoyanie);  // Q триггера RS
  digitalWrite(SVETODIOD_OR, !trigger_sostoyanie); // /Q (инверсный выход)
  digitalWrite(SVETODIOD_NOT,t_trigger_sostoyanie);  // Q триггера T
  digitalWrite(SVETODIOD_XOR, !t_trigger_sostoyanie); // /Q триггера T
}
void narisovat_ekran_triggerov() {
  u8g2.setFont(u8g2_font_6x12_t_cyrillic);

  bool vhod_S = digitalRead(KNOPKA1);
  bool vhod_R = digitalRead(KNOPKA2);

  // Заголовки двух триггеров
  u8g2.setCursor(0, 10);  
  u8g2.print("RS-триггер");
  u8g2.setCursor(70, 10); 
  u8g2.print("T-триггер");
  u8g2.drawHLine(0, 12, 128);

  // RS: входы
  char rs[24];
  sprintf(rs, "S=%d  R=%d", vhod_S, vhod_R);
  u8g2.drawStr(0, 25, rs);

  // RS: выход Q
  char rs_q[16];
  sprintf(rs_q, "Q=%d  /Q=%d", trigger_sostoyanie, !trigger_sostoyanie);
  u8g2.drawStr(0, 37, rs_q);

  // Запрещённое состояние
  if (vhod_S && vhod_R) {
  u8g2.setCursor(0, 52); 
  u8g2.print("!! ЗАПРЕЩЕНО !!");
  }

  // T: выход
  char t_q[16];
  sprintf(t_q, "Q=%d /Q=%d", t_trigger_sostoyanie, !t_trigger_sostoyanie);
  u8g2.drawStr(70, 25, t_q);

  // CLK состояние
  bool clk = digitalRead(KNOPKA3);
  u8g2.drawStr(70, 37, clk ? "CLK: 1" : "CLK: 0");
}

// ДЕМОНСТРАЦИЯ ДЕБАУНСА (закомментирована)
// Показывает разницу между кнопкой с дребезгом и без:
// Левый светодиод  без защиты (может мигать при нажатии)
// Правый светодиод  с защитой (стабильный сигнал)



void laba3_debounce() {
    // Без защиты. читаем напрямую

  bool bez_zashity = digitalRead(KNOPKA1);
    // С защитой. читаем через нашу функцию

  bool s_zashitoy  = schitat_knopku_bez_drebezga(KNOPKA1);

  // Считаем передние фронты
  if (bez_zashity && !debounce_pred_bez) debounce_bez_zaschity++;
  if (s_zashitoy  && !debounce_pred_s)   debounce_s_zaschitoy++;

  debounce_pred_bez = bez_zashity;
  debounce_pred_s   = s_zashitoy;

  digitalWrite(SVETODIOD_AND, bez_zashity); // левый  *- без защиты
  digitalWrite(SVETODIOD_OR,  s_zashitoy); // правый - с защитой
}
void narisovat_ekran_debounce() {
  u8g2.setFont(u8g2_font_6x12_t_cyrillic);

  u8g2.setCursor(0, 10);
  u8g2.print("ДРЕБЕЗГ");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setCursor(0, 28);
  u8g2.print("Без защиты: ");
  u8g2.print(debounce_bez_zaschity);

  u8g2.setCursor(0, 42);
  u8g2.print("С защитой:  ");
  u8g2.print(debounce_s_zaschitoy);

  u8g2.setCursor(0, 58);
  u8g2.print("К1 - жми!");
}

// РЕГИСТР СДВИГА (закомментирована)
// Регистр сдвига — цепочка триггеров.При каждом тактовом импульсе все биты сдвигаются на одну позицию, а на освободившееся место справа записывается новый бит с входа DATA.
// KNOPKA1 = DATA (какой бит добавить: 0 или 1)
// KNOPKA2 = CLK (тактовый импульс = сдвинуть)
// KNOPKA3 = RESET (сбросить всё в 0)
// Светодиоды 1-8 показывают содержимое регистра побитово


void laba4_registr() {
  bool data = digitalRead(KNOPKA1);
  bool clk = schitat_knopku_bez_drebezga(KNOPKA2);
  bool sbros= digitalRead(KNOPKA3);
  if (sbros) {
    registr_znachenie = 0; // сброс всех битов в 0
  }
  // Передний фронт CLK = сдвигаем
  if (clk && !registr_clk_pred) {
    registr_znachenie = (registr_znachenie<<1)|(data ? 1 : 0);
    // <<1 = сдвиг всех битов влево на 1
    // |data = добавить новый бит справа
  }
  registr_clk_pred = clk;
  // Показываем каждый бит на своём светодиоде
  digitalWrite(SVETODIOD_AND, (registr_znachenie >> 7) & 1); // бит 7 (старший)
  digitalWrite(SVETODIOD_OR, (registr_znachenie >> 6) & 1);
  digitalWrite(SVETODIOD_NOT, (registr_znachenie >> 5) & 1);
  digitalWrite(SVETODIOD_XOR, (registr_znachenie >> 4) & 1);
  digitalWrite(SVETODIOD_NAND,(registr_znachenie >> 3) & 1);
  digitalWrite(SVETODIOD_NOR, (registr_znachenie >> 2) & 1);
  digitalWrite(SVETODIOD_XNOR, (registr_znachenie >> 1) & 1);
  digitalWrite(SVETODIOD_STATUS,(registr_znachenie >> 0) & 1); // бит 0 (младший)
}
void narisovat_ekran_registra() {
  u8g2.setFont(u8g2_font_6x12_t_cyrillic); // перевод на русский, без русского u8g2_font_6x12_tr

  bool data = digitalRead(KNOPKA1);
  u8g2.setCursor(0, 10);
   u8g2.print("СДВИГ  DATA=");
    u8g2.print(data);

  u8g2.drawHLine(0, 12, 128);

  // 8 квадратиков — каждый бит
  for (int i = 7; i >= 0; i--) {
    int bit = (registr_znachenie >> i) & 1;
    int x = (7 - i) * 16 + 2;  // шаг 16px
    if (bit) {
      u8g2.drawBox(x, 18, 13, 13);      // закрашен = 1
    } else {
      u8g2.drawFrame(x, 18, 13, 13);    // контур = 0
    }
  }

  // Номера битов под квадратиками
  u8g2.setFont(u8g2_font_5x8_t_cyrillic); // аналогичная песня про русский язык
  for (int i = 7; i >= 0; i--) {
    char n[3];
    sprintf(n, "%d", i);
    u8g2.drawStr((7 - i) * 16 + 4, 42, n);
  }

  // Hex и Dec значение
  u8g2.setFont(u8g2_font_6x12_t_cyrillic);
  char hex[8];
  u8g2.setCursor(0, 58);
  u8g2.print("Цифры:");
  u8g2.print(registr_znachenie);
  u8g2.print("16-ая:");
  sprintf(hex, "%02X", registr_znachenie); // %3d вывести целое число, минимум 3 символа в ширину %02X вывести число в шестнадцатеричном виде
  u8g2.print(hex);
}

// ДВОИЧНЫЙ СЧЁТЧИК (комент)
// Счётчик считает нажатия кнопки CLK и показывает число в двоичном виде.
// Каждый светодиод = один бит числа (8 светодиодов = 8 бит = 0..255)
// KNOPKA1 = CLK (нажать = +1 к счётчику)
// KNOPKA2 = UP/DOWN (HIGH = считаем вверх, LOW = вниз)
// KNOPKA3 = RESET  (сбросить в 0)

void laba5_schetchik() {
  bool clk = schitat_knopku_bez_drebezga(KNOPKA1);
  bool napravlenie = digitalRead(KNOPKA2); // true = вверх, false = вниз
  bool sbros = digitalRead(KNOPKA3);

  if (sbros) {
    schetchik_znachenie = 0;
  }

  // Передний фронт CLK = считаем
  if (clk && !schetchik_knopka_pred) {
    if (napravlenie) {
      schetchik_znachenie++;  // вверх
      if (schetchik_znachenie > 255) schetchik_znachenie = 0; // переполнение
    } else {
      schetchik_znachenie--;// вниз
      if (schetchik_znachenie < 0) schetchik_znachenie = 255; // переполнение
    }
  }
  schetchik_knopka_pred = clk;
  // Показываем число на светодиодах в двоичном виде
  digitalWrite(SVETODIOD_AND, (schetchik_znachenie >> 7) & 1);
  digitalWrite(SVETODIOD_OR,  (schetchik_znachenie >> 6) & 1);
  digitalWrite(SVETODIOD_NOT, (schetchik_znachenie >> 5) & 1);
  digitalWrite(SVETODIOD_XOR, (schetchik_znachenie >> 4) & 1);
  digitalWrite(SVETODIOD_NAND, (schetchik_znachenie >> 3) & 1);
  digitalWrite(SVETODIOD_NOR,  (schetchik_znachenie >> 2) & 1);
  digitalWrite(SVETODIOD_XNOR, (schetchik_znachenie >> 1) & 1);
  digitalWrite(SVETODIOD_STATUS, (schetchik_znachenie >> 0) & 1);
}
void narisovat_ekran_schetchika() {
  u8g2.setFont(u8g2_font_6x12_t_cyrillic);

  bool napravlenie = digitalRead(KNOPKA2);
  u8g2.setCursor(0, 10);
  u8g2.print(napravlenie ? "СЧЁТЧИК ВВЕРХ" : "СЧЁТЧИК ВНИЗ");
  u8g2.drawHLine(0, 12, 128);

  // Те же квадратики что в лабе 4
  for (int i = 7; i >= 0; i--) {
    int bit = (schetchik_znachenie >> i) & 1;
    int x = (7 - i) * 16 + 2;
    if (bit) u8g2.drawBox(x, 18, 13, 13);
    else     u8g2.drawFrame(x, 18, 13, 13);
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  for (int i = 7; i >= 0; i--) {
    char n[3];
    sprintf(n, "%d", i);
    u8g2.drawStr((7 - i) * 16 + 4, 42, n);
  }

  u8g2.setFont(u8g2_font_6x12_t_cyrillic);
  char hex[8];
  u8g2.setCursor(0, 58);
  u8g2.print("Цифры:");
  u8g2.print(schetchik_znachenie);
  u8g2.print("16-ая:");
  sprintf(hex, "%02X", schetchik_znachenie);
  u8g2.print(hex);
}
// АНИМАЦИЯ ГЛАЗ 

void obnovit_animaciyu_glaz(unsigned long vremya_ms) {
  // Если показываем таблицу — глаза плавно "закрываются" (morphT 1.0)
  if (vybrannyiElement != 0) {
    morphT += (1.0 - morphT) * 0.3;
    return;
  }
  switch (glaza_sostoyanie) {
    // Состояние 0:ЧИИИЛ
    case 0:
      morphT             = 0.0;
      glaza_smeshenie_X  = 0.0;
      glaza_smeshenie_Y  = 0.0;
      glaza_tsel_X       = 0.0;
      glaza_tsel_Y       = 0.0;
      glaza_masshtab     = 1.0;

      if (vremya_ms - glaza_taimer > glaza_pauza) {
        glaza_sostoyanie = 1;
        glaza_taimer     = vremya_ms;
      }
      break;

    // Состояние 1: превращение в прямоугольники
    case 1:
      morphT += (1.0 - morphT) * 0.40;

      if (morphT >= 0.98) {
        morphT           = 1.0;
        glaza_sostoyanie = 2;
        glaza_taimer     = vremya_ms;

        glaza_tsel_X        = random(-11, 13);
        glaza_tsel_Y        = random(-10, 5);
        glaza_masshtab_tsel = random(12, 18) / 10.0;
        glaza_skorost       = random(5, 12) / 100.0;
      }
      break;

    // Состояние 2: смотрят в сторону
    case 2:
      glaza_smeshenie_X += (glaza_tsel_X - glaza_smeshenie_X) * glaza_skorost;
      glaza_smeshenie_Y += (glaza_tsel_Y - glaza_smeshenie_Y) * glaza_skorost;
      glaza_masshtab    += (glaza_masshtab_tsel - glaza_masshtab) * 0.05;

      if (vremya_ms - glaza_taimer > 1500) {
        glaza_tsel_X = 0.0;
        glaza_tsel_Y = 0.0;
      }
      if (vremya_ms - glaza_taimer > 2000) {
        glaza_sostoyanie = 3;
        glaza_taimer     = vremya_ms;
      }
      break;

    // Состояние 3: возврат к полуокру
    case 3:
      morphT            += (0.0 - morphT)            * 0.40;
      glaza_smeshenie_X += (0.0 - glaza_smeshenie_X) * glaza_skorost;
      glaza_smeshenie_Y += (0.0 - glaza_smeshenie_Y) * glaza_skorost;
      glaza_masshtab    += (1.0 - glaza_masshtab)     * 0.05;

      if (morphT <= 0.02) {
        morphT           = 0.0;
        glaza_sostoyanie = 0;
        glaza_taimer     = vremya_ms;
        glaza_pauza      = random(2000, 5000);
      }
      break;
  }
}
// Рисуем один глаз
// cx, cy  — центр глаза на экране (пиксели)
// t  — форма: 0.0=полуокружность, 1.0=прямоугольник
// offX/Y — смещение (куда смотрит)
// sc — масштаб
void narisovat_glaz(int cx, int cy, float t, float offX, float offY, float sc) {
  int ox = cx + (int)offX;
  int oy = cy + (int)offY;

  // Чистая полуокру
  if (t <= 0.0) {
    u8g2.drawEllipse(ox, oy,
      (int)(10 * sc),
      (int)(10 * sc),
      U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    return;
  }

  // Чистый прямоугольник
  if (t >= 1.0) {
    int rw = (int)(12 * sc);
    int rh = (int)(20 * sc);
    u8g2.drawRBox(ox - rw / 2, oy - rh / 2 - 4, rw, rh, 4);
    return;
  }

  // Переходное состояние — две половины для плавности
  if (t < 0.5) {
    float tt = smooth(t * 2.0);
    u8g2.drawEllipse(ox, oy,
      (int)(lerp(10, 6, tt) * sc),
      (int)(lerp(10, 5, tt) * sc),
      U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  } else {
    float tt = smooth((t - 0.5) * 2.0);
    int rw = (int)(12 * sc);
    int rh = (int)(lerp(10, 20, tt) * sc);
    int ry = oy - rh / 2 - (int)(lerp(0, 4, tt));
    u8g2.drawRBox(ox - rw / 2, ry, rw, rh, 4);
  }
}


//ОБНОВЛЕНИЕ ЭКРАНА

void obnovit_ekran() {
  unsigned long vremya_ms = millis();

  u8g2.firstPage();
  do {
    switch (nomerLaby) {
      case 0:
        obnovit_animaciyu_glaz(vremya_ms);
        narisovat_glaz(43, 30, morphT, glaza_smeshenie_X, glaza_smeshenie_Y, glaza_masshtab);
        narisovat_glaz(82, 30, morphT, glaza_smeshenie_X, glaza_smeshenie_Y, glaza_masshtab);
        break;

      case 1:
        if (vybrannyiElement == 0) {
          obnovit_animaciyu_glaz(vremya_ms);
          narisovat_glaz(43, 30, morphT, glaza_smeshenie_X, glaza_smeshenie_Y, glaza_masshtab);
          narisovat_glaz(82, 30, morphT, glaza_smeshenie_X, glaza_smeshenie_Y, glaza_masshtab);
        } else {
          narisovat_tablicu_istinnosti();
        }
        break;

      case 2: narisovat_ekran_triggerov();  break;
      case 3: narisovat_ekran_debounce();   break;
      case 4: narisovat_ekran_registra();   break;
      case 5: narisovat_ekran_schetchika(); break;
    }
  } while (u8g2.nextPage());
}


// SETUP (выполняется один раз при включении)

void setup() {
  u8g2.begin();
  u8g2.enableUTF8Print();

  // Настраиваем тумблеры как входы
  // INPUT (без подтяжки) — тумблеры подключены на +5В
  // тумблер ВКЛ = HIGH = true, тумблер ВЫКЛ = LOW = false
  pinMode(KNOPKA1, INPUT);
  pinMode(KNOPKA2, INPUT);
  pinMode(KNOPKA3, INPUT);
  pinMode(KNOPKA4, INPUT);
  pinMode(KNOPKA5, INPUT);
  pinMode(KNOPKA6, INPUT);
  pinMode(KNOPKA7, INPUT);
  pinMode(KNOPKA8, INPUT);
  // Настраиваем светодиоды как выходы
  pinMode(SVETODIOD_AND,    OUTPUT);
  pinMode(SVETODIOD_OR,     OUTPUT);
  pinMode(SVETODIOD_NOT,    OUTPUT);
  pinMode(SVETODIOD_XOR,    OUTPUT);
  pinMode(SVETODIOD_NAND,   OUTPUT);
  pinMode(SVETODIOD_NOR,    OUTPUT);
  pinMode(SVETODIOD_XNOR,   OUTPUT);
  pinMode(SVETODIOD_STATUS, OUTPUT);
  // Случайное зерно от незаземлённого пина (шум = случайность)
  randomSeed(analogRead(A15));
  // Запускаем таймер анимации глаз
  glaza_taimer = millis();
}


//  LOOP (выполняется бесконечно)

void loop() {

  // Шаг 1: читаем все тумблеры
  chitat_vse_vhody();

  // Шаг 2: проверяем кнопку переключения лабораторной
  perekluchit_labu_esli_nazhata();

  // Шаг 3: выполняем логику текущей лабораторной
  switch (nomerLaby) {
    case 0:
      // Заставка — только анимация глаз, ничего не делаем
      break;

    case 1:
      // Лаба 1: логические элементы
      opredelit_vybrannyy_element();
      obnovit_svetodiody();
      break;

    // РАСКОМЕНТ И КОММЕНТ В СООТВЕТСВТИИ К ЛОАБЕ
    case 2: laba2_triggery();  break;
    case 3: laba3_debounce();  break;
    case 4: laba4_registr();   break;
    case 5: laba5_schetchik(); break;
  }

  // Шаг 4: обновляем экран
  obnovit_ekran();

  // Шаг 5: пауза 20мс = +/-50 кадров в секунду
  delay(20);
}

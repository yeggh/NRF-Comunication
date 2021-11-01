#include <Keypad.h>
#include <Arduino.h>

const int KEY_PAD_ROWS = 4;
const int KEY_PAD_COLS = 3;

byte keys[KEY_PAD_ROWS][KEY_PAD_COLS] = {
  {1, 2, 3},
  {4, 5, 6},
  {7, 8, 9},
  {10, 11, 12}
};



byte rowPins[KEY_PAD_ROWS] = {8, 7, 6, 5}; //connect to the row pinouts of the keypad

byte colPins[KEY_PAD_COLS] = {2, 3, 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, KEY_PAD_ROWS, KEY_PAD_COLS );

void configure_keypad_pins(byte row_pins[KEY_PAD_ROWS], byte col_pins[KEY_PAD_COLS]) {
	for (int i = 0; i < KEY_PAD_ROWS; i++)
		rowPins[i] = row_pins[i];
	for (int i = 0; i < KEY_PAD_COLS; i++)
		colPins[i] = col_pins[i];
}

int get_number() {
  int key = keypad.getKey();
  //Serial.print("Got number: ");
  //Serial.println(key);
  if (key < 10 && key > 0) 
    return key;
  if (key == 11)
    return 0;
  return -1;
}

struct Editor{
  const int LINE_NUM = 4;
  const int LINE_SIZE = 14;
  const int CLOCK_LIMIT = 10;
  const String button_strings[12] = {
    "1", "2ABC", "3DEF", "4GHI", "5JKL", "6MNO", "7PQRS", "8TUV", "9WXYZ", "*", "0", "#"
  };

  bool returned = false;
  bool entered = false;
  String packet = "";
  int cur_r = 0;
  int cur_c = 0;
  char text[5][14];
  int press_counter = 0;
  int current_clock = 0;
  int last_key = 0; // if zero then the key pressing has not started yet

  Editor() {
    for (int i = 0; i < LINE_NUM; i++)
      for (int j = 0; j < LINE_SIZE; j++) 
        text[i][j] = ' ';
    text[cur_r][cur_c] = '_';
    
  }

  void move_front() {
    if (cur_r == LINE_NUM){
      return;
    }
    cur_c++;
    if (cur_c == LINE_SIZE) {
      cur_r++;
      cur_c = 0;
    }
  }
  
  void move_back() {
    if (cur_r == 0 && cur_c == 0){
      return;
    }
    cur_c--;
    if (cur_c < 0) {
      cur_r--;
      cur_c = LINE_SIZE - 1;
    }
  }
  
  void output_character(int key, int counter, bool is_real) {
    if (key == 10) {
      text[cur_r][cur_c] = ' ';
      move_back();
      text[cur_r][cur_c] = '_';
    } else if (key == 11) {
      for (int i = 0; i < cur_r * LINE_SIZE + cur_c; i++) {
        packet += text[i/LINE_SIZE][i%LINE_SIZE];
      }
      entered = true;
    } else if (key == 12) {
      returned = true;
    } else {
      char c = button_strings[key-1][counter % button_strings[key-1].length()];
      text[cur_r][cur_c] = c;
      if (is_real) {
        move_front();
        text[cur_r][cur_c] = '_';
      }
    }
    if (is_real) {
      last_key = 0;
      press_counter = 0;
    }
  }

  void handle_keys() {
    int key = keypad.getKey();
    if (key != NO_KEY) {
      if (key == 1 || key > 9) {
        output_character(key, 0, true); 
      } else {
        if (last_key != 0 && key != last_key) {
          output_character(last_key, press_counter, true);
        } else if (key == last_key) {
          press_counter++;
        }
        current_clock = 0;
        last_key = key;
        output_character(key, press_counter, false);
      }
    }
    if (current_clock > CLOCK_LIMIT) {
      if (last_key != 0) {
        output_character(last_key, press_counter, true);
      }
      current_clock = 0;
    }
    current_clock++;
  }
};


int get_input() {
  int t = keypad.getKey();
  if(t){
    Serial.print("Got input: ");
    Serial.println(t);
  }
  return t;
}

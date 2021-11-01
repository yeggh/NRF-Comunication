// nrf final!

#include "KEYPAD_Functions.h"
#include "LCD_Functions.h"

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <string.h>

RF24 radio(8, 9); // CNS, CE

void send_package(String packet, byte address);
bool check_sent(byte address);
String receive_package(byte address);

//int PA_LEVEL;
//int FREQUENCY;

//int CHANNEL = -1; // between [0, 128)

//const int PA_OPTIONS_SIZE = 5;
//String pa_levels[PA_OPTIONS_SIZE] = {"DEF", "MIN", "LOW", "MAX", "HIGH"};
//int pa_level_index = 0;

//const int RATE_OPTIONS_SIZE = 4;
//String rates[RATE_OPTIONS_SIZE] = {"DEF", "250kb", "rf_1mb", "rf_2mb"};
//int rate_index = 0;

const byte NRFaddress[] = "00002";

bool setup_receiver = false;

void setup() {
    
    pinMode(A4, INPUT);
    digitalWrite(A4, LOW);
    pinMode(A3, INPUT);
    digitalWrite(A3, LOW);
    pinMode(A0, INPUT);
    digitalWrite(A0, LOW);
    pinMode(A5, INPUT);
    digitalWrite(A5, LOW);
    pinMode(A1, INPUT);
    digitalWrite(A1, LOW);

  // put your setup code here, to run once:
  byte row_bytes[4] = {A0, A4, A3, 10};
  //byte col_bytes[3] = {2, A5, A1};
  byte col_bytes[3] = {2, A5, A1};
  configure_keypad_pins(row_bytes, col_bytes);// first row pins then the column pins
  configure_lcd_pins(6, 7, 5, 4, 3, 0);
  
  Serial.begin(4800);

  lcdBegin(); // This will setup our pins, and initialize the LCD
  delay(2000);
  
  setContrast(60); // Good values range from 40-60
  delay(2000);
  clearDisplay(BLACK);
  updateDisplay();
}


void loop()
{
   menu_handler();
}

const int PAGE_LOOP_SIZE = 10;
int page = 1; 
// 1 for select menu 
// 3 enter address
// send: 2 for editor menu 4 for accepted

Editor* editor;
int page_loop = 0;
String packet;
bool send_mode = false;
int selected_item = 0;

int waiting_start;

const int WAITING_LIMIT = 1000;
const int ATTEMPT_LIMIT = 3;

byte address = 0;
int cnt_input_address = 0;
int attempt_counter = 0;

int getMod(int t, int mod) {
  return (t % mod + mod) % mod;
}

void menu_handler() {
  String output;
  int d, t;
  String item1, item2, item3;

  switch(page) {
    case 1:
      clearDisplay(WHITE);
      setStr("WELCOME TO NRF", 0, 0, BLACK);
      item1 = "recieve mode";
      item2 = "send mode";
      item3 = "setting";
      
      if (getMod(selected_item, 3) == 0) {
        item1 = ">" + item1;
      } else if (getMod(selected_item, 3) == 1) {
        item2 = ">" + item2; 
      } else {
        item3 = ">" + item3;
      }
      setStr(item1, 0, 10, BLACK);
      setStr(item2, 0, 20, BLACK);
      setStr(item3, 0, 30, BLACK);
      updateDisplay();
      t = get_input();
      if (t == 11) {
        if (getMod(selected_item, 3) == 1) {
          send_mode = true;
          editor = new Editor();
          page = 2;
        } else if (getMod(selected_item, 3) == 0) {
          send_mode = false;
          page = 3;
        } else {
          //page = 11;
        }
      }
      if (t == 8) {
        selected_item++;
      } else if(t == 2) {
        selected_item--;
      }
      break;
      
    case 2: // maale sender
      editor->handle_keys();
      clearDisplay(WHITE);
      setStr("--Enter Text--", 0, 0, BLACK);
      
      for (int i = 0; i < editor->LINE_NUM; i++) {
        for (int j = 0; j < editor->LINE_SIZE; j++) {
          setChar(editor->text[i][j], j * 6, (i + 1) * 10, BLACK);
        }
      }
      updateDisplay();
      
      if (editor->returned) {
        page = 1;
      }
      if (editor->entered) {
        packet = editor->packet;
        page = 3;
      }
      break;
    case 3: // enter address
      clearDisplay(WHITE);
      if (send_mode) {
        setStr("Your packet is", 0, 0, BLACK);
        output = "";
        for (int i = 0; i < packet.length() && i < 14-3; i++)
         output += packet[i];
        if (packet.length() > 14-3)
          output += "...";
        setStr(output, 0, 10, BLACK);
      } else {
        setStr("To recieve", 0, 0, BLACK);
        setStr("enter address", 0, 10, BLACK);
      }
      setStr("Enter two digi", 0, 20, BLACK);
      setStr("ts for address", 0, 30, BLACK);
      setStr(">", 0, 40, BLACK);
      setChar(char('0' + address/10), 6, 40, BLACK);
      setChar(char('0' + address%10), 12, 40, BLACK);
      updateDisplay();
      d = get_number();
      if (d != -1) {
        if (cnt_input_address < 2)
          address = address * 10 + d;
        cnt_input_address++;
        if (cnt_input_address == 3) {
          if (send_mode) {
            page = 10;
            clearDisplay(WHITE);
            setStr(packet, 0, 0, BLACK);
            updateDisplay();
            delay(1000);
            send_package(packet, address);
            page_loop = 0;
          } else {
            page = 5;
            waiting_start = millis();
          }
        }
      }
      break;
    case 4: // sent successfully
      clearDisplay(WHITE);
      setStr("Your packet:", 0, 0, BLACK);
      output = "";
      for (int i = 0; i < packet.length() && i < 14-3; i++)
        output += packet[i];
      if (packet.length() > 14-3)
        output += "...";
      setStr(output, 0, 10, BLACK);
      setStr("has been sent", 0, 20, BLACK);
      output = "on address ";
      output += char(address / 10 + '0');
      output += char(address % 10 + '0');
      setStr(output, 0, 30, BLACK); 
      updateDisplay();
      
      break;
    case 5: // waiting page
      clearDisplay(WHITE);
      setStr("Waiting...", 0, 0, BLACK);
      updateDisplay();
      if (millis() - waiting_start > 10 * WAITING_LIMIT) {
        page = 6;
      } else {
        String rec = receive_package(address);
        if (rec != "NULL") {
          clearDisplay(WHITE);
          setStr(rec, 0, 0, BLACK);
          updateDisplay();
          delay(5000);
          page = 7;
        }
      }
      break;
    case 6: // failed to receive
      clearDisplay(WHITE);
      setStr("Failed to", 0, 0, BLACK);
      setStr("receive!", 0, 10, BLACK);
      updateDisplay();
     
      break;
    case 7: // successfully received
      clearDisplay(WHITE);
      setStr("successfully", 0, 0, BLACK);
      setStr("received!", 0, 10, BLACK);
      updateDisplay();
      
      break;
    case 10: // sending page
      clearDisplay(WHITE);
      setStr("Sending...", 0, 0, BLACK);
      updateDisplay();
      
      if (attempt_counter > ATTEMPT_LIMIT) {
        clearDisplay(WHITE);
        setStr("Done Sending!", 0, 0, BLACK);
        updateDisplay();
        delay(5000);
        page = 1;
      } else {
        if (check_sent(address)){
          page = 4;
        } else {
           attempt_counter++;
           
           clearDisplay(WHITE);
           setStr("Sending Again!", 0, 0, BLACK);
           updateDisplay();
      
           send_package(packet, address);
           delay(1000);
        }
      }
      break;
    
    /*
    case 11:
      clearDisplay(WHITE);
      setStr("Setting", 0, 0, BLACK);
      // channel:
      String chnl = "";
      if (CHANNEL != -1) {
        int tmp = CHANNEL;
        while (tmp > 0) {
          chnl = char('0' + tmp % 10) + chnl;
          tmp /= 10;
        }
        if (chnl == "")
          chnl = "0";
      } else
        chnl = "DEF";
      item1 = "Chnl: " + chnl;

      // rate
      item2 = "Rate: " + rates[rate_index];

      // PA
      item3 = "PA: " + pa_levels[pa_level_index];

      if (getMod(selected_item, 3) == 0) {
        item1 = ">" + item1;
      } else if(getMod(selected_item, 3) == 1) {
        item2 = ">" + item2; 
      } else {
        item3 = ">" + item3;
      }
      setStr(item1, 0, 10, BLACK);
      setStr(item2, 0, 20, BLACK);
      setStr(item3, 0, 30, BLACK);
      updateDisplay();
      t = get_input();
      if (t == 11) {
        page = 1;
      } else if (t == 2) {
        selected_item--;
      } else if (t == 8) {
        selected_item++;
      } else if (t == 4 || t == 6) {    
        if (getMod(selected_item, 3) == 0) {
          if (t == 4) {
            if (CHANNEL > 0)
              CHANNEL = max(CHANNEL - 8, 0);
            if (CHANNEL == 0)
              CHANNEL = -1;
          }
          if (t == 6) {
            if (CHANNEL == -1)
              CHANNEL = 0;
            else
              CHANNEL = min(CHANNEL + 8, 128 - 8);
          }
          Serial.println(CHANNEL);
         } else if(getMod(selected_item, 3) == 1) {
            if (t == 4)
              rate_index = max(rate_index - 1, 0);
            if (t == 6)
              rate_index = min(rate_index + 1, RATE_OPTIONS_SIZE - 1);
         } else {
            if (t == 4)
              pa_level_index = max(pa_level_index - 1, 0);
            if (t == 6)
              pa_level_index = min(pa_level_index + 1, PA_OPTIONS_SIZE - 1);  
         }
      }
       
      break;
      */
  }
  page_loop = (page_loop + 1) % PAGE_LOOP_SIZE;
}

// SENDER:
// try to send a package
void send_package(String packet, byte address) { // address has two digits
    radio.begin(); 
    /*
    if (CHANNEL != -1)
      radio.setChannel(CHANNEL);
    */
    
    radio.openWritingPipe(NRFaddress);
    radio.setPALevel(RF24_PA_MIN);
    
    /*
    switch(pa_level_index) {
      case 0:
      case 1:
        radio.setPALevel(RF24_PA_MIN);
        break;
      case 2:
        radio.setPALevel(RF24_PA_LOW);
        break;
      case 3:
        radio.setPALevel(RF24_PA_MAX);
        break;
      case 4:
        radio.setPALevel(RF24_PA_HIGH);
        break;
    }

    
     switch(rate_index) {
       case 1:
          radio.setDataRate(RF24_250KBPS);
          break;
       case 2:
          radio.setDataRate(RF24_1MBPS);
          break;
       case 3:
          radio.setDataRate(RF24_2MBPS);
          break;
     }
     */
      
    radio.stopListening();
    char text[100];
    //sprintf(text, "%d, ", *my_address);
    
    for (int i = 0; i < packet.length(); i++)
      text[i] = packet[i]; 
    text[packet.length()] = address;
    text[packet.length() + 1] = '\0';
    radio.write(&text, sizeof(text));
    
    if (send_mode) {
      radio.openReadingPipe(0, NRFaddress);
       radio.setPALevel(RF24_PA_MIN);
      radio.startListening();
    }
}

// SENDER:
// returns true if package has been received by receiver 
bool has_been_sent = false;
bool check_sent(byte address) {
  if (has_been_sent)
    return true;
  if (radio.available()) {
    char text[100] = "";
    radio.read(&text, sizeof(text));
    char ack[10];
    sscanf(text, "%s", ack);
    if (strcmp(ack, "ACK")) {
      return has_been_sent = true;
    }
  } else {
  }
  return false;
}

// RECIEVER
// returns true if package has been received by receiver
// and send ACK to sender  
String receive_package(byte address) {
  if(!setup_receiver){
    radio.begin();
    /*
    if (CHANNEL != -1)
      radio.setChannel(CHANNEL);
    }
    */
    
    radio.openReadingPipe(0, NRFaddress);
    radio.setPALevel(RF24_PA_MIN);
    
    /*
    switch(pa_level_index) {
      case 0:
      case 1:
        radio.setPALevel(RF24_PA_MIN);
        break;
      case 2:
        radio.setPALevel(RF24_PA_LOW);
        break;
      case 3:
        radio.setPALevel(RF24_PA_MAX);
        break;
      case 4:
        radio.setPALevel(RF24_PA_HIGH);
        break;
     }
     switch(rate_index) {
       case 1:
          radio.setDataRate(RF24_250KBPS);
          break;
       case 2:
          radio.setDataRate(RF24_1MBPS);
          break;
       case 3:
          radio.setDataRate(RF24_2MBPS);
          break;
     }

     */
    radio.startListening();
    setup_receiver = true;
  }
  
  if (radio.available()) {
    char text[100] = "";
    radio.read(&text, sizeof(packet));
    int len = 0;
    for(;text[len];len++);
    byte income_address = text[len - 1];
    String ret = "";
    for (int i = 0; i < len - 1; i++)
      ret += text[i]; 
    if (address == income_address) {
      send_package("ACK", income_address);
      return ret;
    }
  }
  return String("NULL");
}

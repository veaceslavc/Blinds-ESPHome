#include "esphome.h"

#define TRANSMIT_PIN             D2
#define REPEAT_COMMAND           8       // How many times to repeat the same command: original remotes repeat 8 (multi) or 10 (single) times by default
#define DEBUG                    false   // Do note that if you add serial output during transmit, it will cause delay and commands may fail

#define AOK_DOWN                        "10100011010001011101111011011110001111110000000001000011100000111"
#define AOK_UP                          "10100011010001011101111011011110001111110000000000001011010010111"
#define AOK_AFTER_UP_DOWN               "10100011010001011101111011011110001111110000000000100100011001001"
#define AOK_STOP                        "10100011010001011101111011011110001111110000000000100011011000111"
#define AOK_PROGRAM                     "10100011010001011101111011011110001111110000000001010011100100111"

#define AOK_AGC1_PULSE                   5300  // 234 samples
#define AOK_AGC2_PULSE                   530   // 24 samples after the actual AGC bit
#define AOK_RADIO_SILENCE                5030  // 222 samples

#define AOK_PULSE_SHORT                  270   // 12 samples
#define AOK_PULSE_LONG                   565   // 25 samples, approx. 2 * AOK_PULSE_SHORT

#define AOK_COMMAND_BIT_ARRAY_SIZE       65    // Command bit count


class BlindsCover : public Component, public Cover {
 public:
  void setup() override {
    // This will be called by App.setup()
    pinMode(LED_BUILTIN, OUTPUT);
  }

  CoverTraits get_traits() override {
    auto traits = CoverTraits();
    traits.set_is_assumed_state(true);
    traits.set_supports_position(false);
    traits.set_supports_tilt(false);
    traits.set_supports_toggle(false);
    return traits;
  }

  void control(const CoverCall &call) override {
    // This will be called every time the user requests a state change.
    if (call.get_position().has_value()) {
      float pos = *call.get_position();
      // Write pos (range 0-1) to cover
      if (pos == 0) {
        sendAOKCommand(AOK_DOWN);
      }
      if (pos == 1) {
        sendAOKCommand(AOK_UP);
      }
    }
    if (call.get_stop()) {
      // User requested cover stop
      sendAOKCommand(AOK_STOP);
    }
  }


void sendAOKCommand(char* command) {
  
  if (command == NULL) {
    ESP_LOGE("blinds_comp", "sendAOKCommand(): Command array pointer was NULL, cannot continue.");
    return;
  }

  // Prepare for transmitting and check for validity
  pinMode(TRANSMIT_PIN, OUTPUT); // Prepare the digital pin for output
  
  if (strlen(command) < AOK_COMMAND_BIT_ARRAY_SIZE) {
    ESP_LOGE("blinds_comp", "sendAOKCommand(): Invalid command (too short), cannot continue.");
    return;
  }
  if (strlen(command) > AOK_COMMAND_BIT_ARRAY_SIZE) {
    ESP_LOGE("blinds_comp", "sendAOKCommand(): Invalid command (too long), cannot continue.");
    return;
  }
  
  // Repeat the command:
  for (int i = 0; i < REPEAT_COMMAND; i++) {
    doAOKTribitSend(command);
  }

  // Disable output to transmitter to prevent interference with
  // other devices. Otherwise the transmitter will keep on transmitting,
  // disrupting most appliances operating on the 433.92MHz band:
  digitalWrite(TRANSMIT_PIN, LOW);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void doAOKTribitSend(char* command) {

  // Starting (AGC) bits:
  transmitHigh(AOK_AGC1_PULSE);
  transmitLow(AOK_AGC2_PULSE);

  // Transmit command:
  for (int i = 0; i < AOK_COMMAND_BIT_ARRAY_SIZE; i++) {

      // If current bit is 0, transmit HIGH-LOW-LOW (100):
      if (command[i] == '0') {
        transmitHigh(AOK_PULSE_SHORT);
        transmitLow(AOK_PULSE_LONG);
      }

      // If current bit is 1, transmit HIGH-HIGH-LOW (110):
      if (command[i] == '1') {
        transmitHigh(AOK_PULSE_LONG);
        transmitLow(AOK_PULSE_SHORT);
      }   
   }

  // Radio silence at the end.
  // It's better to go a bit over than under minimum required length:
  transmitLow(AOK_RADIO_SILENCE);
  
/*
  if (DEBUG) {
    Serial.println();
    Serial.print("Transmitted ");
    Serial.print(AOK_COMMAND_BIT_ARRAY_SIZE);
    Serial.println(" bits.");
    Serial.println();
  }
*/
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitHigh(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, HIGH);
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitLow(int delay_microseconds) {
  digitalWrite(TRANSMIT_PIN, LOW);
  delayMicroseconds(delay_microseconds);
}

};


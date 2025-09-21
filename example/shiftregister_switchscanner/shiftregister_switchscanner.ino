#include "shiftregister_switchscanner.hpp"

// create instance for switch scanner
kinoshita_lab::ShiftregisterSwitchScanner<8, 4, D4, D5, D3> switch_scanner; // put num switches, bounce_delay, #PL, #CP, Q7(Register_Out) to template parameter

void switchChanged(const uint32_t switch_index, const int off_on)
{
  Serial.print("Button changed: index: ");
  Serial.print(switch_index);
  Serial.print(", state = ");
  Serial.println(off_on ? "ON" : "OFF");
}
void setup() 
{
  Serial.begin(115200); // for serial monitor

  // configure function to call on switch change
  switch_scanner.onChange(switchChanged);
}

void loop() 
{
  switch_scanner.update();
}

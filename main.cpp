#include <iostream>
#include <pigpiod_if2.h>
#include <csignal>
#include <thread>
#include <algorithm>
#include "mcp3008.h"

using namespace std;
static bool is_cont;

struct joystick {
  int x;
  int y;
};

static joystick jdev = {0,0};

void finish_trap(int, void* user_data) {
  int *pPi = static_cast<int*>(user_data);
  if (*pPi >= 0) pigpio_stop(*pPi);
}

int main(int argc, char **argv)
{
  int pi;
  is_cont = true;
  on_exit(finish_trap, &pi);

  pi = pigpio_start(nullptr, nullptr);
  if (pi<0) {
      cerr << pigpio_error(pi) << endl;
      exit (pi);
    }

  sigset(SIGINT, [](int){
      is_cont = false;
    });
  cout << "Ctrl+C - exit" << endl;

  MCP3008 adc;
  adc.setChannels(0x83, 0x83);

  static MCP3008_CallBack adc_sens = [](map<int8_t, int16_t> data) {
    extern joystick jdev;
    for_each(data.begin(), data.end(), [&](std::pair<int8_t, int16_t> value){
             printf("ADC[%1d]=%4d;", value.first, value.second);
      });
    printf("\r");
    auto pdata = data.begin();
    jdev.x = pdata->second;
    pdata++;
    jdev.y = pdata->second;
  };

  for (unsigned char chn = 0; chn < 8; ++chn) {
      cout << "MCP3008[" << chn << "]::" << adc.singleMeasure(chn) << endl;
    }

  adc.run(adc_sens, {0,100});

  set_mode(pi, 18, PI_OUTPUT);
  set_mode(pi, 23, PI_OUTPUT);
  set_servo_pulsewidth(pi, 18, 1500);
  set_servo_pulsewidth(pi, 23, 1500);
  while (is_cont) {
      struct timeval tv = {0,1000};
      select(0,nullptr,nullptr,nullptr,&tv);
      set_servo_pulsewidth(pi, 18, 1000 + jdev.y);
      set_servo_pulsewidth(pi, 23, 1000 + jdev.x);
    }

  exit (EXIT_SUCCESS);
}

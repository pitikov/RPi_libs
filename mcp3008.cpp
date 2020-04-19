#include "mcp3008.h"
#include <pigpiod_if2.h>
#include <thread>
#include <cassert>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

MCP3008::MCP3008(char *shost, char *sport, unsigned int spi_channel, unsigned int brate, unsigned int flags)
  : pi(-1)
  , spi(-1)
  , channels(0)
  , nDiffs(0)
  , worker(nullptr)
  , is_finish(false)
{
  pi = pigpio_start(shost, sport);
  if (pi < 0) {
      pigpio_error(pi);
      assert(ENOENT);
    }
  spi = spi_open(pi, spi_channel, brate, flags);
  if (spi < 0) {
      pigpio_error(pi);
      assert(ENOENT);
    }
}

MCP3008::~MCP3008()
{
  stop();
}

uint16_t MCP3008::singleMeasure(uint8_t channel, bool nDiff)
{
  if (worker!=nullptr) {
      cerr << "Not in this time" << endl;
      return MCP3008_Error;
    }
  if (channel>7) {
      cerr << "Uncorrect channel number" << endl;
      return MCP3008_Error;
    }
  char request[3] = {
    1,                                                    // start bit
    char(((nDiff ? 1 : 0) << 7) | ((channel & 7) << 4)),  // Select mode and channel
    0,                                                    // not used
  };
  char answer[3];
  auto retsize = spi_xfer(pi, unsigned(spi), request, answer, 3);
  if (retsize != 3) {
      cerr << "Transmitt data error" << endl;
      return MCP3008_Error;
    }
  return uint16_t(uint16_t (answer[2]) | (uint16_t(answer[1]&3)<<8));
}

uint16_t MCP3008::setChannels(uint8_t channels_mask, uint8_t nDiff_mask)
{
  if (worker!=nullptr) {
      cerr << "Not in this time" << endl;
      return MCP3008_Error;
    }
  channels = channels_mask;
  nDiffs = nDiff_mask;
  return EXIT_SUCCESS;
}

uint16_t MCP3008::run(MCP3008_CallBack callback, timeval tv)
{
  if (channels == 0) {
      cerr << "Don`t have configured channels" << endl;
      return MCP3008_Error;
    }
  struct worker_params {
    MCP3008_CallBack fnc;
    struct timeval tv;
  } params = {callback, tv};

  worker = new thread([&](worker_params params){
      //todo prepare channels config
      struct CHN_CFG {
        char request[3];
        char answer[3];
      };
    vector<CHN_CFG> adc_cfg;
    for(int c=0; c<8; c++) {
      if (channels & (1<<c)) {
          struct CHN_CFG chn = {
            {   // Answer
              1,                                            // Start bit
              uint8_t(((nDiffs&(1<<c)?1:0)<<7)|((c&7)<<4)), // Mode and channel number
              0                                             // Unused
            },
            {0,0,0} // Request
          };
          adc_cfg.push_back(chn);
        }
    }
    while(!is_finish) {
      map<int8_t, int16_t> result;
      for_each(adc_cfg.begin(), adc_cfg.end(), [&](CHN_CFG data) {
          auto ret = spi_xfer(pi, unsigned(spi), data.request, data.answer, 3);
          if (ret == 3) {
              result.insert(std::pair<int8_t, int16_t>(
                              int8_t((data.request[1]&0x70)>>4),
                              int16_t((int16_t(data.answer[1]&3)<<8)|data.answer[2])
                  ));
            } else {
              result.insert(std::pair<int8_t, int16_t>(
                              int8_t((data.request[1]&0x70)>>4),
                              int16_t(MCP3008_Error)
                  ));
            }
        });
      params.fnc(result);
      auto tv = params.tv;
      select(0,nullptr, nullptr, nullptr, &tv);
      }
  }, params);
  return EXIT_SUCCESS;
}

void MCP3008::stop()
{
  if(worker) {
      is_finish = true;
      worker->join();
      delete worker;
    }
  cerr << "No running ADC" << endl;
}

#ifndef MCP3008_H
#define MCP3008_H

#include <pigpiod_if2.h>
#include <thread>
#include <map>

#define MCP3008_Error   (0xFFFF)
typedef void(*MCP3008_CallBack)(std::map<int8_t, int16_t>);

class MCP3008
{
public:
    explicit MCP3008(char* shost = nullptr, char* sport = nullptr, unsigned int spi_channel = 0, unsigned int brate = 100000, unsigned int flags = 0);
    virtual ~MCP3008();

    uint16_t singleMeasure(uint8_t channel, bool nDiff = true);
    uint16_t setChannels(uint8_t channels_mask, uint8_t nDiff_mask);
    uint16_t run(MCP3008_CallBack callback, struct timeval tv);
    void stop();

private:
    int pi;
    int spi;
    uint8_t channels;
    uint8_t nDiffs;
    std::thread* worker;
    bool is_finish;
};

#endif // MCP3008_H

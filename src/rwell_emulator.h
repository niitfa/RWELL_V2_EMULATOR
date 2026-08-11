#ifndef RWELL_EMULATOR_H
#define RWELL_EMULATOR_H

#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <arpa/inet.h>
#include <stdint.h>

// command list and sequence as it is in MCU program
enum RWELLCommandCode
{
    Empty = 0x00,
    Set_HV = 0x01,
    Set_Band = 0x02
};

// value list and sequence as it is in MCU program
enum RWELLValueCode
{
   	Message_Num,
    ADC_DR,
    HV_Out,
    Pressure,
    Temperature,
    Band,
	Size 
};

enum RWELLSocketState
{
    INIT,
    ESTABLISHED,
    CLOSED
};

class RWELLEmulator
{
    std::string ip;
    uint16_t port;

    // server var
    int socket_desc;
    int client_sock;
	struct sockaddr_in server;
    struct sockaddr_in client;
    int addr_size;

    static const int kRxBufferSize = 12;
    static const int kTxBufferSize = 64;
    uint8_t rxBuffer [kRxBufferSize];
	uint8_t txBuffer [kTxBufferSize];

    RWELLSocketState state = RWELLSocketState::CLOSED;

    int shortDelay_ms = 50;
    int longDelay_ms = 200;
    int delay_ms = longDelay_ms;

    struct ValueAddress
    {
        uint32_t offset;
        uint32_t size;
    };

    std::vector<ValueAddress> map;

    // emulated values
    int id = 0;
public:
    RWELLEmulator(std::string ip, uint16_t port);
    ~RWELLEmulator() = default;
    void loop();
private:
    bool socket_();
    bool listen_();
    void accept_();
    void disconnect_();
    void close_();
    int send_();
    int receive_();
    void sleep_(int ms);

    // msg handlers
    void rxHandle();
    void txHandle();

    // emulated values
    void increaseID();
    void emulateADCValue(int val);
    void emulateHVOut(uint16_t volt);
    void emulateTemperature(int celsius_0p01);
    void emulatePressure(int mbar_0p01);
    void emulateBand(uint8_t band);
};

#endif
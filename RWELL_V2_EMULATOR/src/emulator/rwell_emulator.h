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
    std::string ip = "0.0.0.0";
    uint16_t port = 22250;
    const struct timeval timeout = { 0, 200 * 1000 };

    // server var
    int socket_desc;
    int client_sock;
	struct sockaddr_in server;
    struct sockaddr_in client;
    int addr_size;

    // thread
    std::vector<int> cpus;
    std::atomic<bool> is_launched{false}; // для контроля наличия активного потока
    std::atomic<bool> is_initialized{false};  // для контроля блокировки в главном потоке в процессе инициализации
    std::atomic<bool> is_stop_forced{false}; // для размещения в условии while и остановки при вызове stop()
    std::mutex rxMutex, txMutex;

    std::function<void(void)> messageUpdatedCallback = [](void) {};

    static const int kRxBufferSize = 12;
    static const int kTxBufferSize = 64;
    uint8_t rxBuffer [kRxBufferSize];
	uint8_t txBuffer [kTxBufferSize];

    RWELLSocketState state = RWELLSocketState::CLOSED;

    int shortDelay_ms = 50;
    int longDelay_ms = 50;
    int delay_ms = longDelay_ms;

    struct ValueAddress
    {
        uint32_t offset;
        uint32_t size;
    };

    std::vector<ValueAddress> map;

    // emulated values
    int id = 0;
    uint8_t lastBand = 0;
    int activityCounts = 0;
    int noiseCounts = 0;
    int actualActivity = 0;
    bool activityEnabled = true;
    int averageTemperature = 2750;
    int temperature = averageTemperature; // 100 = 1 градус
    int averagePressure = 105500;
    int pressure = 100000; // 10000 = 1 бар
    int voltage = 0;
public:
    RWELLEmulator(std::string ip, uint16_t port);
    ~RWELLEmulator() = default;
    void setMessageUpdatedCallback(std::function<void ()>&);
    void start();
    void enableActivity(bool en);
    void setTemperature(int temperature);
    void setPressure(int pressure);
    void setVoltage(int voltage);

    int getTemperature();
    int getPressure();
    int getVoltage();
    int getActivity();
private:
    void handler();
    bool socket_();
    bool listen_();
    void accept_();
    void disconnect_();
    void close_();
    int send_();
    int receive_();
    void sleep_(int ms);
    void updateMessage_();

    // msg handlers
    void rxHandle();
    void txHandle();

    void updateActivity();
    void updateNoise();

    // emulated values
    void increaseID();
    void emulateADCValue(int val);
    void emulateHVOut(uint16_t volt);
    void emulateTemperature(int celsius_0p01);
    void emulatePressure(int mbar_0p01);
    void emulateBand(uint8_t band);

    double getBandFactor();
};

#endif

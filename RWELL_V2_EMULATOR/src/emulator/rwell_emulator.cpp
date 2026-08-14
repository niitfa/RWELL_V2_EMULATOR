#include "rwell_emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <math.h>

RWELLEmulator::RWELLEmulator(std::string ip, uint16_t port) : ip { ip }, port { port }
{
    memset(rxBuffer, 0, kRxBufferSize);
    memset(txBuffer, 0, kTxBufferSize);

    map = std::vector<ValueAddress>(RWELLValueCode::Size);
    map[RWELLValueCode::Message_Num]   = {0, 4};
    map[RWELLValueCode::ADC_DR]        = {4, 4};
    map[RWELLValueCode::HV_Out]        = {8, 2};
    map[RWELLValueCode::Pressure]      = {10, 4};
    map[RWELLValueCode::Temperature]   = {14, 4};
    map[RWELLValueCode::Band]          = {18, 1};
}

void RWELLEmulator::setMessageUpdatedCallback(std::function<void ()> &f)
{
     this->messageUpdatedCallback = f;
}

void RWELLEmulator::start()
{
    if (!is_launched.load())
    {
        is_initialized.store(false);
        is_stop_forced.store(false);
        is_launched.store(true);
        std::thread thrd(&RWELLEmulator::handler, this);
        thrd.detach();
        while (!is_initialized.load()) {}
    }
}

void RWELLEmulator::enableActivity(bool en)
{
    this->activityEnabled = en;
}

void RWELLEmulator::setTemperature(int temperature)
{
    txMutex.lock();
    averageTemperature = temperature;
    txMutex.unlock();
}

void RWELLEmulator::setPressure(int pressure)
{
    txMutex.lock();
    averagePressure = pressure;
    txMutex.unlock();
}

void RWELLEmulator::setVoltage(int volt)
{
    txMutex.lock();
    this->voltage = volt;
    txMutex.unlock();
}

int RWELLEmulator::getTemperature()
{
    return temperature;
}

int RWELLEmulator::getPressure()
{
    return pressure;
}

int RWELLEmulator::getVoltage()
{
    return voltage;
}

int RWELLEmulator::getActivity()
{
    return actualActivity;
}

void RWELLEmulator::handler()
{
    int r;
    delay_ms = longDelay_ms;
    is_initialized.store(true);
    while (!is_stop_forced.load())
    {
        delay_ms = longDelay_ms;
        switch(state)
        {
        case RWELLSocketState::INIT:
            socket_();
            listen_();
            accept_();
            state = RWELLSocketState::ESTABLISHED;
            break;
        case RWELLSocketState::ESTABLISHED:
            r = receive_();
            if(r <= 0)
            {
                state = RWELLSocketState::CLOSED;
            }
            else if(r > 0)
            {
                // rx handle
                rxHandle();
                txHandle();
                // tx handle
                if(send_() < 0)
                {
                    state = RWELLSocketState::CLOSED;
                }
            }
            delay_ms = shortDelay_ms;
            break;
        case RWELLSocketState::CLOSED:
            disconnect_();
            close_();
            state = RWELLSocketState::INIT;
            break;
        }
        updateMessage_();
        sleep_(delay_ms);
    }
    is_launched.store(false);
}
bool RWELLEmulator::socket_()
{
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    setsockopt (socket_desc, SOL_SOCKET, SO_RCVTIMEO, &this->timeout, sizeof (timeout));
    setsockopt (socket_desc, SOL_SOCKET, SO_SNDTIMEO, &this->timeout, sizeof (timeout));
    if(socket_desc == -1)
    {
        //std::cout << "socket error\n";
        return false;
    }
    server.sin_family = AF_INET;
    //server.sin_addr.s_addr = INADDR_LOOPBACK; // 127.0.0.1
	server.sin_port = htons( port );
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0 )
    {
        return false;
    }
    //std::cout << "bind socket to " << ip << ":" << port << std::endl;
    return true;
}
bool RWELLEmulator::listen_()
{
    bool l = (listen (socket_desc, 1) != -1);
   // std::cout << "listening...\n";
    return l;
}
void RWELLEmulator::accept_()
{
    addr_size = sizeof(struct sockaddr_in);
    client_sock = accept4(socket_desc, (struct sockaddr *)&client, (socklen_t*)&addr_size, SOCK_NONBLOCK);
    //std::cout << "accept()\n";
}
void RWELLEmulator::disconnect_()
{
    shutdown(client_sock, SHUT_RDWR);
    shutdown(socket_desc, SHUT_RDWR);
}
void RWELLEmulator::close_()
{
    close(client_sock);
    close(socket_desc);
    //std::cout << "close socket\n";
}
int RWELLEmulator::send_()
{
    int res = send(client_sock, txBuffer, kTxBufferSize, 0);
    return res;
}
int RWELLEmulator::receive_()
{
    int res = recv(client_sock, rxBuffer, kRxBufferSize, 0);
    return res;
}

void RWELLEmulator::sleep_(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void RWELLEmulator::updateMessage_()
{
    txMutex.lock();
    messageUpdatedCallback();
    txMutex.unlock();
}

void RWELLEmulator::rxHandle()
{
    rxMutex.lock();
    int command = 0;
	int parameter_1 = 0;
	int parameter_2 = 0;

    memcpy(&command, rxBuffer, 4);
	memcpy(&parameter_1, rxBuffer + 4, 4);
	memcpy(&parameter_2, rxBuffer + 8, 4);

    switch(command)
	{
    case RWELLCommandCode::Empty:
        break;
	case RWELLCommandCode::Set_HV:
        emulateHVOut(parameter_1);
		break;
	case RWELLCommandCode::Set_Band:
        emulateBand(parameter_1);
		break;
	default:
		break;
	}
    rxMutex.unlock();
}

void RWELLEmulator::txHandle()
{
    txMutex.lock();

    increaseID();
    updateActivity(); // emulateADCValue
    updateNoise();
    emulateADCValue( noiseCounts + activityEnabled * activityCounts );
    int t_rand = (rand() % 20);
    emulateTemperature(averageTemperature  + t_rand);
    int p_rand = (rand() % 200);
    emulatePressure(averagePressure  + p_rand);
    emulateHVOut(voltage);

    txMutex.unlock();
}

void RWELLEmulator::updateActivity()
{
    const int activity0 = 3000000;
    const double f18_half_life_s = 109.771 * 60;
    double t = (double)id * shortDelay_ms / 1000;
    int activityRand = (rand() % 1000) - 500;
    activityCounts = (activity0 * exp2(-t / f18_half_life_s) + activityRand) / getBandFactor();
}

void RWELLEmulator::updateNoise()
{
    const int noiseAverage = 5000;
    int noiseRand = (rand() % 1000) - 500;
    noiseCounts = (noiseAverage + noiseRand) / getBandFactor();
}

void RWELLEmulator::increaseID()
{
    id++;
    memcpy(
        txBuffer + map[RWELLValueCode::Message_Num].offset,
        &id,
        map[RWELLValueCode::Message_Num].size
    );
}
void RWELLEmulator::emulateADCValue(int val)
{
    actualActivity = val;
    memcpy(
        txBuffer + map[RWELLValueCode::ADC_DR].offset,
        &val,
        map[RWELLValueCode::ADC_DR].size
    );
}
void RWELLEmulator::emulateHVOut(uint16_t volt)
{
    voltage = volt;
    memcpy(
        txBuffer + map[RWELLValueCode::HV_Out].offset,
        &voltage,
        map[RWELLValueCode::HV_Out].size
    );
}
void RWELLEmulator::emulateTemperature(int celsius_0p01)
{
    temperature = celsius_0p01;
    memcpy(
        txBuffer + map[RWELLValueCode::Temperature].offset,
        &celsius_0p01,
        map[RWELLValueCode::Temperature].size
    );
}
void RWELLEmulator::emulatePressure(int mbar_0p01)
{
    pressure = mbar_0p01;
    memcpy(
        txBuffer + map[RWELLValueCode::Pressure].offset,
        &mbar_0p01,
        map[RWELLValueCode::Pressure].size
    );
}
void RWELLEmulator::emulateBand(uint8_t band)
{
    band &= 0b00000011;
    this->lastBand = band;
    memcpy(
        txBuffer + map[RWELLValueCode::Band].offset,
        &band,
        map[RWELLValueCode::Band].size
    );
}

double RWELLEmulator::getBandFactor()
{
    if (lastBand == 0) return 1.; // high sensivity
    if (lastBand == 1) return 10.; // medium sensivity
    if (lastBand == 2) return 100.; // low sensivity
    return 1;
}

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

void RWELLEmulator::loop()
{
    int r;
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
    sleep_(delay_ms);
}
bool RWELLEmulator::socket_()
{
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    //socket_desc = socket(AF_INET , SOCK_STREAM | SOCK_NONBLOCK , 0);
    if(socket_desc == -1)
    {
        std::cout << "socket error\n";
        return false;
    }
     std::cout << "socket()\n";

    server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_LOOPBACK; // 127.0.0.1
	server.sin_port = htons( port );
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0 )
    {
        return false;
    }
    std::cout << "bind()\n";
    return true;
}
bool RWELLEmulator::listen_()
{
    bool l = (listen (socket_desc, 1) != -1);
    std::cout << "listen()\n";
    return l;
}
void RWELLEmulator::accept_()
{
    addr_size = sizeof(struct sockaddr_in);
    //client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&addr_size);
    client_sock = accept4(socket_desc, (struct sockaddr *)&client, (socklen_t*)&addr_size, SOCK_NONBLOCK);
    std::cout << "accept()\n";
}
void RWELLEmulator::disconnect_()
{
    shutdown(client_sock, SHUT_RDWR);
    shutdown(socket_desc, SHUT_RDWR);
    std::cout << "disconnect()\n";
}
void RWELLEmulator::close_()
{
    close(client_sock);
    close(socket_desc);
    std::cout << "closed()\n";
}
int RWELLEmulator::send_()
{
    int res = send(client_sock, txBuffer, kTxBufferSize, 0);
    std::cout << "send " << res << " bytes\n";
    return res;
}
int RWELLEmulator::receive_()
{
    int res = recv(client_sock, rxBuffer, kRxBufferSize, 0);
    std::cout << "receive " << res << " bytes\n";
    return res;
}

void RWELLEmulator::sleep_(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void RWELLEmulator::rxHandle()
{
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
}

void RWELLEmulator::txHandle()
{
    increaseID();

    const int y0 = 3000000;
    const double f18_half_life_s = 109.771 * 60;
    double t = (double)id * shortDelay_ms / 1000;
    int y_rand = (rand() % 20) - 10;
    emulateADCValue((y0 * exp2(-t / f18_half_life_s) + y_rand) / 1);

    const int t0 = 2750;
    int t_rand = (rand() % 20);
    emulateTemperature(t0  + t_rand);

    const int p0 = 105500; 
    int p_rand = (rand() % 200);
    emulatePressure(p0  + p_rand);
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
    memcpy(
        txBuffer + map[RWELLValueCode::ADC_DR].offset,
        &val,
        map[RWELLValueCode::ADC_DR].size
    );
}
void RWELLEmulator::emulateHVOut(uint16_t volt)
{
    volt = std::min(volt, (uint16_t)500);
    memcpy(
        txBuffer + map[RWELLValueCode::HV_Out].offset,
        &volt,
        map[RWELLValueCode::HV_Out].size
    );
}
void RWELLEmulator::emulateTemperature(int celsius_0p01)
{
    memcpy(
        txBuffer + map[RWELLValueCode::Temperature].offset,
        &celsius_0p01,
        map[RWELLValueCode::Temperature].size
    );
}
void RWELLEmulator::emulatePressure(int mbar_0p01)
{
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

#include <string>
#include <Windows.h>
#include <ostream>

#include "serial.h"

// Can be changed, but this value is fast enought not to introduce any significant latency and pretty reliable
constexpr DWORD BAUD_RATE = 19200;

//HANDLE hPort;
DCB serialParams;
OVERLAPPED osReader{ 0 };

static void (*OnCharReceived)(char c);

void Serial::FindAvailableSerialPorts()
{
	Serial::availablePorts.clear();
	
	char lpTargetPath[MAX_PATH];
	for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
	{
		std::string portName = "COM" + std::to_string(i); // converting to COM0, COM1, COM2
		DWORD test = QueryDosDevice(portName.c_str(), lpTargetPath, MAX_PATH);

		if (test != NULL)
		{
			Serial::availablePorts.push_back(portName);
		}
	}
}

bool Serial::Setup(const char* szPortName, void (*OnCharReceivedFunc)(char c))
{
	if(OnCharReceivedFunc != nullptr)
		OnCharReceived = OnCharReceivedFunc;

	FindAvailableSerialPorts();

	if (isConnected)
		Close();

	osReader.hEvent = INVALID_HANDLE_VALUE;

	std::string serialCom = "\\\\.\\";
	serialCom += szPortName;
	hPort = CreateFile(
		serialCom.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, // Make reading async
		NULL
	);

	DCB serialParams;
	serialParams.ByteSize = sizeof(serialParams);

	if (!GetCommState(hPort, &serialParams))
		return false;

	serialParams.BaudRate = BAUD_RATE;
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;

	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 50;
	timeout.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hPort, &timeout))
		return false;

	PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

	isConnected = true;
	return true;
}

void Serial::HandleInput()
{
	if (!isConnected || !hPort)
		return;

	DWORD dwBytesTransferred;
	DWORD dwCommModemStatus{};
	BYTE byte[5]{};

	DWORD dwRead;
	BOOL fWaitingOnRead = FALSE;

	if (osReader.hEvent == INVALID_HANDLE_VALUE)
	{
		printf("Creating a new reader Event\n");
		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	//DWORD written;
	//const char* szBuff{ "pppppppppppp" };
	//if (!WriteFile(hPort, "p", 2, &written, NULL))
	//{
	//	char lastError[1024];
	//	FormatMessage(
	//		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	//		NULL,
	//		GetLastError(),
	//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//		lastError,
	//		1024,
	//		NULL);
	//	printf(lastError);
	//}
	//else
	//	printf("Sending 'p', written: %i\n", written);


	if (!fWaitingOnRead)
	{
		// Issue read operation. Try to read as many bytes as possible to empty the buffer and avoid any additional latency.
		// The timeouts are all set to 0, so if there are less than 5 bytes to read it will just read all available,
		// set "dwRead" to the value of bytes read and then process all the bytes separately. 
		if (!ReadFile(hPort, &byte, 5, &dwRead, &osReader))
		{
			// This error most likely means that the device was disconnected, or something bad happened to it. It's better to close the serial either way.
			if (GetLastError() != ERROR_IO_PENDING)
			{
				printf("IO Error");
				Close();
			}
			else
				fWaitingOnRead = TRUE;
		}
		else
		{
			// read completed immediately
			if (dwRead)
				if (OnCharReceived)
				{
					for (int i = 0; i < dwRead; i++) 
					{
						OnCharReceived(byte[i]);
					}
				}
		}
	}

	/*
	const DWORD READ_TIMEOUT = 1;

	DWORD dwRes;

	if (fWaitingOnRead) {
		dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);
		switch (dwRes)
		{
			// Read completed.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(hPort, &osReader, &dwRead, FALSE))
				printf("IO Error");
			// Error in communications; report it.
			else
				// Read completed successfully.
				OnCharReceived(byte);

			//  Reset flag so that another opertion can be issued.
			fWaitingOnRead = FALSE;
			break;

			//case WAIT_TIMEOUT:
			//	break;

		default:
			printf("Event Error");
			break;
		}
	}
	*/


	if (byte == NULL)
		return;
}

bool Serial::Write(const char* c, size_t size)
{
	DWORD dwRead;
	if (!WriteFile(hPort, c, size, &dwRead, &osReader)) {
		//handle error here
		return false;
	}
	return true;
}

void Serial::Close()
{
	if (isConnected)
	{
		// Clear the serial port Buffer
		PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

		CloseHandle(hPort);
		CloseHandle(osReader.hEvent);

		hPort = INVALID_HANDLE_VALUE;
		osReader.hEvent = INVALID_HANDLE_VALUE;
	}

	isConnected = false;
}
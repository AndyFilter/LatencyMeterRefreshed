#include <string>
#include <Windows.h>

#include "serial.h"

HANDLE hPort;
DCB serialParams;
OVERLAPPED osReader = { 0 };

static void (*OnCharReceived)(char c);

void FindAvailableSerialPorts()
{
	std::string serialCom = "\\\\.\\COM";
	
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

bool Serial::Setup(char COM_Number, void (*OnCharReceivedFunc)(char c))
{
	OnCharReceived = OnCharReceivedFunc;

	FindAvailableSerialPorts();

	std::string serialCom = "\\\\.\\COM";
	serialCom += COM_Number;
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

	serialParams.BaudRate = 19200; // Can be changed, but this value is fast enought not to introduce any significant latency and pretty reliable
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;

	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	if (SetCommTimeouts(hPort, &timeout))
		return false;

	//SetCommMask(hPort, EV_RXCHAR | EV_ERR); //receive character event

	return true;
}

void Serial::HandleInput()
{
	DWORD dwBytesTransferred;
	DWORD dwCommModemStatus{};
	BYTE byte = NULL;

	DWORD dwRead;
	BOOL fWaitingOnRead = FALSE;

	if (osReader.hEvent == NULL)
	{
		printf("Creating a new reader Event\n");
		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	if (!fWaitingOnRead)
	{
		// Issue read operation.
		if (!ReadFile(hPort, &byte, 1, &dwRead, &osReader))
		{
			if (GetLastError() != ERROR_IO_PENDING)
				printf("IO Error");
			else
				fWaitingOnRead = TRUE;
		}
		else
		{
			// read completed immediately
			if (dwRead)
				OnCharReceived(byte);
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

void Serial::Close()
{
	CloseHandle(hPort);
	CloseHandle(osReader.hEvent);
}
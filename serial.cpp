#include <string>
#include <Windows.h>
#include <ostream>
#include <iostream>
#include "serial.h"

// Can be changed, but this value is fast enought not to introduce any significant latency and pretty reliable
constexpr DWORD BAUD_RATE = 19200;
const byte BYTES_TO_READ = 10;

//HANDLE hPort;
DCB serialParams;
OVERLAPPED osReader{ 0 };

//char buffer[256]{ 0 };

//bool isWaitingForWrite = false;

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

//void ReadIO(HANDLE hSerial, LPOVERLAPPED(osOverlapped), LPVOID buf)
//{
//	static bool wasLastTimeAnswer{ false };
//
//	while (true)
//	{
//		if (isWaitingForWrite)
//			continue;
//		DWORD dwCommModemStatus{};
//		BYTE byte[BYTES_TO_READ]{};
//
//		DWORD dwRead = 0;
//
//		BOOL fWaitingOnRead = FALSE;
//
//		char* text = (char*)buf;
//
//		if (osOverlapped->hEvent == INVALID_HANDLE_VALUE)
//		{
//			printf("Creating a new reader Event\n");
//			osOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//		}
//
//		//WaitCommEvent(hSerial, &dwCommModemStatus, osOverlapped);
//
//		//if(dwCommModemStatus & EV_RXCHAR)
//		//{ 
//		//	if (ReadFile(hSerial, byte, BYTES_TO_READ, &dwRead, osOverlapped))
//		//	{
//		//		if (dwRead)
//		//			strcat_s((char*)buf, 256, (char*)byte);
//		//		auto x = 0;
//		//	}
//		//	else
//		//	{
//		//		char lastError[1024];
//		//		FormatMessage(
//		//			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//		//			NULL,
//		//			GetLastError(),
//		//			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//		//			lastError,
//		//			1024,
//		//			NULL);
//		//		printf(lastError);
//		//	}
//		//}
//
//		// Create the overlapped event. Must be closed before exiting to avoid a handle leak.
//		if (!fWaitingOnRead) {
//			if (!ReadFile(hSerial, byte, BYTES_TO_READ, &dwRead, osOverlapped)) {
//				if (GetLastError() != ERROR_IO_PENDING) // read not delayed ?
//					printf("error in comm");
//				else
//					fWaitingOnRead = TRUE;
//			}
//			else {
//				// read completed immediately
//				if (dwRead)
//					strcat_s((char*)buf, 256, (char*)byte);
//				//strcpy_s((char*)buf, dwRead, (char*)byte);
//			}
//		}
//
//
//#define READ_TIMEOUT 0 // milliseconds
//
//		DWORD dwRes;
//		if (fWaitingOnRead) {
//			dwRes = WaitForSingleObject(osOverlapped->hEvent, READ_TIMEOUT);
//			switch (dwRes)
//			{
//				// Read completed.
//			case WAIT_OBJECT_0:
//				if (!GetOverlappedResult(hSerial, osOverlapped, &dwRead,
//					FALSE))
//					printf("error in comm");
//				// Error in communications; report it.
//				else
//					// Read completed successfully.
//					if (dwRead)
//						strcat_s((char*)buf, 256, (char*)byte);
//				fWaitingOnRead = FALSE;
//				break;
//			case WAIT_TIMEOUT:
//				break;
//			default:
//				break;
//			}
//		}
//
//		if (dwRead)
//		{
//			wasLastTimeAnswer = false;
//			for (int i = 0; i < dwRead; i++)
//			{
//				if (byte[i] == 'e')
//				{
//					isWaitingForWrite = true;
//					wasLastTimeAnswer = true;
//					break;
//				}
//			}
//		}
//
//		if (wasLastTimeAnswer)
//		{
//			CancelIoEx(hSerial, osOverlapped);
//			Sleep(10);
//			isWaitingForWrite = false;
//		}
//
//
//		//			if (!fWaitingOnRead) {
//		//				// Issue read operation.
//		//				if (!ReadFile(hSerial, byte, BYTES_TO_READ, &dwRead, &osOverlapped)) {
//		//					if (GetLastError() != ERROR_IO_PENDING)     // read not delayed?
//		//						printf("comm error");
//		//					   // Error in communications; report it.
//		//					else
//		//						fWaitingOnRead = TRUE;
//		//				}
//		//				else {
//		//					// read completed immediately
//		//					strcat_s((char*)buf, 256, (char*)byte);
//		//				}
//		//			}
//		//
//		//
//		//#define READ_TIMEOUT      10      // milliseconds
//		//
//		//			DWORD dwRes;
//		//
//		//			if (fWaitingOnRead) {
//		//				dwRes = WaitForSingleObject(osOverlapped.hEvent, READ_TIMEOUT);
//		//				switch (dwRes)
//		//				{
//		//					// Read completed.
//		//				case WAIT_OBJECT_0:
//		//					if (!GetOverlappedResult(hSerial, &osOverlapped, &dwRead, FALSE))
//		//						printf("comm error");
//		//						// Error in communications; report it.
//		//					else
//		//						// Read completed successfully.
//		//						strcat_s((char*)buf, 256, (char*)byte);
//		//
//		//					//  Reset flag so that another opertion can be issued.
//		//					fWaitingOnRead = FALSE;
//		//					break;
//		//
//		//				case WAIT_TIMEOUT:
//		//					// Operation isn't complete yet. fWaitingOnRead flag isn't
//		//					// changed since I'll loop back around, and I don't want
//		//					// to issue another read until the first one finishes.
//		//					//
//		//					// This is a good time to do some background work.
//		//					break;
//		//
//		//				default:
//		//					// Error in the WaitForSingleObject; abort.
//		//					// This indicates a problem with the OVERLAPPED structure's
//		//					// event handle.
//		//					break;
//		//				}
//		//			}
//
//
//	}
//}

bool Serial::Setup(const char* szPortName, void (*OnCharReceivedFunc)(char c))
{
	if (OnCharReceivedFunc != nullptr)
		OnCharReceived = OnCharReceivedFunc;

	FindAvailableSerialPorts();

	if (isConnected)
		Close();

	osReader.hEvent = INVALID_HANDLE_VALUE;

	std::string serialCom = "\\\\.\\";
	serialCom += szPortName;

#ifdef BufferedSerialComm
	isConnected = true;
	return 	_sopen_s(&fd, serialCom.c_str(), _O_BINARY | _O_CREAT | _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#endif // BufferedSerialComm

	//auto r = fopen_s(&hFile, serialCom.c_str(), "ab+");

	//isConnected = true;
	//return true;

	hPort = CreateFile(
		serialCom.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, // Make reading async (FILE_FLAG_OVERLAPPED). Also FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING
		NULL
	);

	DCB serialParams;
	ZeroMemory(&serialParams, sizeof(serialParams));

	serialParams.DCBlength = sizeof(serialParams);

	//if (!GetCommState(hPort, &serialParams))
	//	return false;

	serialParams.BaudRate = BAUD_RATE;
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;
	serialParams.fBinary = TRUE;
	serialParams.fParity = FALSE;
	serialParams.fOutxCtsFlow = FALSE;
	serialParams.fOutxDsrFlow = FALSE;
	serialParams.fDtrControl = DTR_CONTROL_DISABLE;
	serialParams.fDsrSensitivity = FALSE;
	//serialParams.fDtrControl = DTR_CONTROL_ENABLE;


	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(hPort, &timeout))
		return false;

	//ReadIO(hPort, &osReader, buffer);


	PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

	SetCommMask(hPort, EV_RXCHAR | EV_ERR);

	//ioThread = std::thread(ReadIO, hPort, &osReader, &buffer);

	isConnected = true;
	return true;
}
char buffer[4]{ 0 };
std::chrono::steady_clock::time_point internalStartTime;
void Serial::HandleInput()
{
#ifdef BufferedSerialComm
	if (fd == -1)
		return;

	//unsigned int nbytes = 3;
	//char buffer[BYTES_TO_READ]{0};

	//int bytesRead = _read(fd, buffer, nbytes);

	//if (bytesRead > 0)
	//{
	//	if (OnCharReceived)
	//	{
	//		for (int i = 0; i < bytesRead; i++)
	//		{
	//			OnCharReceived(buffer[i]);
	//		}
	//	}
	//}

	//if (_write(fd, "r", 1))
	//{
	//	//internalStartTime = std::chrono::high_resolution_clock::now();
	//	printf("Wrote succ\n");
	//}

	int bytesread;
	unsigned int nbytes = 4;

	//auto eof = _eof(fd);

	bytesread = _read(fd, buffer, nbytes);
	//buffer[3] = '\0';

	if (bytesread >= 1)
	{
			if (OnCharReceived)
			{
				for (int i = 0; i < bytesread; i++)
				{
					OnCharReceived(buffer[i]);
				}
			}
		//printf("Ping time: %lld\n", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - internalStartTime).count());
		printf("Message: %s\n", buffer);
		//printf("Read: %s", buffer);
		ZeroMemory(buffer, 4);
	}

	//Sleep(1000);

	ZeroMemory(buffer, 4);

	return;
#endif // BufferedSerialComm

	if (!isConnected || !hPort)
		return;

	//if (OnCharReceived)
	//{
	//	for (int i = 0; i < strlen(buffer); i++)
	//	{
	//		OnCharReceived(buffer[i]);
	//	}
	//	if (strlen(buffer) > 0)
	//		printf("buffer: %s\n", buffer);
	//	memset(buffer, 0, 256);
	//}
	//return;

	DWORD dwCommModemStatus{};
	BYTE byte[BYTES_TO_READ]{};

	DWORD dwRead = 0;
	static BOOL fWaitingOnRead{ FALSE };



	//dwRead = fread(&byte, 1, sizeof(byte), hFile);
	//hFile.read((char*)byte, BYTES_TO_READ);

	//if (dwRead)
	//	if (OnCharReceived)
	//	{
	//		for (int i = 0; i < dwRead; i++)
	//		{
	//			OnCharReceived(byte[i]);
	//		}
	//	}


	//Synchronous
	//static COMSTAT status{};
	//DWORD errors = 0;

	//auto clear = ClearCommError(hPort, &errors, &status);
	////ReadFileEx()
	//if(!clear || errors)
	//{
	//char lastError[1024];
	//FormatMessage(
	//	FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	//	NULL,
	//	GetLastError(),
	//	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//	lastError,
	//	1024,
	//	NULL);
	//printf(lastError);
	//}

	//if (status.cbInQue > 0)
	//{
	//	if (!ReadFile(hPort, byte, min(status.cbInQue, BYTES_TO_READ), &dwRead, NULL))
	//	{
	//		printf("Error reading hPort");
	//	}
	//	else
	//		if (dwRead)
	//			if (OnCharReceived)
	//			{
	//				for (int i = 0; i < dwRead; i++) 
	//				{
	//					OnCharReceived(byte[i]);
	//				}
	//			}
	//}

	//return;



	if (osReader.hEvent == INVALID_HANDLE_VALUE)
	{
		printf("Creating a new reader Event\n");
		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	//DWORD written;
	//const char* szBuff{ "pppppppppppp" };
	//if (!WriteFile(hPort, "p", 2, &written, osReader))
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
		// The timeouts are all set to 0, so if there are less than BYTES_TO_READ bytes to read it will just read all available,
		// set "dwRead" to the value of bytes read and then process all the bytes separately. 
		if (!ReadFile(hPort, &byte, BYTES_TO_READ, &dwRead, &osReader))
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

	DWORD dwRes;
	if (fWaitingOnRead) {
		dwRes = WaitForSingleObject(osReader.hEvent, 1);
		switch (dwRes)
		{
			// Read completed.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(hPort, &osReader, &dwRead,	FALSE))
				printf("error in comm");
			// Error in communications; report it.
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
			fWaitingOnRead = FALSE;
			break;
		case WAIT_TIMEOUT:
			break;
		default:
			break;
		}
	}


//	// Create the overlapped event. Must be closed before exiting
//	// to avoid a handle leak.
//	if (!fWaitingOnRead) {
//		if (!ReadFile(hPort, byte, BYTES_TO_READ, &dwRead, &osReader)) {
//			if (GetLastError() != ERROR_IO_PENDING) // read not delayed ?
//				printf("error in comm");
//			else
//				fWaitingOnRead = TRUE;
//		}
//	}
//
//
//#define READ_TIMEOUT 0 // milliseconds
//	DWORD dwRes;
//	if (fWaitingOnRead) {
//		dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);
//		switch (dwRes)
//		{
//			// Read completed.
//		case WAIT_OBJECT_0:
//			if (!GetOverlappedResult(hPort, &osReader, &dwRead,	FALSE))
//				printf("error in comm");
//			// Error in communications; report it.
//			fWaitingOnRead = FALSE;
//			break;
//		case WAIT_TIMEOUT:
//			break;
//		default:
//			break;
//		}
//	}
//
//	isWaitingForWrite = false;
//	for (int i = 0; i < dwRead; i++)
//	{
//		if (byte[i] == 'e')
//		{
//			isWaitingForWrite = true;
//		}
//	}
//
//	if (dwRead)
//		if (OnCharReceived)
//		{
//			for (int i = 0; i < dwRead; i++)
//			{
//				OnCharReceived(byte[i]);
//			}
//		}
}

BOOL WriteABuffer(char* lpBuf, DWORD dwToWrite)
{
	DWORD dwWritten;
	DWORD dwRes{ 0 };
	BOOL fRes{};
	// Create this write operation's OVERLAPPED structure's hEvent.
	// Issue write.
	if (!WriteFile(Serial::hPort, lpBuf, dwToWrite, &dwWritten,
		&osReader)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else
			// Write is pending.
			dwRes = WaitForSingleObject(osReader.hEvent,
				1);
		switch (dwRes)
		{
			// OVERLAPPED structure's event has been signaled.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(Serial::hPort, &osReader, &dwWritten, FALSE))
				fRes = FALSE;
			else
				// Write operation completed successfully.
				fRes = TRUE;
			break;
		default:
			// An error has occurred in WaitForSingleObject.
				// This usually indicates a problem with the
				// OVERLAPPED structure's event handle.
			fRes = FALSE;
			break;
		}
	}
	return fRes;
}

bool Serial::Write(const char* c, size_t size)
{
	DWORD dwRead;
	//return WriteFile(hPort, c, size, &dwRead, &osReader);
	auto res = WriteABuffer((char*)c, size);
	if (!res)
	{
		char lastError[1024];
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			lastError,
			1024,
			NULL);
		printf(lastError);
		ClearCommError(hPort, NULL, NULL);
	}
	return res;
	////tryingToWrite = true;
	////CancelIoEx(hPort, &osReader);
	////CancelIo(&osReader);
	//if (!WriteFile(hPort, c, size, &dwRead, &osReader)) {
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
	//	ClearCommError(hPort, NULL, NULL);
	//	return false;
	//}
	////tryingToWrite = false;
	//return true;
}

void Serial::Close()
{
	if (isConnected)
	{
#ifdef BufferedSerialComm
		_close(fd);
#endif
		// Clear the serial port Buffer
		PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

		CloseHandle(hPort);
		CloseHandle(osReader.hEvent);

		hPort = INVALID_HANDLE_VALUE;
		osReader.hEvent = INVALID_HANDLE_VALUE;
	}

	isConnected = false;
}
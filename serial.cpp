#include <string>
#include <Windows.h>
#include <ostream>
#include <iostream>
#include "serial.h"

// Can be changed, but this value is fast enought not to introduce any significant latency and pretty reliable
constexpr DWORD BAUD_RATE = 19200;
const byte BYTES_TO_READ = 8; // This value is arbitrary, but the normal data send consists of "e" + {1-4 digits}. Which in total makes 5 bytes, add 3 just to be sure it all gets read.

//HANDLE hPort;
DCB serialParams;
OVERLAPPED osReader{ 0 };

//char buffer[256]{ 0 };

//bool isWaitingForWrite = false;

static bool isSafeToClose = false;

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

void ReadIO(HANDLE hSerial, int (&fComm), LPOVERLAPPED osOverlapped, LPVOID dataRxFunc)
{
	auto OnCharReceived = ((void(*)(char))dataRxFunc);

#ifdef BufferedSerialComm

	while (Serial::isConnected)
	{
		if (fComm == -1)
			return;

		unsigned int nbytes = BYTES_TO_READ;
		char buffer[BYTES_TO_READ]{ 0 };

		int bytesRead = _read(fComm, buffer, nbytes);

		if (bytesRead > 0)
		{
			if (OnCharReceived)
			{
				for (int i = 0; i < bytesRead; i++)
				{
					OnCharReceived(buffer[i]);
				}
			}
		}
	}

#else

	while (Serial::isConnected)
	{
		if (!hSerial)
			return;

		DWORD dwCommModemStatus{};
		BYTE byte[BYTES_TO_READ]{};

		DWORD dwRead = 0;
		static BOOL fWaitingOnRead{ FALSE };

		if (osOverlapped->hEvent == INVALID_HANDLE_VALUE)
		{
			printf("Creating a new reader Event\n");
			osOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		}


		if (!fWaitingOnRead)
		{
			// Issue read operation. Try to read as many bytes as possible to empty the buffer and avoid any additional latency.
			// The timeouts are all set to 0, so if there are less than BYTES_TO_READ bytes to read it will just read all available,
			// set "dwRead" to the value of bytes read and then process all the bytes separately. 
			if (!ReadFile(hSerial, &byte, BYTES_TO_READ, &dwRead, osOverlapped))
			{
				// This error most likely means that the device was disconnected, or something bad happened to it. It's better to close the serial either way.
				if (GetLastError() != ERROR_IO_PENDING)
				{
					printf("IO Error");
					Serial::Close();
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
			dwRes = WaitForSingleObject(osOverlapped->hEvent, 1);
			switch (dwRes)
			{
				// Read completed.
			case WAIT_OBJECT_0:
				if (!GetOverlappedResult(hSerial, osOverlapped, &dwRead, FALSE))
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
	}

	CancelIo(hSerial);

#endif

	isSafeToClose = true;
	return;
}

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

	auto openStatus = _sopen_s(&fd, serialCom.c_str(), _O_RDWR, _SH_DENYRW, NULL);

	if (openStatus)
		return false;

	hPort = (HANDLE)_get_osfhandle(fd);

#else // BufferedSerialComm

	hPort = CreateFile(
		serialCom.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, // Make reading async (FILE_FLAG_OVERLAPPED). Also FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING
		NULL
	);

#endif

	DCB serialParams;
	ZeroMemory(&serialParams, sizeof(serialParams));

	serialParams.DCBlength = sizeof(serialParams);

	if (!GetCommState(hPort, &serialParams))
		return false;

	serialParams.BaudRate = 19200;
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;
	serialParams.fBinary = TRUE;
	serialParams.fParity = FALSE;
	//serialParams.fOutxCtsFlow = TRUE;
	//serialParams.fOutxDsrFlow = TRUE;
	//serialParams.fDtrControl = DTR_CONTROL_ENABLE;
	//serialParams.fOutxCtsFlow = TRUE;
	//serialParams.fRtsControl = RTS_CONTROL_ENABLE;
	//serialParams.fDsrSensitivity = TRUE;
	serialParams.fOutxCtsFlow = FALSE;
	serialParams.fOutxDsrFlow = FALSE;
	serialParams.fDtrControl = DTR_CONTROL_DISABLE;
	serialParams.fDsrSensitivity = FALSE; // Can be False, doesn't matter
	serialParams.fTXContinueOnXoff = FALSE;
	serialParams.fOutX = FALSE;
	serialParams.fErrorChar = FALSE;
	serialParams.fNull = FALSE;
	serialParams.fRtsControl = RTS_CONTROL_DISABLE;
	serialParams.fAbortOnError = FALSE;
	serialParams.XonLim = 0;
	serialParams.XoffLim = 0;
	serialParams.XonChar = 0;
	serialParams.XoffChar = 0;
	serialParams.ErrorChar = 0;
	serialParams.EofChar = 0;
	serialParams.EvtChar = 0;

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

	PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

	isSafeToClose = false;
	ioThread = std::thread(ReadIO, hPort, std::ref(fd), &osReader, OnCharReceivedFunc);

	isConnected = true;
	return true;
}

void Serial::HandleInput()
{

#ifdef BufferedSerialComm

	if (fd == -1)
		return;

	unsigned int nbytes = BYTES_TO_READ;
	char buffer[BYTES_TO_READ]{0};

	int bytesRead = _read(fd, buffer, nbytes);

	if (bytesRead > 0)
	{
		if (OnCharReceived)
		{
			for (int i = 0; i < bytesRead; i++)
			{
				OnCharReceived(buffer[i]);
			}
		}
	}

	return;

#else // BufferedSerialComm

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

#endif
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
#ifdef BufferedSerialComm

	auto written = _write(fd, c, size);

	return written > 0 ? true : false;

#else
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
#endif
}

void Serial::Close()
{
	isConnected = false;

	if (ioThread.joinable())
		ioThread.join();

	if (isSafeToClose)
	{
		// Clear the serial port Buffer
		PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);
#ifdef BufferedSerialComm
		_close(fd);
#else
		CloseHandle(hPort);
		CloseHandle(osReader.hEvent);
#endif

		hPort = INVALID_HANDLE_VALUE;
		osReader.hEvent = INVALID_HANDLE_VALUE;
		fd = -1;
	}

	isConnected = false;
}
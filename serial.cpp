#include <string>
#include <ostream>
#include <iostream>
#include "serial.h"

const unsigned char BYTES_TO_READ = 8; // This value is arbitrary, but the normal data send consists of "e" + {1-4 digits}.
                                    // Which in total makes 5 bytes, add 3 just to be sure it all gets read.

#ifdef _WIN32
// Can be changed, but this value is fast enough not to introduce any significant latency and pretty reliable
constexpr unsigned int BAUD_RATE = 19200;


//HANDLE hPort;
DCB serialParams;
OVERLAPPED osReader{ 0 };

//char buffer[256]{ 0 };

static bool isSafeToClose = false;
static LARGE_INTEGER StartingTime{ 0 };
uint64_t micros1();

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

struct IO_Comm_Data
{
	HANDLE hSerial;
	int& fComm;
	LPOVERLAPPED osOverlapped;
	LPVOID dataRxFunc;

	IO_Comm_Data(const HANDLE& hSerial, int& fComm, const LPOVERLAPPED& osOverlapped, const LPVOID& dataRxFunc)
		: hSerial(hSerial), fComm(fComm), osOverlapped(osOverlapped), dataRxFunc(dataRxFunc)
	{
	}
};

static uint64_t startTime = 0;
static uint64_t readCount = 0;


void ReadIO(HANDLE hSerial, int (&fComm), LPOVERLAPPED osOverlapped, LPVOID dataRxFunc)
{
	auto OnCharReceived = ((void(*)(char))dataRxFunc);

	startTime = micros1();
	readCount = 0;

#ifndef BufferedSerialComm
	osOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	SetCommMask(hSerial, EV_RXCHAR);
#endif

	while (Serial::isConnected)
	{
		//nanosleep(10);
		//SleepInUs(1);
		readCount++;
#ifdef BufferedSerialComm

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
					//if (buffer[i] == 'b' && i + 1 == bytesRead)
					//	Sleep(200);
				}
			}
		}

#else

		if (!hSerial)
			return;

		DWORD dwCommModemStatus{};
		BYTE byte[BYTES_TO_READ]{};

		DWORD dwRead = 0;
		static BOOL fWaitingOnRead{ FALSE };

		if (osOverlapped->hEvent == INVALID_HANDLE_VALUE)
		{
			printf("Creating a new reader Event\n");
			osOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

			SetCommMask(hSerial, EV_RXCHAR);
		}
		SetCommMask(hSerial, EV_RXCHAR);

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
				if (dwRead) {
					if (OnCharReceived)
					{
						for (int i = 0; i < dwRead; i++)
						{
							if (byte[i])
								OnCharReceived(byte[i]);
						}
					}
				}
				//fWaitingOnRead = true;
			}
		}

		DWORD commEvent = 0;
		if (!WaitCommEvent(hSerial, &commEvent, osOverlapped))
		{
			DWORD dwRes;
			if (fWaitingOnRead) {
				dwRes = WaitForSingleObject(osOverlapped->hEvent, 1);
				switch (dwRes)
				{
					// Read completed.
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(hSerial, osOverlapped, &dwRead, TRUE))
						printf("error in comm");
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


		//BYTE serialBuffer[16];
		//DWORD bytesRead = 0;

		//DWORD commEvent = 0;

		//if (!osOverlapped->hEvent)
		//osOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		////GetCommMask(hSerial, &commEvent);
		////if((commEvent & EV_RXCHAR) != EV_RXCHAR)
		//SetCommMask(hSerial, EV_RXCHAR);

		//if (WaitCommEvent(hSerial, &commEvent, osOverlapped))
		//{
		//	if ((commEvent & EV_RXCHAR) != EV_RXCHAR)
		//		continue;
		//	// Check GetLastError for ERROR_IO_PENDING, if I/O is pending then
		//	// use WaitForSingleObject() to determine when `o` is signaled, then check
		//	// the result. If a character arrived then perform your ReadFile.
		//	DWORD res = -1;
		//	if (GetLastError() == ERROR_IO_PENDING)
		//		GetOverlappedResult(hSerial, osOverlapped, &bytesRead, true);
		//		//res = WaitForSingleObject(osOverlapped->hEvent, 1);
		//	else if (res == WAIT_OBJECT_0) // object is signalled
		//	{
		//		if (!ReadFile(hSerial, serialBuffer, BYTES_TO_READ, &bytesRead, osOverlapped))
		//			continue;
		//		for (int i = 0; i < bytesRead; i++)
		//		{
		//			if (!OnCharReceived)
		//				break;
		//			OnCharReceived(serialBuffer[i]);
		//		}
		//	}
		//}



#endif


	}

	CancelIo(hSerial);


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

	serialParams.BaudRate = BAUD_RATE;
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
	serialParams.fDsrSensitivity = FALSE; // Can be False or True, doesn't matter
	serialParams.fTXContinueOnXoff = FALSE;
	serialParams.fOutX = FALSE;
	serialParams.fErrorChar = FALSE;
	serialParams.fNull = FALSE;
	serialParams.fRtsControl = RTS_CONTROL_ENABLE; // This has to be enabled for some boards (like Pro Micro),
	// But seems to work just fine disabled with boards like Uno. But having this ON might increase the input latency
	serialParams.fAbortOnError = FALSE;
	serialParams.XonLim = 0;
	serialParams.XoffLim = 0;
	serialParams.XonChar = 0;
	serialParams.XoffChar = 0;
	serialParams.ErrorChar = 0;
	serialParams.EofChar = 0;
	//serialParams.EvtChar = 'C';

	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 1000 / SERIAL_UPDATE_TIME;
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

	return !!written;

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
		if(fd != -1)
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

	printf("Read IO was running on average: %f\n", 1000000.f * (float)readCount / (micros1() - startTime));
}

uint64_t micros1()
{
	LARGE_INTEGER EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);

	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

	return ElapsedMicroseconds.QuadPart;
}

#else
#include <cstring>
#include <termios.h> // Contains POSIX terminal control definitions
#include <csignal>

namespace fs = std::filesystem;

#define BAUD_RATE B19200

bool can_read = true;

void ReadIO(int serialPort, void (*OnCharReceivedFunc)(char c)) {
    while(true) {
        unsigned char buf[8];
        if(!can_read)
            break;

        if(int n = read(serialPort, buf, BYTES_TO_READ); n > 0) {
            for(int i = 0; i < n; i++) {
                OnCharReceivedFunc(buf[i]);
            }
        }

        //usleep(100);
    }
}

bool Serial::Setup(const char* szPortName, void (*OnCharReceivedFunc)(char c)) {
    std::string dev_name = std::string("/dev/tty") + szPortName;
    isConnected = false;

    hPort = open(dev_name.c_str(), O_RDWR);

    // Check for errors
    if (hPort < 0) {
        printf("Error %i in open: %s\n", errno, strerror(errno));
        printf("You might want to run \"sudo adduser $USER tty\" and reboot your machine\n");
        return false;
        // Please use "sudo adduser $USER tty" if you get this error.
        // (You must restart before these group changes come into effect!)
    }

    termios tty{0};
    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_cc[VTIME] = 100;  // Wait 10 seconds for the data. We pretty much want to block as long as we can
                            // because it's a separate thread. This reduces CPU usage by about ~50%
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, BAUD_RATE);
    cfsetospeed(&tty, BAUD_RATE);

    // Save tty settings, also checking for error
    if (tcsetattr(hPort, TCSANOW, &tty) != 0) {
        printf("Error %i in tcsetattr: %s\n", errno, strerror(errno));
        return false;
    }

    ioThread = std::thread(ReadIO, hPort, OnCharReceivedFunc);

    isConnected = true;
    return true;
};

//void HandleInput();
void Serial::Close() {
    if(!isConnected)
        return;
    ioThread.detach();
    isConnected = false;
    close(hPort);
};

void Serial::FindAvailableSerialPorts() {
    availablePorts.clear();
    std::string path = "/dev";
    for (const auto & entry : fs::directory_iterator(path)) {
        if(entry.path().filename().string().find("ttyACM") != std::string::npos) {
            availablePorts.push_back(entry.path().filename().string().substr(3, -1));
#ifdef _DEBUG
            std::cout << entry.path() << std::endl;
#endif
        }
    }
};

bool Serial::Write(const char* c, size_t size) {
    can_read = false;
    usleep(100);
    auto written = write(hPort, c, size);
    can_read = true;

    return written == size;
};


#endif
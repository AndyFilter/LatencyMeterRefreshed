#include <array>
#include <iostream>
#include <ostream>
#include <string>
#include "Serial.h"
#include "Helpers/AppHelper.h"

constexpr unsigned char BYTES_TO_READ = 8; // This value is arbitrary, but the normal data sent consists of "e" + {1-4
									   // digits}. Which in total makes 5 bytes, add 3 just to be sure it all gets read.

#ifdef _WIN32
// Can be changed, but this value is fast enough not to introduce any significant latency and pretty reliable
constexpr unsigned int BAUD_RATE = 19200;


// HANDLE g_hPort;
OVERLAPPED g_osReader{0};

// char buffer[256]{ 0 };

static bool g_isSafeToClose = false;
static LARGE_INTEGER g_StartingTime{};
uint64_t Micros();

static void (*OnCharReceived)(char c);

void Serial::FindAvailableSerialPorts() {
	Serial::g_availablePorts.clear();

	char lpTargetPath[MAX_PATH];
	for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
	{
		const std::string portName = "COM" + std::to_string(i); // converting to COM0, COM1, COM2

		if (QueryDosDevice(portName.c_str(), lpTargetPath, MAX_PATH) != NULL) {
			Serial::g_availablePorts.push_back(portName);
		}
	}
}

struct IO_Comm_Data {
	HANDLE hSerial;
	int &fComm;
	LPOVERLAPPED osOverlapped;
	LPVOID dataRxFunc;

	IO_Comm_Data(const HANDLE &hSerial, int &fComm, const LPOVERLAPPED &osOverlapped, const LPVOID &dataRxFunc) :
		hSerial(hSerial), fComm(fComm), osOverlapped(osOverlapped), dataRxFunc(dataRxFunc) {}
};

static uint64_t g_startTime = 0;
static uint64_t g_readCount = 0;


void ReadIO(const HANDLE hSerial, const int(&fComm), LPOVERLAPPED osOverlapped, const LPVOID dataRxFunc) {
	const auto charCallback = static_cast<void (*)(char)>(dataRxFunc);

	g_startTime = Micros();
	g_readCount = 0;

#ifndef BufferedSerialComm
	osOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	SetCommMask(hSerial, EV_RXCHAR);
#endif

	while (Serial::g_isConnected) {
		g_readCount++;
#ifdef BufferedSerialComm

		if (fComm == -1)
			return;

		constexpr unsigned int NBYTES = BYTES_TO_READ;
		char buffer[BYTES_TO_READ]{0};

		if (const int bytesRead = _read(fComm, buffer, NBYTES); bytesRead > 0) {
			if (charCallback != nullptr) {
				for (int i = 0; i < bytesRead; i++) {
					charCallback(buffer[i]);
					// if (buffer[i] == 'b' && i + 1 == bytesRead)
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
		static BOOL fWaitingOnRead{FALSE};

		if (osOverlapped->hEvent == INVALID_HANDLE_VALUE) {
			DEBUG_PRINT("Creating a new reader Event\n");
			osOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

			SetCommMask(hSerial, EV_RXCHAR);
		}
		SetCommMask(hSerial, EV_RXCHAR);

		if (!fWaitingOnRead) {
			// Issue read operation. Try to read as many bytes as possible to empty the buffer and avoid any additional
			// latency. The timeouts are all set to 0, so if there are less than BYTES_TO_READ bytes to read it will
			// just read all available, set "dwRead" to the value of bytes read and then process all the bytes
			// separately.
			if (!ReadFile(hSerial, &byte, BYTES_TO_READ, &dwRead, osOverlapped)) {
				// This error most likely means that the device was disconnected, or something bad happened to it. It's
				// better to close the serial either way.
				if (GetLastError() != ERROR_IO_PENDING) {
					DEBUG_ERROR_LOC("IO Error");
					Serial::Close();
				} else
					fWaitingOnRead = TRUE;
			} else {
				// read completed immediately
				if (dwRead) {
					if (charCallback) {
						for (int i = 0; i < dwRead; i++) {
							if (byte[i])
								charCallback(byte[i]);
						}
					}
				}
				// fWaitingOnRead = true;
			}
		}

		DWORD commEvent = 0;
		if (!WaitCommEvent(hSerial, &commEvent, osOverlapped)) {
			DWORD dwRes;
			if (fWaitingOnRead) {
				dwRes = WaitForSingleObject(osOverlapped->hEvent, 1);
				switch (dwRes) {
						// Read completed.
					case WAIT_OBJECT_0:
						if (!GetOverlappedResult(hSerial, osOverlapped, &dwRead, TRUE))
							DEBUG_ERROR_LOC("error in comm");
						else {
							// read completed immediately
							if (dwRead)
								if (charCallback) {
									for (int i = 0; i < dwRead; i++) {
										charCallback(byte[i]);
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


		// BYTE serialBuffer[16];
		// DWORD bytesRead = 0;

		// DWORD commEvent = 0;

		// if (!osOverlapped->hEvent)
		// osOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		////GetCommMask(hSerial, &commEvent);
		////if((commEvent & EV_RXCHAR) != EV_RXCHAR)
		// SetCommMask(hSerial, EV_RXCHAR);

		// if (WaitCommEvent(hSerial, &commEvent, osOverlapped))
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
		// }


#endif
	}

	CancelIo(hSerial);


	g_isSafeToClose = true;
	return;
}

bool Serial::Setup(const char *szPortName, void (*OnCharReceivedFunc)(char c)) {
	if (OnCharReceivedFunc != nullptr)
		OnCharReceived = OnCharReceivedFunc;

	FindAvailableSerialPorts();

	if (g_isConnected)
		Close();

	g_osReader.hEvent = INVALID_HANDLE_VALUE;

	std::string serialCom = "\\\\.\\";
	serialCom += szPortName;

#ifdef BufferedSerialComm

	if (_sopen_s(&g_fd, serialCom.c_str(), _O_RDWR, _SH_DENYRW, NULL) != 0)
		return false;

	g_hPort = (HANDLE) _get_osfhandle(g_fd);

#else // BufferedSerialComm

	g_hPort = CreateFile(serialCom.c_str(), GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING,
					   FILE_FLAG_OVERLAPPED, // Make reading async (FILE_FLAG_OVERLAPPED). Also FILE_FLAG_RANDOM_ACCESS
											 // | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING
					   NULL);

#endif

	DCB serialParams;
	ZeroMemory(&serialParams, sizeof(serialParams));

	serialParams.DCBlength = sizeof(serialParams);

	if (GetCommState(g_hPort, &serialParams) == 0)
		return false;

	serialParams.BaudRate = BAUD_RATE;
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;
	serialParams.fBinary = TRUE;
	serialParams.fParity = FALSE;
	// serialParams.fOutxCtsFlow = TRUE;
	// serialParams.fOutxDsrFlow = TRUE;
	// serialParams.fDtrControl = DTR_CONTROL_ENABLE;
	// serialParams.fOutxCtsFlow = TRUE;
	// serialParams.fRtsControl = RTS_CONTROL_ENABLE;
	// serialParams.fDsrSensitivity = TRUE;
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
	// serialParams.EvtChar = 'C';

	if (SetCommState(g_hPort, &serialParams) == 0)
		return false;

	COMMTIMEOUTS timeout = {0};
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 1000 / SERIAL_UPDATE_TIME;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	if (SetCommTimeouts(g_hPort, &timeout) == 0)
		return false;

	PurgeComm(g_hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

	g_isSafeToClose = false;
	g_ioThread = std::thread(ReadIO, g_hPort, std::ref(g_fd), &g_osReader, OnCharReceivedFunc);

	g_isConnected = true;
	return true;
}

BOOL WriteABuffer(const char *lpBuf, const DWORD dwToWrite) {
	DWORD dwWritten;
	DWORD dwRes{0};
	BOOL fRes{};
	// Create this write operation's OVERLAPPED structure's hEvent.
	// Issue write.
	if (WriteFile(Serial::g_hPort, lpBuf, dwToWrite, &dwWritten, &g_osReader) == 0) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		} else
			// Write is pending.
			dwRes = WaitForSingleObject(g_osReader.hEvent, 1);
		switch (dwRes) {
				// OVERLAPPED structure's event has been signalled.
			case WAIT_OBJECT_0:
				if (GetOverlappedResult(Serial::g_hPort, &g_osReader, &dwWritten, FALSE) == 0)
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

bool Serial::Write(const char *c, const size_t size) {
#ifdef BufferedSerialComm

	const auto written = _write(g_fd, c, static_cast<unsigned int>(size));

	return written != 0;

#else
	DWORD dwRead;
	// return WriteFile(g_hPort, c, size, &dwRead, &osReader);
	auto res = WriteABuffer((char *) c, size);
	if (!res) {
		char lastError[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastError, 1024, NULL);
		DEBUG_ERROR_LOC(lastError);
		ClearCommError(g_hPort, NULL, NULL);
	}
	return res;
	////tryingToWrite = true;
	////CancelIoEx(g_hPort, &osReader);
	////CancelIo(&osReader);
	// if (!WriteFile(g_hPort, c, size, &dwRead, &osReader)) {
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
	//	ClearCommError(g_hPort, NULL, NULL);
	//	return false;
	// }
	////tryingToWrite = false;
	// return true;
#endif
}

void Serial::Close() {
	g_isConnected = false;

	if (g_ioThread.joinable())
		g_ioThread.join();

	if (g_isSafeToClose) {
		// Clear the serial port Buffer
		PurgeComm(g_hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);
#ifdef BufferedSerialComm
		if (g_fd != -1)
			_close(g_fd);
#else
		CloseHandle(g_hPort);
		CloseHandle(osReader.hEvent);
#endif

		g_hPort = INVALID_HANDLE_VALUE;
		g_osReader.hEvent = INVALID_HANDLE_VALUE;
		g_fd = -1;
	}

	g_isConnected = false;

	DEBUG_PRINT("Read IO was running on average: %f\n", 1000000.f * static_cast<float>(g_readCount) / (Micros() - g_startTime));
}

uint64_t Micros() {
	LARGE_INTEGER endingTime;
	LARGE_INTEGER elapsedMicroseconds;
	LARGE_INTEGER frequency;

	QueryPerformanceFrequency(&frequency);

	QueryPerformanceCounter(&endingTime);
	elapsedMicroseconds.QuadPart = endingTime.QuadPart - g_StartingTime.QuadPart;

	elapsedMicroseconds.QuadPart *= 1000000;
	elapsedMicroseconds.QuadPart /= frequency.QuadPart;

	return elapsedMicroseconds.QuadPart;
}

#else
#include <csignal>
#include <cstring>
#include <filesystem>
#include <termios.h>

constexpr speed_t BAUD_RATE_VALUE = B19200;

namespace fs = std::filesystem;

// Renamed global variable for clarity
bool g_canRead = true;

// Renamed function and parameters for clarity
void ReadSerialData(const int serialPort, void (*onCharReceived)(char c)) {
	while (true) {
		std::array<unsigned char, BYTES_TO_READ> readBuffer;
		if (!g_canRead) {
			break;
		}
		if (const int bytesRead = read(serialPort, readBuffer.data(), BYTES_TO_READ); bytesRead > 0) {
			for (int i = 0; i < bytesRead; i++) {
				onCharReceived(readBuffer[i]);
			}
		}
	}
}

bool Serial::Setup(const char *szPortName, void (*OnCharReceivedFunc)(char c)) {
	const std::string devName = std::string("/dev/tty") + szPortName;
	g_isConnected = false;

	g_hPort = open(devName.c_str(), O_RDWR);

	// Check for errors
	if (g_hPort < 0) {
		fprintf(stderr, "Error %i in open: %s\n", errno, strerror(errno));
		fprintf(stderr, "You might want to run \"sudo adduser $USER tty\" and reboot your machine\n");
		return false;
		// Please use "sudo adduser $USER tty" if you get this error.
		// (You must restart before these group changes come into effect!)
	}

	termios tty {0};
	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_cc[VTIME] = 100; // Wait 10 seconds for the data. We pretty much want to block as long as we can
						   // because it's a separate thread. This reduces CPU usage by about ~50%
	tty.c_cc[VMIN] = 0;

	cfsetispeed(&tty, BAUD_RATE_VALUE);
	cfsetospeed(&tty, BAUD_RATE_VALUE);

	// Save tty settings, also checking for error
	if (tcsetattr(g_hPort, TCSANOW, &tty) != 0) {
		DEBUG_ERROR_LOC("Error %i in tcsetattr: %s\n", errno, strerror(errno));
		return false;
	}

	g_ioThread = std::thread(ReadSerialData, g_hPort, OnCharReceivedFunc);
	g_isConnected = true;
	return true;
}

// void HandleInput();
void Serial::Close() {
	if (!g_isConnected)
		return;
	g_ioThread.detach();
	g_isConnected = false;
	close(g_hPort);
}

void Serial::FindAvailableSerialPorts() {
	g_availablePorts.clear();
	const std::string path = "/dev";
	try {
		for (const auto &entry: fs::directory_iterator(path)) {
			if (entry.path().filename().string().find("ttyACM") != std::string::npos) {
				g_availablePorts.push_back(entry.path().filename().string().substr(3));
				DEBUG_PRINT("%s\n", entry.path().c_str());
			}
		}
	}
	catch (const std::exception &e) {
		DEBUG_ERROR_LOC("Error while iterating over available serial ports: %s\n", e.what());
	}
}

bool Serial::Write(const char *c, const size_t size) {
	g_canRead = false;
	usleep(100);
	const auto written = write(g_hPort, c, size);
	g_canRead = true;
	return (written == static_cast<ssize_t>(size));
}

#endif


#include <string>
#include <iostream>

#ifdef _WIN32

#include "Windows.h"

std::string GetLastErrorAsString()
{
    DWORD error_message_id = GetLastError();
    if (error_message_id == 0)
    {
        return std::string(); // No error message has been recorded
    }

    LPSTR message_buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_message_id,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&message_buffer,
        0,
        NULL);

    std::string message(message_buffer, size);
    LocalFree(message_buffer); // Free the Win32's string buffer
    return message;
}

void SetThreadAffinity(WORD core_number)
{
    HANDLE thread_handle = GetCurrentThread();
    PROCESSOR_NUMBER processor_number = {core_number};
    processor_number.Group = 0;
    processor_number.Number = 1;

    // Set the ideal processor for the thread
    BOOL result = SetThreadIdealProcessorEx(thread_handle, &processor_number, NULL);

    if (result == 0)
    {
        std::cerr << GetLastErrorAsString() << std::endl;
    }
}

#elif defined(__linux__)

#include "sched.h"
#include "string.h"

void SetThreadAffinity(int core_number)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_number, &cpuset);

    pthread_t thread = pthread_self();
    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0)
    {
        std::cerr << "Error setting thread affinity: " << strerror(result) << std::endl;
    }
}

#else

void SetThreadAffinity(int core_number)
{
    std::cerr << "Unknown OS. Cannot set core!" << std::endl;
}

#endif
//
// Created by root on 10/11/17.
//

#ifndef PARIS_TRACEROUTE_FROM_MAKEFILE_STDCAPTURE_HPP
#define PARIS_TRACEROUTE_FROM_MAKEFILE_STDCAPTURE_HPP
#ifdef _MSC_VER
#include <io.h>
#define popen _popen
#define pclose _pclose
#define stat _stat
#define dup _dup
#define dup2 _dup2
#define fileno _fileno
#define close _close
#define pipe _pipe
#define read _read
#define eof _eof
#else

#include <unistd.h>

#endif

#include <fcntl.h>
#include <stdio.h>
#include <mutex>
#include <thread>
#include <chrono>

class StdCapture {
public:
    static void Init();

    static void BeginCapture();

    static bool IsCapturing();

    static bool EndCapture();
    static std::string GetCapture();



private:
    enum PIPES {
        READ, WRITE
    };
    static int secure_dup(int src);
    static void secure_pipe(int *pipes);
    static void secure_dup2(int src, int dest);

    static void secure_close(int &fd);






    static int m_pipe[2];
    static int m_oldStdOut;
    static int m_oldStdErr;
    static bool m_capturing;
    static std::mutex m_mutex;
    static std::string m_captured;
};



#endif //PARIS_TRACEROUTE_FROM_MAKEFILE_STDCAPTURE_HPP

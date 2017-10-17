//
// Created by root on 10/11/17.
//

#include "StdCapture.hpp"
#define STD_OUT_FD (fileno(stdout))
#define STD_ERR_FD (fileno(stderr))

void StdCapture::Init()
{
    // make stdout & stderr streams unbuffered
    // so that we don't need to flush the streams
    // before capture and after capture
    // (fflush can cause a deadlock if the stream is currently being
    std::lock_guard<std::mutex> lock(m_mutex);
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
}

void StdCapture::BeginCapture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_capturing)
        return;

    secure_pipe(m_pipe);
    m_oldStdOut = secure_dup(STD_OUT_FD);
    m_oldStdErr = secure_dup(STD_ERR_FD);
    secure_dup2(m_pipe[WRITE],STD_OUT_FD);
    secure_dup2(m_pipe[WRITE],STD_ERR_FD);
    m_capturing = true;
#ifndef _MSC_VER
    secure_close(m_pipe[WRITE]);
#endif
}


bool StdCapture::IsCapturing()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_capturing;
}

bool StdCapture::EndCapture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_capturing)
        return;

    m_captured.clear();
    StdCapture::secure_dup2(m_oldStdOut, STD_OUT_FD);
    secure_dup2(m_oldStdErr, STD_ERR_FD);

    const int bufSize = 1025;
    char buf[bufSize];
    int bytesRead = 0;
    bool fd_blocked(false);
    do
    {
        bytesRead = 0;
        fd_blocked = false;
#ifdef _MSC_VER
        if (!eof(m_pipe[READ]))
                bytesRead = read(m_pipe[READ], buf, bufSize-1);
#else
        bytesRead = read(m_pipe[READ], buf, bufSize-1);
#endif
        if (bytesRead > 0)
        {
            buf[bytesRead] = 0;
            m_captured += buf;
        }
        else if (bytesRead < 0)
        {
            fd_blocked = (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR);
            if (fd_blocked)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    while(fd_blocked || bytesRead == (bufSize-1));

    secure_close(m_oldStdOut);
    secure_close(m_oldStdErr);
    secure_close(m_pipe[READ]);
#ifdef _MSC_VER
    secure_close(m_pipe[WRITE]);
#endif
    m_capturing = false;
}

std::string StdCapture::GetCapture() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_captured;
}

int StdCapture::secure_dup(int src) {
    int ret = -1;
    bool fd_blocked = false;
    do {
        ret = dup(src);
        fd_blocked = (errno == EINTR || errno == EBUSY);
        if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (ret < 0);
    return ret;
}

void StdCapture::secure_pipe(int *pipes) {
    int ret = -1;
    bool fd_blocked = false;
    do {
#ifdef _MSC_VER
        ret = pipe(pipes, 65536, O_BINARY);
#else
        ret = pipe(pipes) == -1;
#endif
        fd_blocked = (errno == EINTR || errno == EBUSY);
        if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (ret < 0);
}

void StdCapture::secure_dup2(int src, int dest) {
    int ret = -1;
    bool fd_blocked = false;
    do {
        ret = dup2(src, dest);
        fd_blocked = (errno == EINTR || errno == EBUSY);
        if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (ret < 0);
}

void StdCapture::secure_close(int &fd) {
    int ret = -1;
    bool fd_blocked = false;
    do {
        ret = close(fd);
        fd_blocked = (errno == EINTR);
        if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (ret < 0);

    fd = -1;
}


// actually define vars.
int StdCapture::m_pipe[2];
int StdCapture::m_oldStdOut;
int StdCapture::m_oldStdErr;
bool StdCapture::m_capturing;
std::mutex StdCapture::m_mutex;
std::string StdCapture::m_captured;

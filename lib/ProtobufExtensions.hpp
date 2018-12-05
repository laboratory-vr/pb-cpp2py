#pragma once

#include <chrono>
#include <google/protobuf/message_lite.h>
#if defined(_WIN64) || defined(_WIN32)
extern "C"
{
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")
}
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#define SOCKET int
#define SOCKET_ERROR -1
unsigned long WSAGetLastError();
void closesocket(int n);
#define WSACleanup()
#endif
//#include <opencv2/core/core.hpp>

class DataFrame;

namespace aemass
{
    //! Send until all data has been sent.
    /*!
     * @param s - The socket index.
     * @param *buf - a pointer to the buffer to send.
     * @param *len - a pointer to an int containing how many bytes to send.
     * @return the number of bytes sent, or -1 for failure.
     */
    int sendAll(int s, char *buf, int *len);

    //! Receive one protobuf.
    /*!
    * Reads the input socket until a protobuf delimiter is detected,
    * then returns the buffer and the number of bytes read.
    * @param sock - The socket index.
    * @param **buffer - a pointer to an unallocated buffer. This buffer will be
    * allocated by the method.
    * @return The number of bytes read (and the size of *buffer).
    */
    const int recvDelimProtobuf(int sock, unsigned char **buffer,
                            const uint64_t nTimeoutMS = 3000);

    //! Receive a protobuf on a socket and deserialize it.
    /*!
    * @param nSocketIndex - The socket index.
    * @param message - The output message.
    * @return true upon success, false otherwise.
    */
    bool recvAndDeserialize(const int &nSocketIndex,
                            google::protobuf::MessageLite& message,
                            const uint64_t nTimeoutMS = 3000);

    //! Serialize and send a protobuf.
    /*!
    * @param nSocketIndex - The socket index.
    * @param message - The input message.
    * @return true upon success, false otherwise.
    */
    bool serializeAndSend(const int &nSocketIndex,
                          const google::protobuf::MessageLite& message);

    //! Perform handshake for a "slave" (data sender)
    /*!
     * The role of a slave is to receive data requests and send data along.
     * This method performs a time synchronization sequence and determines
     * its offset from the "master" clock, so that it can use this data to
     * time its capture.
     * @param nSocketIndex - The socket index.
     * @param nRequestedFPS - The FPS that the "master" requests, output.
     * @param nTImeDeltaClientServer - The delta between the client and
     *                                 server, output.
     * @return true upon success, false otherwise.
     */
	bool performSlaveHandshake(const int &nSocketIndex,
                               uint64_t &nRequestedFPS,
                               int64_t &nTimeDeltaClientServer);

    //! Perform handshake for a "master" (data receiver)
    /*!
     * The role of a master is to request data from a slave. This method provides
     * the slave data about its clock state through a handshake sequence, and gives
     * the slave the FPS that the master needs, which the slave can then use to
     * time its data capture.
     * @param nSocketIndex - The socket index.
     * @param nRequestedFPS - The FPS to send to the slave.
     * @param nStartTime - A specific time point that serves as the "start" of time
     *                     synch. This is needed when several slaves serve one master.
     *                     If they all synchronize to the same start time, then they
     *                     can all target a certain time for data capture.
     * @return true upon success, false otherwise.
     */
	bool performMasterHandshake(const int &nSocketIndex,
                                const uint64_t &nRequestedFPS,
                                uint64_t nStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

   //! This method defines what we consider a "timestamp"
    /*!
     * We define a timestamp as the number of MS since the epoch as a uint64_t>
     */
   uint64_t getCurrentTimestamp();

   bool writeBufferToFile(const char *strPath, const char *bufferToWrite, const size_t &nBytes);
}

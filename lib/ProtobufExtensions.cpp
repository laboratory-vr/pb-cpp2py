
// Standard
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#ifdef MACOS
#include <netinet/tcp.h>
#endif

// OpenCV
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>

// Google Protobuf
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

// Local
#include "soccer.pb.h"
#include "ProtobufExtensions.hpp"
// #include "common/ImageProcessing.hpp"
#if 0
#include "common/DepthDenoiser.hpp"
#endif
//#include "common/RegistrationUtilities.hpp"


#if !defined(_WIN32) && !defined(_WIN64)
unsigned long WSAGetLastError()
{
	return 0;
}
void closesocket(int n)
{
	close(n);
}
#endif

namespace aemass
{
	uint64_t getCurrentTimestamp()
	{
#if defined(MACOS) || defined(_WIN32) || defined(_WIN64)
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
#else
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
	}

    const int recvDelimProtobuf(int sock,
                                unsigned char **buffer,
                                const uint64_t nTimeoutMS)
    {
        //read the delimiting varint byte by byte
		uint64_t nStartTime = getCurrentTimestamp();
        unsigned int length=0;
        size_t recv_bytes=0;
        char bite;
        int received=recv(sock, &bite, 1, 0);
        while(received < 0 && getCurrentTimestamp() - nStartTime < nTimeoutMS){
            received=recv(sock, &bite, 1, 0);
        }
        if(received<0)
            return received;
        else
            recv_bytes += received;
        length = (bite & 0x7f);
        while(bite & 0x80 && getCurrentTimestamp() - nStartTime < nTimeoutMS){
            memset(&bite, 0, 1);
            received=recv(sock, &bite, 1, 0);
            if(received<0)
                return received;
            else
                recv_bytes += received;
            length|= (bite & 0x7F) << (7*(recv_bytes-1));
        }

        // receive remainder of message
        recv_bytes=0;
        *buffer=(unsigned char *)malloc(sizeof(unsigned char) * length);
		while (recv_bytes < length && getCurrentTimestamp() - nStartTime < nTimeoutMS){
#if defined(_WIN32) || defined(_WIN64)
            received=recv(sock, (char *)*buffer + (sizeof(unsigned char) * recv_bytes), length-recv_bytes, 0);
#else
			received = recv(sock, *buffer + (sizeof(unsigned char) * recv_bytes), length - recv_bytes, 0);
#endif
            if(received<0)
                return received;
            else
                recv_bytes+=received;
        }
        return recv_bytes;
    }

    int sendAll(int s,
                char *buf,
                int *len)
    {
        int total = 0; // how many bytes we've sent
        int bytesleft = *len; // how many we have left to send
        int n = 0;
		uint64_t nStartTime = getCurrentTimestamp();
        while(total < *len && getCurrentTimestamp() - nStartTime < 1000) {
#if defined(MACOS) || defined(_WIN32) || defined(_WIN64)
            n = send(s, buf+total, bytesleft, 0);
#else
            n = send(s, buf+total, bytesleft, MSG_NOSIGNAL);
#endif
        if (n == -1) {
            /* print/log error details */
            break;
        }
        total += n;
        bytesleft -= n;
        }
        *len = total; // return number actually sent here
        return n==-1?-1:0; // return -1 on failure, 0 on success
    }

	bool performSlaveHandshake(const int &nSocketIndex,
                               uint64_t &nRequestedFPS,
                               int64_t &nTimeDeltaClientServer)
	{
#ifdef MACOS
    int state = 0;
    setsockopt(nSocketIndex, IPPROTO_TCP, TCP_NOPUSH, &state, sizeof(state));
#endif
		bool bIsConnected = true;
        bool bHandshakeDone = false;
		size_t nPacketNumber = 0;
		nTimeDeltaClientServer = 0;
		while (bIsConnected && !bHandshakeDone)
		{
			// Receive request for data.
			tutorial::TimeSynchronizationPacket packetNew;
            recvAndDeserialize(nSocketIndex,packetNew);

            // Create response
			tutorial::TimeSynchronizationPacket packetResponse;
			nRequestedFPS = packetNew.requested_fps();
			nPacketNumber = packetNew.packet_number();
			uint64_t nTimeNow = getCurrentTimestamp();
			nTimeDeltaClientServer = (int64_t) nTimeNow - (int64_t) packetNew.timestamp_on_client_clock_upon_arrival_to_server();
			// Now, in general, client timestamp for given time point = local system clock since epoch - nTimeDeltaClientServer
			packetResponse.set_timestamp_on_client_clock_upon_arrival_to_server(5000); // dummy
			nPacketNumber++;
			packetResponse.set_packet_number(nPacketNumber);
			packetResponse.set_requested_fps(nRequestedFPS);

			// Stop if we have enough packets to establish a good average for transit time.
			if (nPacketNumber > 200)
			{
				packetResponse.set_continue_(false);
                bHandshakeDone = true;
			}
			else
			{
				packetResponse.set_continue_(true);
			}

            // Serialize/send response
            bIsConnected = serializeAndSend(nSocketIndex, packetResponse);
		}
		std::cout << "Slave handshake complete; client and server clocks differ by "
			<< nTimeDeltaClientServer << " ms"
			<< " and requested fps is " << nRequestedFPS << "."
			<< std::endl;
        return bHandshakeDone;
	}
	bool performMasterHandshake(const int &nSocketIndex,
                                const uint64_t &nRequestedFPS,
                                uint64_t nStartTime)
    {
#ifdef MACOS
    int state = 0;
    setsockopt(nSocketIndex, IPPROTO_TCP, TCP_NOPUSH, &state, sizeof(state));
#endif
        bool bConnected = true;
        uint64_t nPacketNumber = 0;
        uint64_t nTimeSinceFirstPacket = 0;
        uint64_t nPacketTransitTime = 0;
        bool bHandshakeDone = false;
        while (bConnected && !bHandshakeDone)
        {
            // Create packet
            tutorial::TimeSynchronizationPacket packetToSend;
            uint64_t nTimeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            if (nPacketNumber == 0)
                nTimeSinceFirstPacket = nTimeNow;
            nPacketNumber++;
            // Attach best effort time estimate when this packet
            // will arrive, in client time "zone".
            packetToSend.set_timestamp_on_client_clock_upon_arrival_to_server(getCurrentTimestamp() + nPacketTransitTime);
            packetToSend.set_continue_(true);
            packetToSend.set_packet_number(nPacketNumber);
            packetToSend.set_requested_fps(nRequestedFPS);

            // Send packet
            bConnected = serializeAndSend(nSocketIndex,packetToSend);

            // Receive response
            tutorial::TimeSynchronizationPacket packetNew;
            bConnected = recvAndDeserialize(nSocketIndex,packetNew);

            // Manage local variables
            nTimeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            nPacketTransitTime = (nTimeNow - nTimeSinceFirstPacket)/packetNew.packet_number();
            if (!packetNew.continue_())
            {
                bHandshakeDone = true;
            }
        }
		std::cout << "Master handshake complete;"
			<< " requested fps is " << nRequestedFPS << "."
			<< std::endl;
        return bHandshakeDone;
    }

    bool recvAndDeserialize(const int &nSocketIndex,
            google::protobuf::MessageLite& message,
            const uint64_t nTimeoutMS)
    {
        // Receive
        unsigned char *pReceiveBuffer = NULL;
        int nBytes = recvDelimProtobuf(nSocketIndex, &pReceiveBuffer,nTimeoutMS);
        if (nBytes < 0)
        {
            std::cout << "RECV failed." << std::endl;
            return false;
        }

        // Deserialize
        google::protobuf::io::ArrayInputStream arrayIn(pReceiveBuffer, nBytes);
        google::protobuf::io::CodedInputStream codedIn(&arrayIn);
        google::protobuf::io::CodedInputStream::Limit msgLimit =
                codedIn.PushLimit(nBytes);
        message.ParseFromCodedStream(&codedIn);
        codedIn.PopLimit(msgLimit);
        if (pReceiveBuffer)
            free(pReceiveBuffer);
        if (!message.IsInitialized())
        {
            std::cout << "Incomplete packet buffer received, parse failed." << std::endl;
            return false;
        }
        return true;
    }
    bool serializeAndSend(const int &nSocketIndex,
                          const google::protobuf::MessageLite& message)
    {
        // Serialize
        int nVarIntSize = google::protobuf::io::CodedOutputStream::VarintSize32(message.ByteSize());
        int nSendSize = message.ByteSize()+nVarIntSize;
        char *pSendBuffer = new char[nSendSize];
        google::protobuf::io::ArrayOutputStream arrayOut(pSendBuffer, nSendSize);
        google::protobuf::io::CodedOutputStream codedOut(&arrayOut);
        codedOut.WriteVarint32(message.ByteSize());
        message.SerializeToCodedStream(&codedOut);

        // Send buffer
        int nBytes = sendAll(nSocketIndex, pSendBuffer, &nSendSize);

        // Clean up send buffer.
        delete[](pSendBuffer);

        // Exit if connection fails
        if (nBytes < 0)
        {
            std::cout << "Couldn't send a packet; disconnecting." << std::endl;
            return false;
        }
        return true;
    }
    bool writeBufferToFile(const char *strPath, const char *bufferToWrite, const size_t &nBytes)
    {
        FILE * pFile;
        pFile = fopen (strPath, "wb");
        if (!pFile)
            return false;
        size_t nWritten = fwrite(bufferToWrite,sizeof(char),nBytes,pFile);
        fclose (pFile);
        return (nBytes == nWritten);
    }
}


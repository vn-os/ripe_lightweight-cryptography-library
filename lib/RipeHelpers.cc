//
//  Ripe.cc
//
//  Copyright © 2017 Muflihun.com. All rights reserved.
//

#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>
#include "include/RipeHelpers.h"
#include "include/Ripe.h"
#include "include/log.h"

std::string RipeHelpers::encryptRSA(std::string& data, const std::string& key, const std::string& outputFile, int length) noexcept
{
    std::stringstream ss;
    byte* newData = new byte[length];
    int newLength = Ripe::encryptStringRSA(data, key.c_str(), newData);
    if (newLength == -1) {
        delete[] newData;
        unsigned int maxBlockSize = Ripe::maxRSABlockSize(length);
        if (data.size() > maxBlockSize) {
            RLOG(FATAL) << "Data size should not exceed " << maxBlockSize << " bytes. You have " << data.size() << " bytes";
        }
        Ripe::printLastError("Failed to encrypt");
    } else {
        std::string encrypted = Ripe::base64Encode(newData, newLength);
        if (!outputFile.empty()) {
            std::ofstream out(outputFile);
            out << encrypted;
            out.close();
            out.flush();
        } else {
            ss << encrypted;
        }
    }
    delete[] newData;
    return ss.str();
}

std::string RipeHelpers::decryptRSA(std::string& data, const std::string& key, bool isBase64, int length) noexcept
{

    if (isBase64) {
        data = Ripe::base64Decode(data);
    }
    int dataLength = static_cast<int>(data.size());

    std::stringstream ss;
    byte* newData = new byte[length];

    int newLength = Ripe::decryptRSA(reinterpret_cast<byte*>(const_cast<char*>(data.c_str())), dataLength, key.c_str(), newData);
    if (newLength == -1) {
        delete[] newData;
        unsigned int maxBlockSize = Ripe::maxRSABlockSize(length);
        if (data.size() > maxBlockSize) {
            RLOG(FATAL) << "Data size should not exceed " << maxBlockSize << " bytes. You have " << data.size() << " bytes";
        }

        Ripe::printLastError("Failed to decrypt");
    } else {
        ss << newData;
    }
    delete[] newData;
    return ss.str();
}

void RipeHelpers::writeRSAKeyPair(const std::string& publicFile, const std::string& privateFile, int length) noexcept
{
    RLOG(INFO) << "Generating key pair that can encrypt " << Ripe::maxRSABlockSize(length) << " bytes";
    if (!Ripe::writeRSAKeyPair(publicFile.c_str(), privateFile.c_str(), length)) {
        RLOG(ERROR) << "Failed to generate key pair! Please check logs for details" << std::endl;
        Ripe::printLastError("Failed to decrypt");
        return;
    }
    RLOG(INFO) << "Successfully saved!";
}

std::string RipeHelpers::generateRSAKeyPair(int length) noexcept
{
    Ripe::KeyPair pair = Ripe::generateRSAKeyPair(length);
    if (pair.first.empty() || pair.second.empty()) {
        RLOG(ERROR) << "Failed to generate key pair! Please check logs for details" << std::endl;
        Ripe::printLastError("Failed to decrypt");
        return "";
    }
    return std::string(Ripe::base64Encode(pair.first) + ":" + Ripe::base64Encode(pair.second));
}

std::string RipeHelpers::encryptAES(std::string& data, const std::string& hexKey, const std::string& clientId, const std::string& outputFile)
{
    std::stringstream ss;
    if (!outputFile.empty()) {
        std::vector<byte> iv;
        std::string encrypted = Ripe::encryptAES(data.data(), hexKey, iv);
        std::ofstream out(outputFile);
        out << encrypted.data();
        out.close();
        ss << "IV: " << std::hex << std::setfill('0');
        for (byte b : iv) {
            ss << std::setw(2) << static_cast<unsigned int>(b);
        }
        ss << std::endl;
    } else {
        ss << Ripe::prepareData(data.data(), hexKey, clientId.c_str());
    }
    return ss.str();
}

std::string RipeHelpers::decryptAES(const std::string& d, const std::string& hexKey, std::string& ivec, bool isBase64)
{
    std::string data(d);
    if (ivec.empty() && isBase64) {
        // Extract IV from data
        std::size_t pos = data.find_first_of(':');
        if (pos == 32) {
            ivec = data.substr(0, pos);
            Ripe::normalizeIV(ivec);
            data = data.substr(pos + 1);
            pos = data.find_first_of(':');
            if (pos != std::string::npos) {
                // We ignore clientId which is = data.substr(0, pos);
                data = data.substr(pos + 1);
            }
        }
    }
    if (ivec.size() == 32) {
        // Condensed form needs to be normalized
        Ripe::normalizeIV(ivec);
    }

    byte* iv = reinterpret_cast<byte*>(const_cast<char*>(ivec.data()));

    if (isBase64) {
        data = Ripe::base64Decode(data);
    }

    return Ripe::decryptAES(data.data(), hexKey, iv);
}

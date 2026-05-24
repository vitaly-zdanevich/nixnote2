/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2013 Randy Baumgarte

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
***********************************************************************************/

#include "encrypt.h"

#include <array>
#include <cstdint>

#include <QByteArray>
#include <QCryptographicHash>

namespace {

constexpr std::array<quint8, 256> PITABLE = {
    217,120,249,196, 25,221,181,237, 40,233,253,121, 74,160,216,157,
    198,126, 55,131, 43,118, 83,142, 98, 76,100,136, 68,139,251,162,
     23,154, 89,245,135,179, 79, 19, 97, 69,109,141,  9,129,125, 50,
    189,143, 64,235,134,183,123, 11,240,149, 33, 34, 92,107, 78,130,
     84,214,101,147,206, 96,178, 28,115, 86,192, 20,167,140,241,220,
     18,117,202, 31, 59,190,228,209, 66, 61,212, 48,163, 60,182, 38,
    111,191, 14,218, 70,105,  7, 87, 39,242, 29,155,188,148, 67,  3,
    248, 17,199,246,144,239, 62,231,  6,195,213, 47,200,102, 30,215,
      8,232,234,222,128, 82,238,247,132,170,114,172, 53, 77,106, 42,
    150, 26,210,113, 90, 21, 73,116, 75,159,208, 94,  4, 24,164,236,
    194,224, 65,110, 15, 81,203,204, 36,145,175, 80,161,244,112, 57,
    153,124, 58,133, 35,184,180,122,252,  2, 54, 91, 37, 85,151, 49,
     45, 93,250,152,227,138,146,174,  5,223, 41, 16,103,108,186,201,
    211,  0,230,207,225,158,168, 44, 99, 22,  1, 63, 88,226,137,169,
     13, 56, 52, 27,171, 51,255,176,187, 72, 12, 95,185,177,205, 46,
    197,243,219, 71,229,165,156,119, 10,166, 32,104,254,127,193,173
};

quint16 rotateLeft(quint16 value, int shift)
{
    return static_cast<quint16>((value << shift) | (value >> (16 - shift)));
}

quint16 rotateRight(quint16 value, int shift)
{
    return static_cast<quint16>((value >> shift) | (value << (16 - shift)));
}

quint16 readWord(const QByteArray &data, int offset)
{
    return static_cast<quint16>(
        static_cast<quint8>(data[offset]) |
        (static_cast<quint8>(data[offset + 1]) << 8));
}

void writeWord(QByteArray &data, int offset, quint16 value)
{
    data[offset] = static_cast<char>(value & 0xff);
    data[offset + 1] = static_cast<char>((value >> 8) & 0xff);
}

std::array<quint16, 64> expandRc2Key(const QByteArray &key, int effectiveBits)
{
    std::array<quint8, 128> expanded {};
    const int keyBytes = key.size();

    for (int i = 0; i < keyBytes; ++i) {
        expanded[i] = static_cast<quint8>(key[i]);
    }

    for (int i = keyBytes; i < 128; ++i) {
        expanded[i] = PITABLE[(expanded[i - 1] + expanded[i - keyBytes]) & 0xff];
    }

    const int t8 = (effectiveBits + 7) / 8;
    const quint8 tm = static_cast<quint8>(0xff >> ((8 * t8) - effectiveBits));
    expanded[128 - t8] = PITABLE[expanded[128 - t8] & tm];

    for (int i = 127 - t8; i >= 0; --i) {
        expanded[i] = PITABLE[expanded[i + 1] ^ expanded[i + t8]];
    }

    std::array<quint16, 64> result {};
    for (int i = 0; i < 64; ++i) {
        result[i] = static_cast<quint16>(expanded[2 * i] | (expanded[2 * i + 1] << 8));
    }

    return result;
}

void encryptRc2Block(QByteArray &data, int offset, const std::array<quint16, 64> &key)
{
    quint16 r0 = readWord(data, offset);
    quint16 r1 = readWord(data, offset + 2);
    quint16 r2 = readWord(data, offset + 4);
    quint16 r3 = readWord(data, offset + 6);

    int j = 0;
    for (int round = 0; round < 16; ++round) {
        r0 = rotateLeft(static_cast<quint16>(r0 + (r1 & ~r3) + (r2 & r3) + key[j++]), 1);
        r1 = rotateLeft(static_cast<quint16>(r1 + (r2 & ~r0) + (r3 & r0) + key[j++]), 2);
        r2 = rotateLeft(static_cast<quint16>(r2 + (r3 & ~r1) + (r0 & r1) + key[j++]), 3);
        r3 = rotateLeft(static_cast<quint16>(r3 + (r0 & ~r2) + (r1 & r2) + key[j++]), 5);

        if (round == 4 || round == 10) {
            r0 = static_cast<quint16>(r0 + key[r3 & 63]);
            r1 = static_cast<quint16>(r1 + key[r0 & 63]);
            r2 = static_cast<quint16>(r2 + key[r1 & 63]);
            r3 = static_cast<quint16>(r3 + key[r2 & 63]);
        }
    }

    writeWord(data, offset, r0);
    writeWord(data, offset + 2, r1);
    writeWord(data, offset + 4, r2);
    writeWord(data, offset + 6, r3);
}

void decryptRc2Block(QByteArray &data, int offset, const std::array<quint16, 64> &key)
{
    quint16 r0 = readWord(data, offset);
    quint16 r1 = readWord(data, offset + 2);
    quint16 r2 = readWord(data, offset + 4);
    quint16 r3 = readWord(data, offset + 6);

    int j = 63;
    for (int round = 15; round >= 0; --round) {
        if (round == 4 || round == 10) {
            r3 = static_cast<quint16>(r3 - key[r2 & 63]);
            r2 = static_cast<quint16>(r2 - key[r1 & 63]);
            r1 = static_cast<quint16>(r1 - key[r0 & 63]);
            r0 = static_cast<quint16>(r0 - key[r3 & 63]);
        }

        r3 = static_cast<quint16>(rotateRight(r3, 5) - ((r0 & ~r2) + (r1 & r2) + key[j--]));
        r2 = static_cast<quint16>(rotateRight(r2, 3) - ((r3 & ~r1) + (r0 & r1) + key[j--]));
        r1 = static_cast<quint16>(rotateRight(r1, 2) - ((r2 & ~r0) + (r3 & r0) + key[j--]));
        r0 = static_cast<quint16>(rotateRight(r0, 1) - ((r1 & ~r3) + (r2 & r3) + key[j--]));
    }

    writeWord(data, offset, r0);
    writeWord(data, offset + 2, r1);
    writeWord(data, offset + 4, r2);
    writeWord(data, offset + 6, r3);
}

quint32 rawCrc32(const QByteArray &data)
{
    quint32 crc = 0xffffffffU;
    for (const char c : data) {
        crc ^= static_cast<quint8>(c);
        for (int bit = 0; bit < 8; ++bit) {
            const quint32 mask = -(crc & 1U);
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    }
    return crc;
}

QString crcHeader(const QByteArray &data)
{
    return QString::number(rawCrc32(data), 16).left(4).toUpper();
}

QByteArray rc2Transform(const QByteArray &input, const QByteArray &keyBytes, int effectiveBits, bool encrypt)
{
    QByteArray output = input;
    const auto key = expandRc2Key(keyBytes, effectiveBits);

    for (int i = 0; i < output.size(); i += 8) {
        if (encrypt) {
            encryptRc2Block(output, i, key);
        } else {
            decryptRc2Block(output, i, key);
        }
    }

    return output;
}

} // namespace

EnCrypt::EnCrypt() = default;

int EnCrypt::decrypt(QString &result, QString text, QString passphrase)
{
    return decrypt(result, text, passphrase, "RC2", 64);
}

int EnCrypt::decrypt(QString &result, QString text, QString passphrase, QString cipher, int length)
{
    result.clear();

    if (cipher.compare(QStringLiteral("RC2"), Qt::CaseInsensitive) != 0) {
        return Invalid_Method;
    }

    if (length <= 0 || length > 1024) {
        return Invalid_Arguments;
    }

    const QByteArray encrypted = QByteArray::fromBase64(text.toLatin1());
    if (encrypted.isEmpty() || (encrypted.size() % 8) != 0) {
        return Invalid_Key;
    }

    const QByteArray key = QCryptographicHash::hash(passphrase.toUtf8(), QCryptographicHash::Md5);
    const QByteArray decrypted = rc2Transform(encrypted, key, length, false);
    if (decrypted.size() < 4) {
        return Invalid_Key;
    }

    const QByteArray expectedCrc = decrypted.left(4);
    QByteArray textBytes = decrypted.mid(4);
    if (crcHeader(textBytes).toLatin1() != expectedCrc) {
        return Invalid_Key;
    }

    while (!textBytes.isEmpty() && textBytes.endsWith('\0')) {
        textBytes.chop(1);
    }

    result = QString::fromUtf8(textBytes);
    return 0;
}

int EnCrypt::encrypt(QString &result, QString text, QString passphrase)
{
    return encrypt(result, text, passphrase, "RC2", 64);
}

int EnCrypt::encrypt(QString &result, QString text, QString passphrase, QString cipher, int length)
{
    result.clear();

    if (cipher.compare(QStringLiteral("RC2"), Qt::CaseInsensitive) != 0) {
        return Invalid_Method;
    }

    if (length <= 0 || length > 1024) {
        return Invalid_Arguments;
    }

    QByteArray textBytes = text.toUtf8();
    const int align = (textBytes.size() + 4) % 8;
    const int paddingNeeded = 8 - align;
    textBytes.append(QByteArray(paddingNeeded, '\0'));

    QByteArray clearText = crcHeader(textBytes).toUtf8();
    clearText.append(textBytes);

    const QByteArray key = QCryptographicHash::hash(passphrase.toUtf8(), QCryptographicHash::Md5);
    const QByteArray encrypted = rc2Transform(clearText, key, length, true);
    result = QString::fromLatin1(encrypted.toBase64());
    return 0;
}

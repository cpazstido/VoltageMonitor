#ifndef UTILS
#define UTILS
typedef unsigned char BYTE;
#include <QDebug>
#include <QString>

QString byteToHexStr(BYTE *b, int length);
BYTE *RTU_CRC(BYTE *crc, BYTE *puchMsg, int usDataLen);

#endif // UTILS


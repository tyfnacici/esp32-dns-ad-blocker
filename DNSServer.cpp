#include "DNSServer.h"
#include <lwip/def.h>
#include <WiFi.h>
#include <Arduino.h>

DNSServer::DNSServer()
{
  _ttl = htonl(60);
  _errorReplyCode = DNSReplyCode::NonExistentDomain;
}

bool DNSServer::start(const uint16_t &port, const String &domainName, const IPAddress &resolvedIP)
{
  _port = port;
  _buffer = NULL;
  _domainName = domainName;
  _resolvedIP[0] = resolvedIP[0];
  _resolvedIP[1] = resolvedIP[1];
  _resolvedIP[2] = resolvedIP[2];
  _resolvedIP[3] = resolvedIP[3];
  downcaseAndRemoveWwwPrefix(_domainName);
  _queryDomainName = "";
  return _udp.begin(_port) == 1;
}

void DNSServer::setErrorReplyCode(const DNSReplyCode &replyCode)
{
  _errorReplyCode = replyCode;
}

void DNSServer::setTTL(const uint32_t &ttl)
{
  _ttl = htonl(ttl);
}

void DNSServer::stop()
{
  _udp.stop();
  free(_buffer);
  _buffer = NULL;
}

void DNSServer::downcaseAndRemoveWwwPrefix(String &domainName)
{
  domainName.toLowerCase();
}

int DNSServer::processNextRequest()
{
  int ret = -2;
  _queryDomainName = "";
  _currentPacketSize = _udp.parsePacket();
  if (_currentPacketSize)
  {
    if (_buffer != NULL) free(_buffer);
    _buffer = (unsigned char *)malloc(_currentPacketSize * sizeof(char));
    if (_buffer == NULL) return -2;
    _udp.read(_buffer, _currentPacketSize);
    _dnsHeader = (DNSHeader *)_buffer;

    if (_dnsHeader->QR == DNS_QR_QUERY &&
        _dnsHeader->OPCode == DNS_OPCODE_QUERY &&
        requestIncludesOnlyOneQuestion() &&
        (_domainName == "*" || getDomainNameWithoutWwwPrefix() == _domainName))
    {
      String dom = getDomainNameWithoutWwwPrefix();
      _queryDomainName = dom;
      return 0;
    }
    else if (_dnsHeader->QR == DNS_QR_QUERY)
    {
      replyWithCustomCode();
      ret = -1;
    }
  }
  return ret;
}

bool DNSServer::requestIncludesOnlyOneQuestion()
{
  return ntohs(_dnsHeader->QDCount) == 1 &&
         _dnsHeader->ANCount == 0 &&
         _dnsHeader->NSCount == 0 &&
         _dnsHeader->ARCount == 0;
}

String DNSServer::getDomainNameWithoutWwwPrefix()
{
  String parsedDomainName = "";
  if (_buffer == NULL)
  {
    _queryDomainName = parsedDomainName;
    return parsedDomainName;
  }
  unsigned char *start = _buffer + 12;
  if (*start == 0)
  {
    _queryDomainName = parsedDomainName;
    return parsedDomainName;
  }
  int pos = 0;
  while (true)
  {
    unsigned char labelLength = *(start + pos);
    for (int i = 0; i < labelLength; i++)
    {
      pos++;
      parsedDomainName += (char)*(start + pos);
    }
    pos++;
    if (*(start + pos) == 0)
    {
      downcaseAndRemoveWwwPrefix(parsedDomainName);
      _queryDomainName = parsedDomainName;
      return parsedDomainName;
    }
    else
    {
      parsedDomainName += ".";
    }
  }
}

void DNSServer::replyWithIP(const IPAddress &resolvedIP)
{
  if (_buffer == NULL) return;
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->ANCount = _dnsHeader->QDCount;
  _dnsHeader->QDCount = _dnsHeader->QDCount;

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write(_buffer, _currentPacketSize);

  _udp.write((uint8_t)192); // answer name is a pointer
  _udp.write((uint8_t)12);  // pointer to offset at 0x00c

  _udp.write((uint8_t)0);   // 0x0001 answer is type A query (host address)
  _udp.write((uint8_t)1);

  _udp.write((uint8_t)0);   // 0x0001 answer is class IN (internet address)
  _udp.write((uint8_t)1);

  _udp.write((unsigned char *)&_ttl, 4);

  // Length of RData is 4 bytes (because, in this case, RData is IPv4)
  _udp.write((uint8_t)0);
  _udp.write((uint8_t)4);

  _resolvedIP[0] = resolvedIP[0];
  _resolvedIP[1] = resolvedIP[1];
  _resolvedIP[2] = resolvedIP[2];
  _resolvedIP[3] = resolvedIP[3];

  _udp.write(_resolvedIP, sizeof(_resolvedIP));
  _udp.endPacket();

  String dom = getDomainNameWithoutWwwPrefix();
}

void DNSServer::replyWithCustomCode()
{
  if (_buffer == NULL) return;
  _dnsHeader->QR = DNS_QR_RESPONSE;
  _dnsHeader->RCode = (unsigned char)_errorReplyCode;
  _dnsHeader->QDCount = 0;

  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write(_buffer, sizeof(DNSHeader));
  _udp.endPacket();
}

String DNSServer::getQueryDomainName()
{
  return _queryDomainName;
}

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "DNSServer.h"

// WiFi settings
#define WIFI_SSID "TUTANKHAMUN_GUEST"
#define WIFI_PASSWORD "Ly36F8bp"

// DNS settings
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer webServer(80);

void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA); // Station mode only
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

void listSPIFFSContents() {
  Serial.println("\n=== SPIFFS Contents ===");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int fileCount = 0;
  long totalSize = 0;
  
  while (file) {
    Serial.printf("File: %s, Size: %ld bytes\n", file.name(), file.size());
    fileCount++;
    totalSize += file.size();
    file = root.openNextFile();
  }
  
  Serial.printf("\nTotal files: %d\n", fileCount);
  Serial.printf("Total size: %ld bytes\n", totalSize);
  Serial.printf("Space used: %ld bytes\n", SPIFFS.usedBytes());
  Serial.printf("Space total: %ld bytes\n", SPIFFS.totalBytes());
  Serial.println("=====================");
}

bool isDomainBlocked(const String &domain) {
  char filename[32];
  snprintf(filename, sizeof(filename), "/hosts_%d", domain.length());
  
  if (!SPIFFS.exists(filename)) {
    Serial.printf("Hosts file not found: %s\n", filename);
    return false;
  }
  
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.printf("Error: Cannot open file %s\n", filename);
    return false;
  }
  
  String searchStr = "," + domain + ",";
  bool found = file.findUntil(searchStr.c_str(), "@@@");
  file.close();
  
  return found;
}

void resolveDomain(const String &domain) {
  IPAddress ip;
  unsigned long startTime = millis();
  bool resolved = WiFi.hostByName(domain.c_str(), ip);
  unsigned long resolveTime = millis() - startTime;
  
  if (resolved) {
    Serial.printf("Resolved domain %s to IP: %s (took %lu ms)\n", 
                 domain.c_str(), ip.toString().c_str(), resolveTime);
    dnsServer.replyWithIP(ip);
  } else {
    Serial.printf("Failed to resolve domain: %s\n", domain.c_str());
    dnsServer.replyWithIP(IPAddress(0, 0, 0, 0));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 DNS Ad Blocker Starting...");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  listSPIFFSContents();
  setup_wifi();
  
  // Start DNS server
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  if (dnsServer.start(DNS_PORT, "*", WiFi.localIP())) {
    Serial.println("DNS Server started successfully");
  } else {
    Serial.println("Failed to start DNS Server!");
  }
  
  Serial.println("ESP32 DNS Ad Blocker Ready!");
}

void loop() {
  int dnsResult = dnsServer.processNextRequest();
  
  if (dnsResult == 0) {  // Valid DNS query received
    String domain = dnsServer.getQueryDomainName();
    
    if (!domain.isEmpty()) {
      Serial.printf("\nProcessing DNS query for: %s\n", domain.c_str());
      
      if (isDomainBlocked(domain)) {
        Serial.println("Domain BLOCKED!");
        dnsServer.replyWithIP(IPAddress(0, 0, 0, 0));
      } else {
        resolveDomain(domain);
      }
    }
  }
  
  yield();  // Give other tasks a chance to run
}

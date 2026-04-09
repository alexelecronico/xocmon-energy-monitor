#include "helpers/OTAClient.h"

OTAClient::OTAClient()
{
  this->wifiOTAClient = WiFiClient();
}

void OTAClient::setDeviceName(const char deviceName[])
{
  if (this->deviceName != nullptr)
  {
    free((void *)this->deviceName);
  }

  this->deviceName = strdup(deviceName);
}

String OTAClient::getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

bool OTAClient::validateUpdateMessage(String textPayload, String *updateVersion)
{
  // Calculate number of occurrences of separators
  int valueIndex = 0;
  String currentValue = "";
  bool validationResult = false;
  for (int i = 0; i < textPayload.length(); i++)
  {
    if (String(",").compareTo(String(textPayload[i])) == 0 || String(":").compareTo(String(textPayload[i])) == 0)
    {
      if (valueIndex == 1 && currentValue.compareTo(String("3312")) == 0)
      {
        validationResult = true;
      }
      currentValue = F("");
      valueIndex++;
    }
    else
    {
      currentValue += String(textPayload[i]);
    }
  }

  *updateVersion = currentValue;

  return validationResult;
}

void OTAClient::update(String binFile)
{
  long contentLength = 0;
  bool isValidContentType = false;
  unsigned long timeout = millis();

  int port = 80;
  this->file = binFile;

  Serial.print(binFile);
  //String host = "https://www.dropbox.com/sh/slmqs0kqnky5hcw/AAA9VRAAjtnXwwJsLOnKF4aJa?dl=0/";
  //String host = "esp32-exc.s3.amazonaws.com";
  //String host = "dropbox.com/s/jz95zwvgvt21nvp";
  String host = "otabucketm.s3.amazonaws.com";  //m3tr S3 carpeta

  

  Serial.println("[INFO]: OTA Connecting to: " + host + this->file);

  if (this->wifiOTAClient.connect(host.c_str(), port))
  {
    Serial.println("[INFO]: Fetching Bin: " + this->file);

    // Make request
    this->wifiOTAClient.print(String("GET ") + this->file + " HTTP/1.1\r\n" +
                              "Host: " + host + "\r\n" +
                              "Cache-Control: no-cache\r\n" +
                              "Connection: close\r\n\r\n");

    // Wait until server response
    while (this->wifiOTAClient.available() == 0)
    {
      if (millis() - timeout > 5000)
      {
        Serial.println("[ERROR]: OTA client Timeout.");
        timeout = millis();
        wifiOTAClient.stop();
        break;
      }
    }

    // Read response
    while (this->wifiOTAClient.available())
    {
      String line = wifiOTAClient.readStringUntil('\n');
      line.trim();

      if (!line.length())
      {
        break;
      }

      if (line.startsWith("HTTP/1.1"))
      {
        if (line.indexOf("200") < 0)
        {
          Serial.println("[ERROR]: Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      if (line.startsWith("Content-Length: "))
      {
        contentLength = atol((this->getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("[INFO]: Got " + String(contentLength) + " bytes from server");
      }

      if (line.startsWith("Content-Type: "))
      {
        String contentType = this->getHeaderValue(line, "Content-Type: ");
        Serial.println("[INFO]: Got " + contentType + " payload.");

        if (contentType == "application/octet-stream")
        {
          isValidContentType = true;
        }
      }
    }
  }
  else
  {
    // TODO: retry
    Serial.println("[ERROR]: Connection to " + host + this->file + " failed. Please check your setup");
  }

  // Check contentLength and content type
  if (contentLength && isValidContentType)
  {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);

    // If yes, begin
    if (canBegin)
    {
      Serial.println("[INFO]: Begin OTA ...");
      size_t written = Update.writeStream(wifiOTAClient);

      if (written == contentLength)
      {
        Serial.println("[INFO]: Written : " + String(written) + " successfully");
      }
      else
      {
        // TODO: retry
        Serial.println("[INFO]: Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
      }

      if (Update.end())
      {
        Serial.println("[INFO]: OTA done!");
        if (Update.isFinished())
        {
          Serial.println("[INFO]: OTA update successfully completed. Rebooting ...");
          delay(1000);
          ESP.restart();
        }
        else
        {
          Serial.println("[ERROR]: Update not finished? Something went wrong!");
        }
      }
      else
      {
        Serial.println("[ERROR]: Error Occurred. Error #: " + String(Update.getError()));
      }
    }
    else
    {
      // not enough space to begin OTA
      // Understand the partitions and space availability
      Serial.println("[ERROR]: Not enough space to begin OTA");
      this->wifiOTAClient.stop();
    }
  }
  else
  {
    Serial.println("[ERROR]: There was no content in the response");
    this->wifiOTAClient.stop();
  }

  return;
}
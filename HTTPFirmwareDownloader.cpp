/*   2log.io
 *   Copyright (C) 2021 - 2log.io | mail@2log.io,  sascha@2log.io
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "HTTPFirmwareDownloader.h"
#include "IFirmwareWriter.h"

extern "C"
{
	#include <esp_log.h>
}

namespace
{
	const char*		LOG_TAG						= "IDFix::HTTPFirmwareDownloader";
	const size_t	HTTP_RECEIVE_BUFFER_SIZE	= 1024;
}

namespace IDFix
{
	namespace FOTA
	{
		HTTPFirmwareDownloader::HTTPFirmwareDownloader()
		{

		}

		void HTTPFirmwareDownloader::setFirmwareWriter(IFirmwareWriter *writer)
		{
			_firmwareWriter = writer;
		}

		int HTTPFirmwareDownloader::downloadFirmware(esp_http_client_config_t *httpConfig)
		{
			if ( _firmwareWriter == nullptr )
			{
				ESP_LOGE(LOG_TAG, "Download error: no firmware writer set!");
				return -1;
			}

			_httpClient = esp_http_client_init(httpConfig);

			esp_err_t errorCode;

			if ( (errorCode = esp_http_client_open(_httpClient, 0) ) != ESP_OK )
			{
				ESP_LOGE(LOG_TAG, "Failed to open HTTP connection: %d", errorCode);
				return -1;
			}

			int contentLength =  esp_http_client_fetch_headers(_httpClient);
			ESP_LOGI(LOG_TAG, "Content length: %d", contentLength);


			char *readBuffer = new char[HTTP_RECEIVE_BUFFER_SIZE];

				if ( readBuffer == nullptr )
				{
					ESP_LOGE(LOG_TAG, "could not allocate memory for http read buffer");
					return -1;
				}

				int totalReadBytes = 0;
				int currentReadBytes = 0;
				bool downloadSuccessful = true;

				do
				{
					currentReadBytes = esp_http_client_read(_httpClient, readBuffer, HTTP_RECEIVE_BUFFER_SIZE);

					if ( currentReadBytes >= 0)
					{
						totalReadBytes = totalReadBytes + currentReadBytes;

						ESP_LOGI(LOG_TAG, "[*] %.*f %% | Downloaded %d from %d Bytes", 2, (100.0f / contentLength) * totalReadBytes, totalReadBytes, contentLength );
						if ( (errorCode = _firmwareWriter->writeFirmwareBytes(readBuffer, currentReadBytes) ) != ESP_OK )
						{
							ESP_LOGE(LOG_TAG, "failed writeFirmwareBytes with result %s", esp_err_to_name(errorCode) );
							downloadSuccessful = false;
							break;
						}

					}
					else
					{
						ESP_LOGE(LOG_TAG, "could not read from http stream...");
						downloadSuccessful = false;
						break;
					}

				}
				while ( totalReadBytes < contentLength );

			delete [] readBuffer;

			if ( downloadSuccessful == false )
			{
				return -1;
			}

			return 0;
		}

	}
}

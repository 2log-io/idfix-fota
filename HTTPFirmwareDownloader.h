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

#ifndef HTTPFIRMWAREDOWNLOADER_H
#define HTTPFIRMWAREDOWNLOADER_H

extern "C"
{
	#include "esp_http_client.h"
}

namespace IDFix
{
	namespace FOTA
	{
		class IFirmwareWriter;

        /**
         * @brief The HTTPFirmwareDownloader class provides a possibility to download a firmware image via HTTP.
         *
         * The downloaded firmware will be written via a IFirmwareWriter to an approrpiate location.
         */
		class HTTPFirmwareDownloader
		{
			public:

									HTTPFirmwareDownloader();

                /**
                 * @brief           Set the IFirmwareWriter used to write the downloaded firmware
                 * @param writer    the IFirmwareWriter used to write the firmware
                 */
				void				setFirmwareWriter(IFirmwareWriter* writer);

                /**
                 * @brief           Start the firmware download from HTTP
                 *
                 * @param httpConfig    the IDF http configuration to be used
                 *
                 * @return          \c 0 if download was successful
                 * @return          \c -1 if download failed
                 */
				int					downloadFirmware(esp_http_client_config_t *httpConfig);

			private:

				IFirmwareWriter*			_firmwareWriter = { nullptr };
				esp_http_client_handle_t	_httpClient = { nullptr };
		};
	}
}

#endif

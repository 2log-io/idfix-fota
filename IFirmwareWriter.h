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

#ifndef IFIRMWAREWRITER_H
#define IFIRMWAREWRITER_H

extern "C"
{
	#include "esp_err.h"
}

namespace IDFix
{
	namespace FOTA
	{
        /**
         * @brief The IFirmwareWriter class provides an interface abstraction used by firmware downloaders
         *          to write the received firmware to the flash.
         */
		class IFirmwareWriter
		{
			public:

				virtual				~IFirmwareWriter() {}

                /**
                 * \brief           Write OTA firmware bytes continuously to flash
                 *
                 * This function is called multiple times as data is received
                 * to write the firmware sequentially to flash.
                 *
                 * \param data      Data buffer to write
                 * \param size      Size of data buffer in bytes.
                 *
                 * \return          ESP_OK on success
                 * \return          the error code from IDF get_ota_partition_count
                 */
				virtual esp_err_t	writeFirmwareBytes(const void* data, size_t size) = 0;
		};
	}
}

#endif // IFIRMWAREWRITER_H

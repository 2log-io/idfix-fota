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

#include "FirmwareUpdater.h"
#include "MutexLocker.h"

#include "SignatureVerifier.h"
#include "HashAlgorithm.h"

extern "C"
{
	#include <esp_log.h>
	#include <string.h>
}

namespace
{
	const char*		LOG_TAG					= "IDFix::FirmwareUpdater";
	const size_t	HASH_READ_BUFFER_SIZE	= 256;
}

namespace IDFix
{
	namespace FOTA
	{
		bool FirmwareUpdater::__updateIsRunning = false;
		IDFix::Mutex FirmwareUpdater::__updaterMutex;

		FirmwareUpdater::FirmwareUpdater()
		{

		}

		bool FirmwareUpdater::beginUpdate(size_t imageSize, const esp_partition_t *updatePartition)
		{
			if ( ! lockUpdate() )
			{
				return false;
			}

			if ( updatePartition == nullptr )
			{
				updatePartition = esp_ota_get_next_update_partition(nullptr);
				if ( updatePartition == nullptr )
				{
					ESP_LOGE(LOG_TAG, "Failed to get available update partition. Aborting...");
					unlockUpdate();
					return false;
				}
			}

			_updatePartition = updatePartition;

			esp_err_t result = esp_ota_begin(_updatePartition, imageSize, &_updateHandle);

			if ( result != ESP_OK )
			{
				ESP_LOGE(LOG_TAG, "esp_ota_begin failed with result %s", esp_err_to_name(result) );
				unlockUpdate();
				return false;
			}

			return true;
		}

		bool FirmwareUpdater::isUpdateRunning()
		{
			MutexLocker locker(__updaterMutex);

			return __updateIsRunning;
		}

		esp_err_t FirmwareUpdater::writeFirmwareBytes(const void *data, size_t size)
		{
			if ( isUpdateRunning() && _updateHandle != 0 )
			{
				esp_err_t result = esp_ota_write(_updateHandle, data, size);

				if ( result == ESP_OK )
				{
					_firmwareSize += size;
				}

				return result;
			}
			return ESP_FAIL;
		}

		bool FirmwareUpdater::finishUpdate()
		{
			if ( isUpdateRunning() && _updateHandle != 0 )
			{
				esp_err_t result;

				result = esp_ota_end(_updateHandle);

				if ( result != ESP_OK )
				{
					ESP_LOGE(LOG_TAG, "esp_ota_end failed with result %s", esp_err_to_name(result) );
					unlockUpdate();
					return false;
				}

				if ( checkFirmware() == false )
				{
					ESP_LOGE(LOG_TAG, "Firmware check failed! Aborting firmware update...");
					unlockUpdate();
					return false;
				}

				result = esp_ota_set_boot_partition(_updatePartition);

				if ( result != ESP_OK )
				{
					ESP_LOGE(LOG_TAG, "esp_ota_set_boot_partition failed with result %s", esp_err_to_name(result) );
					unlockUpdate();
					return false;
				}

				ESP_LOGI(LOG_TAG, "Firmware update finished successful, firmware size: %u bytes", _firmwareSize);

                unlockUpdate();
				return true;
			}
			return false;
		}

		bool FirmwareUpdater::abortUpdate()
		{
			if ( isUpdateRunning() && _updateHandle != 0 )
			{
				esp_err_t result;

				result = esp_ota_end(_updateHandle);

				if ( result != ESP_OK )
				{
					ESP_LOGW(LOG_TAG, "esp_ota_end failed with result %s", esp_err_to_name(result) );
				}

				unlockUpdate();
				return true;
			}
            return false;
        }

        bool FirmwareUpdater::activateNextUpdatePartition()
        {
            volatile MutexLocker locker(__updaterMutex);

            if ( __updateIsRunning )
			{
				return false;
			}

			__updateIsRunning = true;

			const esp_partition_t *updatePartition = esp_ota_get_next_update_partition(nullptr);
            if ( updatePartition != nullptr )
            {
                esp_err_t result = esp_ota_set_boot_partition(updatePartition);

                if ( result == ESP_OK )
                {
                    __updateIsRunning = false;
                    return true;
                }
                else
                {
                    ESP_LOGE(LOG_TAG, "esp_ota_set_boot_partition failed with result %s", esp_err_to_name(result) );
                }
            }
            else
            {
                ESP_LOGE(LOG_TAG, "Failed to get available update partition. Aborting...");
            }

            __updateIsRunning = false;
            return false;
        }

		const esp_partition_t *FirmwareUpdater::getUpdatePartition() const
		{
			return _updatePartition;
		}

		bool FirmwareUpdater::setMagicBytes(const char *magicBytes, size_t length)
		{
			if ( _magicBytes != nullptr )
			{
				delete [] _magicBytes;
			}

			_magicBytes = new char [length];
			if ( _magicBytes == nullptr )
			{
				ESP_LOGE(LOG_TAG, "could not allocate memory for magic bytes");
				return false;
			}

			_magicBytesLength = length;
			memcpy(_magicBytes, magicBytes, length);

			return true;
		}

		bool FirmwareUpdater::installSignatureVerifier(SignatureVerifier *verifier, HashAlgorithm *hashAlgo)
		{
			if ( (verifier == nullptr) || (hashAlgo == nullptr) )
			{
				return false;
			}

			_signatureVerifier = verifier;
			_hashAlgorithm = hashAlgo;

			return true;
		}

		bool FirmwareUpdater::lockUpdate()
		{
			MutexLocker locker(__updaterMutex);

			if ( __updateIsRunning )
			{
				return false;
			}

			__updateIsRunning = true;
			_firmwareSize = 0;
			return true;
		}

		void FirmwareUpdater::unlockUpdate()
		{
			MutexLocker locker(__updaterMutex);

			__updateIsRunning = false;
			_updatePartition = nullptr;
			_updateHandle = 0;
		}

		bool FirmwareUpdater::checkFirmware()
		{
			ESP_LOGI(LOG_TAG, "Update partition start address: %08x", _updatePartition->address );
			ESP_LOGI(LOG_TAG, "Firmware size: %u bytes", _firmwareSize);

			uint32_t signatureLength = 0;

			if ( esp_partition_read(_updatePartition, _firmwareSize - sizeof(signatureLength), &signatureLength, sizeof(signatureLength) ) != ESP_OK )
			{
				ESP_LOGE(LOG_TAG, "could not read signature length from flash!");
				return false;
			}

			ESP_LOGI(LOG_TAG, "Signature length: %u bytes", signatureLength);

			size_t appendixSize = signatureLength + _magicBytesLength + sizeof(signatureLength);

			if ( appendixSize > _firmwareSize )
			{
				ESP_LOGE(LOG_TAG, "Invalid appendix length");
				return false;
			}

			if ( magicBytesUsed() )
			{
				size_t magicBytesOffset = _firmwareSize - appendixSize;
				if ( checkMagicBytes(magicBytesOffset) == false )
				{
					ESP_LOGE(LOG_TAG, "Invalid magic bytes!");
					return false;
				}
			}

			if ( signatureUsed() )
			{
				if ( checkFirmwareSignature(signatureLength) == false )
				{
					ESP_LOGE(LOG_TAG, "Firmware signature check failed!");
					return false;
				}
				else
				{
					ESP_LOGI(LOG_TAG, "Firmware signature check success!");
				}
			}

			return true;
		}

		bool FirmwareUpdater::checkFirmwareSignature(uint32_t signatureLength)
		{

			if ( signatureLength == 0 )
			{
				return false;
			}

			unsigned char *readBuffer = new unsigned char[HASH_READ_BUFFER_SIZE];

			if ( readBuffer == nullptr )
			{
				ESP_LOGE(LOG_TAG, "could not allocate memory for hashing buffer");
				return false;
			}

			size_t remaningBytesToHash = _firmwareSize - signatureLength - sizeof(signatureLength);;
			size_t numberOfBytesHashed  = 0;

			_hashAlgorithm->begin();

			ESP_LOGI(LOG_TAG, "Calculating hash of update");

			while (remaningBytesToHash > 0)
			{
				if ( esp_partition_read(_updatePartition, numberOfBytesHashed, readBuffer, HASH_READ_BUFFER_SIZE ) != ESP_OK )
				{
					ESP_LOGE(LOG_TAG, "could not read from flash for hashing");
					_hashAlgorithm->end();
					delete [] readBuffer;
					return false;
				}

				if ( remaningBytesToHash > HASH_READ_BUFFER_SIZE )
				{
					_hashAlgorithm->addData(readBuffer, HASH_READ_BUFFER_SIZE);
					remaningBytesToHash = remaningBytesToHash - HASH_READ_BUFFER_SIZE;
					numberOfBytesHashed = numberOfBytesHashed + HASH_READ_BUFFER_SIZE;
				}
				else
				{
					_hashAlgorithm->addData(readBuffer, remaningBytesToHash);
					numberOfBytesHashed = numberOfBytesHashed + remaningBytesToHash;
					remaningBytesToHash = 0;
				}
			}

			delete [] readBuffer;

			_hashAlgorithm->end();

			unsigned char *signature = new unsigned char[signatureLength];

			if ( signature == nullptr )
			{
				ESP_LOGE(LOG_TAG, "could not allocate memory for signature");
				return false;
			}

			size_t signatureOffset = _firmwareSize - signatureLength - sizeof(signatureLength);

			bool signatureValid = false;

			if ( esp_partition_read(_updatePartition, signatureOffset, signature, signatureLength) == ESP_OK )
			{
				if ( _signatureVerifier->verify( _hashAlgorithm->getHash(), _hashAlgorithm->hashLength(), signature, signatureLength) == 0 )
				{
					signatureValid = true;
				}
			}
			else
			{
				ESP_LOGE(LOG_TAG, "could not read signature bytes from flash!");
			}

			delete [] signature;

			return signatureValid;
		}

		bool FirmwareUpdater::checkMagicBytes(size_t magicBytesOffset)
		{
			if ( _magicBytesLength == 0 )
			{
				return false;
			}

			char* magicBytesRead = new char [_magicBytesLength];

			if ( magicBytesRead == nullptr )
			{
				ESP_LOGE(LOG_TAG, "could not allocate memory for magic bytes");
				return false;
			}

			bool magicBytesMatch = false;

			if ( esp_partition_read(_updatePartition, magicBytesOffset, magicBytesRead, _magicBytesLength ) == ESP_OK )
			{
				ESP_LOGI(LOG_TAG, "Magic bytes read: %.*s",     _magicBytesLength, magicBytesRead);
				ESP_LOGI(LOG_TAG, "Magic bytes expected: %.*s", _magicBytesLength, _magicBytes);

				if ( memcmp(_magicBytes, magicBytesRead, _magicBytesLength) == 0 )
				{
					magicBytesMatch = true;
				}
			}
			else
			{
				ESP_LOGE(LOG_TAG, "could not read magic bytes from flash!");
			}

			delete [] magicBytesRead;

			return magicBytesMatch;
		}

	}
}

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

#ifndef FIRMWAREUPDATER_H
#define FIRMWAREUPDATER_H

extern "C"
{
    #include "esp_ota_ops.h"
    #include "esp_system.h"
}

#include "IFirmwareWriter.h"
#include "Mutex.h"
#include "SignatureVerifier.h"
#include "HashAlgorithm.h"

namespace IDFix
{
    namespace FOTA
    {
        using namespace IDFix::Crypto;

        /**
         * @brief The FirmwareUpdater class provides methods to write a firmware update to the flash.
         *
         * It implements the IFirmwareWriter interface which different download sources can use to write the
         * firmware to the flash.
         */
		class FirmwareUpdater : public IFirmwareWriter
        {
            public:

                FirmwareUpdater();

                /**
                 * @brief                   Start an update transaction
                 *
                 * @param imageSize         Optional size of the new firmware or OTA_SIZE_UNKNOWN - affects the portion of the partition that will be erased. By default the entire partition is erased.
                 * @param updatePartition   Optional pointer to partition information used for the update
                 *
                 * @return                  \c true if update transaction was started, otherwise \c false
                 */
                bool                        beginUpdate(size_t imageSize = OTA_SIZE_UNKNOWN, const esp_partition_t *updatePartition = nullptr);

                /**
                 * \brief               Write OTA firmware bytes continuously to flash
                 *
                 * This function is called multiple times as data is received
                 * to write the firmware sequentially to flash.
                 *
                 * \param data          Data buffer to write
                 * \param size          Size of data buffer in bytes.
                 *
                 * \return              ESP_OK on success
                 * \return              the error code from IDF get_ota_partition_count
                 */
				esp_err_t               writeFirmwareBytes(const void* data, size_t size) override;

                /**
                 * @brief               Finish the update transaction and check the updated firmware
                 *
                 * This function finishes and checks the written update (if used) for the expected magic bytes
                 * and verifies the signature. If everything is correct the update partition is set as boot partition.
                 *
                 * @return              true on success
                 */
                bool                    finishUpdate();


                /**
                 * @brief               Abort a running update transaction, active boot partition is not altered
                 * @return              true on success
                 */
				bool					abortUpdate();

                /**
                 * \brief               Set the next available OTA partition as boot partition
                 *
                 * Can be used to activate a pre-installed firmware image.
                 *
                 * \return              true on success
                 */
				static bool             activateNextUpdatePartition(void);

                static bool             isUpdateRunning();

                /**
                 * \brief               Get the partition information of the partition used for the update
                 *
                 * \return              pointer to partition information for active update
                 * \return              nullptr if no update is running
                 */
                const esp_partition_t*  getUpdatePartition() const;

                /**
                 * @brief               Set the magic bytes expected in the firmware image
                 *
                 * @param magicBytes    Data buffer containing the magic bytes - will be copied
                 * @param length        Length of the magic bytes buffer
                 *
                 * @return              true on success, false if memory could not be allocated
                 */
				bool                    setMagicBytes(const char* magicBytes, size_t length);

                /**
                 * @brief               Install a SignatureVerifier to verify the written firmware
                 *
                 * @param verifier      pointer to the SignatureVerifier object
                 * @param hashAlgo      pointer to the used HashAlgorithm implementation
                 *
                 * @return              true if verifier was successfully installed, otherwise \c false
                 */
                bool                    installSignatureVerifier(SignatureVerifier *verifier, HashAlgorithm *hashAlgo);

            protected:

				static bool             __updateIsRunning;
				static Mutex            __updaterMutex;

                /**
                 * @brief               Lock the update for further transactions.
                 *
                 * @return              true if update lock was aquired
                 * @return              false if update transaction is already locked
                 */
				bool                    lockUpdate();

                /**
                 * @brief               Unlock an ongoing update transaction
                 */
				void                    unlockUpdate();

			private:

                /**
                 * @brief               Wrapper function for the final firmware check
                 *
                 * Calculates the firmware image offset for the signature and magic bytes and calls the appropriate checking functions.
                 *
                 * @return              true if firmware image is ok, otherwise false
                 */
				bool                    checkFirmware();


                /**
                 * @brief               Check the signature of the firmware image
                 *
                 * @param signatureLength   the length of the actual signature in bytes
                 *
                 * @return              true if firmware signature is ok, otherwise false
                 */
				bool                    checkFirmwareSignature(uint32_t signatureLength);

                /**
                 * @brief               Check if the firmware image contains the correct magic bytes
                 *
                 * @param magicBytesOffset  the magic bytes location offset into the firmware image
                 *
                 * @return              true if magic bytes were found, otherwise false
                 */
				bool                    checkMagicBytes(size_t magicBytesOffset);

                /**
                 * @brief               Check usage of magic bytes
                 *
                 * @return              true if magic bytes were configured, otherwise false
                 */
				inline bool				magicBytesUsed() { return _magicBytesLength > 0; }

                /**
                 * @brief               Check usgae of signature
                 *
                 * @return              true if firmware signature was configured to be used, otherwise false
                 */
				inline bool				signatureUsed() { return _signatureVerifier != nullptr; }

                esp_ota_handle_t        _updateHandle = { 0 } ;
                const esp_partition_t*  _updatePartition = { nullptr };
                uint32_t                _firmwareSize = { 0 };
                SignatureVerifier*      _signatureVerifier = { nullptr };
                HashAlgorithm*          _hashAlgorithm = { nullptr };

				char*                   _magicBytes = {nullptr};
				size_t                  _magicBytesLength = { 0 };
        };
    }
}

#endif // WEBSOCKET_H

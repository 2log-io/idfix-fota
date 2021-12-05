#   2log.io
#   Copyright (C) 2021 - 2log.io | mail@2log.io,  sascha@2log.io
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES idfix-core openssl app_update idfix-crypto esp_http_client)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS	"FirmwareUpdater.h" "FirmwareUpdater.cpp"
			"IFirmwareWriter.h" "IFirmwareWriter.cpp"
			"HTTPFirmwareDownloader.h" "HTTPFirmwareDownloader.cpp" )

set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_options(-std=gnu++17)


.PHONY: _port_on_config
_port_on_config:
	python -m venv ../.venv
	../.venv/bin/pip install esptool
	git submodule update --init port/esp_common/esp-idf

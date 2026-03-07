# binance-data-collector
Binance market data collection and aggregation service (WebSocket).

## Build

```bash
cmake --preset PRESET_NAME -B BUILD_DIR -S .
cmake --build BUILD_DIR
```

### to code style(if set in preset)
```bash
cmake --build BUILD_DIR --target format_apply
```

### to run tests(if set in preset)
```bash
cd BUILD_DIR && ctest
```

## Install as systemd service
this would be done by package creation and installation in future

```bash
sudo cmake --build BUILD_DIR --target install
sudo systemctl daemon-reload
sudo systemctl enable --now binance-data-collector-service
```

Installed files:

- Binary: `/usr/local/bin/binance-data-collector` (or `${CMAKE_INSTALL_PREFIX}/bin/binance-data-collector`)
- Config: `/etc/binance-data-collector/config.yaml`
- Unit: `/etc/systemd/system/binance-data-collector-service.service`

# binance-data-collector
Binance market data collection and aggregation service (WebSocket).


_please check config.json(working template) and run --help after building, to know which arguments are supported_

## Run with Podman (Ubuntu host)

### Host packages required

On Ubuntu/Debian hosts:

```bash
sudo apt-get update
sudo apt-get install -y podman
```

### Build image


```bash
cd SRC_DIR podman build -t binance-data-collector:ubuntu24 .
```

This image already runs Conan dependency installation and project build during `podman build`.

### Run service in container

Start an interactive shell in the container:

```bash
podman run -it --name bdc binance-data-collector:ubuntu24
```

## local build/run

### Install dependencies and build with Conan:

```bash
conan profile detect --force
conan install . --build=missing -s build_type=BUILD_TYPE
conan build . -s build_type=BUILD_TYPE
```

##  More details for devs:

```bash
cmake --preset PRESET_NAME -B BUILD_DIR -S .
cmake --build BUILD_DIR
```

### For code style fix and check

```bash
cmake --build BUILD_DIR --target format_apply # will change all cpp files to follow the code style
cmake --build BUILD_DIR --target format_check # will check and give warnings(no changes applied)
```

### Unit tests

```bash
cd BUILD_DIR && ctest
```

### Install as systemd service
(not a very good idea, but for time saving i decided to make this way yet)

```bash
sudo cmake --build BUILD_DIR --target install
sudo systemctl daemon-reload
sudo systemctl enable --now binance-data-collector
```

Installed files:

- Binary: `/usr/local/bin/binance-data-collector` (or `${CMAKE_INSTALL_PREFIX}/bin/binance-data-collector`)
- Config: `/etc/binance-data-collector/config.json`
- Unit: `/etc/systemd/system/binance-data-collector.service`

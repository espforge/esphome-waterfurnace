# Development

## Local Development

For local development, use the dev config in `tests/` (a `secrets.yaml` with test credentials is already included):

```sh
# Run all tests (unit, integration, config validation, compile)
tests/run_tests.sh

# Or compile and flash directly
cd tests && esphome run waterfurnace-test.yaml
```

The `tests/waterfurnace-test.yaml` overrides the component source to use local files via the `component_source` substitution.

## Testing

Unit tests and integration tests are in `tests/`. The integration tests run the actual C++ protocol code against the [waterfurnace_aurora](https://github.com/ccutrer/waterfurnace_aurora) Ruby gem's ModBus server via Docker. See [tests/README.md](tests/README.md) for details.

```sh
# Unit tests (native, needs g++ and googletest)
cd tests/unit && make test

# Integration tests (needs Docker)
cd tests && docker compose up --build --abort-on-container-exit
```

## Testing against a development branch

When testing changes from a branch, set `refresh: 0s` so ESPHome always pulls the latest code instead of using a cached copy.

### Package import

```yaml
packages:
  espforge.waterfurnace:
    url: https://github.com/espforge/esphome-waterfurnace
    file: waterfurnace-esp32-s3.yaml
    ref: main
    refresh: 0s
```

### External component import

```yaml
external_components:
  - source: github://espforge/esphome-waterfurnace@main
    refresh: 0s
```

Without `refresh: 0s`, ESPHome caches the component source and won't pick up new commits until the default cache period (1d) expires.

## Release & Deployment

See **[RELEASES.md](RELEASES.md)** for complete documentation on:
- Creating releases
- Testing on a fork
- CI/CD workflows
- Troubleshooting deployments

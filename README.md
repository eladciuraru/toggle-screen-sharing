# toggle-screen-sharing

A small macOS utility that toggles the system Screen Sharing permission
by talking directly to `tccd` via XPC.

## How to build

```bash
./build.sh
```

## How to run

this tool is compiled to `dyld` so it can be injected into another binary,
the reason for this is that you need `com.apple.private.tcc.manager.access.modify` entitlement
with both `kTCCServicePostEvent` and `kTCCServiceScreenCapture`.

you can search for such a binary with in [here](https://blacktop.github.io/ipsw/entitlements/).

for example using `mdmclient` since it has these requirements under the current
newest macOS 26.3:

```shell
# toggling on
env SCREEN_SHARING_TOGGLE=on "DYLD_INSERT_LIBRARIES=bin/libscreen_sharing.dylib" /usr/libexec/mdmclient

# toggling off
env SCREEN_SHARING_TOGGLE=off "DYLD_INSERT_LIBRARIES=bin/libscreen_sharing.dylib" /usr/libexec/mdmclient
```

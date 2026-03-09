# toggle-screen-sharing

A small macOS utility that toggles the system Screen Sharing permission
by talking directly to `tccd` via XPC.

## How to build

```bash
./build.sh
```

## How to run

there are 2 modes supported for running this tool.
1. executable
2. injectable dynamic library

for both of the modes, SIP needs to be disabled

### Executable Mode

because this is a self signed executable, `amfi` needs to be disabled,
either with `amfi_get_out_of_my_way` boot arg or the recommended way using
[amfidont](https://github.com/zqxwce/amfidont) to allow this specific executable.

then it can just run as a regular executable from command line.
for example:

```shell
# toggling on
./bin/screen_sharing --toggle on

# toggling off
./bin/screen_sharing --toggle off
```

### Dynamic Library

with this mode the library needs to be injected into another binary
with the right entitlements.
for enabling screen sharing you need `com.apple.private.tcc.manager.access.modify` entitlement
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

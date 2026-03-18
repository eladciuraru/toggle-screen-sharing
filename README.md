# toggle-screen-sharing

A small macOS utility that toggles the system Screen Sharing and
Remote Login permission by talking directly to `tccd` via XPC.

## How to build

```bash
./build.sh
```

## How to run

there are 2 modes supported for running this tool.
1. executable
2. injectable dynamic library

for both of the modes, SIP needs to be disabled,
and should be run as root.

### Executable Mode

because this is a self signed executable, `amfi` needs to be disabled,
either with `amfi_get_out_of_my_way` boot arg or the recommended way using
[amfidont](https://github.com/zqxwce/amfidont) to allow this specific executable.

then it can just run as a regular executable from command line.
for example:

```shell
# toggling on
sudo ./bin/screen_sharing --toggle on

# toggling off
sudo ./bin/screen_sharing --toggle off
```

### Dynamic Library

with this mode the library needs to be injected into another binary
with the right entitlements.
for enabling screen sharing you need `com.apple.private.tcc.manager.access.modify` entitlement
with both `kTCCServicePostEvent` and `kTCCServiceScreenCapture`.
for enabling remote login you need `kTCCServiceSystemPolicyAllFiles`.

you can search for such a binary with in [here](https://blacktop.github.io/ipsw/entitlements/).

for example using `writeconfig` since it has these requirements under the current
newest macOS 26.3:

```shell
# toggling on
sudo env SCREEN_SHARING_TOGGLE=on REMOTE_LOGIN_TOGGLE=on "DYLD_INSERT_LIBRARIES=./bin/libscreen_sharing.dylib" /System/Library/PrivateFrameworks/SystemAdministration.framework/XPCServices/writeconfig.xpc/Contents/MacOS/writeconfig

# toggling off
sudo env SCREEN_SHARING_TOGGLE=off REMOTE_LOGIN_TOGGLE=off "DYLD_INSERT_LIBRARIES=./bin/libscreen_sharing.dylib" /System/Library/PrivateFrameworks/SystemAdministration.framework/XPCServices/writeconfig.xpc/Contents/MacOS/writeconfig
```

## TODO
- [ ] also handle screen sharing service
- [ ] move from manually creating XPC to calling private TCC framework APIs
- [ ] change so only provided toggle flags are used (e.g. if only remote loging is used then only send remote loging requests)

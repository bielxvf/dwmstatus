# dwmstatus
Suckless' dwmstatus
# Build
```sh
make
```
# Install
```sh
if command -v doas > /devl/null 2>&1; then
    doas make install
elif command -v sudo > /dev/null 2>&1; then
    sudo make install
fi
```
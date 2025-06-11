Installation:
Deliverables for origin feature: ~/daemon prod

Dependencies:
clang
libbpf-dev
libelf-dev
zlib1g-dev
llvm-12-tools
sqlite3
build-essential
linux-headers-$(uname -r)

when in folder /daemon prod, simply run make

To start the daemons: sudo ./daemon && sudo ./daemon_download_origin

Final commit:
